#ifndef __gpuedit_fileBrowser_h__
#define __gpuedit_fileBrowser_h__

#include "ui/gui.h"



// the full-functioning dialog contents
typedef struct GUIFileBrowser {
	GUIHeader header;
	
	unsigned int showPathBar     : 1;
	unsigned int editablePathBar : 1;
	unsigned int multipleSelect  : 1;
	unsigned int onlyExisting    : 1;
	unsigned int allowDirs       : 1;
	unsigned int onlyDirs        : 1;
	unsigned int showNewFolder   : 1;
	unsigned int showNewFile     : 1;
	unsigned int noEmptyAccept   : 1;
	
	GUIFileBrowserControl* fbc;
	GUIWindow* tray;
	GUIEdit* filenameBar;
	GUIButton* newDirBtn;
	GUIButton* newFileBtn;
	GUIButton* acceptBtn; // Save/Open/etc
	GUIButton* cancelBtn;
	
	char* curDir;
	
	GlobalSettings* gs;
	char* acceptButtonLabel;
	
	GUI_Cmd* commands;
	
} GUIFileBrowser;

GUIFileBrowser* GUIFileBrowser_New(GUIManager* gm, char* path);
void GUIFileBrowser_Destroy(GUIFileBrowser* w);
void GUIFileBrowser_UnselectAll(GUIFileBrowser* w);

void GUIFileBrowser_ProcessCommand(GUIFileBrowser* w, GUI_Cmd* cmd);

void GUIFileBrowser_Refresh(GUIFileBrowser* w);



#endif // __gpuedit_fileBrowser_h__
