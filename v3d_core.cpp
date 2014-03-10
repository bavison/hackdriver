#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

int v3dfd;
bool v3d2_init() {
	v3dfd = open("/dev/v3d2",O_RDWR);
	if (v3dfd == -1) {
		printf("cant open v3d2, %s\n",strerror(errno));
		return true;
	}
	return false;
}
int v3d2_get_fd() {
	return v3dfd;
}
