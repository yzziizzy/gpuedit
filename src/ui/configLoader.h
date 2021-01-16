#ifndef __gputk_configLoader_h__
#define __gputk_configLoader_h__




GUIHeader* GUICL_CreateFromConfig(GUIManager* gm, json_value_t* cfg);

void GUICL_LoadChildren(GUIManager* gm, GUIHeader* parent, json_value_t* cfg);





#endif // __gputk_configLoader_h__
