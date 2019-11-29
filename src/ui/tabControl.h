
typedef struct GUITabControl {
	GUIHeader header;
	
	int currentIndex;
	float tabHeight;
	
} GUITabControl;



GUITabControl* GUITabControl_New(GUIManager* gm);

// returns the new current tab's contents
GUIObject* GUITabControl_NextTab(GUITabControl* w, char cyclic);
GUIObject* GUITabControl_PrevTab(GUITabControl* w, char cyclic);
GUIObject* GUITabControl_GoToTab(GUITabControl* w, int i);


