#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>

#include "memory_v3d2.h"
#include "v3d2_ioctl.h"

class V3D2MemReference : public MemoryReference {
friend class V3D2Allocator;
	~V3D2MemReference();
	virtual void *mmap();
	virtual void munmap();
private:
	int handle,v3d2;
};
V3D2MemReference::~V3D2MemReference() {
	ioctl(v3d2,V3D2_MEM_FREE,&handle);
}
void *V3D2MemReference::mmap() {
	if (virt) {
		mapcount++;
		return virt;
	}
	ioctl(v3d2,V3D2_MEM_SELECT,&handle);
	virt = ::mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,v3d2,0);
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
	v3d2 = open("/dev/v3d2",O_RDWR);
	if (v3d2 == -1) {
		printf("cant open v3d2, %s\n",strerror(errno));
	}
	ioctl(v3d2,V3D2_QPU_ENABLE,1);
}
MemoryReference *V3D2Allocator::Allocate(int size) {
	struct mem_alloc_request request;
	request.size = size;
	if (ioctl(v3d2,V3D2_MEM_ALLOC,&request)) {
		puts("Error: Unable to allocate memory");
		return NULL;
	}
	V3D2MemReference *ref = new V3D2MemReference();
	ref->handle = request.handle;
	ref->busAddress = request.physicalAddress;
	ref->v3d2 = v3d2;
	ref->size = size;
	ref->virt = 0;
	ref->mapcount = 0;
	return ref;
}
