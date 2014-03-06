#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "memory.h"

bool AllocatorBase::v3d2Supported() {
	int fd = open("/dev/v3d2",O_RDWR);
	if (fd == -1) {
		printf("cant open v3d2, %s\n",strerror(errno));
		return false;
	}
	// FIXME, add version checks
	close(fd);
	return true;
}
