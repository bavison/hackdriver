OBJECTS=mailbox.o test.o memory.o memory_v3d2.o memory_mailbox.o
test: ${OBJECTS}
	g++ ${OBJECTS} -o $@ -Wall -Wextra -g
%.o: %.cpp
	g++ -o $@ -Wall -Wextra -g $< -c

test.o: test.cpp memory_mailbox.h v3d.h v3d2_ioctl.h memory.h memory_v3d2.h mailbox.h
mailbox.o: mailbox.cpp mailbox.h
memory.o: memory.cpp memory.h
memory_v3d2.o: memory_v3d2.cpp memory_v3d2.h memory.h v3d2_ioctl.h
memory_mailbox.o: memory_mailbox.cpp memory_mailbox.h mailbox.h
