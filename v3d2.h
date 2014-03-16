#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "bcm_host.h"
#include "v3d2_ioctl.h"

struct V3D2MemoryReference {
	V3dMemoryHandle handle; // private
	void *virt;
	uint32_t phys; // TODO: remove
	uint32_t size;
	int mapcount;
};
bool v3d2_init();
void v3d2_shutdown();
int v3d2_get_fd();
int v3d2Supported();

struct V3D2MemoryReference *V3D2Allocate(uint32_t size);
void V3D2Free(struct V3D2MemoryReference *reference);

void *V3D2mmap(struct V3D2MemoryReference *reference);
void V3D2munmap(struct V3D2MemoryReference *reference);
#ifdef __cplusplus
};
#endif
