OBJECTS=mailbox.o test.o memory.o memory_v3d2.o memory_mailbox.o nopsled.o controllist.o binner.o v3d_core.o triangle.o
all: test server
CLIENT_OBJECTS=client.o
client: ${CLIENT_OBJECTS}
	g++ ${CLIENT_OBJECTS} -o $@ -Wall -Wextra -g
SERVER_OBJECTS=server.o
server: ${SERVER_OBJECTS}
	g++ ${SERVER_OBJECTS} -o $@ -Wall -Wextra -g
test: ${OBJECTS}
	g++ ${OBJECTS} -o $@ -Wall -Wextra -g -L/opt/vc/lib/ -lbcm_host
%.o: %.cpp
	g++ -o $@ -Wall -Wextra -g $< -c -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads/ -I/opt/vc/include/interface/vmcs_host/linux/

test.o: test.cpp memory_mailbox.h v3d.h v3d2_ioctl.h memory.h memory_v3d2.h mailbox.h controllist.h nopsled.h nopsled.h binner.h v3d_core.h
mailbox.o: mailbox.cpp mailbox.h
memory.o: memory.cpp memory.h
memory_v3d2.o: memory_v3d2.cpp memory_v3d2.h memory.h v3d2_ioctl.h v3d_core.h
memory_mailbox.o: memory_mailbox.cpp memory_mailbox.h mailbox.h
nopsled.o: nopsled.cpp nopsled.h controllist.h v3d.h
controllist.o: controllist.cpp controllist.h v3d.h v3d_core.h
server.o: server.cpp
client.o: client.cpp v3d2_ioctl.h
binner.o: binner.cpp controllist.h v3d.h
v3d_core.o: v3d_core.cpp
triangle.o: triangle.cpp compiler.h
