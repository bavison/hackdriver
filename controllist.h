#ifndef CONTROLLIST_H
#define CONTROLLIST_H

#include "memory.h"

class ControlList {
public:
	ControlList(AllocatorBase *allocator);
	~ControlList();
protected:
	void PostJob(int thread,unsigned int start, unsigned int size,volatile unsigned*v3d);
	void WaitForJob(int thread,volatile unsigned*v3d);
	AllocatorBase *allocator;
	bool Allocate(int size);
	MemoryReference *ref;
};
#endif
