#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>

#include "mailbox.h"
#include "v3d.h"
#include "v3d2_ioctl.h"
#include "memory.h"
#include "memory_v3d2.h"
#include "memory_mailbox.h"
#include "controllist.h"
#include "nopsled.h"

// I/O access
volatile unsigned *v3d;

int main(int argc, char **argv) {
	bool v3dsupported = AllocatorBase::v3d2Supported();
	AllocatorBase *allocator;
	if (v3dsupported) {
		puts("using new v3d2 driver");
		allocator = new V3D2Allocator();
	} else {
		puts("using mailbox fallback");
		allocator = new MailboxAllocator();
	}
	
	// map v3d's registers into our address space.
	v3d = (unsigned *) mapmem(0x20c00000, 0x1000);
	
	if(v3d[V3D_IDENT0] != 0x02443356) { // Magic number.
		printf("Error: V3D pipeline isn't powered up and accessable.\n");
		exit(-1);
	}
	
	// We now have access to the v3d registers, we should do something.
	for (int i=12; i<21; i+=2) {
		NopSled sled(allocator,(1<<i)+0xa);
		sled.benchmark(v3d);
	}
	
	return 0;
}
