#ifndef CONTROLLIST_H
#define CONTROLLIST_H

#include <stdint.h>
#include "memory.h"
#include "v3d2_ioctl.h"

class ControlList {
public:
	ControlList(AllocatorBase *allocator);
	virtual ~ControlList();
	//bool Allocate(int size);
	MemoryReference *getRef() { return ref; }
	void setBinner(uint8_t *code, uint32_t size);
	void compileAndRun();
	void AddNop() { list[compilePointer++] = 1; };
	void AddByte(uint8_t d) {
		list[compilePointer++] = d;
	}
	void AddWord(uint32_t d) {
		list[compilePointer++] = (d) & 0xff;
		list[compilePointer++] = (d >> 8)  & 0xff;
		list[compilePointer++] = (d >> 16) & 0xff;
		list[compilePointer++] = (d >> 24) & 0xff;
	}
	void AddShort(uint16_t d) {
		list[compilePointer++] = (d) & 0xff;
		list[compilePointer++] = (d >> 8)  & 0xff;
	}
	void AddFloat(float f) {
		uint32_t d = *((uint32_t *)&f);
		list[compilePointer++] = (d) & 0xff;
		list[compilePointer++] = (d >> 8)  & 0xff;
		list[compilePointer++] = (d >> 16) & 0xff;
		list[compilePointer++] = (d >> 24) & 0xff;
	}
	void AddTileBinningConfig(uint32_t tileAddr, uint32_t tileSize, uint32_t tileState, uint8_t width, uint8_t height, uint8_t config) {
		AddByte(112);
		AddWord(tileAddr);
		AddWord(tileSize);
		AddWord(tileState);
		AddByte(width);
		AddByte(height);
		AddByte(config);
	}

	unsigned int compilePointer;
	uint8_t *list;
protected:
	void PostJob(int thread,unsigned int start, unsigned int size,volatile unsigned*v3d);
	void WaitForJob(int thread,volatile unsigned*v3d);

	AllocatorBase *allocator;
	MemoryReference *ref;
	uint8_t *binnerVirt;
	uint32_t binnerSize;
	V3dMemoryHandle binnerHandle;
};
#endif
