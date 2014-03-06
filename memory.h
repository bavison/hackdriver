#ifndef MEMORY_H
#define MEMORY_H
class MemoryReference {
public:
	unsigned int getBusAddress() { return busAddress; }
	unsigned int getSize() { return size; }
	virtual void *mmap() = 0;
	virtual void munmap() = 0;
protected:
	unsigned int busAddress,size;
	void *virt;
	unsigned int mapcount;
};
class AllocatorBase {
public:
	virtual MemoryReference *Allocate(int size) = 0;
	static bool v3d2Supported();
};
#endif
