#include <stdio.h>
#include <assert.h>

#include "controllist.h"
#include "v3d.h"

void testBinner(AllocatorBase *allocator, volatile unsigned *v3d) {
// Like above, we allocate/lock/map some videocore memory
  // 64kb, 4k alignment
	ControlList job(allocator);
	if (job.Allocate(0x10000)) {
    printf("Error: Unable to allocate memory");
    return;
  }
v3d[V3D_INTENA] = 3;
  uint32_t bus_addr = job.getRef()->getBusAddress();
  uint8_t *list = (uint8_t*) job.getRef()->mmap();

	job.compilePointer = 0;
	job.list = list;

// Configuration stuff
  // Tile Binning Configuration.
  //   Tile state data is 48 bytes per tile, I think it can be thrown away
  //   as soon as binning is finished.
	job.AddByte(112);
	job.AddWord(bus_addr + 0x6200); // tile allocation memory address
	job.AddWord(0x9c00); // tile allocation memory size
	job.AddWord(bus_addr + 0x100); // Tile state data address
	job.AddByte(30); // 1920/64
	job.AddByte(17); // 1080/64 (16.875)
	job.AddByte(0x04); // config

  // Start tile binning.
	job.AddByte(6);

  // Primitive type
	job.AddByte(56);
	job.AddByte(0x32); // 16 bit triangle

  // Clip Window
	job.AddByte(102);
	job.AddShort(0);
	job.AddShort(0);
	job.AddShort(1920); // width
	job.AddShort(1080); // height

  // State
	job.AddByte(96);
	job.AddByte(0x03); // enable both foward and back facing polygons
	job.AddByte(0x00); // depth testing disabled
	job.AddByte(0x02); // enable early depth write

  // Viewport offset
	job.AddByte(103);
	job.AddShort(0);
	job.AddShort(0);

// The triangle
  // No Vertex Shader state (takes pre-transformed vertexes, 
  // so we don't have to supply a working coordinate shader to test the binner.
	job.AddByte(65);
	job.AddWord(bus_addr + 0x80); // Shader Record

  // primitive index list
	job.AddByte(32);
	job.AddByte(0x04); // 8bit index, trinagles
	job.AddWord(3); // Length
	job.AddWord(bus_addr + 0x70); // address
	job.AddWord(2); // Maximum index

// End of bin list
  // Flush
	job.AddByte(5);
  // Nop
	job.AddNop();
  // Halt
	job.AddByte(0);

  int length = job.compilePointer;
  assert(length < 0x80);

// Shader Record
	job.compilePointer = 0x80;
	job.AddByte(0x01); // flags
	job.AddByte(6*4); // stride
	job.AddByte(0xcc); // num uniforms (not used)
	job.AddByte(3); // num varyings
	job.AddWord(0xaaaaaaaa); // Fragment shader code
	job.AddWord(0xbbbbbbbb); // Fragment shader uniforms
	job.AddWord(bus_addr + 0xa0); // Vertex Data

// Vertex Data
	job.compilePointer = 0xa0;
  // Vertex: Top, red
	job.AddShort(200 << 4); // Y in 12.4 fixed point
	job.AddShort((1920/2) << 4); // X in 12.4 fixed point
	job.AddFloat(1.0f); // Z
	job.AddFloat(1.0f); // 1/W
	job.AddFloat(1.0f); // Varying 0 (Red)
	job.AddFloat(0.0f); // Varying 1 (Green)
	job.AddFloat(0.0f); // Varying 2 (Blue)

  // Vertex: bottom left, Green
	job.AddShort(800 << 4); // Y in 12.4 fixed point
	job.AddShort(560 << 4); // X in 12.4 fixed point
	job.AddFloat(1.0f); // Z
	job.AddFloat(1.0f); // 1/W
	job.AddFloat(0.0f); // Varying 0 (Red)
	job.AddFloat(1.0f); // Varying 1 (Green)
	job.AddFloat(0.0f); // Varying 2 (Blue)

  // Vertex: bottom right, Blue
	job.AddShort(800 << 4); // Y in 12.4 fixed point
	job.AddShort(1460 << 4); // X in 12.4 fixed point
	job.AddFloat(1.0f); // Z
	job.AddFloat(1.0f); // 1/W
	job.AddFloat(0.0f); // Varying 0 (Red)
	job.AddFloat(0.0f); // Varying 1 (Green)
	job.AddFloat(1.0f); // Varying 2 (Blue)

// Vertex list
	job.compilePointer = 0x70;
	job.AddByte(0); // top
	job.AddByte(1); // bottom left
	job.AddByte(2); // bottom right

// Run our control list
  printf("Binner control list constructed\n");
  printf("Start Address: 0x%08x, length: 0x%x\n", bus_addr, length);

  v3d[V3D_CT0CA] = bus_addr;
  v3d[V3D_CT0EA] = bus_addr + length;
  printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);

  // Wait for control list to execute
  while(v3d[V3D_CT0CS] & 0x20);
  
  printf("V3D_CT0CS: 0x%08x, Address: 0x%08x\n", v3d[V3D_CT0CS], v3d[V3D_CT0CA]);

  // just dump the buffer to a file
  FILE *f = fopen("binner_dump.bin", "w");
  for(int i = 0; i < 0x10000; i++) {
    fputc(list[i], f);
  }
  fclose(f);
  printf("Buffer containing binned tile lists dumpped to binner_dump.bin\n");

// Release resources
	job.getRef()->munmap();
}
