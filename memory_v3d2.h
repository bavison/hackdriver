#include "memory.h"
class V3D2Allocator : public AllocatorBase {
public:
	V3D2Allocator();
	virtual MemoryReference *Allocate(int size);
private:
};
class V3D2MemReference : public MemoryReference {
friend class V3D2Allocator;
public:
	virtual ~V3D2MemReference();
	virtual void *mmap();
	virtual void munmap();
//private:
	int handle;
};
