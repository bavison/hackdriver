#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>
#include <assert.h>

#include "bcm_host.h"

#include "mailbox.h"
#include "v3d.h"
#include "v3d2_ioctl.h"
#include "memory.h"
#include "memory_v3d2.h"
#include "memory_mailbox.h"
#include "controllist.h"
#include "nopsled.h"
#include "binner.h"
#include "v3d2.h"
#include "triangle.h"

// I/O access
volatile unsigned *v3d;

int is_pi2;

void dispmanxtest() {
	DISPMANX_DISPLAY_HANDLE_T display;
	int ret;
	DISPMANX_MODEINFO_T displayinfo;
	DISPMANX_RESOURCE_HANDLE_T res;
	int width = 1024;
	int height = 576;
	uint32_t vc_image_ptr;
	DISPMANX_UPDATE_HANDLE_T update;
	VC_RECT_T dst_rect,src_rect;
	DISPMANX_ELEMENT_HANDLE_T element;

	
	bcm_host_init();
	display = vc_dispmanx_display_open(0);
	ret = vc_dispmanx_display_get_info( display, &displayinfo);
	assert(ret==0);
	printf("display is %dx%d\n",displayinfo.width,displayinfo.height);
	res = vc_dispmanx_resource_create(VC_IMAGE_YUV420,width,height,&vc_image_ptr);
	vc_image_ptr = vc_dispmanx_resource_get_image_handle(res);
	printf("vc_image_ptr %x\n",vc_image_ptr);
	assert(res);
	update = vc_dispmanx_update_start(10);
	assert(update);
	vc_dispmanx_rect_set(&src_rect,0,0,width<<16,height<<16);
	vc_dispmanx_rect_set(&dst_rect,0,(displayinfo.height - height)/2,width-32,height);
	element = vc_dispmanx_element_add(update,display,2000,&dst_rect,res,&src_rect,DISPMANX_PROTECTION_NONE,NULL,NULL,DISPMANX_NO_ROTATE);
	ret = vc_dispmanx_update_submit_sync(update);
	assert(ret==0);
	uint8_t *rawfb = (uint8_t*)mapmem(vc_image_ptr,0x1000);
	for (int i=0; i<0x100; i++) {
		printf("%02x ",rawfb[i]);
	}
	printf("\n");
	unmapmem(rawfb,0x1000);
	puts("sleeping");
	sleep(10);
	update = vc_dispmanx_update_start(10);
	assert(update);
	ret = vc_dispmanx_element_remove(update,element);
	assert(ret==0);
	ret = vc_dispmanx_update_submit_sync(update);
	assert(ret==0);
	ret = vc_dispmanx_resource_delete(res);
	assert(ret==0);
	ret = vc_dispmanx_display_close(display);
	assert(ret==0);
}

void determine_pi_version(void) {
  FILE *f;
  char line[256];

  is_pi2 = 1;
  f = fopen("/proc/cpuinfo", "r");
  while (!feof(f))
  {
    if (fscanf(f, "%s", line) && strstr(line, "BCM2708"))
      is_pi2 = 0;
  }
  fclose(f);
}

int main(int argc, char **argv) {
	bool v3dsupported = AllocatorBase::v3d2Supported();
	AllocatorBase *allocator;
	(void) argc;
	(void) argv;
	determine_pi_version();
	//dispmanxtest();
	//return 0;
	if (v3dsupported) {
		v3d2_init();
		puts("using new v3d2 driver");
		allocator = new V3D2Allocator();
	} else {
		puts("using mailbox fallback");
		allocator = new MailboxAllocator();
	}
	
	// map v3d's registers into our address space.
	v3d = (unsigned *) mapmem(is_pi2 ? 0x3fc00000 : 0x20c00000, 0x1000);
	
	if(v3d[V3D_IDENT0] != 0x02443356) { // Magic number.
		printf("Error: V3D pipeline isn't powered up and accessable.\n");
		exit(-1);
	}
	
	int mbox = mbox_open();
	testTriangle(mbox,allocator);
	//testBinner(allocator,v3d);
	return 0;
	// We now have access to the v3d registers, we should do something.
	for (int i=12; i<26; i+=1) {
		NopSled sled(allocator,(1<<i)+0xa);
		sled.benchmark(v3d);
	}
	
	return 0;
}
