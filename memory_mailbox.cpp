#include <stdint.h>
#include <unistd.h>
#include "memory_mailbox.h"
#include "mailbox.h"

class MailboxReference : public MemoryReference {
friend class MailboxAllocator;
public:
	~MailboxReference();
	virtual void *mmap();
	virtual void munmap();
private:
	unsigned int handle;
	int mbox;
};
MailboxReference::~MailboxReference() {
	mem_unlock(mbox,handle);
	mem_free(mbox,handle);
}
void *MailboxReference::mmap() {
	if (virt) {
		mapcount++;
		return virt;
	}
	virt = mapmem(busAddress,size);
	return virt;
}
void MailboxReference::munmap() {
	if (mapcount) {
		mapcount--;
	}
	unmapmem(virt,size);
	virt = 0;
}
MailboxAllocator::MailboxAllocator() {
	mbox = mbox_open();
	qpu_enable(mbox, 1);
}
MemoryReference *MailboxAllocator::Allocate(int size) {
	unsigned int handle = mem_alloc(mbox,size,0x1000,MEM_FLAG_COHERENT | MEM_FLAG_ZERO);
	if (!handle) return NULL;
	uint32_t bus_addr = mem_lock(mbox,handle); // FIXME, this may harm performance
	MailboxReference *ref = new MailboxReference();
	ref->handle = handle;
	ref->busAddress = bus_addr;
	ref->size = size;
	ref->virt = 0;
	ref->mapcount = 0;
	ref->mbox = mbox;
	return ref;
}
