#ifndef __gputk_performanceGraph_h__
#define __gputk_performanceGraph_h__





typedef struct GUIPerformanceGraph {
	GUIHeader header;
	
	int length;
	float* times;
	float* normTimes;
	
	int cursor;
	
	Vector4 color, hcolor;
	
} GUIPerformanceGraph;




GUIPerformanceGraph* guiPerformanceGraphNew(GUIManager* gm, Vector2 size, float zIndex, int length);
void GUIPerformanceGraph_AddTime(GUIPerformanceGraph* w, float t);





#endif // __gputk_performanceGraph_h__
