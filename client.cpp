#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "v3d2_ioctl.h"

int main(int argc, char **argv) {
	int client = socket(AF_UNIX,SOCK_STREAM,0);
	int size;
	struct cmsghdr *cmsg;
	const char *buf;
	int v3d2 = -1;
	if (client < 0) {
		printf("error creating socket %s\n",strerror(errno));
		return -1;
	}
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path,"socket");
	if (connect(client,(sockaddr*)&addr,sizeof(addr))) {
		printf("error connecting %s\n",strerror(errno));
		return -2;
	}
	buf = "init";
	write(client,buf,strlen(buf));

	struct msghdr msg;
	memset(&msg,0,sizeof(msg));
	char extra[512];
	char inbound[1024];
	struct iovec fragment;
	fragment.iov_base = inbound;
	fragment.iov_len = 1024;
	struct msghdr message;
	message.msg_name = NULL;
	message.msg_control = NULL;
	message.msg_iov = &fragment;
	message.msg_iovlen = 1;
	message.msg_control = extra;
	message.msg_controllen = 512;
	size = recvmsg(client,&message,0);
	printf("got %d bytes back\n",size);
	inbound[size] = 0;
	if (strcmp("OK",inbound) == 0) {
		puts("its a v3d2 handle");
		cmsg = CMSG_FIRSTHDR(&message);
		while (cmsg != NULL) {
			if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
				int *fds = (int*)CMSG_DATA(cmsg);
				v3d2 = fds[0];
			}
			cmsg = CMSG_NXTHDR(&msg,cmsg);
		}
	}
	printf("v3d2 handle %d\n",v3d2);
	ioctl(v3d2,V3D2_QPU_ENABLE,1);
	close(v3d2);
	close(client);
}
