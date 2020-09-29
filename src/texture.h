#ifndef __EACSMB_texture_h__
#define __EACSMB_texture_h__


#include "sti/sti.h"
#include "utilities.h"
#include "settings.h"



typedef struct {
	GLuint tex_id;
	short width;
	short height;
	
	char* name;
	
} Texture;

typedef struct {
	char* path;
	short width, height;
	uint32_t* data;
} BitmapRGBA8;


enum TextureDepth {
	TEXDEPTH_8,
	TEXDEPTH_16,
	TEXDEPTH_32,
	TEXDEPTH_FLOAT,
	TEXDEPTH_DOUBLE,
	TEXDEPTH_MAXVALUE
};


typedef struct TexBitmap {
	short channels;
	enum TextureDepth depth;
	unsigned int width, height;
	
	union {
		uint8_t* data8;
		uint16_t* data16;
		uint32_t* data32;
		float* fdata;
		double* ddata;
	};
	
} TexBitmap;







typedef struct FloatBitmap {
	unsigned int w, h;
	float* data;
} FloatBitmap;

typedef struct FloatTex {
	char channels;
	unsigned int w, h;
	FloatBitmap* bmps[4];
} FloatTex;






typedef struct TexArray {
	unsigned short width, height;
	int depth;
	
	GLuint tex_id;
} TexArray;


typedef struct TexEntry {
	
	Texture* tex;
	char* path;
	int refs;

	
} TexEntry;


typedef struct TextureManager {
	
	HT(int) texLookup;
	VEC(TexEntry) texEntries;
	
	
	Vector2i targetRes; // x,y dimensions of tex array
	
	GLuint tex_id; // id of tex array
	int depth; // frozen depth of the array
	int mipLevels;
	
} TextureManager;





Texture* loadDataTexture(unsigned char* data, short width, short height);

BitmapRGBA8* readPNG(char* path);
int readPNG2(char* path, BitmapRGBA8* bmp);

Texture* loadBitmapTexture(char* path);


TexArray* loadTexArray(char** files);


//Texture* Texture_acquireCustom(char* name);
Texture* Texture_acquirePath(char* path);
void Texture_release(Texture* tex);


void initTextures();

TexBitmap* TexBitmap_create(int w, int h, enum TextureDepth d, int channels); 



TextureManager* TextureManager_alloc();
void TextureManager_init(TextureManager* tm);
int TextureManager_reservePath(TextureManager* tm, char* path);
int TextureManager_loadAll(TextureManager* tm, Vector2i targetRes); 
 
	
	

int TexBitmap_pixelStride(TexBitmap* bmp);
int TexBitmap_componentSize(TexBitmap* bmp); 
void* TexBitmap_pixelPointer(TexBitmap* bmp, int x, int y);

void TexBitmap_sampleFloat(TexBitmap* bmp, int x, int y, float* out);

BitmapRGBA8* TexGen_Generate(char* source, Vector2i size); 

FloatBitmap* FloatBitmap_alloc(int width, int height);
FloatTex* FloatTex_alloc(int width, int height, int channels);
void FloatTex_free(FloatTex* ft);
FloatTex* FloatTex_similar(FloatTex* orig);
FloatTex* FloatTex_copy(FloatTex* orig);
float FloatTex_sample(FloatTex* ft, float xf, float yf, int channel);
float FloatTex_texelFetch(FloatTex* ft, int x, int y, int channel); 

BitmapRGBA8* FloatTex_ToRGBA8(FloatTex* ft);






// found in textureAtlas.c

typedef struct TextureAtlasItem {
	Vector2 offsetPx;
	Vector2 offsetNorm;
	
	Vector2 sizePx;
	Vector2 sizeNorm;
	
	int index;
	
} TextureAtlasItem;


// for building the atlas
typedef struct TextureAtlasSource {
	char* name;
	
	float aspectRatio;
	Vector2 size;
	uint8_t* data;
} TextureAtlasSource;


typedef struct TextureAtlas {
	HT(TextureAtlasItem*) items;
	
	int width;
	VEC(uint32_t*) atlas;
	
	
	VEC(TextureAtlasSource*) sources;
	
} TextureAtlas;


TextureAtlas* TextureAtlas_alloc(GlobalSettings* gs);
void TextureAtlas_init(TextureAtlas* ta, GlobalSettings* gs); 
void TextureAtlas_initGL(TextureAtlas* ta, GlobalSettings* gs); 

void TextureAtlas_addPNG(TextureAtlas* ta, char* name, char* path);
void TextureAtlas_addFolder(TextureAtlas* ta, char* prefix, char* dirPath, int recursive);
void TextureAtlas_finalize(TextureAtlas* ta);

#endif // __EACSMB_texture_h__
