#ifndef __EACSMB_font_h__
#define __EACSMB_font_h__

#include <stdatomic.h>

#include "common_math.h"
#include "shader.h"
#include "sti/sti.h"

#include "settings.h"


// here until factored better:
#include "ui/gui_settings.h"

#define popcount(x) __builtin_popcount(x)


#include <ft2build.h>
#include FT_FREETYPE_H


struct charInfo {
	uint32_t code; // unicode codepoint
	
	// final output texture coordinates
	int texIndex;
	Vector2i texelOffset; // from the top left
	Vector2i texelSize; // size of the character data in texels
	Vector2 texNormOffset; // normalized texture coordinates
	Vector2 texNormSize; 
	
	// The following metrics are all "normalized" to a 1px font. Multiply them by your desired font
	//   size in pixels, not 'points' or any other such archaic nonsense. 
	
	double advance; // horizontal distance to advance after this char
	Vector2 topLeftOffset; // offset from the baseline to the top left vertex of the *quad*
	Vector2 bottomRightOffset;
};



typedef struct GUIFont {
	
	char* name;
	char empty; // indicates is the font was merely requested but never filled in
	int bitmapSize; // only for bitmap fonts
	
	int charsLen;
	struct charInfo* regular;
	struct charInfo* italic;
	struct charInfo* bold;
	struct charInfo* boldItalic;
	
	double ascender;
	double descender;
	double height;
	// TODO: kerning info
	
	uint32_t bitmapSizes; // Bit field representing available bitmap sizes
	                      //  The first bit represents size 4px
	struct GUIFont* bitmapFonts[32];
	
	
	int sdfGenSize;
	VEC(struct {int min; int max;}) codeRanges;
	
} GUIFont;


typedef struct FontGenBMP {
	GUIFont* font;
	
	int code;
	char bold;
	char italic;
	float pxsize;
	
	uint8_t* glyph;
	Vector2i glyphSize; // size of the bitmap
	
	Vector3 bearing;
	Vector3 advance;
	
	// final texture data
	struct charInfo charinfo;
} FontGenBMP;



// generation info for a single character
typedef struct FontGen {
	GUIFont* font;
	
	int code;
	
	unsigned int italic : 1;
	unsigned int bold : 1;
	unsigned int bitmap : 1;
	
	// the raw glyph is oversampled by FontManager.oversample times
	int magnitude; // this is the maximum range of the Distance Field from the edge of
	               //   the glyph, measured in output pixels
	
	int nominalRawSize; // pixel size requested from FreeType for the raw image 
	
	float ioRatio; // how many raw pixels fit into an sdf pixel
	
	// metrics for the raw glyph, in pixels
	uint8_t* rawGlyph;
	Vector2i rawGlyphSize; // size of the raw bitmap

	Vector2 rawBearing; // distance from the origin to the top left corner of the glyph, in input pixels
	float rawAdvance; // horizontal advance, in input pixels
	
	// the sdf is smaller than the raw glyph
	
	// metrics for the sdf glyph, in pixels
	uint8_t* sdfGlyph;
	Vector2i sdfGlyphSize; // size of the sdf bitmap
	AABB2 sdfBounds; // bounding box of the non-empty data in the sdf bitmap, in pixels
	Vector2i sdfDataSize; // size of the non-empty sdf data in the bitmap
	
	
	// The following metrics are all "normalized" to a 1px font. Multiply them by your desired font
	//   size in pixels, not 'points' or any other such archaic nonsense. 
	Vector3 sdfBearing; // distance from the origin to the top left corner of the clipped sdf data
	float sdfAdvance; // horizontal advance, in pixels
	
	// final texture data
	struct charInfo charinfo;
} FontGen;


typedef struct FontManager {
	HT(GUIFont*) fonts;
	
	// SDF generation 
	VEC(FontGen*) gen;
	atomic_int genCounter;
	Vector2 maxRawSize;
	
	// SDF config
	int magnitude;
	
	int maxAtlasSize;
	VEC(uint8_t*) atlas;
	uint32_t atlasSize;
	
	
	// temp hacky stuff
	GUIFont* helv;
	
	VEC(struct {int min; int max;}) codeRanges;
} FontManager;


void FontManager_createAtlas(FontManager* fm);
void FontManager_saveAtlas(FontManager* fm, char* path);
int FontManager_loadAtlas(FontManager* fm, char* path);
void FontManager_addFont(FontManager* fm, char* name, int genSize);
void FontManager_addFont2(FontManager* fm, char* name, char bold, char italic, int genSize);
void FontManager_finalize(FontManager* fm);

GUIFont* FontManager_findFont(FontManager* fm, char* name);
GUIFont* FontManager_AssertFont(FontManager* fm, char* name);
GUIFont* FontManager_AssertBitmapSize(FontManager* fm, char* name, int size);
void FontManager_AssertCodeRange(FontManager* fm, char* name, int minCode, int maxCode);
void FontManager_AssertDefaultCodeRange(FontManager* fm, int minCode, int maxCode);

FontManager* FontManager_alloc();
void FontManager_init(FontManager* fm, GUISettings* gs);

void GUIFont_Destroy(GUIFont* f);

void gen_sdf_test_samples(char* fontName, int code);



#endif // __EACSMB_font_h__
