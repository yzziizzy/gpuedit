 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <unistd.h>

#include "dumpImage.h"

#include <png.h>
#include <setjmp.h>


// data is densely packed
int writePNG(char* path, unsigned int channels, char* data, unsigned int w, unsigned int h) {
	
	FILE* f;
	png_byte sig[8];
	png_bytep* rowPtrs;
	int i;
	
	png_structp pngPtr;
	png_infop infoPtr;
	
	int colorTypes[4] = {
		PNG_COLOR_TYPE_GRAY,
		PNG_COLOR_TYPE_GRAY_ALPHA,
		PNG_COLOR_TYPE_RGB,
		PNG_COLOR_TYPE_RGB_ALPHA
	};
	
	int ret = 2;

	printf("png write | w: %d, h: %d \n", w, h);

	if(channels > 4 || channels < 1) {
		return 3;
	}
	
	// file stuff
	f = fopen(path, "wb");
	if(!f) {
		fprintf(stderr, "Could not open \"%s\" (writePNG).\n", path);
		return 1;
	}
	
	/*
	if(png_sig_cmp(sig, 0, 8)) {
		fprintf(stderr, "\"%s\" is not a valid PNG file.\n", path);
		fclose(f);
		return NULL;
	}
	*/
	// init stuff
	pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr) {
		goto CLEANUP1;
	}
	//png_destroy_write_struct (&pngPtr, (png_infopp)NULL);
	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) {
		goto CLEANUP2;
	}
	//if(infoPtr != NULL) png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
	// header stuff
	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP3;
	}
	png_init_io(pngPtr, f);


	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP3;
	}
	png_set_IHDR(pngPtr, infoPtr, w, h,
		8, colorTypes[channels - 1], PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	png_write_info(pngPtr, infoPtr);

	rowPtrs = malloc(h * sizeof(png_bytep));
	for(i = 0; i < h; i++) {
		rowPtrs[i] = data + (i * w * channels);
	}
	
	// write data
	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP4;
	}
	png_write_image(pngPtr, rowPtrs);

	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP4;
	}
	png_write_end(pngPtr, NULL);
	
	// success
	
	ret = 0;

CLEANUP4:
	free(rowPtrs);
	
CLEANUP3:
	if(infoPtr != NULL) png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
	
CLEANUP2:
	png_destroy_write_struct (&pngPtr, (png_infopp)NULL);
	
CLEANUP1:
	fclose(f);
	
	return ret;
}




