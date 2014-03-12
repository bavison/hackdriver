#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <sys/time.h>
#include "bcm_host.h"

#include "mailbox.h"
#include "v3d.h"

// I/O access
volatile unsigned *v3d;
int mbox;
uint8_t *framedata;
DISPMANX_RESOURCE_HANDLE_T resource;

void renderFrame(uint8_t *start,int width,int height);

// Execute a nop control list to prove that we have contol.
void testControlLists() {
// First we allocate and map some videocore memory to build our control list in.

  // ask the blob to allocate us 256 bytes with 4k alignment, zero it.
  unsigned int handle = mem_alloc(mbox, 0x100, 0x1000, MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
  if (!handle) {
    printf("Error: Unable to allocate memory");
    return;
  }
  // ask the blob to lock our memory at a single location at give is that address.
  uint32_t bus_addr = mem_lock(mbox, handle); 
  // map that address into our local address space.
  uint8_t *list = (uint8_t*) mapmem(bus_addr, 0x100);

// Now we construct our control list.
  // 255 nops, with a halt somewhere in the middle
  for(int i = 0; i < 0xff; i++) {
    list[i] = 1; // NOP
  }
  list[0xbb] = 0; // Halt.

// And then we setup the v3d pipeline to execute the control list.
  //printf("V3D_CT0CS: 0x%08x\n", v3d[V3D_CT0CS]); 
  //printf("Start Address: 0x%08x\n", bus_addr);
  // Current Address = start of our list (bus address)
  v3d[V3D_CT0CA] = bus_addr;
  // End Address = just after the end of our list (bus address) 
  // This also starts execution.
  v3d[V3D_CT0EA] = bus_addr + 0x100;
  
  // print status while running
  //printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);

  // Wait a second to be sure the contorl list execution has finished
  while(v3d[V3D_CT0CS] & 0x20);

  // print the status again.
  //printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);

// Release memory;
  unmapmem((void *) list, 0x100);
  mem_unlock(mbox, handle);
  mem_free(mbox, handle);
}

void addbyte(uint8_t **list, uint8_t d) {
  *((*list)++) = d;
}

void addshort(uint8_t **list, uint16_t d) {
  *((*list)++) = (d) & 0xff;
  *((*list)++) = (d >> 8)  & 0xff;
}

void addword(uint8_t **list, uint32_t d) {
  *((*list)++) = (d) & 0xff;
  *((*list)++) = (d >> 8)  & 0xff;
  *((*list)++) = (d >> 16) & 0xff;
  *((*list)++) = (d >> 24) & 0xff;
}

void addfloat(uint8_t **list, float f) {
  uint32_t d = *((uint32_t *)&f);
  *((*list)++) = (d) & 0xff;
  *((*list)++) = (d >> 8)  & 0xff;
  *((*list)++) = (d >> 16) & 0xff;
  *((*list)++) = (d >> 24) & 0xff;
}

unsigned int get_dispman_handle(int file_desc, unsigned int handle)
{
   int i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x30014; // (the tag id)
   p[i++] = 8; // (size of the buffer)
   p[i++] = 4; // (size of the data)
   p[i++] = handle;
   p[i++] = 0; // filler

   p[i++] = 0x00000000; // end tag
   p[0] = i*sizeof *p; // actual size

   mbox_property(file_desc, p);
   printf("success %d, handle %x\n",p[5],p[6]);
   return p[6];
}

// Render a single triangle to memory.
void testTriangle(int redx,int redy) {
// Like above, we allocate/lock/map some videocore memory
  // I'm just shoving everything in a single buffer because I'm lazy
  // 8Mb, 4k alignment
  unsigned int handle = mem_alloc(mbox, 0x800000, 0x1000, MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
  if (!handle) {
    printf("Error: Unable to allocate memory");
    return;
  }
  uint32_t bus_addr = mem_lock(mbox, handle); 
  uint8_t *list = (uint8_t*) mapmem(bus_addr, 0x800000);

  uint8_t *p = list;

// Configuration stuff
  // Tile Binning Configuration.
  //   Tile state data is 48 bytes per tile, I think it can be thrown away
  //   as soon as binning is finished.
  addbyte(&p, 112);
  addword(&p, bus_addr + 0x6200); // tile allocation memory address
  addword(&p, 0x8000); // tile allocation memory size
  addword(&p, bus_addr + 0x100); // Tile state data address
  addbyte(&p, 30); // 1920/64
  addbyte(&p, 17); // 1080/64 (16.875)
  addbyte(&p, 0x04); // config

  // Start tile binning.
  addbyte(&p, 6);

  // Primitive type
  addbyte(&p, 56);
  addbyte(&p, 0x32); // 16 bit triangle

  // Clip Window
  addbyte(&p, 102);
  addshort(&p, 0);
  addshort(&p, 0);
  addshort(&p, 1920); // width
  addshort(&p, 1080); // height

  // State
  addbyte(&p, 96);
  addbyte(&p, 0x03); // enable both foward and back facing polygons
  addbyte(&p, 0x00); // depth testing disabled
  addbyte(&p, 0x02); // enable early depth write

  // Viewport offset
  addbyte(&p, 103);
  addshort(&p, 0);
  addshort(&p, 0);

// The triangle
  // No Vertex Shader state (takes pre-transformed vertexes, 
  // so we don't have to supply a working coordinate shader to test the binner.
  addbyte(&p, 65);
  addword(&p, bus_addr + 0x80); // Shader Record

  // primitive index list
  addbyte(&p, 32);
  addbyte(&p, 0x04); // 8bit index, trinagles
  addword(&p, 3); // Length
  addword(&p, bus_addr + 0x70); // address
  addword(&p, 2); // Maximum index

// End of bin list
  // Flush
  addbyte(&p, 5);
  // Nop
  addbyte(&p, 1);
  // Halt
  addbyte(&p, 0);

  int length = p - list;
  assert(length < 0x80);

// Shader Record
  p = list + 0x80;
  addbyte(&p, 0x01); // flags
  addbyte(&p, 6*4); // stride
  addbyte(&p, 0xcc); // num uniforms (not used)
  addbyte(&p, 3); // num varyings
  addword(&p, bus_addr + 0xfe00); // Fragment shader code
  addword(&p, bus_addr + 0xff00); // Fragment shader uniforms
  addword(&p, bus_addr + 0xa0); // Vertex Data

// Vertex Data
  p = list + 0xa0;
  // Vertex: Top, red
  addshort(&p, redx << 4); // X in 12.4 fixed point
  addshort(&p, redy << 4); // Y in 12.4 fixed point
  addfloat(&p, 1.0f); // Z
  addfloat(&p, 1.0f); // 1/W
  addfloat(&p, 1.0f); // Varying 0 (Red)
  addfloat(&p, 0.0f); // Varying 1 (Green)
  addfloat(&p, 0.0f); // Varying 2 (Blue)

  // Vertex: bottom left, Green
  addshort(&p, 560 << 4); // X in 12.4 fixed point
  addshort(&p, 800 << 4); // Y in 12.4 fixed point
  addfloat(&p, 1.0f); // Z
  addfloat(&p, 1.0f); // 1/W
  addfloat(&p, 0.0f); // Varying 0 (Red)
  addfloat(&p, 1.0f); // Varying 1 (Green)
  addfloat(&p, 0.0f); // Varying 2 (Blue)

  // Vertex: bottom right, Blue
  addshort(&p, 1360 << 4); // X in 12.4 fixed point
  addshort(&p, 800 << 4); // Y in 12.4 fixed point
  addfloat(&p, 1.0f); // Z
  addfloat(&p, 1.0f); // 1/W
  addfloat(&p, 0.0f); // Varying 0 (Red)
  addfloat(&p, 0.0f); // Varying 1 (Green)
  addfloat(&p, 1.0f); // Varying 2 (Blue)

// Vertex list
  p = list + 0x70;
  addbyte(&p, 0); // top
  addbyte(&p, 1); // bottom left
  addbyte(&p, 2); // bottom right

// fragment shader
  p = list + 0xfe00;
  addword(&p, 0x958e0dbf);
  addword(&p, 0xd1724823); /* mov r0, vary; mov r3.8d, 1.0 */
  addword(&p, 0x818e7176); 
  addword(&p, 0x40024821); /* fadd r0, r0, r5; mov r1, vary */
  addword(&p, 0x818e7376); 
  addword(&p, 0x10024862); /* fadd r1, r1, r5; mov r2, vary */
  addword(&p, 0x819e7540); 
  addword(&p, 0x114248a3); /* fadd r2, r2, r5; mov r3.8a, r0 */
  addword(&p, 0x809e7009); 
  addword(&p, 0x115049e3); /* nop; mov r3.8b, r1 */
  addword(&p, 0x809e7012); 
  addword(&p, 0x116049e3); /* nop; mov r3.8c, r2 */
  addword(&p, 0x159e76c0); 
  addword(&p, 0x30020ba7); /* mov tlbc, r3; nop; thrend */
  addword(&p, 0x009e7000);
  addword(&p, 0x100009e7); /* nop; nop; nop */
  addword(&p, 0x009e7000);
  addword(&p, 0x500009e7); /* nop; nop; sbdone */

// Render control list
  p = list + 0xe200;

  // Clear color
  addbyte(&p, 114);
  addword(&p, 0xff000000); // Opaque Black
  addword(&p, 0xff000000); // 32 bit clear colours need to be repeated twice
  addword(&p, 0);
  addbyte(&p, 0);

  int handle2 = get_dispman_handle(mbox,resource);
  uint32_t raw = mem_lock(mbox,handle2);
  printf("dispmanx %d %x\n",handle2,raw);

  // Tile Rendering Mode Configuration
  addbyte(&p, 113);
  addword(&p, raw); // framebuffer addresss
  addshort(&p, 1920); // width
  addshort(&p, 1080); // height
  addbyte(&p, 0x04); // framebuffer mode (linear rgba8888)
  addbyte(&p, 0x00);

  // Do a store of the first tile to force the tile buffer to be cleared
  // Tile Coordinates
  addbyte(&p, 115);
  addbyte(&p, 0);
  addbyte(&p, 0);
  // Store Tile Buffer General
  addbyte(&p, 28);
  addshort(&p, 0); // Store nothing (just clear)
  addword(&p, 0); // no address is needed

  // Link all binned lists together
  for(int x = 0; x < 30; x++) {
    for(int y = 0; y < 17; y++) {

      // Tile Coordinates
      addbyte(&p, 115);
      addbyte(&p, x);
      addbyte(&p, y);
      
      // Call Tile sublist
      addbyte(&p, 17);
      addword(&p, bus_addr + 0x6200 + (y * 30 + x) * 32);

      // Last tile needs a special store instruction
      if(x == 29 && y == 16) {
        // Store resolved tile color buffer and signal end of frame
        addbyte(&p, 25);
      } else {
        // Store resolved tile color buffer
        addbyte(&p, 24);
      }
    }
  }

  int render_length = p - (list + 0xe200);


// Run our control list
  //printf("Binner control list constructed\n");
  //printf("Start Address: 0x%08x, length: 0x%x\n", bus_addr, length);
v3d[V3D_L2CACTL] = 4;

  v3d[V3D_CT0CA] = bus_addr;
  v3d[V3D_CT0EA] = bus_addr + length;
  printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);

  // Wait for control list to execute
  while(v3d[V3D_CT0CS] & 0x20);
  
assert(v3d[V3D_CT0CS] == 0x10);
  printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);
  printf("V3D_CT1CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT1CS], v3d[V3D_CT1CA]);

v3d[V3D_L2CACTL] = 4;


  v3d[V3D_CT1CA] = bus_addr + 0xe200;
  v3d[V3D_CT1EA] = bus_addr + 0xe200 + render_length;

  while(v3d[V3D_CT1CS] & 0x20);
// assert(v3d[V3D_CT1CS] == 0x10);
  
  printf("V3D_CT1CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT1CS], v3d[V3D_CT1CA]);
  v3d[V3D_CT1CS] = 0x20;

  // just dump the frame to a file
//  FILE *f = fopen("frame.data", "w");
//  fwrite(list + 0x10000, (1920*1080*4), 1, f);
//  fclose(f);
//  printf("frame buffer memory dumpped to frame.data\n");
mem_unlock(mbox,handle2);
//memcpy(framedata,list + 0x10000,1920*1080*4);

// Release resources
  unmapmem((void *) list, 0x800000);
  mem_unlock(mbox, handle);
  mem_free(mbox, handle);

//f = fopen("frame.data","r");
//fread(data,1,8294400,f);
//fclose(f);
  //renderFrame(framedata,1920,1080);
}
int displayed = 0;
DISPMANX_ELEMENT_HANDLE_T element = 0;
DISPMANX_MODEINFO_T info;
DISPMANX_DISPLAY_HANDLE_T display;
void renderFrame() {
	VC_RECT_T dst_rect,src_rect;
	int ret;
	int width = 1920;
	int height = 1080;
	//int pitch = ALIGN_UP(width*4, 32);
	DISPMANX_UPDATE_HANDLE_T update;
	//VC_DISPMANX_ALPHA_T alpha;
	//alpha.flags = DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
	//alpha.opacity = 120;
	//alpha.mask = 0;

	//vc_dispmanx_rect_set( &dst_rect, 0, 0, width, height);
	//printf("width:%d, start:%p, height:%d pitch:%d\n",width,start,height,pitch);
	//ret = vc_dispmanx_resource_write_data(resource,VC_IMAGE_RGBA32,pitch,start,&dst_rect);
	//printf("vc_dispmanx_resource_write_data == %d\n",ret);
	//assert( ret == 0 );
	update = vc_dispmanx_update_start( 10 );
	assert(update);
	vc_dispmanx_rect_set( &src_rect, 0, 0, width << 16, height << 16 );
	vc_dispmanx_rect_set( &dst_rect, ( info.width - width ) / 2,
		( info.height - height ) / 2,
		width,
		height );
	if (displayed == 0) {
		element = vc_dispmanx_element_add(update,display,2000,&dst_rect,resource,&src_rect,DISPMANX_PROTECTION_NONE,NULL,NULL,DISPMANX_NO_ROTATE);
		displayed = 1;
	}
	ret = vc_dispmanx_update_submit_sync( update );
	assert( ret == 0 );
}
void initDispman() {
	int ret;
	uint32_t vc_image_ptr;
	VC_IMAGE_TYPE_T type = VC_IMAGE_RGBA32;

	bcm_host_init();
	display = vc_dispmanx_display_open(0);
	ret = vc_dispmanx_display_get_info( display, &info);
	assert(ret == 0);
	printf( "Display is %d x %d\n", info.width, info.height );
	resource = vc_dispmanx_resource_create( type,1920,1080,&vc_image_ptr );
	assert( resource );
	renderFrame();
}

int main(int argc, char **argv) {
  mbox = mbox_open();

  // The blob now has this nice handy call which powers up the v3d pipeline.
  qpu_enable(mbox, 1);

  // map v3d's registers into our address space.
  v3d = (unsigned *) mapmem(0x20c00000, 0x1000);

  if(v3d[V3D_IDENT0] != 0x02443356) { // Magic number.
    printf("Error: V3D pipeline isn't powered up and accessable.\n");
    exit(-1);
  }
initDispman();
  // We now have access to the v3d registers, we should do something.
struct timeval start,stop,spent;
double runtime;
int frames = 50;
framedata = new uint8_t[8294400];
gettimeofday(&start,0);
for (int i=0; i<frames; i++) {
	printf(".");
	testTriangle(1920/2,200+i);
//	sleep(5);
}
gettimeofday(&stop,0);
delete framedata;
timersub(&stop,&start,&spent);
runtime = (double)spent.tv_sec + ((double)spent.tv_usec / 1000000);
printf("did %d frames in %f time, %f fps\n",frames,runtime,frames/runtime);
  return 0;
}





