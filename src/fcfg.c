#include <stdio.h>
#include <stdlib.h>
#include <fontconfig/fontconfig.h>

#include "fcfg.h"


static FcConfig* config = NULL;


void initFontConfig() {
	config = FcInitLoadConfigAndFonts();
}


char* getFontFile(char* fontName) {
	return getFontFile2(fontName, 0, 0);
}

char* getFontFile2(char* fontName, char bold, char italic) {
	FcPattern* pattern, *font;
	FcResult result;
	char* fileName = NULL;
	
	if(!config) initFontConfig();
	
	pattern = FcNameParse((const FcChar8*)fontName);
	
	if(bold) FcPatternAddInteger(pattern, "weight", 200);
	if(italic) FcPatternAddInteger(pattern, "slant", 100);
	
	FcConfigSubstitute(config, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);
	
	font = FcFontMatch(config, pattern, &result);
	
	if(!(font && FcPatternGetString(font, FC_FILE, 0, (FcChar8**)&fileName) == FcResultMatch)) {
		fprintf(stderr, "Could not find a font file for '%s'\n", fontName);
	}
	
	FcPatternDestroy(pattern);
	
	return fileName;
}
