#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>

#include "bcm_host.h"
#include "memory_v3d2.h"
#include "v3d2_ioctl.h"
#include "v3d_core.h"

V3D2MemReference::~V3D2MemReference() {
	puts("releasing memory");
	ioctl(v3d2_get_fd(),V3D2_MEM_FREE,&handle);
}
void *V3D2MemReference::mmap() {
	if (virt) {
		mapcount++;
		return virt;
	}
	ioctl(v3d2_get_fd(),V3D2_MEM_SELECT,&handle);
	virt = ::mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,v3d2_get_fd(),0);
	if ((int)virt == -1) return NULL;
	return virt;
}
void V3D2MemReference::munmap() {
	if (mapcount) {
		mapcount--;
	}
	::munmap(virt,size);
	virt = 0;
}
V3D2Allocator::V3D2Allocator() {
	//ioctl(v3d2_get_fd(),V3D2_QPU_ENABLE,1);
}
MemoryReference *V3D2Allocator::Allocate(int size) {
	struct mem_alloc_request request;
	request.size = size;
	if (ioctl(v3d2_get_fd(),V3D2_MEM_ALLOC,&request)) {
		puts("Error: Unable to allocate memory");
		return NULL;
	}
	V3D2MemReference *ref = new V3D2MemReference();
	ref->handle = request.handle;
	ref->busAddress = request.physicalAddress;
	ref->size = size;
	ref->virt = 0;
	ref->mapcount = 0;
	return ref;
}
