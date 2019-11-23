#ifndef __EACSMB_ui_structAdjuster_h__
#define __EACSMB_ui_structAdjuster_h__


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




#endif //__EACSMB_ui_structAdjuster_h__
