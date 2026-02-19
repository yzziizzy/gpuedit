
#include "app.h"







static char const * const helpText = 
	"Usage: %s [options] [PATH]...\n"
	"Options:\n"
	"  --help                  Show this message then exit.\n"
	"\n"
	"  -c, --config [PATH]     Load PATH as a config file.\n"
	"  --disable-save          Do not save files being edited (debugging).\n"
	"  -f [PATH]               Open PATH in the text editor.\n"
	"  -h [PATH]               Open PATH in the hex editor.\n"
	"  -n                      Start a new session.\n"
	"  --no-sessions           Do not load or save sessions files.\n"
	"  -v[0-9]                 Log verbosity level.\n"
	"\n"
	"Anonymous [PATH] arguments will be probed for type and opened in an \n"
	"   appropriate editor. \n"
;


void AppState_ParseArgs(AppState* as, int argc, char* argv[]) {

// command line args
	for(int i = 1; i < argc; i++) {
		char* a = argv[i];
		
		// for debugging

		
		
		// look for files to load in arguments
		// -f works too
		if(a[0] == '-') {
			if(!strcmp(a, "--help")) {
				printf(helpText, argv[0]);
				exit(1);
			}
			else if(a[1] == 'f' && a[2] == 0) {
				i++;
				if(i < argc) VEC_push(&as->autoload, argv[i]);
			}
			
			else if(a[1] == 'h' && a[2] == 0) {
				i++;
				if(i < argc) VEC_push(&as->autoload_hex, argv[i]);
			}
				
			else if((a[1] == 'c' && a[2] == 0) || !strcmp(a, "--config")) {
				i++;
				if(i <= argc) {
					Settings_LoadFile(as->globalSettings, argv[i], SETTINGS_ALL);
				}
				
				as->suppress_config = 1;
			}
			
			else if(a[1] == 'v' && a[2] >= '0' && a[2] <= '9' && a[3] == 0) {
				g_log_verbosity_level = a[2] - '0';
			}
			else if(a[1] == 'n') {
				// do not load session file
				as->new_session = 1;
			}
			else if(!strcmp(a, "--no-sessions")) {
				// do not load session file
				as->no_sessions = 1;
			}
			else if(!strcmp(a, "--disable-save")) {
				printf("Buffer saving disabled.\n");
				g_DisableSave = 1;
			}
			
			
			continue;
		}
		
		VEC_push(&as->autoload, argv[i]);
	}
	

}
