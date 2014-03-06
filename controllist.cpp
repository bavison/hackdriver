#include "controllist.h"
#include "v3d.h"

ControlList::ControlList(AllocatorBase *allocator) : allocator(allocator) {
}
bool ControlList::Allocate(int size) {
	ref = allocator->Allocate(size);
	if (!ref) return true;
	return false;
}
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
	delete ref;
}
