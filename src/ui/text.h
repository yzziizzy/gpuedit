#ifndef __gputk_text_h__
#define __gputk_text_h__


typedef struct GUIText {
	GUIHeader header;
	
	char* currentStr;
	float fontSize;
	
	// align, height, width wrapping
	
	GUIFont* font;
	
} GUIText;



GUIText* GUIText_new(GUIManager* gm, char* str, char* fontname, float fontSize );

void GUIText_setString(GUIText* gt, char* newval);

float guiTextGetTextWidth(GUIText* gt, int numChars);



#endif // __gputk_text_h__
