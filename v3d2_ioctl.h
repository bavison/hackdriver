typedef int V3dMemoryHandle;
struct mem_alloc_request {
	unsigned int size;
//	unsigned int align;
	unsigned int flags;
	V3dMemoryHandle handle;
	unsigned int physicalAddress;
};


// FIXME, should i allocate this somewhere?
#define V3D2_IOC_MAGIC 0xda

#define V3D2_MEM_ALLOC			_IO(V3D2_IOC_MAGIC,	0) // struct mem_alloc_request*
#define V3D2_MEM_FREE			_IO(V3D2_IOC_MAGIC,	1) // V3dMemoryHandle*
#define V3D2_MEM_DEMO			_IO(V3D2_IOC_MAGIC,	2)
#define V3D2_QPU_ENABLE			_IO(V3D2_IOC_MAGIC,	3) // int, 1==on, 0==off
#define V3D2_MEM_SELECT			_IO(V3D2_IOC_MAGIC,	4) // V3dMemoryHandle*
