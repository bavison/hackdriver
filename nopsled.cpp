#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include "bcm_host.h"
#include "nopsled.h"
#include "v3d.h"
#include "v3d2_ioctl.h"

void testNopSled(AllocatorBase *allocator, int size, volatile unsigned *v3d) {
	ControlList job(allocator);
	struct timeval start,stop,spent;
	double runtime;
	int clockrate;
	uint8_t *list = new uint8_t[size];
	if (!list) {
		puts("memory allocate fail");
		return;
	}
	printf("allocated %dMB\n",size/1024/1024);
	job.compilePointer = 0; // start at begining of object
	job.list = list;
	for(int i = 0; i < size-1; i++) {
		job.AddNop();
	}
	list[size-10] = 0; // Halt.
	job.setBinner(list,size);

	// And then we setup the v3d pipeline to execute the control list.
	printf("V3D_CT0CS: 0x%08x\n", v3d[V3D_CT0CS]);
	gettimeofday(&start,0);
	//PostJob(0,ref->getBusAddress(),ref->getSize(),v3d);
	job.compileAndRun();
	// print status while running
	printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);
	//WaitForJob(0,v3d);
	gettimeofday(&stop,0);
	printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);
	timersub(&stop,&start,&spent);
	runtime = (double)spent.tv_sec + ((double)spent.tv_usec / 1000000);
	clockrate = (size-10)/ runtime;
	printf("control list took %f to complete request 0x%x instructions, estimated clock of %dmhz\n",runtime,size-10,clockrate/1000000);
	delete list;
}
