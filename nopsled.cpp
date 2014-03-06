#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include "nopsled.h"
#include "v3d.h"

NopSled::NopSled(AllocatorBase *allocator, int size): ControlList(allocator) {
	Allocate(size);
	uint8_t *list = (uint8_t*)ref->mmap();
	if ((int)list == -1) {
		printf("errno when doing mmap\n");
		return;
	}
	compilePointer = 0; // start at begining of object
	for(int i = 0; i < size-1; i++) {
		AddNop(list);
	}
	list[size-10] = 0; // Halt.
	instructions = size-10;
	ref->munmap();
}
void NopSled::benchmark(volatile unsigned *v3d) {
	struct timeval start,stop,spent;
	double runtime;
	int clockrate;

	// And then we setup the v3d pipeline to execute the control list.
	printf("V3D_CT0CS: 0x%08x\n", v3d[V3D_CT0CS]);
	printf("Start Address: 0x%08x\n", ref->getBusAddress());
	gettimeofday(&start,0);
	PostJob(0,ref->getBusAddress(),ref->getSize(),v3d);
	// print status while running
	printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);
	WaitForJob(0,v3d);
	gettimeofday(&stop,0);
	printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);
	timersub(&stop,&start,&spent);
	runtime = (double)spent.tv_sec + ((double)spent.tv_usec / 1000000);
	clockrate = instructions / runtime;
	printf("control list took %f to complete request 0x%x instructions, estimated clock of %dmhz\n",runtime,instructions,clockrate/1000000);
}
