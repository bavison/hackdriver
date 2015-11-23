#include <png.h>
#include <assert.h>
#include <stdint.h>
#include <iostream>

#include "tformat.h"

#include "bcm_host.h"
DISPMANX_DISPLAY_HANDLE_T display;
DISPMANX_MODEINFO_T info;
void initDispman() {
	int ret;
	
	bcm_host_init();
	display = vc_dispmanx_display_open(0);
	puts("getting info");
	ret = vc_dispmanx_display_get_info( display, &info);
	assert(ret == 0);
	printf( "Display is %d x %d\n", info.width, info.height );
}
void renderImage(uint8_t *image_data, int width, int height, VC_IMAGE_TYPE_T type, int offset) {
	DISPMANX_RESOURCE_HANDLE_T resource;
	int ret;
	uint32_t vc_image_ptr;
	DISPMANX_UPDATE_HANDLE_T update;
	VC_RECT_T dst_rect,src_rect;

	resource = vc_dispmanx_resource_create( type,width,height,&vc_image_ptr );

	vc_dispmanx_rect_set(&dst_rect,0,0,width,height);
	ret = vc_dispmanx_resource_write_data(resource,type,ALIGN_UP(width*4,32),image_data,&dst_rect);
	
	update = vc_dispmanx_update_start( 10 );
	assert(update);
	vc_dispmanx_rect_set( &src_rect, 0, 0, width << 16, height << 16 );
	vc_dispmanx_rect_set( &dst_rect, ( info.width - (width*offset) ) / 2,
		( info.height - height ) / 2,
		width,
		height );
	vc_dispmanx_element_add(update,display,2000,&dst_rect,resource,&src_rect,DISPMANX_PROTECTION_NONE,NULL,NULL,DISPMANX_NO_ROTATE);
	ret = vc_dispmanx_update_submit_sync( update );
	assert( ret == 0 );
}
// based on http://afsharious.wordpress.com/2011/06/30/loading-transparent-pngs-in-opengl-for-dummies/
int main(int argc, char *argv[]) {
	assert(argc == 2);
	png_byte header[8];
	
	FILE *fp = fopen(argv[1],"rb");
	assert(fp);
	fread(header,1,8,fp);
	int is_png = !png_sig_cmp(header,0,8);
	assert(is_png);
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	assert(png_ptr);
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	assert(info_ptr);
	
	png_infop end_info = png_create_info_struct(png_ptr);
	assert(end_info);
	
	png_init_io(png_ptr,fp);
	
	png_set_sig_bytes(png_ptr,8);
	
	png_read_info(png_ptr,info_ptr);
	
	int bit_depth,color_type;
	png_uint_32 twidth,theight;
	
	png_get_IHDR(png_ptr,info_ptr,&twidth,&theight, &bit_depth, &color_type, NULL, NULL, NULL);
	
	png_read_update_info(png_ptr,info_ptr);
	
	int rowbytes = png_get_rowbytes(png_ptr,info_ptr);
	printf("bytes per row: %d\n",rowbytes);
	
	png_byte *image_data = new png_byte[rowbytes * theight];
	assert(image_data);
	
	png_bytep *row_pointers = new png_bytep[theight];
	assert(row_pointers);
	for (png_uint_32 i=0; i<theight; i++) {
		row_pointers[i] = image_data + i * rowbytes;
	}
	
	png_read_image(png_ptr,row_pointers);

	initDispman();
	renderImage(image_data,twidth,theight,VC_IMAGE_RGBA32,2);
	uint32_t *tformat = (uint32_t*)malloc(256*256*4);
	convert((uint32_t*)image_data,256,256,tformat);
	assert(tformat);
	renderImage((uint8_t*)tformat,twidth,theight,VC_IMAGE_TF_RGBA32,0);
	//std::cin >> foo;
	sleep(10);
	delete []image_data;
	fclose(fp);
}
