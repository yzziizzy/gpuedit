
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h> 
#include <unistd.h>

#include "common_gl.h"
#include "common_math.h"

#include "sti/sti.h"
#include "utilities.h"
#include "texture.h"


// for debugging
#include "dumpImage.h"








TextureAtlas* TextureAtlas_alloc(GlobalSettings* gs) {
	TextureAtlas* ta;
	pcalloc(ta);
	
	TextureAtlas_init(ta, gs);
	
	return ta;
}

void TextureAtlas_init(TextureAtlas* ta, GlobalSettings* gs) {
	VEC_INIT(&ta->sources);
	VEC_INIT(&ta->atlas);
	HT_init(&ta->items, 4);
}

void TextureAtlas_initGL(TextureAtlas* ta, GlobalSettings* gs) {
	
}



void TextureAtlas_addPNG(TextureAtlas* ta, char* name, char* path) {
	BitmapRGBA8 bmp;
	TextureAtlasSource* src;
	int ret;
	
	bmp.data = NULL;
	ret = readPNG2(path, &bmp);
	if(ret) {
		if(bmp.data) free(bmp.data);
		return;
	}
	
	
	pcalloc(src);
	src->name = strdup(name);
	
	src->size.x = bmp.width;
	src->size.y = bmp.height;
	src->aspectRatio = src->size.x / src->size.y;
	src->data = (uint8_t*)bmp.data;
	
	VEC_PUSH(&ta->sources, src);
}


// not the most efficient function. could use some optimization later.
void TextureAtlas_addFolder(TextureAtlas* ta, char* prefix, char* dirPath, int recursive) {
	DIR* d;
	struct dirent *dir;
	char* path;
	int prefixlen = strlen(prefix);
	
	d = opendir(dirPath);
	if(!d) return;
	
	while(dir = readdir(d)) {
		if(0 == strncmp(dir->d_name, ".", 1)) continue;
		if(0 == strncmp(dir->d_name, "..", 2)) continue;
		
		path = path_join(dirPath, dir->d_name);
		
		if(dir->d_type == DT_REG) { // regular file
			int namelen;
			char* ext = pathExt2(dir->d_name, &namelen);
			char* name = malloc(namelen + prefixlen + 2);
			strcpy(name, prefix);
			strcat(name, "/");
			strncat(name, dir->d_name, namelen);
		
			if(streq(ext, "png")) {
// 				printf("loading '%s' into atlas as '%s'\n", path, name);
				TextureAtlas_addPNG(ta, name, path);
			}
			
			free(name);
		}
		else if(recursive && dir->d_type == DT_DIR) {
			char* dirpre = path_join(prefix, dir->d_name);
			
			TextureAtlas_addFolder(ta, dirpre, path, 1);
			free(dirpre);
		}
		
		free(path);
	}
	
	closedir(d);
}





static void blit32(
	int src_x, int src_y, int dst_x, int dst_y, int w, int h,
	int src_w, int dst_w, uint32_t* src, uint32_t* dst) {
	
	
	int y, x, s, d;
	
	for(y = 0; y < h; y++) {
		for(x = 0; x < w; x++) {
			s = ((y + src_y) * src_w) + src_x + x;
			d = ((y + dst_y) * dst_w) + dst_x + x;
			
			dst[d] = src[s];
		}
	}
}


// sort tallest and widest first
static int source_sort_comp(const void* aa, const void * bb) {
	TextureAtlasSource* a = *((TextureAtlasSource**)aa);
	TextureAtlasSource* b = *((TextureAtlasSource**)bb);
	
	if(a->size.y == b->size.y) {
		return b->size.x - a->size.x;
	}
	else {
		return b->size.y - a->size.y;
	}
}

void TextureAtlas_finalize(TextureAtlas* ta) {
	char buf[32]; // for debugging
	
	if(VEC_LEN(&ta->sources) == 0) {
		printf("Texture atlas finalized without any sources\n");
		return;
	}
	
	VEC_SORT(&ta->sources, source_sort_comp);
	
	int width = ta->width;
	int width2 = width * width;
	float fwidth = width;
	
	uint32_t* texData = malloc(sizeof(*texData) * width2);
	memset(texData, 0, sizeof(*texData) * width2);
	
	
	int row = 0;
	int hext = 0;
	int prevhext = VEC_ITEM(&ta->sources, 0)->size.y;
	int rowWidth = 0;
	VEC_EACH(&ta->sources, ind, src) {
		
		if(rowWidth + src->size.x > width) {
			row++;
			rowWidth = 0;
			hext += prevhext;
			prevhext = src->size.y;
			
			// next texture
			if(hext + prevhext > width) { // the texture is square; width == height
				VEC_PUSH(&ta->atlas, texData);
				
// 				sprintf(buf, "texatlas-%d.png", VEC_LEN(&ta->atlas));
// 				writePNG(buf, 4, texData, width, width);
// 				
				texData = malloc(sizeof(*texData) * width2);
				memset(texData, 0, sizeof(*texData) * width2);
				
				hext = 0;
			}
		}
		
		// blit the sdf bitmap data
		blit32(
			0, 0, // src x and y offset for the image
			rowWidth, hext, // dst offset
			src->size.x, src->size.y, // width and height
			src->size.x, width, // src and dst row widths
			(uint32_t*)src->data, // source
			texData); // destination
		
		
		TextureAtlasItem* it = pcalloc(it);
		*it = (TextureAtlasItem){
			.offsetPx = {rowWidth, hext},
			.offsetNorm = {(float)rowWidth / fwidth, (float)hext / fwidth},
				
			.sizePx = src->size,
			.sizeNorm = {src->size.x / fwidth, src->size.y / fwidth},
				
			.index = VEC_LEN(&ta->atlas)
		};
		
// 		printf("added icon '%s'\n", src->name);
		HT_set(&ta->items, strdup(src->name), it);
		
		
		rowWidth += src->size.x;
		
		
// 		writePNG("sourceour.png", 4, src->data, src->size.x, src->size.y);
		
		free(src->name);
		free(src->data);
		free(src);
		
// 		break;
	}
	
// 	printf("last push %p\n", texData);
	VEC_PUSH(&ta->atlas, texData);
	
// 	sprintf(buf, "texatlas-%d.png", VEC_LEN(&ta->atlas));
// 	writePNG(buf, 4, texData, width, width);
}


// bump on format changes. there is no backward compatibility. saving is for caching only.
static uint16_t TEXTURE_ATLAS_FILE_VERSION = 0;

void TextureAtlas_saveAtlas(TextureAtlas* ta, char* path) {
	
}
int TextureAtlas_loadAtlas(TextureAtlas* ta, char* path) {
	return 0;
}


