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




#endif // __EACSMB_ui_formControl__
