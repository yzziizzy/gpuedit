#ifndef __gputk_structAdjuster_h__
#define __gputk_structAdjuster_h__


/*
types:
	c: signed char
	s: signed short
	i: signed int
	l: signed long
	1: unsigned char
	2: unsigned short
	4: unsigned int
	8: unsigned long
	f: float
	d: double
	a: string
	p: pointer
	
TODO: enums, multichoice, color, vector
*/


typedef struct GUISA_Field {
	char* name;
	
	char type;
	int count; // for vectors
	
	void* base;
	ptrdiff_t offset;
	
	char* formatSuffix;
	
} GUISA_Field;



typedef struct GUIStructAdjuster {
	GUIHeader header;
	
	char* formatPrefix;
	
	GUIColumnLayout* column;
	VEC(GUIDebugAdjuster*) adjusters;
	
	void* target;
	VEC(GUISA_Field) fields;
	
} GUIStructAdjuster;







GUIStructAdjuster* GUIStructAdjuster_new(GUIManager* gm, void* target, GUISA_Field* fields);




#endif //__gputk_structAdjuster_h__
