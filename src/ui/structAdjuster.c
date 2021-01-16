#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>


#include "gui.h"
#include "gui_internal.h"

static const char* getTypeFormat(char t) {
	switch(t) {
		case 'c': return "%d";
		case 's': return "%d";
		case 'i': return "%d";
		case 'l': return "%lld";
		
		case '1': return "%u";
		case '2': return "%u";
		case '4': return "%u";
		case '8': return "%llu";

		case 'f': return "%f";
		case 'd': return "%f";
		
		case 'p': return "%p"; // pointer
		
		case 'a': return "%s";
		
		default: return "<unknown type>";
	}
}


static void delete(GUIHeader* go) {
	GUIStructAdjuster* sa = (GUIStructAdjuster*)go;
	
	free(sa->formatPrefix);
	VEC_FREE(&sa->fields);
	
// 	VEC_EACH(&sa->adjusters, ai, a) {
// 		GUIHeader_delete(a);
// 	}
}



GUIStructAdjuster* GUIStructAdjuster_new(GUIManager* gm, void* target, GUISA_Field* fields) {
	GUIStructAdjuster* sa;
	
	static struct gui_vtbl static_vt = {
// 		.Render = render,
		.Delete = delete,
	};
	
	static InputEventHandler input_vt = {
// 		.keyUp = keyUp,
	};
	
	
	pcalloc(sa);
	gui_headerInit(&sa->header, gm, &static_vt, NULL);
// 	sa->header.input_vt = &input_vt;
	
	sa->target = target;
	sa->formatPrefix = strdup("%s: ");
	VEC_INIT(&sa->fields);
	
	// container for the field adjusters
	sa->column = GUIColumnLayout_new(gm, (Vector2){0,0}, 16, 0);
	GUI_RegisterObject(sa, sa->column);
	
	
	
	// walk the fields and initialiaze the adjusters
	for(int i = 0; fields[i].name; i++) {
		VEC_PUSH(&sa->fields, fields[i]);
		GUISA_Field* f = &VEC_TAIL(&sa->fields);
		
		int len;
		len = snprintf(NULL, 0, sa->formatPrefix, f->name);
		char* base = malloc(len+1);
		snprintf(base, len+1, sa->formatPrefix, f->name);
		
		char* fmt = strdup("");//strcatdup2(base, f->formatSuffix ? f->formatSuffix : getTypeFormat(f->type));
		GUIDebugAdjuster* da = GUIDebugAdjuster_new(gm, fmt, target + (ptrdiff_t)f->base + f->offset, f->type);
		free(fmt);
		free(base);
		
		VEC_PUSH(&sa->adjusters, da);
		GUI_RegisterObject(sa->column, da);
	}
	
	
	return sa;
}


