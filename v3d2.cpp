#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>

#include "v3d2.h"
#include "memory.h"

// c style functions

int v3d2Supported() {
	return AllocatorBase::v3d2Supported();
}

int v3dfd;
bool v3d2_init() {
	v3dfd = open("/dev/v3d2",O_RDWR);
	if (v3dfd == -1) {
		printf("cant open v3d2, %s\n",strerror(errno));
		return true;
	}
	puts("v3d2 device open");
	return false;
}
int v3d2_get_fd() {
	return v3dfd;
}
struct V3D2MemoryReference *V3D2Allocate(uint32_t size) {
	struct mem_alloc_request request;
	request.size = size;
	if (ioctl(v3d2_get_fd(),V3D2_MEM_ALLOC,&request)) {
		puts("Error: Unable to allocate memory");
		return NULL;
	}
	struct V3D2MemoryReference *ref = new struct V3D2MemoryReference;
	ref->handle = request.handle;
	ref->phys = request.physicalAddress;
	ref->size = size;
	ref->virt = 0;
	ref->mapcount = 0;
	return ref;
}
void V3D2Free(struct V3D2MemoryReference *ref) {
	ioctl(v3d2_get_fd(),V3D2_MEM_FREE,&ref->handle);
}
void *V3D2mmap(struct V3D2MemoryReference *ref) {
	int ret;
	assert(ref);
	if (ref->virt) {
		ref->mapcount++;
		return ref->virt;
	}
	ret = ioctl(v3d2_get_fd(),V3D2_MEM_SELECT,&ref->handle);
	assert(ret == 0);
	ref->virt = ::mmap(0,ref->size,PROT_READ|PROT_WRITE,MAP_SHARED,v3d2_get_fd(),0);
	if ((int)ref->virt == -1) {
		printf("mmap error: %s\n",strerror(errno));
		return NULL;
	}
	return ref->virt;
}
void V3D2munmap(struct V3D2MemoryReference *ref) {
	if (ref->mapcount) {
		ref->mapcount--;
	}
	::munmap(ref->virt,ref->size);
	ref->virt = 0;
}
