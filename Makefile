OBJECTS=mailbox.o test.o memory.o memory_v3d2.o memory_mailbox.o nopsled.o controllist.o binner.o v3d_core.o triangle.o v3d2.o
LIBOBJECTS=memory.o memory_v3d2.o v3d2.o
CLIENT_OBJECTS=client.o
SERVER_OBJECTS=server.o

all: test server libV3D2.so texturetest
client: ${CLIENT_OBJECTS}
	g++ ${CLIENT_OBJECTS} -o $@ -Wall -Wextra -g
server: ${SERVER_OBJECTS}
	g++ ${SERVER_OBJECTS} -o $@ -Wall -Wextra -g
test: ${OBJECTS}
	g++ ${OBJECTS} -o $@ -Wall -Wextra -g -L/opt/vc/lib/ -lbcm_host
texturetest: test1.o tformat.o
	g++ $+ -o $@ -g -lpng -L/opt/vc/lib/ -lbcm_host
libV3D2.so: ${LIBOBJECTS}
	${LD} -o $@ $+ -shared
	cp -v $@ /media/videos/4tb/rpi/lib/
%.o: %.cpp
	g++ -o $@ -Wall -Wextra -g $< -c -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads/ -I/opt/vc/include/interface/vmcs_host/linux/

test.o: test.cpp memory_mailbox.h v3d.h v3d2_ioctl.h memory.h memory_v3d2.h mailbox.h controllist.h nopsled.h nopsled.h binner.h v3d2.h triangle.h
mailbox.o: mailbox.cpp mailbox.h
memory.o: memory.cpp memory.h
memory_v3d2.o: memory_v3d2.cpp memory_v3d2.h memory.h v3d2_ioctl.h v3d2.h
memory_mailbox.o: memory_mailbox.cpp memory_mailbox.h mailbox.h
nopsled.o: nopsled.cpp nopsled.h controllist.h v3d.h
controllist.o: controllist.cpp controllist.h v3d.h v3d_core.h
server.o: server.cpp
client.o: client.cpp v3d2_ioctl.h
binner.o: binner.cpp controllist.h v3d.h v3d2_ioctl.h
v3d_core.o: v3d_core.cpp
triangle.o: triangle.cpp compiler.h memory.h v3d2_ioctl.h v3d_core.h memory_v3d2.h v3d.h
v3d2.o: v3d2.cpp v3d2.h memory.h
test1.o: test1.cpp tformat.h
tformat.o: tformat.c tformat.h
