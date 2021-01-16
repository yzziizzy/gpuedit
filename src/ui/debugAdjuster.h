#ifndef __gputk_debugAdjuster_h__
#define __gputk_debugAdjuster_h__



typedef struct GUIDebugAdjuster {
	GUIHeader header;
	
	GUIText* text;
	
	char* format;
	void* target;
	char type;
	
	char* buffer;
	size_t bufferLen;
	
	struct {
		float f;
		double d;
		int64_t i;
	} increments[2];
	
	struct {
		double d;
		int64_t i;
		int64_t u;
	} min, max;
	
} GUIDebugAdjuster;



GUIDebugAdjuster* GUIDebugAdjuster_new(GUIManager* gm, char* format, void* target, char type);

static void GUIDebugAdjuster_setMinMaxd(GUIDebugAdjuster* da, double min, double max) {
	da->min.d = min;
	da->max.d = max;
}

static void GUIDebugAdjuster_setMinMaxi(GUIDebugAdjuster* da, int64_t min, int64_t max) {
	da->min.i = min;
	da->max.i = max;
}

static void GUIDebugAdjuster_setMinMaxu(GUIDebugAdjuster* da, uint64_t min, uint64_t max) {
	da->min.u = min;
	da->max.u = max;
}



#endif // __gputk_debugAdjuster_h__
