
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>
#include <limits.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "c3dlas/c3dlas.h"


#include "utilities.h"
#include "config.h"
#include "shader.h"
#include "window.h"
#include "app.h"





static XStuff xs;
static AppState app;
static InputState input;




int main(int argc, char* argv[]) {
	int first = 1;
	int configStatus = 0;
	char* wd;
	
	GlobalSettings_loadDefaults(&app.globalSettings);
// 	GlobalSettings_loadFromFile(&app.globalSettings, "assets/config/core.json");
	
	// init some path info. 
	wd = getcwd(NULL, 0);
// 	app.dataDir = pathJoin(wd, "data");

	free(wd);
	
	input.doubleClickTime = 0.200;
	input.dragMinDist = 4;
	
	memset(&xs, 0, sizeof(XStuff));
	clearInputState(&input);
	
	xs.targetMSAA = 4;
	xs.windowTitle = "gpuedit";
	
	initXWindow(&xs);
	
	
	initApp(&xs, &app);
	
	// main running loop
	while(1) {
		processEvents(&xs, &input, &app.ifs, -1);
		
		if(xs.ready) {
			initAppGL(&xs, &app);
		}
		else {
			appLoop(&xs, &app, &input);
		}
		
		if(app.frameSpan < 1.0/60.0) {
			// shitty estimation based on my machine's heuristics, needs improvement
			float sleeptime = (((1.0/60.0) * 1000000) - (app.frameSpan * 1000000)) * .7;
 			//printf("sleeptime: %f\n", sleeptime / 1000000);
			//sleeptime = 1000;
			if(sleeptime > 0) usleep(sleeptime); // problem... something is wrong in the math
		}
	}
	
	
	
	
	return 0;
}




 
 




 
 
