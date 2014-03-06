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

// Execute a nop control list to prove that we have contol.
void testControlLists(AllocatorBase *allocator) {
	struct timeval start,stop,spent;
	int codesize = 0xf000;
	double runtime;
	int clockrate;
	// First we allocate and map some videocore memory to build our control list in.
	
	// ask the blob to allocate us 256 bytes with 4k alignment, zero it.
	MemoryReference *ref = allocator->Allocate(codesize);
	if (ref == NULL) {
		puts("Error: Unable to allocate memory");
		return;
	}
	// ask the blob to lock our memory at a single location at give is that address.
	uint32_t bus_addr = ref->getBusAddress();
	// map that address into our local address space.
	uint8_t *list = (uint8_t*)ref->mmap();
	if ((int)list == -1) {
		printf("errno %d when doing mmap\n",errno);
		return;
	}

// Now we construct our control list.
  // 255 nops, with a halt somewhere in the middle
  for(int i = 0; i < codesize-1; i++) {
    list[i] = 1; // NOP
  }
  list[codesize-10] = 0; // Halt.

// And then we setup the v3d pipeline to execute the control list.
  printf("V3D_CT0CS: 0x%08x\n", v3d[V3D_CT0CS]); 
  printf("Start Address: 0x%08x\n", bus_addr);
	gettimeofday(&start,0);
  // Current Address = start of our list (bus address)
  v3d[V3D_CT0CA] = bus_addr;
  // End Address = just after the end of our list (bus address) 
  // This also starts execution.
  v3d[V3D_CT0EA] = bus_addr + codesize;
  
  // print status while running
  printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);

	while (v3d[V3D_CT0CS] == 0x20) {}
	gettimeofday(&stop,0);
  // Wait a second to be sure the contorl list execution has finished
  //sleep(1);
  // print the status again.
  printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);
	timersub(&stop,&start,&spent);
	runtime = (double)spent.tv_sec + ((double)spent.tv_usec / 1000000);
	clockrate = (codesize-10) / runtime;
	printf("QPU took %f to complete request 0x%x instructions, estimated clock of %dmhz\n",runtime,codesize-10,clockrate/1000000);

	// Release memory;
	ref->munmap();
	delete ref;
}

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
