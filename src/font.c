



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <dlfcn.h>
#include <pthread.h>
#include <sys/sysinfo.h>

// FontConfig
#include "fcfg.h"

// for sdf debugging
#include "dumpImage.h"


#include "utilities.h"
#include "font.h"


static FT_Error (*_FT_Init_FreeType)(FT_Library *alibrary);
static FT_Error (*_FT_Set_Pixel_Sizes)(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height);
static FT_Error (*_FT_Load_Char)(FT_Face face, FT_ULong char_code, FT_Int32 load_flags);
static FT_Error (*_FT_New_Face)(FT_Library library, const char* filepathname, FT_Long face_index, FT_Face *aface);


static FontGen* addChar(int magnitude, FT_Face* ff, int code, int fontSize, char bold, char italic);


// temp
static void addFont(FontManager* fm, char* name);


void sdfgen_new(FontGen* fg);



FontManager* FontManager_alloc(GUI_GlobalSettings* gs) {
	FontManager* fm;
	
	pcalloc(fm);
	HT_init(&fm->fonts, 4);
	fm->magnitude = 6;
	fm->maxAtlasSize = 512;

// BUG: split out gl init operations now
	FontManager_init(fm, gs);
	
	return fm;
}


void FontManager_init(FontManager* fm, GUI_GlobalSettings* gs) {
	int i = 0;
	int atlas_dirty = 0;
	char* atlas_path = "./fonts.atlas";
	GUIFont* font;

	FontManager_loadAtlas(fm, atlas_path);

	while(gs->fontList[i] != NULL) {
		// printf("checking font: %s\n", gs->Buffer_fontList[i]);
		if(HT_get(&fm->fonts, gs->fontList[i], &font)) {
			atlas_dirty = 1;
			break;
		}
		i++;
	}


	if(/*1 ||*/ atlas_dirty) {
		i = 0;
		while(gs->fontList[i] != NULL) {
			// printf("building font: %s\n", gs->Buffer_fontList[i]);
			FontManager_addFont(fm, gs->fontList[i], 1024);
			i++;
		}
		FontManager_finalize(fm);

		FontManager_createAtlas(fm);
		FontManager_saveAtlas(fm, atlas_path);
	}

	HT_get(&fm->fonts, "Arial", &fm->helv);
}




GUIFont* FontManager_findFont(FontManager* fm, char* name) {
	GUIFont* f;
	
	if(HT_get(&fm->fonts, name, &f)) {
		return fm->helv; // fallback
	}
	
	return f;
}







// new font rendering info
static char* defaultCharset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 `~!@#$%^&*()_+|-=\\{}[]:;<>?,./'\"";
//static char* defaultCharset = "Q";
//static char* defaultCharset = "g0123456789.+-*/()";

// 16.16 fixed point to float conversion
static float f2f(int32_t i) {
	return ((double)(i) / 65536.0);
}

// 26.6 fixed point to float conversion
static float f2f26_6(int32_t i) {
	return ((double)(i) / 64.0);
}


static void blit(
	int src_x, int src_y, int dst_x, int dst_y, int w, int h,
	int src_w, int dst_w, unsigned char* src, unsigned char* dst) {
	
	
	int y, x, s, d;
	
	// this may be upside down...
	for(y = 0; y < h; y++) {
		for(x = 0; x < w; x++) {
			s = ((y + src_y) * src_w) + src_x + x;
			d = ((y + dst_y) * dst_w) + dst_x + x;
			
			dst[d] = src[s];
		}
	}
}

static FT_Library ftLib = NULL;

static void checkFTlib() {
	FT_Error err;
	void* lib;
	char* liberr = NULL;
	
	dlerror();
	lib = dlopen("libfreetype.so", RTLD_LAZY | RTLD_GLOBAL);
	liberr = dlerror();
	if(liberr) {
		fprintf(stderr, "Could not load libfreetype: %s\n", liberr);
		exit(1);
	}
	
	_FT_Init_FreeType = dlsym(lib, "FT_Init_FreeType");
	_FT_Set_Pixel_Sizes = dlsym(lib, "FT_Set_Pixel_Sizes");
	_FT_Load_Char = dlsym(lib, "FT_Load_Char");
	_FT_New_Face = dlsym(lib, "FT_New_Face");
	
	if(!ftLib) {
		err = _FT_Init_FreeType(&ftLib);
		if(err) {
			fprintf(stderr, "Could not initialize FreeType library.\n");
			return;
		}
	}
}

/*
void gen_sdf_test_samples(char* fontName, int code) {
	printf("\n\n-------------------\nGenerating font samples\n-------------------\n\n");
				
	checkFTlib();
	
	ShaderProgram* test_shader = loadCombinedProgram("sdfTest");
	
	quad_info qd;
	mk_quad(&qd);			
	
	for(int size = 2048; size <= 2048; size += 4) {
		for(int bold = 0; bold <= 0; bold++) {
			for(int italic = 0; italic <= 0; italic++) {
		
				FT_Error err;
				FT_Face fontFace;
				
				char* fontPath = getFontFile2(fontName, bold, italic);
				if(!fontPath) {
					fprintf(stderr, "Could not load font '%s'\n", fontName);
					return;
				}
				printf("font path: %s: %s\n", fontName, fontPath);
				
				err = _FT_New_Face(ftLib, fontPath, 0, &fontFace);
				if(err) {
					fprintf(stderr, "Could not access font '%s' at '%s'.\n", fontName, fontPath);
					return;
				}
				
			
				
			//	f->ascender = fontFace->ascender >> 6;
			//	f->descender = fontFace->descender >> 6;
			//	f->height = fontFace->height >> 6;
				
				
				for(int magnitude = 2; magnitude < 8; magnitude += 2) {
					
					//
					// generate the single sdf image
					//
			
					gpu_calc_info gpu;
					
					
	//     		 printf("calc: '%s':%d:%d %c\n", name, bold, italic, defaultCharset[i]);
	//	  		gensize is the desired native font size 
					FontGen* fg = addChar(magnitude , &fontFace, code, size, bold, italic);
					//fg->font = f;
					
					{
						static char buf[200];
						sprintf(buf, "raw-glyph-%d-[%d]-%s%s.png", fg->code, size, bold?"b":"", italic?"i":"");
						writePNG(buf, 1, fg->rawGlyph, fg->rawGlyphSize.x, fg->rawGlyphSize.y);
					}
					
					
					
					sdfgen_new(fg);
					exit(2);
					
						
					init_gpu_sdf(&gpu, magnitude, fg->rawGlyphSize.x, fg->rawGlyphSize.y);
					
					printf("gpucalc: '%s':%d:%d %c\n", fontName, fg->bold, fg->italic, fg->code);
					CalcSDF_GPU(&gpu, fg);
					
					{
						static char buf[200];
						sprintf(buf, "sdf-test-%d-[%d-%d-%s%s].png", fg->code, size, magnitude, bold?"b":"", italic?"i":"");
						writePNG(buf, 1, fg->sdfGlyph, fg->sdfGlyphSize.x, fg->sdfGlyphSize.y);
					}
					
					destroy_gpu_sdf(&gpu);
					
					
					//
					// render some samples
					//
					
					// load the trimmed sdf texture
					GLuint rawID;
					glexit("");
					
					glGenTextures(1, &rawID);
					glBindTexture(GL_TEXTURE_2D, rawID);
					
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glexit("");
					
					glTexImage2D(GL_TEXTURE_2D, // target
						0,  // level, 0 = base, no minimap,
						GL_RED, // internalformat
						fg->rawGlyphSize.x,
						fg->rawGlyphSize.y,
						0,  // border
						GL_RED,  // format
						GL_UNSIGNED_BYTE, // input type
						fg->rawGlyph);
	
					
					for(int sampleSize = 8; sampleSize <= 8; sampleSize += 2) {
						for(int useSmooth = 0; useSmooth <= 1; useSmooth++) {
							for(float stepLow = 0.4; stepLow <= .7; stepLow += 0.1) {
								for(float stepHigh = 0.4; stepHigh <= .7; stepHigh += 0.1) {
						
									// the fbo
									fb_info fb;
									mk_fbo(&fb, fg->sdfGlyphSize.x, fg->sdfGlyphSize.y, GL_RED, GL_UNSIGNED_BYTE);
									
									// the shader
									glUseProgram(test_shader->id);
									glexit("test shading prog");
									
									// the quad
									glBindVertexArray(qd.vao);
									glBindBuffer(GL_ARRAY_BUFFER, qd.vbo);
									glEnableVertexAttribArray(0);
									glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, 0);
									glexit("");
									
									glClear(GL_COLOR_BUFFER_BIT);
									glexit("");
									
								
									
									
									glUniform2iv(glGetUniformLocation(gpu.shader->id, "outSize"), 1, (int*)&fg->sdfGlyphSize);
									printf("outSize: %d,%d\n", fg->sdfGlyphSize.x, fg->sdfGlyphSize.y);
									
									glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
									glexit("quad draw");
									
									// fetch the results. this call will flush the pipeline implicitly	
									fg->sdfGlyph = malloc(fg->sdfGlyphSize.x * fg->sdfGlyphSize.y * sizeof(*fg->sdfGlyph));
									glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
									glReadPixels(0,0,	
										fg->sdfGlyphSize.x,
										fg->sdfGlyphSize.y,
										GL_RED,
										GL_UNSIGNED_BYTE,
										fg->sdfGlyph
									);
											
					
									{
										static char buf[200];
										sprintf(buf, "render-%d-[%d-%d-%s%s]-%0.1f,%0.1f,%s.png", 
											fg->code, size, magnitude, bold?"b":"", italic?"i":"", 
											stepLow, stepHigh, useSmooth?"sm":"");
										writePNG(buf, 1, fg->sdfGlyph, fg->sdfGlyphSize.x, fg->sdfGlyphSize.y);
									}
					
								} // stepHigh
							} // stepLow
						} // useSmooth
					} // sampleSize
							
							
				} // magnitude
			} // italic
		} // bold
	} // size
	
}

*/








static FontGen* addChar(int magnitude, FT_Face* ff, int code, int fontSize, char bold, char italic) {
	FontGen* fg;
	FT_Error err;
	FT_GlyphSlot slot;
	
	pcalloc(fg);
	fg->code = code;
	fg->italic = italic;
	fg->bold = bold;
	fg->magnitude = magnitude;
	fg->nominalRawSize = fontSize;
	fg->ioRatio = floor(192.0 / (float)magnitude);
	
//	int rawSize = fontSize * fm->oversample;
	
	
	err = _FT_Set_Pixel_Sizes(*ff, 0, fontSize);
	if(err) {
		fprintf(stderr, "Could not set pixel size to %dpx.\n", fontSize);
		free(fg);
		return NULL;
	}
	
	printf("[%c] FT pixel size set to %d\n", code, fontSize);
	
	err = _FT_Load_Char(*ff, code, FT_LOAD_DEFAULT | FT_LOAD_MONOCHROME);
	
	//f2f(slot->metrics.horiBearingY);
	
	// draw character to freetype's internal buffer and copy it here
	_FT_Load_Char(*ff, code, FT_LOAD_RENDER /* | FT_LOAD_TARGET_MONO*/);
	// slot is a pointer
	slot = (*ff)->glyph;
	
	// typographic metrics for later. has nothing to do with sdf generation
	fg->rawAdvance = f2f(slot->linearHoriAdvance); 
	fg->rawBearing.x = f2f26_6(slot->metrics.horiBearingX); 
	fg->rawBearing.y = f2f26_6(slot->metrics.horiBearingY); 
	
	// back to sdf generation
	Vector2i rawImgSz = {(slot->metrics.width >> 6), (slot->metrics.height >> 6)};
	
	fg->rawGlyphSize.x = (slot->metrics.width >> 6) + 2; 
	fg->rawGlyphSize.y = (slot->metrics.height >> 6) + 2; 
	printf("[%c] FT glyph size: %d,%d\n", code, fg->rawGlyphSize.x, fg->rawGlyphSize.y);
	
	// the raw glyph is copied to the middle of a larger buffer to make the sdf algorithm simpler 
	fg->rawGlyph = calloc(1, sizeof(*fg->rawGlyph) * fg->rawGlyphSize.x * fg->rawGlyphSize.y);
	
	blit(
		0, 0, // src x and y offset for the image
		1, 1, // dst offset
		rawImgSz.x, rawImgSz.y, // width and height
		slot->bitmap.pitch, fg->rawGlyphSize.x, // src and dst row widths
		slot->bitmap.buffer, // source
		fg->rawGlyph); // destination
	
	
	return fg;
}


void* sdf_thread(void* _fm) {
	FontManager* fm = (FontManager*)_fm;
	
	int i = 0;
	while(1) {
		i = atomic_fetch_add(&fm->genCounter, 1);
		
		if(i >= VEC_LEN(&fm->gen)) break;
		
		FontGen* fg = VEC_ITEM(&fm->gen, i);
		printf("calc: '%s':%d:%d %c\n", fg->font->name, fg->bold, fg->italic, fg->code);

		sdfgen_new(fg);
		
	}
	
	
	pthread_exit(NULL);
}

void FontManager_finalize(FontManager* fm) {
	

	int maxThreads = get_nprocs();
	pthread_t threads[maxThreads];
	
	for(int i = 0; i < maxThreads; i++) {
		int ret = pthread_create(&threads[i], NULL, sdf_thread, fm);
		if(ret) {
			printf("failed to spawn thread in FontManager\n");
			exit(1);
		}
	}
	
	// wait for the work to get done
	for(int i = 0; i < maxThreads; i++) {
		pthread_join(threads[i], NULL);
	}

}


// sorting function for fontgen array
static int gen_comp(const void* aa, const void * bb) {
	FontGen* a = *((FontGen**)aa);
	FontGen* b = *((FontGen**)bb);
	
	if(a->sdfDataSize.y == b->sdfDataSize.y) {
		return b->sdfDataSize.x - a->sdfDataSize.x;
	}
	else {
		return b->sdfDataSize.y - a->sdfDataSize.y;
	}
}



GUIFont* GUIFont_alloc(char* name) {
	GUIFont* f;
	
	pcalloc(f);
	
	f->name = strdup(name);
	f->charsLen = 128;
	f->regular = calloc(1, sizeof(*f->regular) * f->charsLen);
	f->bold = calloc(1, sizeof(*f->bold) * f->charsLen);
	f->italic= calloc(1, sizeof(*f->italic) * f->charsLen);
	f->boldItalic = calloc(1, sizeof(*f->boldItalic) * f->charsLen);
	
	return f;
}



void FontManager_addFont2(FontManager* fm, char* name, char bold, char italic, int genSize) {
	GUIFont* f;
	FT_Error err;
	FT_Face fontFace;
	
	//defaultCharset = "I";
	
	int len = strlen(defaultCharset);
	
	//int fontSize = 32; // pixels
	
	checkFTlib();
	
	// TODO: load font
	char* fontPath = getFontFile2(name, bold, italic);
	if(!fontPath) {
		fprintf(stderr, "Could not load font '%s'\n", name);
		return;
	}
	printf("font path: %s: %s\n", name, fontPath);

	err = _FT_New_Face(ftLib, fontPath, 0, &fontFace);
	if(err) {
		fprintf(stderr, "Could not access font '%s' at '%s'.\n", name, fontPath);
		return;
	}
	
	if(HT_get(&fm->fonts, name, &f)) {
		f = GUIFont_alloc(name);
		HT_set(&fm->fonts, name, f);
	}
	
//	ioRatio = floor(192.0 / 8.0); // HACK
	
	f->ascender = (fontFace->ascender >> 6) / genSize;
	f->descender = fontFace->descender >> 6;
	f->height = fontFace->height >> 6;
	
	for(int i = 0; i < len; i++) {
// 		printf("calc: '%s':%d:%d %c\n", name, bold, italic, defaultCharset[i]);
		FontGen* fg = addChar(fm->magnitude, &fontFace, defaultCharset[i], genSize, bold, italic);
		fg->font = f;
		
		fm->maxRawSize.x = MAX(fm->maxRawSize.x, fg->rawGlyphSize.x);
		fm->maxRawSize.y = MAX(fm->maxRawSize.y, fg->rawGlyphSize.y);
		
		VEC_PUSH(&fm->gen, fg);
	}
	
	

}

void FontManager_addFont(FontManager* fm, char* name, int genSize) {
	FontManager_addFont2(fm, name, 0, 0, genSize);
// 	FontManager_addFont2(fm, name, 1, 0); // DEBUG: temporarily disabled for testing metrics
// 	FontManager_addFont2(fm, name, 0, 1);
// 	FontManager_addFont2(fm, name, 1, 1);
}

void FontManager_createAtlas(FontManager* fm) {
	char buf[32];
	
	// order the characters by height then width, tallest and widest first.
	VEC_SORT(&fm->gen, gen_comp);
	
	int totalWidth = 0;
	VEC_EACH(&fm->gen, ind, gen) {
//		printf("%c: h: %d, w: %d \n", gen->code, gen->sdfDataSize.y, gen->sdfDataSize.x);
		totalWidth += gen->sdfDataSize.x;
	}
	
	int maxHeight = VEC_ITEM(&fm->gen, 0)->sdfDataSize.y;
	int naiveSize = ceil(sqrt(maxHeight * totalWidth));
	int pot = nextPOT(naiveSize);
	int pot2 = naiveSize / 2;
	
	printf("naive min tex size: %d -> %d (%d)\n", naiveSize, pot, totalWidth);
	
	pot = MIN(pot, fm->maxAtlasSize);
	
	
	// test the packing
	int row = 0;
	int hext = maxHeight;
	int rowWidth = 0;
	
	// copy the chars into the atlas, cleaning as we go
	uint8_t* texData = malloc(sizeof(*texData) * pot * pot);
	memset(texData, 255, sizeof(*texData) * pot * pot);
	fm->atlasSize = pot;
	
	
	row = 0;
	hext = 0;
	int prevhext = maxHeight;
	rowWidth = 0;
	VEC_EACH(&fm->gen, ind, gen) {
		
		if(rowWidth + gen->sdfDataSize.x > pot) {
			row++;
			rowWidth = 0;
			hext += prevhext;
			prevhext = gen->sdfDataSize.y;
			
			// next texture
			if(hext + prevhext > pot) { 
				VEC_PUSH(&fm->atlas, texData);
				
				// disabled for debug
				//sprintf(buf, "sdf-comp-%ld.png", VEC_LEN(&fm->atlas));
				//writePNG(buf, 1, texData, pot, pot);
				
				
				texData = malloc(sizeof(*texData) * pot * pot);
				// make everything white, the "empty" value
				memset(texData, 255, sizeof(*texData) * pot * pot);
				
				hext = 0;
			}
		}
		
		// blit the sdf bitmap data
		blit(
			gen->sdfBounds.min.x, gen->sdfBounds.min.y, // src x and y offset for the image
			rowWidth, hext, // dst offset
			gen->sdfDataSize.x, gen->sdfDataSize.y, // width and height
			gen->sdfGlyphSize.x, pot, // src and dst row widths
			gen->sdfGlyph, // source
			texData); // destination
		
		
		// copy info over to font
		struct charInfo* c;
		if(gen->bold && gen->italic) c = &gen->font->boldItalic[gen->code];
		else if(gen->bold) c = &gen->font->bold[gen->code];
		else if(gen->italic) c = &gen->font->italic[gen->code];
		else c = &gen->font->regular[gen->code];
		
		c->texIndex = VEC_LEN(&fm->atlas);
		c->texelOffset.x = rowWidth;
		c->texelOffset.y = hext;
		c->texelSize = gen->sdfDataSize;
		c->texNormOffset.x = (float)rowWidth / (float)pot;
		c->texNormOffset.y = (float)hext / (float)pot;
		c->texNormSize.x = (float)gen->sdfDataSize.x / (float)pot;
		c->texNormSize.y = (float)gen->sdfDataSize.y / (float)pot;
		
		c->advance = gen->charinfo.advance;
		c->topLeftOffset.x = (gen->charinfo.topLeftOffset.x);// + (float)gen->sdfBounds.min.x;
		c->topLeftOffset.y = (gen->charinfo.topLeftOffset.y);// - (float)gen->sdfBounds.min.y;
		c->bottomRightOffset.x = (gen->charinfo.bottomRightOffset.x);// - (float)gen->sdfBounds.min.y;
		c->bottomRightOffset.y = (gen->charinfo.bottomRightOffset.y);// - (float)gen->sdfBounds.min.y;
		
		// TODO: fix		


//		c->genSize.y = gen->sdfDataSize.y;
		
//		printf("toff: %f, %f \n", c->texNormOffset.x, c->texNormOffset.y);
//		printf("tsize: %f, %f \n", c->texNormSize.x, c->texNormSize.y);
//		printf("ltoff: %f, %f \n", c->topLeftOffset.x, c->topLeftOffset.y);
		
		// advance the write offset
		rowWidth += gen->sdfDataSize.x;
		
		// clean up the FontGen struct
		free(gen->sdfGlyph);
		free(gen);
	}
	
	
	VEC_PUSH(&fm->atlas, texData);
	
	// disabled for debugging
	sprintf(buf, "sdf-comp-%ld.png", VEC_LEN(&fm->atlas));
	writePNG(buf, 1, texData, pot, pot);
	
	VEC_FREE(&fm->gen);
	
	
// 	writePNG("sdf-comp.png", 1, texData, pot, pot);

	
	//exit(1);
	
	
}


// bump on format changes. there is no backward compatibility. saving is for caching only.
static uint16_t GUIFONT_ATLAS_FILE_VERSION = 3;

void FontManager_saveAtlas(FontManager* fm, char* path) {
	FILE* f;
	uint16_t u16;
	
	f = fopen(path, "wb");
	if(!f) {
		fprintf(stderr, "Could not save font atlas to '%s'\n", path);
		return;
	}
	
	// write the file version
	fwrite(&GUIFONT_ATLAS_FILE_VERSION, 1, 2, f);
	
	HT_EACH(&fm->fonts, fName, GUIFont*, font) {
		// save the font
		// font identifier
		fwrite("F", 1, 1, f);
		
		// name length
		uint16_t nlen = strlen(font->name); 
		fwrite(&nlen, 1, 2, f);
		
		//name 
		fwrite(font->name, 1, nlen, f);
		
		// global metrics
		fwrite(&font->ascender, 1, 4, f);
		fwrite(&font->descender, 1, 4, f);
		fwrite(&font->height, 1, 4, f);
		
		// number of charInfo structs
		uint32_t clen = font->charsLen;
		fwrite(&clen, 1, 4, f);
		
		// the charInfo structs
		fwrite(font->regular, 1, clen * sizeof(*font->regular), f);
		fwrite(font->bold, 1, clen * sizeof(*font->bold), f);
		fwrite(font->italic, 1, clen * sizeof(*font->italic), f);
		fwrite(font->boldItalic, 1, clen * sizeof(*font->boldItalic), f);
	}
	
	// atlas identifier
	fwrite("A", 1, 1, f);
	
	// max atlas size
	fwrite(&fm->maxAtlasSize, 1, 4, f);
	
	// number of atlas layers
	u16 = VEC_LEN(&fm->atlas);
	fwrite(&u16, 1, 2, f);
	
	// atlas dimension (always square)
	fwrite(&fm->atlasSize, 1, 4, f);
	
	// atlas data
	VEC_EACH(&fm->atlas, ind, at) {
		fwrite(at, 1, fm->atlasSize * fm->atlasSize * sizeof(*at), f);
	}
	
	
	// done
	fclose(f);
}

int FontManager_loadAtlas(FontManager* fm, char* path) {
	FILE* f;
	
	f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "Could not open font atlas '%s'\n", path);
		return 1;
	}
	
	
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	
	// check the file version
	int r = fread(&u16, 1, 2, f);
	if(u16 != GUIFONT_ATLAS_FILE_VERSION) {
		printf("Font atlas file version mismatch. %d != %d, %d, '%s' \n", (int)u16, GUIFONT_ATLAS_FILE_VERSION, r, path);
		fclose(f);
		return 1;
	}
	
	while(!feof(f)) {
		// check the sigil sigil
		fread(&u8, 1, 1, f);
		
		if(u8 == 'F') {
			
			GUIFont* gf = calloc(1, sizeof(*gf)); 
			
			// name length and name string
			fread(&u16, 1, 2, f);
			gf->name = malloc(u16 + 1);
			fread(gf->name, 1, u16, f);
			gf->name[u16] = 0;
			
			HT_set(&fm->fonts, gf->name, gf);
			
			// global metrics
			fread(&gf->ascender, 1, 4, f);
			fread(&gf->descender, 1, 4, f);
			fread(&gf->height, 1, 4, f);
			
			// charInfo array length
			fread(&u32, 1, 4, f);
			gf->charsLen = u32;
			gf->regular = malloc(u32 * sizeof(*gf->regular));
			gf->bold = malloc(u32 * sizeof(*gf->bold));
			gf->italic = malloc(u32 * sizeof(*gf->italic));
			gf->boldItalic = malloc(u32 * sizeof(*gf->boldItalic));
			
			// charInfo structs
			fread(gf->regular, 1, u32 * sizeof(*gf->regular), f);
			fread(gf->bold, 1, u32 * sizeof(*gf->bold), f);
			fread(gf->italic, 1, u32 * sizeof(*gf->italic), f);
			fread(gf->boldItalic, 1, u32 * sizeof(*gf->boldItalic), f);
			
		}
		else if(u8 == 'A') { // atlas
			
			// max atlas size
			fread(&u32, 1, 4, f);
			fm->maxAtlasSize = u32;
			
			// number of layers
			fread(&u16, 1, 2, f);
			int layerNum = u16;
			
			// atlas dimension
			fread(&u32, 1, 4, f);
			fm->atlasSize = u32;
			
			printf("atlas size: %d\n", u32);

			// atlas data
			for(int i = 0; i < layerNum; i++) {
				uint8_t* at;
				
				at = malloc(u32 * u32 * sizeof(*at));
				fread(at, 1, u32 * u32 * 1, f);
				VEC_PUSH(&fm->atlas, at);
			}
			
			break;
		}
	}
	
	fclose(f);
	
	
	HT_get(&fm->fonts, "Arial", &fm->helv);
	
	return 0;
}






void sdfgen_new(FontGen* fg) {

	double start_time = getCurrentTime();

	char* input = fg->rawGlyph;
	int in_size_x = fg->rawGlyphSize.x, in_size_y = fg->rawGlyphSize.y; // size of the input image, in input pixels
//	printf("input size: %d,%d\n", in_size_x, in_size_y);


	//// ----- Init the Output -----

	// out_mag is the size the field is calculated away from the true edge,
	//   in output pixels
	int out_mag = fg->magnitude;

	// number of input pixels inside each output pixel
	int io_ratio = fg->ioRatio;
	printf("io_ratio: %d\n", io_ratio);
	
	
	// how big the core of the output needs to be, in real-valued output pixels
	float out_needed_xf = (float)in_size_x / io_ratio; 
	float out_needed_yf = (float)in_size_y / io_ratio; 
//	printf("out needed: %.2f,%.2f\n", out_needed_xf, out_needed_yf);
	
	int out_padding = out_mag;
	
	// size of the output image, in output pixels, including padding
	int out_size_x = ceil(out_needed_xf) + (out_padding * 2);
	int out_size_y = ceil(out_needed_yf) + (out_padding * 2);
//	printf("outsize: %d,%d\n", out_size_x, out_size_y);
	
	unsigned char* output = malloc(out_size_x * out_size_y * sizeof(output));
	fg->sdfGlyph = output;
	fg->sdfGlyphSize.x = out_size_x;
	fg->sdfGlyphSize.y = out_size_y;
	
	//// ----- Geometric Utilities -----
	
	// offset to the center of an output pixel from the edge
	float io_center_off = (float)io_ratio / 2.0;
	
	#define INPX(x, y)  input[((x) + ((y) * in_size_x))]
	#define OUTPX(x, y)  output[((x) + ((y) * out_size_x))]
	
	
	
	//// ----- Output Color Cache -----
	// The algorithm depends on whether the point being tested is the same color as the pixel under the 
	//   output pixel being calculated. We create a cache here to simply the lookup.
	//unsigned char* out_center_cache = calloc(1, out_size_x * out_size_y * sizeof(*out_center_cache));
	
	// TODO: only the core, non-padded area
	for(int oy = 0; oy < out_size_y; oy++) {
	for(int ox = 0; ox < out_size_x; ox++) {
		// fill the output image with the maximum distance
		OUTPX(ox, oy) = 0xff;
		
	}}
	
	
	
	// convert all input values to 0's and 1's
	for(int iy = 0; iy < in_size_y; iy++) {	
	for(int ix = 0; ix < in_size_x; ix++) {
		INPX(ix, iy) = !!INPX(ix, iy);
	}}
	
	//// ----- SDF Generation -----
	
	// Scan each input line looking for black pixels that touch white pixels.
	// When found, update output pixels with new min distance to an edge in
	// a radius around this point. In/out calculation is determined by the 
	// color of the pixel underneath the output pixel center. All distances
	// are to pixel centers.   
	
	
	// The input image has a 1px border to avoid bounds checking
	for(int iy = 1; iy < in_size_y - 1; iy++) {
	
		
		for(int ix = 1; ix < in_size_x - 1; ix++) {
			int is_diff = 0;
			
			int p = INPX(ix, iy);
			//if(p == 1) continue; // the center pixel should be the white (foreground) one
			
			int t_off_x = 0;
			int t_off_y = 0;
			
			
			do {
				if(p != INPX(ix, iy - 1)) { is_diff = 1; t_off_y = -1; break; }
				if(p != INPX(ix, iy + 1)) { is_diff = 1; t_off_y =  1; break; }
				if(p != INPX(ix - 1, iy)) { is_diff = 1; t_off_x = -1; break; }
				if(p != INPX(ix + 1, iy)) { is_diff = 1; t_off_x =  1; break; }
				
			} while(0);		
			if(!is_diff) continue;
			
			//printf("edge: %d,%d\n", iy, ix);
			//break;
		
			// there is an edge
			// update all the output pixels
			
			// output pixel that the found edge lies in
			int out_cx = floor(((float)ix / (float)io_ratio)/* + 0.5*/);
			int out_cy = floor(((float)iy / (float)io_ratio)/* + 0.5*/);
			//printf("outcxy: %d,%d\n", out_cx, out_cy);
			
			for(int oy = out_cy; oy < out_cy+2*out_mag; oy++) { // in output pixels
			for(int ox = out_cx; ox < out_cx+2*out_mag; ox++) {
				
				// The distance should be calculated in input pixels, to the center of the output pixel
				// The ouput image is padded, so it's possible for output pixels to have negative pixel
				//   coordinate values in the input image coordinate domain				
				
				// input pixels from the edge of the output image to the edge of the input image
				float i_padding = io_ratio * out_padding;
				
				// input pixels from the input edge to the current output tile's edge
				float i_cur_out_x = out_cx * io_ratio;
				float i_cur_out_y = out_cy * io_ratio;
				
				// input pixels from the output image's edge to the test pixel
				float i_from_out_edge_x = ix + i_padding/* + t_off_x*/;  
				float i_from_out_edge_y = iy + i_padding/* + t_off_y*/;  
				
				// input pixels from the output image's edge to the output cell center being updated
				float i_out_center_x = (ox) * io_ratio + io_center_off;
				float i_out_center_y = (oy) * io_ratio + io_center_off;
				
				
				// distance between the test pixel and the updating output pixel center 
				float dx = (i_out_center_x - i_from_out_edge_x);
				float dy = (i_out_center_y - i_from_out_edge_y);
				float d = sqrt(dx * dx + dy * dy);
				
				// normalize d
//				d = d > 191 ? 191;
				d = d / 192.0f;
				
							
				int existing = OUTPX(ox, oy);

				// test pixel is the one with the found edge
				// target pixel is the input pixel under the center of the output pixel being updated
				// p = test pixel color
				int p_target = 0; // everything in the padding area is black
				
				int i_real_ox = i_out_center_x - i_padding;
				int i_real_oy = i_out_center_y - i_padding;
				if(i_real_ox >=0 && i_real_oy >= 0 && i_real_ox < in_size_x && i_real_oy < in_size_y) { 
					p_target = INPX(i_real_ox, i_real_oy);
				}
				
				// output pixels that are inside the glyph should have values from 192 to 255
				// output pixels that are outside the glyph should have values from 0 to 191
				int clamped;
				
				
//				if(inside) norm = -norm;
			
//			o = (norm * 192) + 64;
			
//			return o < 0 ? 0 : (o > 255 ? 255 : o);
			
			
				
				if(p == 0) { // this is the background pixel next to an edge
					if(p_target != 0) { // target pixel is inside the glyph
						d = -d;
						int o = (d * 192) + 64;
						o = o < 0 ? 0 : (o > 255 ? 255 : o);
						
						if(o > existing || existing == 0xff) {
							
							OUTPX(ox, oy) = (unsigned char)o;
						}
						
//						OUTPX(ox, oy) = 0x00;
					}
				}
				else { // this is the foreground pixel next to an edge
					if(p_target == 0) { // target is outside the glyph
						int o = (d * 192) + 64;
						o = o < 0 ? 0 : (o > 255 ? 255 : o);
						
						if(o < existing) {
							
							OUTPX(ox, oy) = (unsigned char)o;
						}
						
//						OUTPX(ox, oy) = 0xff;
					}
					else { // target is inside the glyph
					
						d = -d;
						int o = (d * 192) + 64;
						o = o < 0 ? 0 : (o > 255 ? 255 : o);
						
						if(o > existing || existing == 0xff) {
							
							OUTPX(ox, oy) = (unsigned char)o;
						}
//						OUTPX(ox, oy) = 0x8f;
					
					}
				}
				
				
				
				
//				float norm = d / 192.0f;
//				float scaled = norm * 256;
//				int clamped = d > 192 ? 192 : d;
				
				
				// output encode the distance, overwrite if lower
				
				
				
				
				
//				if(p == p_target) {
			
//				}
	
				/*
				if(p == 1 && p == p_target) {
					
					
					//existing = 64 - existing;
					//printf("clamped:%d \n", clamped);
					if(clamped < existing) { 
						OUTPX(ox, oy) = clamped;
					}
					
				}
				else {
//					clamped += 64;	
					
					//clamped = d > 64 ? 64 : d; 
					//clamped = 64 - clamped;
					
					if(clamped < existing) { 
						//if(clamped < 64) printf("oops\n");
						OUTPX(ox, oy) = 64;//clamped;
					}
				}
				*/
	/*		
		static uint8_t sdfEncode(float d, int inside, float maxDist) {
			int o;
			d = sqrt(d);
			float norm = d / maxDist;
			if(inside) norm = -norm;
			
			o = (norm * 192) + 64;
			
			return o < 0 ? 0 : (o > 255 ? 255 : o);
		}
	*/							
									
				
			}}
			
					
		}
		//break; // only one row
	}

	{
		static char buf[200];
		sprintf(buf, "new-sdf-output-%d.png", fg->code);
		writePNG(buf, 1, output, out_size_x, out_size_y);
	}//*/
	
	
	
	
	
	// find the bounds of the sdf data
	// first rows
	for(int y = 0; y < fg->sdfGlyphSize.y; y++) {
		int hasData = 0;
		for(int x = 0; x < fg->sdfGlyphSize.x; x++) {
			hasData += output[x + (y * fg->sdfGlyphSize.x)];
		}
		
		if(hasData / fg->sdfGlyphSize.x < 255) {
			fg->sdfBounds.min.y = y;
			break;
		}
	}
	for(int y = fg->sdfGlyphSize.y - 1; y >= 0; y--) {
		int hasData = 0;
		for(int x = 0; x < fg->sdfGlyphSize.x; x++) {
			hasData += output[x + (y * fg->sdfGlyphSize.x)];
		}
		
		if(hasData / fg->sdfGlyphSize.x < 255) {
			fg->sdfBounds.max.y = y + 1;
			break;
		}
	}

	for(int x = 0; x < fg->sdfGlyphSize.x; x++) {
		int hasData = 0;
		for(int y = 0; y < fg->sdfGlyphSize.y; y++) {
			hasData += output[x + (y * fg->sdfGlyphSize.x)];
		}
		
		if(hasData / fg->sdfGlyphSize.y < 255) {
			fg->sdfBounds.min.x = x;
			break;
		}
	}
	for(int x = fg->sdfGlyphSize.x - 1; x >= 0; x++) {
		int hasData = 0;
		for(int y = 0; y < fg->sdfGlyphSize.y; y++) {
			hasData += output[x + (y * fg->sdfGlyphSize.x)];
		}
		
		if(hasData / fg->sdfGlyphSize.y < 255) {
			fg->sdfBounds.max.x = x + 1;
			break;
		}
	}
	
	fg->sdfDataSize.x = fg->sdfBounds.max.x - fg->sdfBounds.min.x;
	fg->sdfDataSize.y = fg->sdfBounds.max.y - fg->sdfBounds.min.y;
	
	free(fg->rawGlyph);
	
	float fontScaler = (float)out_mag / (float)fg->nominalRawSize; // the ratio of output pixels to nominal font pixels 
	
	/* // somehow broken
	
	// this is where to draw the quad relative to the current cursor position
	// it must subtract the empty margins and the sdf padding pixels 
	
	fg->charinfo.topLeftOffset.x = (fg->sdfBounds.min.x - out_padding) * fontScaler;
	fg->charinfo.topLeftOffset.y = (fg->sdfBounds.max.y - out_padding) * fontScaler;
	fg->charinfo.bottomRightOffset.x = fg->charinfo.topLeftOffset.x + (fg->sdfDataSize.x + out_padding + out_padding) * fontScaler;
	fg->charinfo.bottomRightOffset.y = fg->charinfo.topLeftOffset.y + (fg->sdfDataSize.y + out_padding + out_padding) * fontScaler;
	*/
	
	//fg->charinfo.topLeftOffset.x = 0;
	//fg->charinfo.topLeftOffset.y = 0;
//	fg->charinfo.bottomRightOffset.x = ((float)fg->sdfGlyphSize.x * (float)out_mag) / (float)fg->nominalRawSize;
//	fg->charinfo.bottomRightOffset.y = ((float)fg->sdfGlyphSize.y * (float)out_mag) / (float)fg->nominalRawSize;
	//fg->charinfo.bottomRightOffset.x = 1.0 * (float)fg->sdfDataSize.x / (float)fg->sdfDataSize.y;/// (float)fg->nominalRawSize;
	
	// if the input glyph was rendered at em 1.0, then:
	
	// size of the full output image in input pixels
	float i_out_size_x = out_size_x * io_ratio;
	float i_out_size_y = out_size_y * io_ratio;
	
	// amount of padding pixels in the output image per edge, in input pixels
	float i_padding = io_ratio * out_padding;
	
	// size of the interior portion of the output image, sans padding, in input pixels
	float i_out_interior_x = i_out_size_x - (i_padding * 2);
	float i_out_interior_y = i_out_size_y - (i_padding * 2);
	
	// the output image is usually slightly larger than the input image scaled down
	//   because the input image is seldom an exact multiple of io_ratio
	// this is the extra pixels due to the fraction
	float i_out_extra_x = i_out_interior_x - in_size_x;
	float i_out_extra_y = i_out_interior_y - in_size_y;
	
	
	// the real image edge is i_padding inward.
	// the data edge is inward more due to trimming empty pixels
	float i_out_data_edge_min_x = i_padding + fg->sdfBounds.min.x * io_ratio;
	float i_out_data_edge_min_y = i_padding + fg->sdfBounds.min.y * io_ratio;
	
	// the real image bounds is also inward, but including the extra fractional pixels
	float i_out_data_edge_max_x = i_padding + fg->sdfBounds.max.x * io_ratio;
	float i_out_data_edge_max_y = i_padding + fg->sdfBounds.max.y * io_ratio;
	
	// the size of the data, in input pixels
	float i_data_size_x = i_out_data_edge_max_x - i_out_data_edge_min_x;
	float i_data_size_y = i_out_data_edge_max_y - i_out_data_edge_min_y;
	
	// the glyph origin is:
	//   bearing_y down from the top of the input image 
	//   bearing_x left from the left edge of the input image 
	
	// the input and output images are y-down; 0,0 is the top left corner
	
	// origin location, in input pixels, relative to the input image's bottom left corner (opposite of the gl)
	// the input image has 1px of padding on all sides
	float ii_origin_x = -fg->rawBearing.x + 1;
	float ii_origin_y = in_size_y - fg->rawBearing.y + 1; 
	
	// origin location, in input pixels, relative to the output image's bottom left corner
	float io_origin_x = ii_origin_x + i_padding;
	float io_origin_y = ii_origin_y + i_padding;
	
	///// origin location, in input pixels, relative to the output image's top left corner
	//float io_origin_x = ii_origin_x + i_padding;
//	float io_origin_y = ii_origin_y + i_padding;
	
	
	// location of the top left corner of the output image relative to the origin
	float io_lt_from_origin_x = -io_origin_x;
	float io_lt_from_origin_y = io_origin_y - i_data_size_y; // hackily wrong, probably
	
	// location of the data's top left corner relative to the origin
	//float iod_tl_from_origin_x = ;
	
	printf("io_tl: %f,%f\n", io_lt_from_origin_x,io_lt_from_origin_y);
	
	
	
	fg->charinfo.topLeftOffset.x = io_lt_from_origin_x;
	fg->charinfo.topLeftOffset.y = io_lt_from_origin_y;
	
	fg->charinfo.bottomRightOffset.x = fg->charinfo.topLeftOffset.x + i_data_size_x;
	fg->charinfo.bottomRightOffset.y = fg->charinfo.topLeftOffset.y + i_data_size_y;
	
//	fg->charinfo.bottomRightOffset.y = 1.0;
	
	fg->charinfo.topLeftOffset.x /= (float)fg->nominalRawSize;
	fg->charinfo.topLeftOffset.y /= (float)fg->nominalRawSize;
	fg->charinfo.bottomRightOffset.x /= (float)fg->nominalRawSize;
	fg->charinfo.bottomRightOffset.y /= (float)fg->nominalRawSize;
	
	fg->charinfo.advance = (float)fg->rawAdvance / (float)fg->nominalRawSize;
	
	printf("tl: %f,%f   adv: %f\n",fg->charinfo.topLeftOffset.x, fg->charinfo.topLeftOffset.y, fg->charinfo.advance);
	printf("qsize: %f,%f  rawsz:%d, inputsz:%d,%d\n",fg->charinfo.bottomRightOffset.x, fg->charinfo.bottomRightOffset.y, fg->nominalRawSize, fg->rawGlyphSize.x, fg->rawGlyphSize.y);
	
	printf("time elapsed: %fms\n", (getCurrentTime() - start_time) * 1000);
}
















