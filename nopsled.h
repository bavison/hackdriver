#ifndef NOPSLED_H
#define NOPSLED_H

#include "controllist.h"

class NopSled : public ControlList {
public:
	NopSled(AllocatorBase *allocator, int size);
	void benchmark(volatile unsigned *);
private:
	int instructions;
};
#endif
