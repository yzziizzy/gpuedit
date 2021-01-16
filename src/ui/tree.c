#include <stdlib.h>
#include <stdio.h>


#include "gui.h"
#include "gui_internal.h"



// static GUIHeader* hitTest(GUITreeControl* cl, Vector2 absTestPos);
static void updatePos(GUITreeControl* cl, GUIRenderParams* grp, PassFrameParams* pfp);




static GUITreeControlItem* GUITreeControlItem_New(GUIManager* gm) {
	GUITreeControlItem* tci;
	
	static struct gui_vtbl static_vt = {
// 		.UpdatePos = updateItemPos,
// 		.Render = ,
	};
	
	pcalloc(tci);
	gui_headerInit(&tci->header, gm, &static_vt, NULL);
	
	return tci;
}


GUITreeControl* GUITreeControl_New(GUIManager* gm) {
	GUITreeControl* tc;
	
	
	static struct gui_vtbl static_vt = {
		.UpdatePos = (void*)updatePos,
// 		.Render = ,
	};
	
	static InputEventHandler input_vt = {
		//.keyUp = builderKeyUp,
	};
	
	
	pcalloc(tc);
	
	gui_headerInit(&tc->header, gm, &static_vt, NULL);
// 	tc->header.input_vt = &input_vt;
	
	
	tc->root = GUITreeControlItem_New(gm);
	GUI_RegisterObject(tc, tc->root);
	
	return tc;
}







static void updateItemPos(
	GUITreeControlItem* it, 
	GUITreeControl* tc, 
	GUIRenderParams* grp, 
	PassFrameParams* pfp
) {
	GUIRenderParams grp2 = *grp;
	
	// draw the icon
// 	GUIHeader_updatePos(it->opener, &grp2, pfp);
	// TODO: calculate rotation
	
	// draw the label
	// TODO: move over for the label
	if(it->elem) GUIHeader_updatePos(it->elem, &grp2, pfp);
	
	if(it->isOpen) {
		// draw the kids
		
		// TODO: fix fencepost issue with spacing
		// TODO: figure out better phase for size calculation
		float total_h = 0.0;
		float max_w = 0.0;
		VEC_EACH(&it->kids, i, child) { 
			total_h += tc->spacing + child->header.size.y;
			max_w = fmax(max_w, child->header.size.x);
		}
		
		tc->header.size.y += total_h;
		tc->header.size.x = fmax(max_w, tc->header.size.x);
		
		Vector2 tl = {0, 0}; //gui_calcPosGrav(&tc->header, grp);
		
		
		// TODO needs to set the treeitem size
		
		// columnlayout works by spoofing the renderparams supplied to each child
		total_h = 0.0;
		VEC_EACH(&it->kids, i, child) { 
			total_h += tc->spacing;
			
			GUIRenderParams grp2 = {
				.clip = grp->clip,
				.size = child->header.size, // sized to the child to eliminate gravity 
				.offset = {
					.x = tl.x,
					.y = tl.y + total_h 
				},
				.baseZ = grp->baseZ + tc->header.z,
			};
			
			updateItemPos(child, tc, &grp2, pfp);
// 			GUIHeader_updatePos(child, &grp2, pfp);
			
			total_h += child->header.size.y;
		}
	}
	
	
}


static void updatePos(GUITreeControl* tc, GUIRenderParams* grp, PassFrameParams* pfp) {
	updateItemPos(tc->root, tc, grp, pfp);
}



GUITreeControlItem* GUITreeControl_Append(GUITreeControl* tc, GUITreeControlItem* parent, GUIHeader* o, char isOpen) {
	
	GUITreeControlItem* it = GUITreeControlItem_New(tc->header.gm);
	it->elem = o;
	it->isOpen = isOpen;
	
	if(!parent) parent = tc->root;
	
	VEC_PUSH(&parent->kids, it);
	
	GUI_RegisterObject(parent, it);
	GUIHeader_RegisterObject(&it->header, o);
	
	return it;
}


GUITreeControlItem* GUITreeControl_AppendLabel(GUITreeControl* tc, GUITreeControlItem* parent, char* text, char isOpen) {
	
	GUIText* gt = GUIText_new(tc->header.gm, text, "Arial", 6.0f);
	
	return GUITreeControl_Append(tc, parent, (GUIHeader*)gt, isOpen);
}
