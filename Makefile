all: test test2
test2: dispmanx.c
	gcc -o test2 dispmanx.c -g -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads/ -I/opt/vc/include/interface/vmcs_host/linux/ -L/opt/vc/lib/ -lbcm_host
test: test.cpp mailbox.o v3d.h
	g++ -o test -Wall -Wextra test.cpp mailbox.o -g -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads/ -I/opt/vc/include/interface/vmcs_host/linux/ -L/opt/vc/lib/ -lbcm_host

mailbox.o: mailbox.c mailbox.h
	g++ -c -o mailbox.o -Wall -Wextra mailbox.c -g

