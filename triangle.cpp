#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#include "bcm_host.h"

#include "compiler.h"
#include "memory.h"
#include "v3d2_ioctl.h"
#include "v3d_core.h"
#include "memory_v3d2.h"
#include "v3d.h"

MemoryReference *makeShaderCode(AllocatorBase *allocator) {
	MemoryReference *shaderCode = allocator->Allocate(0x50);
	uint8_t *shadervirt = (uint8_t*)shaderCode->mmap();
	uint8_t *p = shadervirt;
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
	assert((p - shadervirt) < 0x50);
	shaderCode->munmap();
	return shaderCode;
}
MemoryReference *makeVertextData(AllocatorBase *allocator,int redx, int redy) {
	MemoryReference *vertexData = allocator->Allocate(0x60);
	uint8_t *vertexvirt = (uint8_t*)vertexData->mmap();
	uint8_t *p = vertexvirt;

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

	assert((p - vertexvirt) < 0x60);
	vertexData->munmap();
	return vertexData;
}
MemoryReference *makeShaderRecord(AllocatorBase *allocator,uint32_t shaderCode,uint32_t shaderUniforms, uint32_t vertexData) {
	MemoryReference *shader = allocator->Allocate(0x20);
	uint8_t *shadervirt = (uint8_t*)shader->mmap();
	uint8_t *p = shadervirt;
	addbyte(&p, 0x01); // flags
	addbyte(&p, 6*4); // stride
	addbyte(&p, 0xcc); // num uniforms (not used)
	addbyte(&p, 3); // num varyings
	addword(&p, shaderCode); // Fragment shader code
	addword(&p, shaderUniforms); // Fragment shader uniforms
	addword(&p, vertexData); // Vertex Data
	assert((p - shadervirt) < 0x20);
	shader->munmap();
	return shader;
}
uint8_t *makeRenderer(uint32_t outputFrame, uint32_t tileAllocationAddress, int *sizeOut) {
	uint8_t *render = new uint8_t[0x2000];
	uint8_t *p = render;
	// Render control list
	
	// Clear color
	addbyte(&p, 114);
	addword(&p, 0xff000000); // Opaque Black
	addword(&p, 0xff000000); // 32 bit clear colours need to be repeated twice
	addword(&p, 0);
	addbyte(&p, 0);
	
	// Tile Rendering Mode Configuration
	addbyte(&p, 113);
	addword(&p, outputFrame); // framebuffer addresss
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
			addword(&p, tileAllocationAddress + (y * 30 + x) * 32);
			
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
	int size = p - render;
	*sizeOut = size;
	printf("render size %d\n",size);
	assert(size < 0x2000);
	return render;
}
uint8_t *makeBinner(uint32_t tileAllocationAddress,uint32_t tileAllocationSize, uint32_t tileState, uint32_t shaderRecordAddress, uint32_t primitiveIndexAddress, int *sizeOut) {
	uint8_t *binner = new uint8_t[0x80];
	uint8_t *p = binner;
	// Configuration stuff
	// Tile Binning Configuration.
	//   Tile state data is 48 bytes per tile, I think it can be thrown away
	//   as soon as binning is finished.
	addbyte(&p, 112);
	addword(&p, tileAllocationAddress); // tile allocation memory address
	addword(&p, tileAllocationSize); // tile allocation memory size
	addword(&p, tileState); // Tile state data address
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
	addword(&p, shaderRecordAddress); // Shader Record

	// primitive index list
	addbyte(&p, 32);
	addbyte(&p, 0x04); // 8bit index, triangles
	addword(&p, 3); // Length
	addword(&p, primitiveIndexAddress); // address
	addword(&p, 2); // Maximum index

	// End of bin list
	// Flush
	addbyte(&p, 5);
	// Nop
	addbyte(&p, 1);
	// Nop
	addbyte(&p, 1);

	int length = p - binner;
	*sizeOut = length;
	assert(length < 0x80);
	return binner;
}
MemoryReference *makePrimitiveList(AllocatorBase *allocator) {
	MemoryReference *list = allocator->Allocate(3);
	uint8_t *p = (uint8_t*)list->mmap();
	p[0] = 0;
	p[1] = 1;
	p[2] = 2;
	list->munmap();
	return list;
}
int displayed = 0;
DISPMANX_RESOURCE_HANDLE_T resource;
DISPMANX_ELEMENT_HANDLE_T element = 0;
DISPMANX_MODEINFO_T info;
DISPMANX_DISPLAY_HANDLE_T display;
void renderFrame(uint8_t *start,int width,int height) {
	VC_RECT_T dst_rect,src_rect;
	int ret;
	int pitch = ALIGN_UP(width*4, 32);
	DISPMANX_UPDATE_HANDLE_T update;
	//VC_DISPMANX_ALPHA_T alpha;
	//alpha.flags = DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
	//alpha.opacity = 120;
	//alpha.mask = 0;

	vc_dispmanx_rect_set( &dst_rect, 0, 0, width, height);
	printf("width:%d, start:%p, height:%d pitch:%d\n",width,start,height,pitch);
	ret = vc_dispmanx_resource_write_data(resource,VC_IMAGE_RGBA32,pitch,start,&dst_rect);
	printf("vc_dispmanx_resource_write_data == %d\n",ret);
	assert( ret == 0 );
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

	puts("host init");
	bcm_host_init();
	puts("opening display");
	display = vc_dispmanx_display_open(0);
	puts("getting info");
	ret = vc_dispmanx_display_get_info( display, &info);
	assert(ret == 0);
	printf( "Display is %d x %d\n", info.width, info.height );
	resource = vc_dispmanx_resource_create( type,1920,1080,&vc_image_ptr );
	assert( resource );
}
void testTriangle(AllocatorBase *allocator, int redx,int redy,volatile unsigned *v3d) {
	struct JobStatusPacket status;
	struct timeval start,stop,spent;
	double runtime;
	uint8_t *framebuffer2 = new uint8_t[8294400];
	int width = 1920;
	int height = 1080;
	int tilewidth = 30; // 1920/64
	int tileheight = 17; // 1080/64 (16.875)
	MemoryReference *finalFrame = allocator->Allocate(tilewidth*tileheight*64*64*4);
	void *framedata = finalFrame->mmap();
	MemoryReference *tileAllocation = allocator->Allocate(0x8000);
	MemoryReference *tileState = allocator->Allocate(48 * tilewidth * tileheight);
	MemoryReference *shaderCode = makeShaderCode(allocator);
	MemoryReference *shaderUniforms = allocator->Allocate(0x10);
	MemoryReference *vertexData = makeVertextData(allocator,redx,redy);
	MemoryReference *shaderRecord = makeShaderRecord(allocator,shaderCode->getBusAddress(),shaderUniforms->getBusAddress(),vertexData->getBusAddress());
	MemoryReference *primitiveList = makePrimitiveList(allocator);
	puts("initing dispman");
	//initDispman();
	int binnerSize;
	puts("making binner");
	uint8_t *binner = makeBinner(tileAllocation->getBusAddress(),0x8000,
			tileState->getBusAddress(),
			shaderRecord->getBusAddress(),primitiveList->getBusAddress(),&binnerSize);
	int renderSize;
	uint8_t *render = makeRenderer(finalFrame->getBusAddress(),tileAllocation->getBusAddress(),&renderSize);

	uint8_t *sublists = (uint8_t*)tileAllocation->mmap();
	printf("sublists %p\n",sublists);
	memset(sublists,0,0x8000);

	printf("binner size:%d renderer size:%d\n",binnerSize,renderSize);

	JobCompileRequest job;
	memset(&job,0,sizeof(job));
	job.binner.code = binner;
	job.binner.size = binnerSize;
	job.binner.run = 1;
	job.renderer.code = render;
	job.renderer.size = renderSize;
	job.renderer.run = 1;
	job.jobid = 123;
	status.jobid = 0;
	gettimeofday(&start,0);
	ioctl(v3d2_get_fd(),V3D2_COMPILE_CL,&job);
	gettimeofday(&stop,0);

	//while(v3d[V3D_CT0CS] & 0x20);

	//printf("binner done %x\n",v3d[V3D_CT0CS]);
	
	//while(v3d[V3D_CT1CS] & 0x20);
	read(v3d2_get_fd(),&status,sizeof(status));

	printf("renderer done %x\n",v3d[V3D_CT1CS]);
	timersub(&stop,&start,&spent);
	runtime = (double)spent.tv_sec + ((double)spent.tv_usec / 1000000);
	printf("old method took %f seconds %d\n",runtime,status.jobid);
	
	//V3D2MemReference *rawbinner = new V3D2MemReference;
	//rawbinner->handle = job.binner.handle; // the auto-created handle
	//uint8_t *raw = (uint8_t*)rawbinner->mmap();
	//printf("binner start: %d\n",raw[0]);
	//delete rawbinner;

	memcpy(framebuffer2,framedata,1920*1080*4);
	//renderFrame(framebuffer2,1920,1080);
	//sleep(5);
	finalFrame->munmap();
	delete finalFrame;
	delete tileAllocation;
	delete tileState;
	delete shaderCode;
	delete shaderUniforms;
	delete vertexData;
	delete shaderRecord;
	delete primitiveList;
	delete binner;
	delete render;
	delete framebuffer2;
}
