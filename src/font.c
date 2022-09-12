



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


#include "log.h"
#include "utilities.h"
#include "font.h"


#define GEN_SIZE 1024
#define MAGNITUDE 4


static FT_Error (*_FT_Init_FreeType)(FT_Library *alibrary);
static FT_Error (*_FT_Set_Pixel_Sizes)(FT_Face face, FT_UInt pixel_width, FT_UInt pixel_height);
static FT_Error (*_FT_Load_Char)(FT_Face face, FT_ULong char_code, FT_Int32 load_flags);
static FT_Error (*_FT_New_Face)(FT_Library library, const char* filepathname, FT_Long face_index, FT_Face *aface);
static FT_Error (*_FT_Done_Face)(FT_Face aface);


static FontGen* addSDFChar(int magnitude, FT_Face* ff, int code, int fontSize, char bold, char italic);
static FontGen* addBmpChar(FT_Face* ff, int code, int fontSize, char bold, char italic);


// temp
static void addFont(FontManager* fm, char* name);

static void calc_sdf_data_size(FontGen* fg);
void sdfgen_new(FontGen* fg);

GUIFont* GUIFont_alloc(char* name);


FontManager* FontManager_alloc() {
	FontManager* fm;
	
	pcalloc(fm);
	HT_init(&fm->fonts, 4);
	fm->magnitude = MAGNITUDE;
	fm->maxAtlasSize = 512;

// BUG: split out gl init operations now
	
	
	return fm;
}


void FontManager_init(FontManager* fm, GUISettings* gs) {
	int i = 0;
	int atlas_dirty = 0;
	char* atlas_path = "./fonts.atlas";
	GUIFont* font;


	FontManager_loadAtlas(fm, atlas_path);

	// TODO: check bitmap sizes too
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
			L1("building font: %s\n", gs->fontList[i]);
			
			
			FontManager_AssertFont(fm, gs->fontList[i]);
//			FontManager_addFont(fm, gs->fontList[i], 1024);
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

GUIFont* FontManager_AssertFont(FontManager* fm, char* name) {
	GUIFont* f;
	
	if(HT_get(&fm->fonts, name, &f)) {
		
		f = GUIFont_alloc(name);
		f->empty = 1;
		f->sdfGenSize = GEN_SIZE;
		
		HT_set(&fm->fonts, name, f);
	}
	
	return f;


}


// returns the sub-font struct
GUIFont* FontManager_AssertBitmapSize(FontManager* fm, char* name, int size) {
	
	L1("sz: %d\n", size);
	if(size > 70 || size < 4) return NULL;
	uint64_t mask = (1ul << (size - 4));
	
	GUIFont* f = FontManager_AssertFont(fm, name);
	
	if(f->bitmapSizes & mask) {
		// find and return the correct bitmap font object
		VEC_EACH(&f->bitmapFonts, i, bf) {
			if(bf->bitmapSize == size) return bf;
		};
	}
	
	
	f->bitmapSizes |= mask;
	
	GUIFont* bf = GUIFont_alloc(name);
	bf->bitmapSize = size;
	bf->empty = 1;
	
	VEC_PUSH(&f->bitmapFonts, bf);
	
	return bf;
}


void FontManager_AssertDefaultCodeRange(FontManager* fm, int minCode, int maxCode) {
	VEC_INC(&fm->codeRanges);
	VEC_TAIL(&fm->codeRanges).min = minCode;
	VEC_TAIL(&fm->codeRanges).max = maxCode;
}

void FontManager_AssertCodeRange(FontManager* fm, char* name, int minCode, int maxCode) {
	
	GUIFont* f = FontManager_AssertFont(fm, name);
	
	VEC_INC(&f->codeRanges);
	VEC_TAIL(&f->codeRanges).min = minCode;
	VEC_TAIL(&f->codeRanges).max = maxCode;
}


// new font rendering info
//static char* defaultCharset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 `~!@#$%^&*()_+|-=\\{}[]:;<>?,./'\"";
//static char* defaultCharset = "7";
//static char* defaultCharset = "g0123456789.+-*/()";

// 16.16 fixed point to float conversion
static double f2f(int32_t i) {
	return ((double)(i) / 65536.0);
}

// 26.6 fixed point to float conversion
static double f2f26_6(int32_t i) {
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
		L1("Could not load libfreetype: %s\n", liberr);
		exit(1);
	}
	
	_FT_Init_FreeType = dlsym(lib, "FT_Init_FreeType");
	_FT_Set_Pixel_Sizes = dlsym(lib, "FT_Set_Pixel_Sizes");
	_FT_Load_Char = dlsym(lib, "FT_Load_Char");
	_FT_New_Face = dlsym(lib, "FT_New_Face");
	_FT_Done_Face = dlsym(lib, "FT_Done_Face");
	
	if(!ftLib) {
		err = _FT_Init_FreeType(&ftLib);
		if(err) {
			L1("Could not initialize FreeType library.\n");
			return;
		}
	}
}




static FontGen* addBmpChar(FT_Face* ff, int code, int fontSize, char bold, char italic) {
	FontGen* fg;
	FT_Error err;
	FT_GlyphSlot slot;
	pcalloc(fg);
	
	fg->code = code;
	fg->italic = italic;
	fg->bold = bold;
	fg->nominalRawSize = fontSize;
	fg->bitmap = 1;
	
	err = _FT_Set_Pixel_Sizes(*ff, 0, fontSize);
	if(err) {
		L1("Could not set pixel size to %dpx.\n", fontSize);
		free(fg);
		return NULL;
	}
	
	err = _FT_Load_Char(*ff, code, FT_LOAD_RENDER);
	slot = (*ff)->glyph;
	
	// draw character to freetype's internal buffer and copy it here
//	_FT_Load_Char(*ff, code, FT_LOAD_RENDER);
	
	fg->rawAdvance = f2f(slot->linearHoriAdvance); 
	fg->rawBearing.x = f2f26_6(slot->metrics.horiBearingX); 
	fg->rawBearing.y = f2f26_6(slot->metrics.horiBearingY); 
	
	fg->sdfGlyphSize.x = (slot->metrics.width >> 6); 
	fg->sdfGlyphSize.y = (slot->metrics.height >> 6); 
	
	fg->sdfGlyph = calloc(1, sizeof(*fg->sdfGlyph) * fg->sdfGlyphSize.x * fg->sdfGlyphSize.y);
	
	blit(
		0, 0, // src x and y offset for the image
		0, 0, // dst offset
		fg->sdfGlyphSize.x, fg->sdfGlyphSize.y, // width and height
		slot->bitmap.pitch, fg->sdfGlyphSize.x, // src and dst row widths
		slot->bitmap.buffer, // source
		fg->sdfGlyph); // destination
	
	// fills in sdfDataSize
//	calc_sdf_data_size(fg);
	
	fg->sdfBounds.min.x = 0;
	fg->sdfBounds.min.y = 0;
	fg->sdfBounds.max.x = fg->sdfGlyphSize.x;
	fg->sdfBounds.max.y = fg->sdfGlyphSize.y;
	fg->sdfDataSize = fg->sdfGlyphSize;

	/*
	c->texIndex = VEC_LEN(&fm->atlas);
	c->texelOffset.x = rowWidth;
	c->texelOffset.y = hext;
	c->texelSize = gen->sdfDataSize;
	c->texNormOffset.x = (float)rowWidth / (float)pot;
	c->texNormOffset.y = (float)hext / (float)pot;
	c->texNormSize.x = (float)gen->sdfDataSize.x / (float)pot;
	c->texNormSize.y = (float)gen->sdfDataSize.y / (float)pot;
	*/
	
	
		
	float bearing_o_x = fg->rawBearing.x;
	float bearing_o_y = fg->rawBearing.y;
	
	L4(" '%c' bearing_o: %f,%f\n", code, bearing_o_x, bearing_o_y);
	
	fg->charinfo.topLeftOffset.x = (bearing_o_x) /*- out_padding */+ fg->sdfBounds.min.x;
	fg->charinfo.topLeftOffset.y = (-bearing_o_y) /*- out_padding */+ fg->sdfBounds.min.y;
	       
	fg->charinfo.bottomRightOffset.x = fg->charinfo.topLeftOffset.x + (fg->sdfBounds.max.x - fg->sdfBounds.min.x);
	fg->charinfo.bottomRightOffset.y = fg->charinfo.topLeftOffset.y + (fg->sdfBounds.max.y - fg->sdfBounds.min.y);
	
	
	L4("[raw] tl: %f,%f, br: %f,%f\n", fg->charinfo.topLeftOffset.x, fg->charinfo.topLeftOffset.y, fg->charinfo.bottomRightOffset.x, fg->charinfo.bottomRightOffset.y);
	
	fg->charinfo.topLeftOffset.x /= (float)fg->nominalRawSize;
	fg->charinfo.topLeftOffset.y /= (float)fg->nominalRawSize;
	fg->charinfo.bottomRightOffset.x /= (float)fg->nominalRawSize;
	fg->charinfo.bottomRightOffset.y /= (float)fg->nominalRawSize;
	        
	fg->charinfo.advance = (float)fg->rawAdvance / (float)fg->nominalRawSize;
		
	
	
	return fg;
}

static FontGen* addSDFChar(int magnitude, FT_Face* ff, int code, int fontSize, char bold, char italic) {
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
		L1("Could not set pixel size to %dpx.\n", fontSize);
		free(fg);
		return NULL;
	}
	
	L5("[%c] FT pixel size set to %d\n", code, fontSize);
	
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
	
	fg->rawGlyphSize.x = (slot->metrics.width >> 6); 
	fg->rawGlyphSize.y = (slot->metrics.height >> 6); 
	L5("[%c] FT glyph size: %d,%d\n", code, fg->rawGlyphSize.x, fg->rawGlyphSize.y);
	
	fg->rawGlyph = calloc(1, sizeof(*fg->rawGlyph) * fg->rawGlyphSize.x * fg->rawGlyphSize.y);
	
	blit(
		0, 0, // src x and y offset for the image
		0, 0, // dst offset
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
		
		if(fg->bitmap) {
		
		}
		else {
			L5("calc: '%s':%d:%d %c\n", fg->font->name, fg->bold, fg->italic, fg->code);
			sdfgen_new(fg);
		}
	}
	
	
	pthread_exit(NULL);
}

void FontManager_finalize(FontManager* fm) {
	FT_Error err;
	FT_Face fontFace;
	
	checkFTlib();
	
	// HACK
	int bold = 0;
	int italic = 0;
	
	// sdf sizes
	HT_EACH(&fm->fonts, name, GUIFont*, f) {
		L2("Adding font '%s'\n", name);
		
		// load font face in Freetype
		char* fontPath = getFontFile2(name, bold, italic);
		if(!fontPath) {
			L1("Could not load font '%s'\n", name);
			return;
		}
		L5("font path: %s: %s\n", name, fontPath);
	
		err = _FT_New_Face(ftLib, fontPath, 0, &fontFace);
		if(err) {
			L1("Could not access font '%s' at '%s'.\n", name, fontPath);
			return;
		}
		
		// save some metrics
		f->ascender = (double)(fontFace->ascender) / (double)fontFace->units_per_EM;
		f->descender = (double)(fontFace->descender) / (double)fontFace->units_per_EM;
		f->height = (double)(fontFace->height) / (double)fontFace->units_per_EM;
		
		// default code ranges
		VEC_EACH(&fm->codeRanges, j, cr) {
			for(int code = cr.min; code <= cr.max; code++) {
				FontGen* fg = addSDFChar(fm->magnitude, &fontFace, code, f->sdfGenSize, bold, italic);
				fg->font = f;
				
				fm->maxRawSize.x = MAX(fm->maxRawSize.x, fg->rawGlyphSize.x);
				fm->maxRawSize.y = MAX(fm->maxRawSize.y, fg->rawGlyphSize.y);
				
				VEC_PUSH(&fm->gen, fg);
			}
		}
		
		// font-specific code ranges
		VEC_EACH(&f->codeRanges, j, cr) {
			for(int code = cr.min; code <= cr.max; code++) {
				FontGen* fg = addSDFChar(fm->magnitude, &fontFace, code, f->sdfGenSize, bold, italic);
				fg->font = f;
				
				fm->maxRawSize.x = MAX(fm->maxRawSize.x, fg->rawGlyphSize.x);
				fm->maxRawSize.y = MAX(fm->maxRawSize.y, fg->rawGlyphSize.y);
				
				VEC_PUSH(&fm->gen, fg);
			}
		}
		
		_FT_Done_Face(fontFace);
	}
	
	// start the sdf threads 
	int maxThreads = get_nprocs();
	pthread_t threads[maxThreads];
	
	for(int i = 0; i < maxThreads; i++) {
		int ret = pthread_create(&threads[i], NULL, sdf_thread, fm);
		if(ret) {
			L1("failed to spawn thread in FontManager\n");
			exit(1);
		}
	}
	
	// render the bitmap fonts in the meantime (FreeType is not MT-safe.)
	VEC(FontGen*) bmps;
	VEC_INIT(&bmps);
	
	HT_EACH(&fm->fonts, name, GUIFont*, f) {
			
		// load font face in Freetype
		char* fontPath = getFontFile2(name, bold, italic);
		if(!fontPath) {
			L1("Could not load font '%s'\n", name);
			return;
		}
		L5("font path: %s: %s\n", name, fontPath);
	
		err = _FT_New_Face(ftLib, fontPath, 0, &fontFace);
		if(err) {
			L1("Could not access font '%s' at '%s'.\n", name, fontPath);
			return;
		}
		
		VEC_EACH(&f->bitmapFonts, bfi, fbmp) {
		
			fbmp->ascender = 1.0;//f->ascender;//(double)(fontFace->ascender) / (double)fontFace->units_per_EM;
			fbmp->descender = (double)(fontFace->descender) / (double)fontFace->units_per_EM;
			fbmp->height = (double)(fontFace->height) / (double)fontFace->units_per_EM;
			
			
			int sz = fbmp->bitmapSize;
			L2("Generating bitmap font for %s, size %d\n", name, sz);
			
			// default code ranges
			VEC_EACH(&fm->codeRanges, j, cr) {
				for(int code = cr.min; code <= cr.max; code++) {
					FontGen* fg = addBmpChar(&fontFace, code, sz, bold, italic);
					fg->font = fbmp;
					
	//				fm->maxRawSize.x = MAX(fm->maxRawSize.x, fg->rawGlyphSize.x);
	//				fm->maxRawSize.y = MAX(fm->maxRawSize.y, fg->rawGlyphSize.y);
	
					
					
					VEC_PUSH(&bmps, fg);
				}
			}
			
			// font-specific code ranges
			VEC_EACH(&f->codeRanges, j, cr) {
				for(int code = cr.min; code <= cr.max; code++) {
					FontGen* fg = addBmpChar(&fontFace, code, sz, bold, italic);
					fg->font = fbmp;
					
	//				fm->maxRawSize.x = MAX(fm->maxRawSize.x, fg->rawGlyphSize.x);
	//				fm->maxRawSize.y = MAX(fm->maxRawSize.y, fg->rawGlyphSize.y);
					
					VEC_PUSH(&bmps, fg);
				}
			}
		}
		
		_FT_Done_Face(fontFace);
	
	}
	
	// wait for the sdf calculation to finish
	for(int i = 0; i < maxThreads; i++) {
		pthread_join(threads[i], NULL);
	}
	
	VEC_CAT(&fm->gen, &bmps);
	VEC_FREE(&bmps);

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
	f->sdfGenSize = GEN_SIZE;
	f->regular = calloc(1, sizeof(*f->regular) * f->charsLen);
	f->bold = calloc(1, sizeof(*f->bold) * f->charsLen);
	f->italic= calloc(1, sizeof(*f->italic) * f->charsLen);
	f->boldItalic = calloc(1, sizeof(*f->boldItalic) * f->charsLen);
	
	return f;
}




void FontManager_AddFontRange(FontManager* fm, char* name, char bold, char italic, int genSize) {
	GUIFont* f;
	FT_Error err;
	FT_Face fontFace;
	
	//defaultCharset = "I";
	
	//int len = strlen(defaultCharset);
	
	//int fontSize = 32; // pixels
	
	if(HT_get(&fm->fonts, name, &f)) {
		f = GUIFont_alloc(name);
		HT_set(&fm->fonts, name, f);
	}
	
	
	checkFTlib();
	
	// TODO: load font
	char* fontPath = getFontFile2(name, bold, italic);
	if(!fontPath) {
		L1("Could not load font '%s'\n", name);
		return;
	}
	L5("font path: %s: %s\n", name, fontPath);

	err = _FT_New_Face(ftLib, fontPath, 0, &fontFace);
	if(err) {
		L1("Could not access font '%s' at '%s'.\n", name, fontPath);
		return;
	}
	

//	ioRatio = floor(192.0 / 8.0); // HACK
	
	f->ascender = (double)(fontFace->ascender) / (double)fontFace->units_per_EM;
	f->descender = (double)(fontFace->descender) / (double)fontFace->units_per_EM;
	f->height = (double)(fontFace->height) / (double)fontFace->units_per_EM;
	/*
	for(int code = minCode; code <= maxCode; code++) {
// 		printf("calc: '%s':%d:%d %c\n", name, bold, italic, defaultCharset[i]);
		FontGen* fg = addChar(fm->magnitude, &fontFace, code, genSize, bold, italic);
		fg->font = f;
		
		fm->maxRawSize.x = MAX(fm->maxRawSize.x, fg->rawGlyphSize.x);
		fm->maxRawSize.y = MAX(fm->maxRawSize.y, fg->rawGlyphSize.y);
		
		VEC_PUSH(&fm->gen, fg);
	}
	*/
	_FT_Done_Face(fontFace);

}

void FontManager_addFont(FontManager* fm, char* name, int genSize) {
//	FontManager_addFont2(fm, name, 0, 0, genSize);
// 	FontManager_addFont2(fm, name, 1, 0); // DEBUG: temporarily disabled for testing metrics
// 	FontManager_addFont2(fm, name, 0, 1);
// 	FontManager_addFont2(fm, name, 1, 1);
}

void FontManager_createAtlas(FontManager* fm) {
	char buf[32];
	int padding = 1; // in pixels, on all sides
	
	
	// order the characters by height then width, tallest and widest first.
	VEC_SORT(&fm->gen, gen_comp);
	
	int totalWidth = 0;
	VEC_EACH(&fm->gen, ind, gen) {
	
//		printf("%c: h: %d, w: %d \n", gen->code, gen->sdfDataSize.y, gen->sdfDataSize.x);
		totalWidth += gen->sdfDataSize.x;
	}
	
	VEC_EACH(&fm->atlas, i, d) {
		free(d);
	}
	
	VEC_TRUNC(&fm->atlas);
	
	
	int maxHeight = VEC_ITEM(&fm->gen, 0)->sdfDataSize.y;
	int naiveSize = ceil(sqrt(maxHeight * totalWidth));
	int pot = nextPOT(naiveSize);
	int pot2 = naiveSize / 2;
	
	L5("naive min tex size: %d -> %d (%d)\n", naiveSize, pot, totalWidth);
	
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
//		if(gen->bitmap) continue;
		
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
}


// bump on format changes. there is no backward compatibility. saving is for caching only.
static uint16_t GUIFONT_ATLAS_FILE_VERSION = 70;

void FontManager_saveAtlas(FontManager* fm, char* path) {
	FILE* f;
	uint16_t u16;
	
	f = fopen(path, "wb");
	if(!f) {
		L1("Could not save font atlas to '%s'\n", path);
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
		fwrite(&font->ascender, 1, 8, f);
		fwrite(&font->descender, 1, 8, f);
		fwrite(&font->height, 1, 8, f);
		
		// number of bitmap fonts
		uint32_t nbfonts = VEC_LEN(&font->bitmapFonts);
		fwrite(&nbfonts, 1, 4, f);
		
		// bitmap bit field
		fwrite(&font->bitmapSizes, 1, 8, f);
		
		
		// number of charInfo structs
		uint32_t clen = font->charsLen;
		fwrite(&clen, 1, 4, f);
		
		// the charInfo structs
		fwrite(font->regular, 1, clen * sizeof(*font->regular), f);
		fwrite(font->bold, 1, clen * sizeof(*font->bold), f);
		fwrite(font->italic, 1, clen * sizeof(*font->italic), f);
		fwrite(font->boldItalic, 1, clen * sizeof(*font->boldItalic), f);
		
		
		
		VEC_EACH(&font->bitmapFonts, bfi, bfont) {
			fwrite("B", 1, 1, f);
						
			// font size
			fwrite(&bfont->bitmapSize, 1, 4, f);
			
			// name is inherited from parent
			
			// global metrics
			fwrite(&bfont->ascender, 1, 8, f);
			fwrite(&bfont->descender, 1, 8, f);
			fwrite(&bfont->height, 1, 8, f);
			
			// number of charInfo structs
			uint32_t clen = bfont->charsLen;
			fwrite(&clen, 1, 4, f);
			
			// the charInfo structs
			fwrite(bfont->regular, 1, clen * sizeof(*bfont->regular), f);
			fwrite(bfont->bold, 1, clen * sizeof(*bfont->bold), f);
			fwrite(bfont->italic, 1, clen * sizeof(*bfont->italic), f);
			fwrite(bfont->boldItalic, 1, clen * sizeof(*bfont->boldItalic), f);
		
		}
		
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
		L1("Could not open font atlas '%s'\n", path);
		return 1;
	}
	
	
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	
	// check the file version
	int r = fread(&u16, 1, 2, f);
	if(u16 != GUIFONT_ATLAS_FILE_VERSION) {
		L2("Font atlas file version mismatch. %d != %d, %d, '%s' \n", (int)u16, GUIFONT_ATLAS_FILE_VERSION, r, path);
		fclose(f);
		return 1;
	}
	
	while(!feof(f)) {
		// check the sigil sigil
		fread(&u8, 1, 1, f);
		
		if(u8 == 'F') {
			char* name;
			
			
			// name length and name string
			fread(&u16, 1, 2, f);
			name = malloc(u16 + 1);
			fread(name, 1, u16, f);
			name[u16] = 0;
			
			GUIFont* gf = FontManager_AssertFont(fm, name); 
			gf->sdfGenSize = GEN_SIZE;
			
			
			// global metrics
			fread(&gf->ascender, 1, 8, f);
			fread(&gf->descender, 1, 8, f);
			fread(&gf->height, 1, 8, f);
			
			L5("height: %lf, x14: %f\n", gf->height, gf->height * 14);
			L5("ascender: %lf, x14: %f\n", gf->ascender, gf->ascender * 14);
			L5("descender: %lf, x14: %f\n", gf->descender, gf->descender * 14);
			
			
			// number of bitmap fonts
			uint32_t nbfonts;
			fread(&nbfonts, 1, 4, f);
			L5("bitmap font count: %d\n", nbfonts);
			
		
			// bitmap bit field
			fread(&gf->bitmapSizes, 1, 8, f);
			
			
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
			
			for(int bfi = 0; bfi < nbfonts; bfi++) {
			
				// check the sigil sigil
				fread(&u8, 1, 1, f);
		
				if(u8 == 'B') {
					// fonst size
					fread(&u32, 1, 4, f);
					
					GUIFont* bf = FontManager_AssertBitmapSize(fm, name, u32);
					bf->bitmapSize = u32;
					bf->empty = 0;
					VEC_PUSH(&gf->bitmapFonts, bf);
					 
					// global metrics
					fread(&bf->ascender, 1, 8, f);
					fread(&bf->descender, 1, 8, f);
					fread(&bf->height, 1, 8, f);
					
					L5("height: %lf, x14: %f\n", bf->height, bf->height * 14);
					L5("ascender: %lf, x14: %f\n", bf->ascender, bf->ascender * 14);
					L5("descender: %lf, x14: %f\n", bf->descender, bf->descender * 14);
					
								// charInfo array length
					fread(&u32, 1, 4, f);
					bf->charsLen = u32;
					bf->regular = malloc(u32 * sizeof(*bf->regular));
					bf->bold = malloc(u32 * sizeof(*bf->bold));
					bf->italic = malloc(u32 * sizeof(*bf->italic));
					bf->boldItalic = malloc(u32 * sizeof(*bf->boldItalic));
					
					// charInfo structs
					fread(bf->regular, 1, u32 * sizeof(*bf->regular), f);
					fread(bf->bold, 1, u32 * sizeof(*bf->bold), f);
					fread(bf->italic, 1, u32 * sizeof(*bf->italic), f);
					fread(bf->boldItalic, 1, u32 * sizeof(*bf->boldItalic), f);
			
				}
				else {
					L4("Missing bitmap font #%d in %s.\n", bfi, name);
				}
			}
			
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
			
//			printf("atlas size: %d\n", u32);

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



static void calc_sdf_data_size(FontGen* fg) {
	unsigned char* output = fg->sdfGlyph;

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
//	printf("io_ratio: %d\n", io_ratio);
	
	
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
	
	
	for(int iy = 0; iy < in_size_y; iy++) {
		for(int ix = 0; ix < in_size_x; ix++) {
			
			int is_diff = 0;
			
			int p = INPX(ix, iy);
			
			int t_off_x = 0;
			int t_off_y = 0;
			
			
			int ym = iy > 0             ? INPX(ix, iy - 1) : 0;
			int yp = iy < in_size_y - 1 ? INPX(ix, iy + 1) : 0;
			int xm = ix > 0             ? INPX(ix - 1, iy) : 0;
			int xp = ix < in_size_x - 1 ? INPX(ix + 1, iy) : 0;
				
			do {
				if(p != ym) { is_diff = 1; t_off_y = -1; break; }
				if(p != yp) { is_diff = 1; t_off_y =  1; break; }
				if(p != xm) { is_diff = 1; t_off_x = -1; break; }
				if(p != xp) { is_diff = 1; t_off_x =  1; break; }
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
				float i_from_out_edge_x = ix + i_padding;  
				float i_from_out_edge_y = iy + i_padding;  
				
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
			}}
			
					
		}
		//break; // only one row
	}

	/*{
		static char buf[200];
		sprintf(buf, "new-sdf-output-%d.png", fg->code);
		writePNG(buf, 1, output, out_size_x, out_size_y);
	}//*/
	
	fg->sdfBounds.min.x = 0;
	fg->sdfBounds.min.y = 0;
	fg->sdfBounds.max.x = fg->sdfGlyphSize.x;
	fg->sdfBounds.max.y = fg->sdfGlyphSize.y;
	
	fg->sdfDataSize.x = fg->sdfBounds.max.x - fg->sdfBounds.min.x;
	fg->sdfDataSize.y = fg->sdfBounds.max.y - fg->sdfBounds.min.y;
	
//	calc_sdf_data_size(fg);
	
	free(fg->rawGlyph);
	
	float fontScaler = (float)out_mag / (float)fg->nominalRawSize; // the ratio of output pixels to nominal font pixels 
	
	
/*	


_ii  input pixels,  input coordinates
_io input pixels,  output coordinates
_oo  output pixels, output coordinates
_os output pixels, sdf coordinates
_if input pixels, font-origin coordinates
_of output pixels, font-origin coordinates

bearing_i: bearing metrics from FreeType, in input pixels (direct from Freetype)
padding_o: padding, in output pixels
padding_i = io_ratio * padding_o : padding, in input pixels


need:
	tl_off_of
	br_off_of

then:
	in_origin_oo = out_origin_oo + padding_o
	sdf_origin_oo = out_origin_oo + sdf_off_o
	
	bearing_o = bearing_i / io_ratio
	
	font_origin_oo.x = in_origin_oo.x - bearing_o.x
	font_origin_oo.y = in_origin_oo.y + bearing_o.y
	
	sdf_origin_of.x = out_origin_of.x + sdf_off.x
	
	out_origin_of.x = in_origin_of.x - padding_o
	
	in_origin_of.x = font_origin_of.x + bearing_o.x
	font_origin_of = font_origin_if / io_ratio
	
	
	tl_off_of = sdf_origin_of
	=
	tl_off_of.x = out_origin_of.x + sdf_off.x
	tl_off_of.y = out_origin_of.y + sdf_off.y
	=
	tl_off_of.x = in_origin_of.x - padding_o + sdf_off.x
	tl_off_of.y = in_origin_of.y - padding_o + sdf_off.y
	=
	tl_off_of.x = font_origin_of.x + bearing_o.x - padding_o + sdf_off.x
	tl_off_of.y = font_origin_of.y - bearing_o.y - padding_o + sdf_off.y
	=
	tl_off_of.x = (font_origin_if.x / io_ratio) + (bearing_i.x / io_ratio) - padding_o + sdf_off.x
	tl_off_of.y = (font_origin_if.y / io_ratio) - (bearing_i.y / io_ratio) - padding_o + sdf_off.y
	=
	tl_off_of.x = (bearing_i.x / io_ratio) - padding_o + sdf_off.x
	tl_off_of.y = (-bearing_i.y / io_ratio) - padding_o + sdf_off.y
	
	
	br_off_of = sdf_size_of - tl_off_of 
*/

	/*
	L5("fg->code: '%c'\n", fg->code);
	L5("IO ratio: %d\n", io_ratio);
	L5("in_size_x/y: %d,%d\n", in_size_x, in_size_y);
	L5("out_size_x/y: %d,%d\n", out_size_x, out_size_y);
	L5("padding_o: %d\n", out_padding);
	L5("fg->rawBearing.x/y: %f,%f\n", fg->rawBearing.x, fg->rawBearing.y);
	L5("fg->sdfBounds.min.x/y: %f,%f\n", fg->sdfBounds.min.x, fg->sdfBounds.min.y);
	L5("fg->sdfBounds.max.x/y: %f,%f\n", fg->sdfBounds.max.x, fg->sdfBounds.max.y);
	*/
	
	/*
	tl_off_of.x = (bearing_i.x / io_ratio) - padding_o + sdf_off.x
	tl_off_of.y = (-bearing_i.y / io_ratio) - padding_o + sdf_off.y

	br_off_os = tl_off_of + sdf_size_of        
	*/
	
	float bearing_o_x = (fg->rawBearing.x) / (float)io_ratio;
	float bearing_o_y = (fg->rawBearing.y) / (float)io_ratio;
	
//	L5("bearing_o: %f,%f\n", bearing_o_x, bearing_o_y);
	
	fg->charinfo.topLeftOffset.x = (bearing_o_x) - out_padding + fg->sdfBounds.min.x;
	fg->charinfo.topLeftOffset.y = (-bearing_o_y) - out_padding + fg->sdfBounds.min.y;
	       
	fg->charinfo.bottomRightOffset.x = fg->charinfo.topLeftOffset.x + (fg->sdfBounds.max.x - fg->sdfBounds.min.x);
	fg->charinfo.bottomRightOffset.y = fg->charinfo.topLeftOffset.y + (fg->sdfBounds.max.y - fg->sdfBounds.min.y);
	
	
//	L5("[raw] tl: %f,%f, br: %f,%f\n", fg->charinfo.topLeftOffset.x, fg->charinfo.topLeftOffset.y, fg->charinfo.bottomRightOffset.x, fg->charinfo.bottomRightOffset.y);
	
	fg->charinfo.topLeftOffset.x /= (float)fg->nominalRawSize / (float)io_ratio;
	fg->charinfo.topLeftOffset.y /= (float)fg->nominalRawSize / (float)io_ratio;
	fg->charinfo.bottomRightOffset.x /= (float)fg->nominalRawSize / (float)io_ratio;
	fg->charinfo.bottomRightOffset.y /= (float)fg->nominalRawSize / (float)io_ratio;
	        
	fg->charinfo.advance = (float)fg->rawAdvance / (float)fg->nominalRawSize;
		
//	L5("tl: %f,%f, br: %f,%f   adv: %f\n", fg->charinfo.topLeftOffset.x, fg->charinfo.topLeftOffset.y, fg->charinfo.bottomRightOffset.x, fg->charinfo.bottomRightOffset.y, fg->charinfo.advance);
//	L5("rawsz:%d, inputsz:%d,%d\n", fg->nominalRawSize, fg->rawGlyphSize.x, fg->rawGlyphSize.y);
//	
//	L5("time elapsed: %fms\n\n", (getCurrentTime() - start_time) * 1000);
}
















