#ifndef __gputk_tree_h__
#define __gputk_tree_h__




typedef struct GUITreeControlItem {
	GUIHeader header;
	
	GUIWindow* opener;
	
	GUIHeader* elem;
	int indentLevel;
	char isOpen;
	
	VEC(struct GUITreeControlItem*) kids;
} GUITreeControlItem;


typedef struct GUITreeControl {
	GUIHeader header;
	
	float spacing;
	
	GUITreeControlItem* root;
	
} GUITreeControl;



GUITreeControl* GUITreeControl_New(GUIManager* gm);

GUITreeControlItem* GUITreeControl_AppendLabel(GUITreeControl* tc, GUITreeControlItem* parent, char* text, char isOpen);
GUITreeControlItem* GUITreeControl_Append(GUITreeControl* tc, GUITreeControlItem* parent, GUIHeader* o, char isOpen);



#endif // __gputk_tree_h__
