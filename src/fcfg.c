#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <fontconfig/fontconfig.h>

#include "fcfg.h"


static FcConfig* config = NULL;

static FcConfig* (*_FcInitLoadConfigAndFonts)();
static FcPattern* (*_FcNameParse)(const FcChar8 *name);
static FcBool (*_FcPatternAddInteger)(FcPattern *p, const char *object, int i);
static FcBool (*_FcConfigSubstitute)(FcConfig *config, FcPattern *p, FcMatchKind kind);
static void (*_FcDefaultSubstitute)(FcPattern *pattern);
static FcPattern* (*_FcFontMatch)(FcConfig *config, FcPattern *p, FcResult *result);
static FcResult (*_FcPatternGetString)(const FcPattern *p, const char *object, int n, FcChar8 ** s);
static void (*_FcPatternDestroy)(FcPattern *p);


void initFontConfig() {
	void* lib;
	char* liberr = NULL;
	
	dlerror();
	lib = dlopen("libfontconfig.so", RTLD_LAZY | RTLD_GLOBAL);
	liberr = dlerror();
	if(liberr) {
		fprintf(stderr, "Could not load libfontconfig: %s\n", liberr);
		exit(1);
	}
	
	_FcInitLoadConfigAndFonts = dlsym(lib, "FcInitLoadConfigAndFonts");
	_FcNameParse = dlsym(lib, "FcNameParse");
	_FcPatternAddInteger = dlsym(lib, "FcPatternAddInteger");
	_FcConfigSubstitute = dlsym(lib, "FcConfigSubstitute");
	_FcDefaultSubstitute = dlsym(lib, "FcDefaultSubstitute");
	_FcFontMatch = dlsym(lib, "FcFontMatch");
	_FcPatternGetString = dlsym(lib, "FcPatternGetString");
	_FcPatternDestroy = dlsym(lib, "FcPatternDestroy");
	
	config = _FcInitLoadConfigAndFonts();
}


char* getFontFile(char* fontName) {
	return getFontFile2(fontName, 0, 0);
}

char* getFontFile2(char* fontName, char bold, char italic) {
	FcPattern* pattern, *font;
	FcResult result;
	char* fileName = NULL;
	
	if(!config) initFontConfig();
	
	pattern = _FcNameParse((const FcChar8*)fontName);
	
	if(bold) _FcPatternAddInteger(pattern, "weight", 200);
	if(italic) _FcPatternAddInteger(pattern, "slant", 100);
	
	_FcConfigSubstitute(config, pattern, FcMatchPattern);
	_FcDefaultSubstitute(pattern);
	
	font = _FcFontMatch(config, pattern, &result);
	
	if(!(font && _FcPatternGetString(font, FC_FILE, 0, (FcChar8**)&fileName) == FcResultMatch)) {
		fprintf(stderr, "Could not find a font file for '%s'\n", fontName);
	}
	
	_FcPatternDestroy(pattern);
	
	return fileName;
}
