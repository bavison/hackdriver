#ifndef CONTROLLIST_H
#define CONTROLLIST_H

#include <stdint.h>
#include "memory.h"

class ControlList {
public:
	ControlList(AllocatorBase *allocator);
	~ControlList();
protected:
	void PostJob(int thread,unsigned int start, unsigned int size,volatile unsigned*v3d);
	void WaitForJob(int thread,volatile unsigned*v3d);
	bool Allocate(int size);
	void AddNop(uint8_t *list) { list[compilePointer++] = 1; };

	AllocatorBase *allocator;
	MemoryReference *ref;
	unsigned int compilePointer;
};
#endif
