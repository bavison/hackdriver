#include "memory.h"
class V3D2Allocator : public AllocatorBase {
public:
	V3D2Allocator();
	virtual MemoryReference *Allocate(int size);
private:
	int v3d2;
};
