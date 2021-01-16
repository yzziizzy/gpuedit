#ifndef __gputk_formControl__
#define __gputk_formControl__



enum {
	GUIFORMCONTROL_NONE = 0,
	GUIFORMCONTROL_STRING,
	GUIFORMCONTROL_INT,
	GUIFORMCONTROL_FLOAT,
	GUIFORMCONTROL_SELECT,
	// checkbox
	// radio
	// textarea
};


typedef struct GUIFormControl {
	GUIHeader header;
	
	char* label;
	
	int type;
	union {
		GUIEdit* edit;
		GUISelectBox* select;
	};
	
} GUIFormControl;





GUIFormControl* GUIFormControl_New(GUIManager* gm, int type, char* label);

void GUIFormControl_SetString(GUIFormControl* w, char* str);


char* GUIFormControl_GetString(GUIFormControl* w);
double GUIFormControl_GetDouble(GUIFormControl* w);
int64_t GUIFormControl_GetInt(GUIFormControl* w);
void* GUIFormControl_GetData(GUIFormControl* w); // mostly for select boxes




#endif // __gputk_formControl__
