#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv) {
	(void) argc;
	(void) argv;
	int server = socket(AF_UNIX,SOCK_STREAM,0);
	int ret,newclient,size;
	int v3d2;
	v3d2 = open("/dev/v3d2",O_RDWR);
	if (v3d2 == -1) {
		printf("cant open v3d2, %s\n",strerror(errno));
		return -6;
	}
	if (server < 0) {
		printf("error creating socket %s\n",strerror(errno));
		return -1;
	}
	ret = unlink("socket");
	if (ret && (errno!=ENOENT)) {
		printf("cant clean up socket %s %d\n",strerror(errno),errno);
		return -3;
	}
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path,"socket");
	if (bind(server,(sockaddr*)&addr,sizeof(addr))) {
		printf("error binding %s\n",strerror(errno));
		return -2;
	}
	if (fchmod(server,0777)) {
		printf("error fixing perms %s\n",strerror(errno));
		return -7;
	}
	if (listen(server,10)) {
		printf("error listening %s\n",strerror(errno));
		return -5;
	}
	while ((newclient = accept(server,NULL,NULL))) {
		if (newclient < 0) {
			printf("accept error %s\n",strerror(errno));
			return -4;
		}
		struct iovec fragment;
		char *inbound = new char[1024];
		fragment.iov_base = inbound;
		fragment.iov_len = 1024;
		struct msghdr message;
		message.msg_name = NULL;
		message.msg_control = NULL;
		message.msg_iov = &fragment;
		message.msg_iovlen = 1;
		while ((size = recvmsg(newclient,&message,0))) {
			if (size < 0) {
				printf("recvmsg error %s\n",strerror(errno));
				break;
			}
			inbound[size] = 0;
			printf("received %d bytes\n",size);
			if (strcmp("init",inbound) == 0) {
				// send a reduced capacity v3d handle over
				// based on man cmsg 
				struct msghdr msg;
				memset(&msg,0,sizeof(msg));
				struct cmsghdr *cmsg;
				int myfds[1];
				myfds[0] = v3d2;
				char buf[CMSG_SPACE(sizeof myfds)];
				int *fdptr;
				struct iovec iov;
				char reply[3];
				
				msg.msg_control = buf;
				msg.msg_controllen = sizeof buf;
				cmsg = CMSG_FIRSTHDR(&msg);
				cmsg->cmsg_level = SOL_SOCKET;
				cmsg->cmsg_type = SCM_RIGHTS;
				cmsg->cmsg_len = CMSG_LEN(sizeof(int));
				fdptr = (int*) CMSG_DATA(cmsg);
				memcpy(fdptr,myfds,sizeof(int));
				msg.msg_controllen = cmsg->cmsg_len;
				msg.msg_iovlen = 1;
				strcpy(reply,"OK");
				iov.iov_base = reply;
				iov.iov_len = 2;
				msg.msg_iov = &iov;
				sendmsg(newclient,&msg,0);
			}
		}
		close(newclient);
		delete inbound;
	}
}
