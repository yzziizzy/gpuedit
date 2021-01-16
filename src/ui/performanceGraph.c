
#include "stdlib.h"
#include "string.h"



#include "gui.h"
#include "gui_internal.h"







void guiPerformanceGraphRender(GUIPerformanceGraph* w, PassFrameParams* pfp) {
	if(w->header.hidden || w->header.deleted) return;
	
	Vector2 tl = w->header.absTopLeft;
	
	float h = w->header.size.y;
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(w->header.gm, w->length);

	// scale and normalize the times
	float max = 0, min = 999999;
	for(int i = 0; i < w->length; i++) {
		max = fmax(w->times[i], max);
		min = fmin(w->times[i], min);
	}
	
	float off = min * (1.0 - 1.0/h);
	float scale = 1.0 / (max - off);
	for(int i = 0; i < w->length; i++) {
		w->normTimes[i] = (w->times[i] - off) * scale;
	}
	
	// regular and cursor colors
	struct Color4 ac = {w->color.x * 255, w->color.y * 255, w->color.z * 255, 255};
	struct Color4 bc = {w->hcolor.x * 255, w->hcolor.y * 255, w->hcolor.z * 255, 255};
	
	for(int i = 0; i < w->length; i++) {
		
		float ih = h - (h * w->normTimes[i]);
		Color4 cc = i == w->cursor ? bc : ac;
		
		*v = (GUIUnifiedVertex){
			.pos = {tl.x + 2 * i, tl.y + ih,
				tl.x + 2 * i + 2, tl.y + 20},
			.clip = {0, 0, 800, 800},
			
			.texIndex1 = 0,
			.texIndex2 = 0,
			.texFade = .5,
			.guiType = 0, // window (just a box)
			
			.texOffset1 = 0,
			.texOffset2 = 0,
			.texSize1 = 0,
			.texSize2 = 0,
			
			.fg = {0, 0, 0, 0},
			.bg = GUI_COLOR4_TO_SHADER(cc),
			
			.z = w->header.z,
			.alpha = w->header.alpha,
		};
		
		v++;
	}
	
	
	
}

void guiPerformanceGraphDelete(GUIPerformanceGraph* sw) {
	free(sw->times);
	free(sw->normTimes);
}


Vector2 guiPerformanceGraphGetClientSize(GUIPerformanceGraph* go) {
	return (Vector2){go->length*2, 20};
}


void GUIPerformanceGraph_AddTime(GUIPerformanceGraph* w, float t) {
	w->times[w->cursor] = t;
	w->cursor = (w->cursor + 1) % w->length;
}





GUIPerformanceGraph* guiPerformanceGraphNew(GUIManager* gm, Vector2 size, float zIndex, int length) {
	
	GUIPerformanceGraph* sw;

	static struct gui_vtbl static_vt = {
		.Render = (void*)guiPerformanceGraphRender,
		.Delete = (void*)guiPerformanceGraphDelete,
		.GetClientSize = (void*)guiPerformanceGraphGetClientSize,
// 		.SetClientSize = guiPerformanceGraphSetClientSize,
	};
	
	
	sw = calloc(1, sizeof(*sw));
	CHECK_OOM(sw);
	
	gui_headerInit(&sw->header, gm, &static_vt, NULL);
	
	sw->header.size = (Vector2){0, 20};
	
	sw->color = (Vector4){0.1, 0.9, 0.1, 1.0};
	sw->hcolor = (Vector4){0.9, 0.9, 0.1, 1.0};
	
	sw->times = calloc(1, sizeof(*sw->times) * length);
	sw->normTimes = calloc(1, sizeof(*sw->times) * length);
	sw->length = length;
	
	
// 	for(int i = 0; i < length; i++) {
// 		sw->times[i] = frandNorm() * 3;
// 	}
	
	return sw;
}








