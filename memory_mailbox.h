#include "memory.h"
class MailboxAllocator : public AllocatorBase {
public:
	MailboxAllocator();
	virtual MemoryReference *Allocate(int size);
private:
	int mbox;
};
