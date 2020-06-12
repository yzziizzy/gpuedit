#ifndef __EACSMB_ui_formControl__
#define __EACSMB_ui_formControl__



enum {
	GUIFORMCONTROL_NONE = 0,
	GUIFORMCONTROL_STRING,
	GUIFORMCONTROL_INT,
	GUIFORMCONTROL_FLOAT,
	// checkbox
	// radio
	// select
	// textarea
};


typedef struct GUIFormControl {
	GUIHeader header;
	
	char* label;
	
	int type;
	union {
		GUIEdit* edit;
	};
	
} GUIFormControl;





GUIFormControl* GUIFormControl_New(GUIManager* gm, int type, char* label);

void GUIFormControl_SetString(GUIFormControl* w, char* str);


char* GUIFormControl_GetString(GUIFormControl* w);
double GUIFormControl_GetDouble(GUIFormControl* w);
int64_t GUIFormControl_GetInt(GUIFormControl* w);




#endif // __EACSMB_ui_formControl__
