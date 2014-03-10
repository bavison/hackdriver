#include <sys/ioctl.h>
#include <stdio.h>

#include "controllist.h"
#include "v3d.h"
#include "v3d_core.h"

ControlList::ControlList(AllocatorBase *allocator) : allocator(allocator) {
	binnerHandle = -1;
}
/*bool ControlList::Allocate(int size) {
	ref = allocator->Allocate(size);
	if (!ref) return true;
	return false;
}*/
void ControlList::PostJob(int thread, unsigned int start, unsigned int size, volatile unsigned *v3d) {
	if (thread == 0) {
		// Current Address = start of our list (bus address)
		v3d[V3D_CT0CA] = start;
		// End Address = just after the end of our list (bus address)
		// This also starts execution.
		v3d[V3D_CT0EA] = start + size;
	}
}
void ControlList::WaitForJob(int thread,volatile unsigned *v3d) {
	if (thread == 0) {
		while (v3d[V3D_CT0CS] == 0x20) {}
	}
}
ControlList::~ControlList() {
	//delete ref;
}
void ControlList::setBinner(uint8_t *code, uint32_t size) {
	binnerVirt = code;
	binnerSize = size;
}
void ControlList::compileAndRun() {
	JobCompileRequest req;
	req.binner.code = binnerVirt;
	req.binner.size = binnerSize;
	req.binner.handle = binnerHandle; // TODO, send unused handles to a pool, and then find handles of the right size for reuse
	req.binner.run = 1;
	printf("binner at %p\n",binnerVirt);
	if (ioctl(v3d2_get_fd(),V3D2_COMPILE_CL,&req)) { // error
		puts("error compiling control lists");
	}
	binnerHandle = req.binner.handle; // this object is currently in-use, being ran on the binner core
}
