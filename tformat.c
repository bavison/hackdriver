#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define WIDTH 256
#define READPIX(x,y) input[x + (y*WIDTH)]
inline void microtile(int x1, int y1, uint32_t *input, uint32_t *out, int offset) {
	int x,y;
	int off = offset * 16;
	for (y = (y1+3); y >= y1; y--) {
		for (x = x1; x < (x1+4); x++) {
			//printf("%d,%d == %08x -> %x\n",x,y,READPIX(x,y),off);
			out[off] = READPIX(x,y);
			off++;
		}
	}
}
inline void subtile(int x1, int y1, uint32_t *input, uint32_t *output, int offset) {
	int x,y;
	int pos = offset*16;
	for (y=y1+12; y>=y1; y-=4) {
		x=x1;
		microtile(x,y,input,output,pos++);
		x+=4;
		microtile(x,y,input,output,pos++);
		x+=4;
		microtile(x,y,input,output,pos++);
		x+=4;
		microtile(x,y,input,output,pos++);
	}
}
inline void ord1(int x1, int y1, uint32_t *input, uint32_t *output, int offset) {
	int pos = offset * 4;
	//printf("%3d,%3d -> %3d\n",x1,y1,offset);
	subtile(   x1,y1+16,input,output,pos++);
	subtile(   x1,   y1,input,output,pos++);
	subtile(x1+16,   y1,input,output,pos++);
	subtile(x1+16,16+y1,input,output,pos++);
}
inline void ord2(int x1, int y1, uint32_t *input, uint32_t *output, int offset) {
	int pos = offset * 4;
	//printf("%3d,%3d -> %3d\n",x1,y1,offset);
	subtile(16+x1,   y1,input,output,pos++);
	subtile(16+x1,16+y1,input,output,pos++);
	subtile(   x1,16+y1,input,output,pos++);
	subtile(   x1,   y1,input,output,pos++);
}
uint32_t *convert(uint32_t *input, int width, int height, uint32_t *output) {
	int x,y;
	int pos = 0;
	memset(output,0x80,256*256*4);
	
	for (y=height-64; y>=0; y-=64) {
		for (x=0; x<256; x+=32) {
			ord1(x,y+32,input,output,pos++);
		}
		for (x=224; x>=0; x-=32) {
			ord2(x,y,input,output,pos++);
		}
	}
	return output;
}
