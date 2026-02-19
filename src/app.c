
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>
#include <termios.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "c3dlas/c3dlas.h"
#include "c_json/json.h"
#include "json_gl.h"

#include "sti/sti.h"

#include "utilities.h"
#include "shader.h"
#include "texture.h"
#include "app.h"



GLuint proj_ul, view_ul, model_ul;


int g_DisableSave = 0; // debug flag to disable saving

RenderPipeline* rpipe;


// in renderLoop.c, a temporary factoring before a proper renderer is designed
void drawFrame(XStuff* xs, AppState* as, InputState* is);
void setupFBOs(AppState* as, int resized);


// MapBlock* map;
// TerrainBlock* terrain;

void resize_callback(XStuff* xs, void* gm_) {
	GUIManager* gm = (GUIManager*)gm_;
	
	/*
	GUIEvent gev = {
		.type = GUIEVENT_ParentResize,
		.size = {.x = xs->winSize.x, .y = xs->winSize.y},
		.originalTarget = gm->root,
	};
	*/
//	GUIHeader_TriggerEvent(gm->root, &gev);
}

static struct child_process_info* cc;

static int_vlist* bookmark_lines_from_json(json_value_t* lines) {
	if(!lines) return NULL;
	if(lines->type != JSON_TYPE_ARRAY) return NULL;
	
	int_vlist* out = pcalloc(out);
	VEC_init(out);
	
	json_link_t* link = lines->arr.head;
	json_value_t* l = NULL;
	for(;link; link = link->next) {
		VEC_push(out, json_as_int(link->v));
	}
	
	return out;
}

// nothing in here can use opengl at all.
void AppState_Init(AppState* as, int argc, char* argv[]) {
	srand((unsigned int)time(NULL));

	int suppress_config = 0;
	char* homedir = getenv("HOME");
	char* curdir = getenv("PWD");
	
	VEC(char*) autoload;
	VEC_INIT(&autoload);
	VEC(char*) autoload_hex;
	VEC_INIT(&autoload_hex);
	
	as->gui = GUIManager_alloc();

	
	as->globalSettings = calloc(1, sizeof(*as->globalSettings));
	
	#define RS(name, key, data) \
		Settings_RegisterSection(as->globalSettings, SETTINGS_##name, key, data, (void*)name##Settings_Alloc, (void*)name##Settings_Copy, (void*)name##Settings_Free, (void*)name##Settings_LoadDefaults, (void*)name##Settings_LoadJSON);
	
	RS(GUI, "gui", as->gui)
	RS(Buffer, "buffer", NULL)
	RS(Theme, "theme", NULL)
	RS(General, "general", NULL)
			
	Settings_LoadDefaults(as->globalSettings, SETTINGS_ALL);
	
	int new_session = 0;
	int no_sessions = 0;
	
	// command line args
	for(int i = 1; i < argc; i++) {
		char* a = argv[i];
		
		// for debugging
		if(0 == strcmp(a, "--disable-save")) {
			printf("Buffer saving disabled.\n");
			g_DisableSave = 1;
		}
		
		
		// look for files to load in arguments
		// -f works too
		if(a[0] == '-') {
			if(a[1] == 'f' && a[2] == 0) {
				i++;
				if(i < argc) VEC_PUSH(&autoload, argv[i]);
			}
			
			else if(a[1] == 'h' && a[2] == 0) {
				i++;
				if(i < argc) VEC_PUSH(&autoload_hex, argv[i]);
			}
				
			else if((a[1] == 'c' && a[2] == 0) || !strcmp(a, "--config")) {
				i++;
				if(i <= argc) {
					Settings_LoadFile(as->globalSettings, argv[i], SETTINGS_ALL);
				}
				
				suppress_config = 1;
			}
			
			else if(a[1] == 'v' && a[2] >= '0' && a[2] <= '9' && a[3] == 0) {
				g_log_verbosity_level = a[2] - '0';
			}
			else if(a[1] == 'n') {
				// do not load session file
				new_session = 1;
			}
			else if(!strcmp(a, "--no-sessions")) {
				// do not load session file
				no_sessions = 1;
			}
			
			continue;
		}
		
		VEC_PUSH(&autoload, argv[i]);
	}
	
	
	
	if(!suppress_config) {
		Settings_ReadAllJSONAt(as->globalSettings, "/etc/gpuedit/", SETTINGS_ALL);
		
		Settings_ReadDefaultFilesAt(as->globalSettings, homedir, SETTINGS_ALL);
		Settings_ReadDefaultFilesAt(as->globalSettings, curdir, SETTINGS_ALL);
	}
	
	as->gs = Settings_GetSection(as->globalSettings, SETTINGS_General);
	
//	printf("err path: %s\n", as->gs->gccErrorJSONPath);
	
	if(no_sessions) as->gs->enableSessions = 0;
	
	ThemeSettings* theme = Settings_GetSection(as->globalSettings, SETTINGS_Theme);
	char* tmp = path_join(homedir, "/.gpuedit/themes/", as->gs->Theme_path);
	Settings_LoadFile(as->globalSettings, tmp, SETTINGS_ALL);
	free(tmp);


	BufferSettings* bs = Settings_GetSection(as->globalSettings, SETTINGS_Buffer);
	FontManager_AssertBitmapSize(as->gui->fm, bs->font, bs->fontSize);
	
	
	FontManager_AssertDefaultCodeRange(as->gui->fm, ' ', '~'); // all of printable 7-bit ascii
	
	

	GUISettings* guiSettings = Settings_GetSection(as->globalSettings, SETTINGS_GUI);
	GUIManager_Init(as->gui, guiSettings);
	
	CommandList_loadJSONFile(as->gui, as->gs->commandsPath);
	
	
	as->bufferCache = BufferCache_New();
	
	
	as->mc = MainControl_New(as->gui, as->globalSettings);
	as->mc->as = as;
	as->mc->gm = as->gui;
	as->mc->bufferCache = as->bufferCache;
	as->gui->renderRootData = as->mc;
	as->gui->renderRootFn = (void*)MainControl_Render;
	
	
	VEC_EACH(&autoload, i, file) {
		MainControl_LoadFile(as->mc, file);
	}
	
	VEC_FREE(&autoload);
	
	
	VEC_EACH(&autoload_hex, i, file) {
		MainControl_Hexedit(as->mc, file);
	}
	
	VEC_FREE(&autoload_hex);
	
//	MainControl_LoadFile(as->mc, "testfile.h");
//	MainControl_LoadFile(as->mc, "testfile.c");
	
	
	json_file_t* jsf = NULL;
	char* session_file_path = "./.gpuedit.session";
	if(!new_session) jsf = json_load_path(session_file_path);
	if(jsf && jsf->error) {
		L_ERROR("error while loading config file: '%s'\n", session_file_path);
		L_ERROR("json error: %s %ld:%ld\n", jsf->error_str, jsf->error_line_num, jsf->error_char_num);
		L_ERROR("session file exists, but continuing without session\n");
	} else if(jsf) {
		
		json_value_t* paneLayout = json_obj_get_val(jsf->root, "paneLayout");
		int xdivs = json_obj_get_int(paneLayout, "x", 1);
		int ydivs = json_obj_get_int(paneLayout, "y", 1);
		
		// TODO: set up pane layout
		MainControl_ExpandPanes(as->mc, xdivs, ydivs);
		
		json_value_t* panes = json_obj_get_val(jsf->root, "panes");
		json_link_t* link = panes->arr.head;
		for(; link; link = link->next) {
			
			json_value_t* jpane = link->v;
			
			int x = json_obj_get_int(jpane, "x", 0);
			int y = json_obj_get_int(jpane, "y", 0);
			int is_focused_pane = json_obj_get_int(jpane, "is_focused_pane", 0);
			MainControlPane* pane = MainControl_GetPane(as->mc, x, y);
			if(is_focused_pane) {
				MainControl_FocusPane(as->mc, x, y);
			}
			
			// Todo: type
			json_value_t* jtabs = json_obj_get_val(jpane, "tabs");
			int active_tab = json_obj_get_int(jpane, "active_tab", 0);
			
			
			json_link_t* tlink = jtabs->arr.head;
			for(; tlink; tlink = tlink->next) {
				json_value_t* jtab = tlink->v;
				
				char* type = json_obj_get_str(jtab, "type");
				
				if(0 == strcmp("Buffer", type)) {
					
					json_value_t* jdata = json_obj_get_val(jtab, "data");
					
					char* path = json_obj_get_str(jdata, "path");
					if(!path) continue;
					
					MainControlTab* mct = MainControlPane_LoadFile(pane, path);
					if(!mct) {
						fprintf(stderr, "Error: could not open session file with path <%s>.\n", path);
						continue;
					}
					
					GUIBufferEditor_LoadSessionState((GUIBufferEditor*)mct->client, jdata);
					
					mct->accessIndex = json_obj_get_int(jtab, "accessIndex", 0);
					pane->lastTabAccessIndex = MAX(mct->accessIndex, pane->lastTabAccessIndex);
				}
				else if(0 == strcmp("FuzzyOpener", type)) {
					json_value_t* jdata = json_obj_get_val(jtab, "data");
					
					char* path = json_obj_get_str(jdata, "path");
					printf("loading FuzzyOpener with path <%s>\n", path);
					if(!path) path = "";
					
					MainControlTab* mct = MainControlPane_FuzzyOpener(pane, path);
					if(!mct) {
						fprintf(stderr, "Error: could not open session FuzzyOpener with path <%s>.\n", path);
						continue;
					}
					
					mct->accessIndex = json_obj_get_int(jtab, "accessIndex", 0);
					pane->lastTabAccessIndex = MAX(mct->accessIndex, pane->lastTabAccessIndex);
				}
				else if(0 == strcmp("GrepOpener", type)) {
					json_value_t* jdata = json_obj_get_val(jtab, "data");
					
					char* query = json_obj_get_str(jdata, "query");
					printf("loading GrepOpener with query <%s>\n", query);
					if(!query) query = "";
					
					MainControlTab* mct = MainControlPane_GrepOpen(pane, query);
					if(!mct) {
						fprintf(stderr, "Error: could not spawn session GrepOpener with query <%s>.\n", query);
						continue;
					}
					
					mct->accessIndex = json_obj_get_int(jtab, "accessIndex", 0);
					pane->lastTabAccessIndex = MAX(mct->accessIndex, pane->lastTabAccessIndex);
				}
			}
			
			MainControlPane_GoToTab(pane, active_tab);
		}
		
		MainControlPane_GoToTab(as->mc->focusedPane, as->mc->focusedPane->currentIndex);
		
		// load buffer history
		if(as->mc->gs->sessionFileHistory > 0) {
			json_value_t* jhistory = json_obj_get_val(jsf->root, "history");
			JSON_OBJ_EACH(jhistory, key, val) {
				int line = json_obj_get_int(val, "line", 1);
				int col = json_obj_get_int(val, "col", 0);
				
				int_vlist* bookmark_lines = bookmark_lines_from_json(json_obj_get_val(val, "bookmarks"));
				
				BufferCache_SetPathHistory(as->mc->bufferCache, key, line, col, bookmark_lines);
			}
		}
		
		json_file_free(jsf);
	}
	else {
		
		int i = 0;
		TabSpec* ts = as->gs->MainControl_startupTabs;
		while(ts[i].type != MCTAB_None) {
			switch(ts[i].type) {
				case MCTAB_Buffer:
					MainControl_LoadFile(as->mc, ts[i].path);
					break;
				case MCTAB_FileOpener:
					MainControl_OpenFileBrowser(as->mc, ts[i].path);
					break;
				case MCTAB_FuzzyOpener:
					MainControl_FuzzyOpener(as->mc, NULL);
					break; /*
				case MCTAB_GrepOpener:
					GUIMainControl_GrepOpen(as->mc, NULL);
					break;*/
			}
			i++;
		}
		MainControlPane_GoToTab(as->mc->focusedPane, 0);
	}
	as->mc->sessionLoaded = 1;
//	as->mc->focusedPane = as->mc->paneSet[1];
//	MainControl_LoadFile(as->mc, "testfile.c");
//	MainControl_LoadFile(as->mc, "testfile.h");
	
	
	
	// set up matrix stacks
	MatrixStack* view, *proj;
	
	view = &as->view;
	proj = &as->proj;
	
	msAlloc(2, view);
	msAlloc(2, proj);

	msIdent(view);
	msIdent(proj);
	
}


void AppState_InitGL(XStuff* xs, AppState* as) {
	glerr("left over error on app initgl");
	
	as->lastFrameTime = getCurrentTime();
	as->lastFrameDrawTime = 0;
	/*
	struct child_process_info* cc;
	cc = AppState_ExecProcessPipe(NULL, "ls", args);

	char buf[1024];
	while(!feof(cc->f_stdout)) {
		size_t sz = fread(buf, 1, 1024, cc->f_stdout);
		if(sz) printf("'%*s'\n", sz, buf);
	}	
	*/	
	// this costs 5mb of ram
// 	json_gl_init_lookup();
	
	
	as->ta = TextureAtlas_alloc(as->globalSettings);
	as->ta->width = 32;
	TextureAtlas_addFolder(as->ta, "icon", as->gs->imagesPath, 0);
	TextureAtlas_finalize(as->ta);
// 	
	
	/*
	Highlighter* ch = pcalloc(ch);
	initCStyles(ch);
	Highlighter_PrintStyles(ch);
	Highlighter_LoadStyles(ch, "config/c_colors.txt");
	*/
	
	as->gui->ta = as->ta;
	GUIManager_InitGL(as->gui);
	
	xs->onResize = resize_callback;
	xs->onResizeData = as->gui;
	
	as->gui->windowTitleSetFn = (void*)XStuff_SetWindowTitle;
	as->gui->windowTitleSetData = xs;
	
	as->gui->mouseCursorSetFn = (void*)XStuff_SetMouseCursor;
	as->gui->mouseCursorSetData = xs;
	
	
		
	
	as->frameCount = 0;
	
	as->debugMode = 0;
	
	int ww, wh;
	ww = xs->winAttr.width;
	wh = xs->winAttr.height;
	
	as->screen.wh.x = (float)ww;
	as->screen.wh.y = (float)wh;
	as->gui->screenSize = (Vector2i){ww, wh};
	as->gui->screenSizef = (Vector2){ww, wh};
	
	as->screen.aspect = as->screen.wh.x / as->screen.wh.y;
	as->screen.resized = 0;
	

	
	TextureAtlas_initGL(as->ta, as->globalSettings);
	

	as->guiPass = GUIManager_CreateRenderPass(as->gui);
	

	initRenderLoop(as);
	initRenderPipeline();
		
	initTextures();

	/*
	getPrintGLEnum(GL_MAX_COLOR_ATTACHMENTS, "meh");
	getPrintGLEnum(GL_MAX_DRAW_BUFFERS, "meh");
	getPrintGLEnum(GL_MAX_FRAMEBUFFER_WIDTH, "meh");
	getPrintGLEnum(GL_MAX_FRAMEBUFFER_HEIGHT, "meh");
	getPrintGLEnum(GL_MAX_FRAMEBUFFER_LAYERS, "meh");
	getPrintGLEnum(GL_MAX_FRAMEBUFFER_SAMPLES, "meh");
	getPrintGLEnum(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, "meh");
	getPrintGLEnum(GL_MAX_ARRAY_TEXTURE_LAYERS, "meh");
	getPrintGLEnum(GL_MAX_SAMPLES, "meh");
	getPrintGLEnum(GL_MAX_VERTEX_ATTRIBS, "meh");
	getPrintGLEnum(GL_MIN_PROGRAM_TEXEL_OFFSET, "meh");
	getPrintGLEnum(GL_MAX_PROGRAM_TEXEL_OFFSET, "meh");
	getPrintGLEnum(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, "meh");
	getPrintGLEnum(GL_MAX_UNIFORM_BLOCK_SIZE, "meh");
	getPrintGLEnum(GL_MAX_TEXTURE_SIZE, "meh");
	
	*/	
}








// effectively a better, asynchronous version of system()
void AppState_ExecProcess(AppState* as, char* execPath, char* args[]) {
	
	
	int childPID = fork();
	
	if(childPID == -1) {
		fprintf(stderr, "failed to fork trying to execute '%s'\n", execPath);
		perror(strerror(errno));
		return;
	}
	else if(childPID == 0) { // child process
		
		execvp(execPath, args); // never returns if successful
		
		fprintf(stderr, "failed to execute '%s'\n", execPath);
		exit(1); // kill the forked process 
	}
	else { // parent process
		// TODO: put the pid and info into an array somewhere
		
// 		int status;
		// returns 0 if nothing happened, -1 on error
// 		pid = waitpid(childPID, &status, WNOHANG);
		
	}
}

// http://git.suckless.org/st/file/st.c.html#l786

// effectively a better, asynchronous version of system()
// redirects and captures the child process i/o
struct child_pty_info* AppState_ExecProcessPTY(AppState* as, char* execPath, char* args[]) {
	
	int master, slave; // pty
	
	
	errno = 0;
	if(openpty(&master, &slave, NULL, NULL, NULL) < 0) {
		fprintf(stderr, "Error opening new pty for '%s' [errno=%d]\n", execPath, errno);
		return NULL;
	}
	
	errno = 0;
	
	int childPID = fork();
	if(childPID == -1) {
		
		fprintf(stderr, "failed to fork trying to execute '%s'\n", execPath);
		perror(strerror(errno));
		return NULL;
	}
	else if(childPID == 0) { // child process
		
		setsid();
		
		// redirect standard fd's to the pipe fd's 
		if(dup2(slave, fileno(stdin)) == -1) {
			printf("failed 1\n");
			exit(errno);
		}
		if(dup2(slave, fileno(stdout)) == -1) {
			printf("failed 2\n");
			exit(errno);
		}
		if(dup2(slave, fileno(stderr)) == -1) {
			printf("failed 3\n");
			exit(errno);
		}
		
		if(ioctl(slave, TIOCSCTTY, NULL) < 0) {
			fprintf(stderr, "ioctl TIOCSCTTY failed: %s, %d\n", execPath, errno);
		}
		
		// close original fd's
		close(master);
		close(slave);
		
		// die when the parent does (linux only)
		prctl(PR_SET_PDEATHSIG, SIGHUP);
		
		// swap for the desired program
		execvp(execPath, args); // never returns if successful
		 
		fprintf(stderr, "failed to execute '%s'\n", execPath);
		exit(1); // kill the forked process 
	}
	else { // parent process
		
		// close the child-end of the pipes
		struct child_pty_info* cpi;
		cpi = calloc(1, sizeof(*cpi));
		cpi->pid = childPID;
		cpi->pty = master;
		
		// set to non-blocking
		fcntl(master, F_SETFL, fcntl(master, F_GETFL) | FNDELAY | O_NONBLOCK);
		
		close(slave);
		
// 		tcsetattr(STDIN_FILENO, TCSANOW, &master);
// 		fcntl(master, F_SETFL, FNDELAY);
		
// 		int status;
		// returns 0 if nothing happened, -1 on error
// 		pid = waitpid(childPID, &status, WNOHANG);
		
		return cpi;
	}
	
	return NULL; // shouldn't reach here
	
}


// effectively a better, asynchronous version of system()
// redirects and captures the child process i/o
struct child_process_info* AppState_ExecProcessPipe(char* execPath, char* args[]) {
	
	int master, slave; //pty
	int in[2]; // io pipes
	int out[2];
	int err[2];
	
	const int RE = 0;
	const int WR = 1;
	
	// 0 = read, 1 = write
	
	if(pipe(in) < 0) {
		return NULL;
	}
	if(pipe(out) < 0) {
		close(in[0]);
		close(in[1]);
		return NULL;
	}
	if(pipe(err) < 0) {
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);
		return NULL;
	}
	
	errno = 0;
	if(openpty(&master, &slave, NULL, NULL, NULL) < 0) {
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);
		fprintf(stderr, "Error opening new pty for '%s' [errno=%d]\n", execPath, errno);
		return NULL;
	}
	
	errno = 0;
	
	int childPID = fork();
	if(childPID == -1) {
		
		fprintf(stderr, "failed to fork trying to execute '%s'\n", execPath);
		perror(strerror(errno));
		return NULL;
	}
	else if(childPID == 0) { // child process
		
		// redirect standard fd's to the pipe fd's 
		if(dup2(in[RE], fileno(stdin)) == -1) {
			printf("failed 1\n");
			exit(errno);
		}
		if(dup2(out[WR], fileno(stdout)) == -1) {
			printf("failed 2\n");
			exit(errno);
		}
		if(dup2(err[WR], fileno(stderr)) == -1) {
			printf("failed 3\n");
			exit(errno);
		}
		
		// close original fd's used by the parent
		close(in[0]);
		close(in[1]);
		close(out[0]);
		close(out[1]);
		close(err[0]);
		close(err[1]);
		
		close(master);
		close(slave);
		
		// die when the parent does (linux only)
		prctl(PR_SET_PDEATHSIG, SIGHUP);
		
		// swap for the desired program
		execvp(execPath, args); // never returns if successful
		
		fprintf(stderr, "failed to execute '%s'\n", execPath);
		exit(1); // kill the forked process 
	}
	else { // parent process
		
		// close the child-end of the pipes
		struct child_process_info* cpi;
		cpi = calloc(1, sizeof(*cpi));
		
		cpi->child_stdin = in[WR];
		cpi->child_stdout = out[RE];
		cpi->child_stderr = err[RE];
		cpi->f_stdin = fdopen(cpi->child_stdin, "wb");
		cpi->f_stdout = fdopen(cpi->child_stdout, "rb");
		cpi->f_stderr = fdopen(cpi->child_stderr, "rb");
		
		// set to non-blocking
		fcntl(cpi->child_stdout, F_SETFL, fcntl(cpi->child_stdout, F_GETFL) | O_NONBLOCK);
		fcntl(cpi->child_stderr, F_SETFL, fcntl(cpi->child_stderr, F_GETFL) | O_NONBLOCK);
		
		close(in[0]);
		close(out[1]); 
		close(err[1]); 
		
		close(slave);
		
		cpi->pid = childPID;
		
// 		int status;
		// returns 0 if nothing happened, -1 on error
// 		pid = waitpid(childPID, &status, WNOHANG);
		
		return cpi;
	}
	
	return NULL; // shouldn't reach here
}





int execProcessPipe_strlist(char* args[], char*** charpp_out, size_t* n_out) {
	int res = 0;
	
	struct child_process_info* cc;
	int bufferLength = 1024;

	size_t max_contents = 4 * bufferLength * sizeof(char);
	char* contents = malloc(max_contents);
	size_t offset = 0, n_read;
	char buffer[bufferLength];

	char** filepaths;
	size_t n_filepaths = 0;

	cc = AppState_ExecProcessPipe(args[0], args);
	
	if(!cc) {
		res = 1;
		return res;
	}
	
	if(!cc->f_stdout) {
		res = 2;
		return res;
	}
	
	while(!feof(cc->f_stdout)) {
		size_t n_read = fread(buffer, 1, bufferLength, cc->f_stdout);
		if(n_read && (offset + n_read) >= max_contents) {
			max_contents *= 2;
			contents = realloc(contents, max_contents);
		}

		// printf("copy at [%ld]: [[%s]]\n", offset, buffer);
		memcpy((char*)((size_t)contents+offset), buffer, n_read);
		offset += n_read;
	}
	contents[offset] = '\0';

	fclose(cc->f_stdin);
	fclose(cc->f_stdout);
	fclose(cc->f_stderr);
	
	// clean up the zombie process
	int status;
	waitpid(cc->pid, &status, 0); 
	
	

	size_t split_out = 0;
	if(charpp_out) *charpp_out = strsplit_inplace(contents, '\n', &split_out);
	if(n_out) *n_out = split_out;
	
	return res;
}





/*
char buffer[1024];


errno = 0;
// int len = read(buffer, 5, cc->child_stdout);
int len = read(cc->child_stdout, buffer, 1023);
if(len == -1 && errno != EWOULDBLOCK) {
	printf("1: %d %s\n", errno,  strerror(errno));
}

if(len > 0) {
	buffer[len] = 0;
	printf("from bash[%d]: '%.*s'\n", len, len, buffer);
}

errno = 0;
len = read(cc->child_stderr, buffer, 1023);
if(len == -1 && errno != EWOULDBLOCK) {
	printf("2: %d %s\n", errno, strerror(errno));
}
if(len > 0) {
	buffer[len] = 0;
	printf("from bash[%d]: '%.*s'\n", len, len, buffer);
}
*/




void AppState_UpdateSettings(AppState* as, Settings* gs) {
	
	printf("\n\n-- BROKEN FN: AppState_UpdateSettings --\n\n\n");
	// TODO: VERY BROKEN
	as->globalSettings = gs;
	
	MainControl_UpdateSettings(as->mc, gs);
}



Vector2i viewWH = {
	.x = 0,
	.y = 0
};
void checkResize(XStuff* xs, AppState* as) {
	if(viewWH.x != xs->winAttr.width || viewWH.y != xs->winAttr.height) {
		
		// TODO: destroy all the textures too
		
		//printf("screen 0 resized\n");
		
		viewWH.x = xs->winAttr.width;
		viewWH.y = xs->winAttr.height;
		
		as->screen.wh.x = (float)xs->winAttr.width;
		as->screen.wh.y = (float)xs->winAttr.height;
		as->gui->screenSize = (Vector2i){xs->winAttr.width, xs->winAttr.height};
		as->gui->screenSizef = (Vector2){xs->winAttr.width, xs->winAttr.height};
		
		as->screen.aspect = as->screen.wh.x / as->screen.wh.y;
		
		as->screen.resized = 1;
		
	}
}



void handleEvent(AppState* as, InputState* is, InputEvent* ev, PassFrameParams* pfp) {
// 	printf("%d %c/* */%d-\n", ev->type, ev->character, ev->keysym);
	
	switch(ev->type) {
		case EVENT_KEYUP:
		case EVENT_KEYDOWN:
			GUIManager_HandleKeyInput(as->gui, is, ev, pfp);
			break;
		case EVENT_MOUSEUP:
		case EVENT_MOUSEDOWN:
			GUIManager_HandleMouseClick(as->gui, is, ev, pfp);
			break;
		case EVENT_MOUSEMOVE:
			GUIManager_HandleMouseMove(as->gui, is, ev, pfp);
			break;
	}
	
}


void prefilterEvent(AppState* as, InputState* is, InputEvent* ev, PassFrameParams* pfp) {
	// drags, etc
	
	// TODO: fix; passthrough atm
	handleEvent(as, is, ev, pfp);
	
	
	
}




void initRenderLoop(AppState* as) {
	
	// timer queries

	query_queue_init(&as->queries.gui);
	
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	glBindTexture(GL_TEXTURE_2D, 0);
}



void SetUpPDP(AppState* as, PassDrawParams* pdp) {
	
	pdp->mWorldView = msGetTop(&as->view);
	pdp->mViewProj = msGetTop(&as->proj);
	
	pdp->mProjView = &as->invProj;
	pdp->mViewWorld = &as->invView;
	
	mInverse(pdp->mViewProj, &as->invProj);
	mInverse(pdp->mWorldView, &as->invView);
	
// 	pdp->eyeVec = as->eyeDir;
// 	pdp->eyePos = as->eyePos;
	pdp->targetSize = (Vector2i){as->screen.wh.x, as->screen.wh.y};
	pdp->timeSeconds = (float)(long)as->frameTime;
	pdp->timeFractional = as->frameTime - pdp->timeSeconds;
	
}




#include "ui/gui_internal.h"

//#define PERF(...) __VA_ARGS__
#define PERF(...) 


void appLoop(XStuff* xs, AppState* as, InputState* is) {
	PassDrawParams pdp = {0};		
	PassFrameParams pfp = {0};
	
	// in seconds
	double frameTimeTarget = 1.0 / (as->gs->frameRate);
	double lastFrameCost = 0.0;
	double lastFrameStart = getCurrentTime();
	
	// the gui system needs this structure to reasonably exist
	SetUpPDP(as, &pdp);
	pfp.timeElapsed = as->frameSpan;
	pfp.appTime = as->frameTime; // this will get regenerated from save files later
	pfp.wallTime = as->frameTime;
	pfp.dp = &pdp;
		
	
	// main running loop
	while(1) {
		InputEvent iev;
		double frameCostStart;
		double frameStart;
		double lastFrameSpan;
		double totalEventsTime = 0.0;
		double totalSleepTime = 0.0;
		
		double now;
		PERF(printf("\n\n"));
		
		
		frameStart = getCurrentTime();
		lastFrameSpan = frameStart - lastFrameStart;
		PERF(printf("last frame span: %fus [%ffps]\n", 
			lastFrameSpan * 1000000.0,
			1.0 / lastFrameSpan
		));
		lastFrameStart = frameStart;
		
		
		double estimatedSleepTime;
		
		// frameCost is the amount of time it takes to generate a new frame
		estimatedSleepTime = (frameTimeTarget - lastFrameCost) * .98;
		
		struct timespec sleepInterval;
		sleepInterval.tv_sec = estimatedSleepTime;
		sleepInterval.tv_nsec = fmod(estimatedSleepTime, 1.0) * 1000000000.0;
			
		
		// sleep through the beginning of the frame, checking events occasionally
		for(int i = 0; ; i++) {
			int drawRequired = 0;
			PERF(now = getCurrentTime());
				
			if(processEvents(xs, is, &iev, -1)) {
				// handle the event
				
				prefilterEvent(as, is, &iev, &pfp);
				
				PERF(totalEventsTime += timeSince(now));
			}
			else { // no events, see if we need to sleep longer
				now = getCurrentTime();
				
				if(now - frameStart < estimatedSleepTime) {
					
					nanosleep(&sleepInterval, &sleepInterval);
					
					PERF(totalSleepTime += timeSince(now));
				}
				else {
					break; // time to renter the frame
				}
			}
		}
		
		
		
		PERF(printf("events / sleep / total: %fus / %fus / %fus\n", 
			totalEventsTime * 1000000.0,
			totalSleepTime * 1000000.0,
			timeSince(frameStart) * 1000000.0
		));
		PERF(totalSleepTime = 0);
		PERF(totalEventsTime = 0);
		
		frameCostStart = getCurrentTime();
		
		
		//  internal frame time is at the start of render, for the least lag
		as->frameTime = getCurrentTime();
		as->frameSpan = as->frameTime - as->lastFrameTime;		
		as->lastFrameTime = as->frameTime;
		
		
		BufferCache_CheckWatches(as->bufferCache);
		
		checkResize(xs, as);
		
			
		/*
		if(frameCounter == 0) {
			
			uint64_t qtime;
	
			#define query_update_gui(qname)		\
			if(!query_queue_try_result(&as->queries.qname, &qtime)) {\
				sdtime = ((double)qtime) / 1000000.0;\
			}\
			snprintf(frameCounterBuf, 128, #qname ":  %.2fms", sdtime);\
			GUIText_setString(gt_##qname, frameCounterBuf);
	
	
			//query_update_gui(gui);
			
			lastPoint = now;
		}
	*/
	
	
			
		
		PERF(now = getCurrentTime());
		
		SetUpPDP(as, &pdp);
		
		pfp.dp = &pdp;
		pfp.timeElapsed = as->frameSpan;
		pfp.appTime = as->frameTime; // this will get regenerated from save files later
		pfp.wallTime = as->frameTime;
		
		glexit("");
		
		glViewport(0, 0, as->screen.wh.x, as->screen.wh.y);
		PERF(printf("pre-render time: %fus\n", timeSince(now) * 1000000.0));
		
		
	// 	glEnable(GL_CULL_FACE);
	// 	glFrontFace(GL_CW); // this is backwards, i think, because of the scaling inversion for z-up
		
		// the cpu will generally block here
		PERF(now = getCurrentTime());
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		PERF(printf("glBindFramebuffer: %fus\n", timeSince(now) * 1000000.0));
		
		PERF(now = getCurrentTime());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		PERF(printf("glClear: %fus\n", timeSince(now) * 1000000.0));
		
		
		// frameCost is the amount of time it takes to generate a new frame
		frameCostStart = getCurrentTime();
		
		
		glexit("");
	// 	query_queue_start(&as->queries.gui);
		
	// 	glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glexit("");
		
		glDisable(GL_DEPTH_TEST);
		
		PERF(now = getCurrentTime());
		RenderPass_preFrameAll(as->guiPass, &pfp);
		
		// TODO: new gui code
//		gui_drawBox(as->gui, (Vector2){10,10},(Vector2){100,100}, &as->gui->curClip, 2, &(Color4){1,1,1,1});

				
		RenderPass_renderAll(as->guiPass, pfp.dp);
		RenderPass_postFrameAll(as->guiPass);
		PERF(printf("gui render time: %fus\n", timeSince(now) * 1000000.0));
		
	// 	query_queue_stop(&as->queries.gui);
	
		glexit("");
		
		msPop(&as->view);
		msPop(&as->proj);

		lastFrameCost = timeSince(frameCostStart);
		PERF(printf("frame cost: %fus\n", lastFrameCost * 1000000.0));
		
//		now = getCurrentTime();
//		glFinish();
//		printf("glFinish: %fus\n", timeSince(now) * 1000000.0);
		
		PERF(now = getCurrentTime());
		glXSwapBuffers(xs->display, xs->clientWin);
		PERF(printf("glXSwapBuffers: %fus\n", timeSince(now) * 1000000.0));
	
		
		
		
		as->screen.resized = 0;
		

		now = getCurrentTime();
		as->lastFrameDrawTime = now - as->frameTime; 
	
		as->perfTimes.draw = as->lastFrameDrawTime;
	
	
		PERF(now = getCurrentTime());
//		GUIManager_Reap(as->gui);
		PERF(printf("GUIManager_Reap: %fus\n", timeSince(now) * 1000000));
	}
}


