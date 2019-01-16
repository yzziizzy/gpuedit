#ifndef __EACSMB_ui_configLoader_h__
#define __EACSMB_ui_configLoader_h__

#include "../c_json/json.h"



GUIObject* GUICL_CreateFromConfig(GUIManager* gm, json_value_t* cfg);

void GUICL_LoadChildren(GUIManager* gm, GUIHeader* parent, json_value_t* cfg);





#endif // __EACSMB_ui_configLoader_h__
