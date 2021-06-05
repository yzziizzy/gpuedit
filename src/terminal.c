
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>
#include <utmp.h>
#include <termios.h>


#include "terminal.h"

#include "ui/gui_internal.h"

/*
struct child_process_info {
	int pid;
	int child_stdin;
	int child_stdout;
	int child_stderr;
	FILE* f_stdin;
	FILE* f_stdout;
	FILE* f_stderr;
};
*/


typedef struct CSI {
	int params[6];
	int nParams;
	int final;
} CSI;


static struct child_pty_info* exec_process(char* execPath, char* args[]);


static int parseColor(GUITerminal* w, char* s, TermStyle* ts) {
	
	
	return 0;
}



static void parseCSI(char** s, int len, CSI* csi) {
	char* e;
	long n;
	
	csi->nParams = 0;
	
	while(1) {
		int c = **s;
		
		if(c == ';' || c == ':') {
			n = -1;
			
			csi->params[csi->nParams] = n;
			csi->nParams++;
			(*s)++;
		}
		else if(c >= '0' && c <= '9') {
			n = strtol(*s, &e, 10);
			*s = e;
			
			csi->params[csi->nParams] = n;
			csi->nParams++;
		}
		else if(c >= 0x40 && c <= 0x7e) {
			csi->final = c;
			(*s)++;
			
//			return 0;
		}			
		else {
//			return 0;
		}
		
	}
	
//	return 0;
}



enum {
	HUNGRY = 0,
	STARVED,
	READY,
};

enum {
	ST_NONE = 0,
	ST_ESC,
};

typedef struct ParseState {
	int state;
	int c;
	
} ParseState;

typedef struct TermBuffer {
	char* buf;
	int whead; // last byte written 
	int rtail; // last byte read
	int alloc;
} TermBuffer;

#define BETWEEN(a, c, b) (((a) <= (c)) && ((c) <= (b)))

static int parse(ParseState* st, TermBuffer* b) {
	
	if(b->rtail >= b->whead) {
		return STARVED;
	} 
	
	int c = b->buf[b->rtail++];
	
	
	if(st->state == ST_NONE) {
		if(isprint(c) || BETWEEN(0x07, c, 0x0d)) {
			st->c = c;
			b->rtail++;
			return READY;
		}
	}

	return 0;
}










static void updateScreen(GUITerminal* w, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	GUIManager* gm = h->gm;
	GlobalSettings* gs = w->gs;
	
	Vector2 tl = h->absTopLeft;
	
	
	float lh = gs->Terminal_lineHeight;
	float cw = gs->Terminal_colWidth;
	float fsz = .5;// gs->Terminal_fontSize;
	GUIFont* font = w->font ? w->font : gm->defaults.font;
	
	font = FontManager_findFont(gm->fm, gs->Terminal_fontName);
	
	int lnum = 0;
	int cnum = 0;
	Vector2 ctl = tl;
	Vector2 csz = {cw, lh};
	Color4 color = {1,1,1,1};
	
	w->vcLen = 0;
	
	VEC_EACH(&w->lines, i, line) {
		
		for(int n = 0; n < line->textLen; n++) {
			int c = line->text[n];
			
			if(isprint(c)) {
				gui_drawCharacter(gm, 
					ctl, csz,
					&h->absClip, h->absZ,
					c,
					&color,
					font, fsz
				);	
				
				cnum++;
				if(cnum >= w->ncols) {
					ctl.y += lh;
					ctl.x = tl.x;
					cnum = 0;
				}
				else {
					ctl.x += cw;
				}
			}
			else {
				if(c == '\n') {
					ctl.y += lh;
					lnum++;
				}
				else if(c == '\r') {
					ctl.x = tl.x;
					cnum = 0;
				}
				
				
				else if(c == 0x1b) { // escape
					c = line->text[++n];
					if(c == '[') { // terminates with first byte 0x40 <= x <= 0x7e
						c = line->text[++n];
						while(c < 0x40 || c > 0x7e)
							c = line->text[++n];
						
					}
					else if(c == ']') { // operating system command
						c = line->text[++n];
						if(c == '0') { // set window title
							c = line->text[++n];
							if(c == ';') {
								c = line->text[++n];
								while(c != 0x07 /*&& c != 0x9c*/)
									c = line->text[++n];
							}
						}
						
					}
					else {
						printf("%x ", c);
					}
				}
			}
			
			
			
		}
		
	}
	
	
}






static void render(GUIHeader* w_, PassFrameParams* pfp) {
	GUITerminal* w = (GUITerminal*)w_;
	GUIManager* gm = w->header.gm;
	GUIUnifiedVertex* v;

	Vector2 tl = w->header.absTopLeft;
	
	//if(w->dirty) {
		updateScreen(w, pfp);
	//	w->dirty = 0;
//	}
	
	//if(w->vcLen) {
	//	GUIManager_copyElements(gm, w->vertexCache, w->vcLen);
	//}
	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIHeader* h, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUITerminal* w = (GUITerminal*)h;
	GUIManager* gm = h->gm;
	GlobalSettings* gs = w->gs;
	
	float lh = gs->Terminal_lineHeight;
	float cw = gs->Terminal_colWidth;
	
	
	w->ncols = floor(h->size.x / cw);
	w->nrows = floor(h->size.y / lh);
	
	
	// HACK
	
	// read from the pty
	size_t bufferLength = 2048;
	char buffer[2048];
	
	errno = 0;
	int n_read = read(w->cpi->pty, buffer, bufferLength);
	
	if(n_read < 0) {
        switch(errno) {
			case EAGAIN: // also EWOULDBLOCK:
//				printf("eagain\n");
				break;
			case EIO:
				printf("EIO\n");
				break;
				
			default:
				fprintf (stderr, "error reading from master pty: %s\n", strerror (errno));
		
		}

	}
	
	if(n_read > 0) {
		for(int i = 0; i < n_read; i++) {
			GUITerminalLine* line = VEC_TAIL(&w->lines);
			
			if(line->textLen >= line->textAlloc) {
				line->textAlloc *= 2;
				line->text = realloc(line->text, line->textAlloc * sizeof(*line->text));
			}
			
			line->text[line->textLen++] = buffer[i];
			
			if(buffer[i] == '\n') {
				pcalloc(line);
				line->textAlloc = 128;
				line->text = malloc(line->textAlloc * sizeof(*line->text));
				
				VEC_PUSH(&w->lines, line);
			}
		}
		
		w->dirty = 1;
	}
	
	
	gui_defaultUpdatePos(&w->header, grp, pfp);
}



static int handleCommand(GUIHeader* w_, GUI_Cmd* cmd) {
	GUITerminal* w = (GUITerminal*)w_;
	GUITerminal_ProcessCommand(w, cmd);
	
	return 0;
}

void GUITerminal_ProcessCommand(GUITerminal* w, GUI_Cmd* cmd) {
	long amt;

	switch(cmd->cmd) {
//		case FuzzyMatcherCmd_Exit:
			
	}
	
}




static void reap(GUIHeader* w_) {
	GUITerminal* w = (GUITerminal*)w_;
	
	
}


static void keyDown(GUIHeader* w_, GUIEvent* gev) {
	GUITerminal* w = (GUITerminal*)w_;
	
	
}




GUITerminal* GUITerminal_New(GUIManager* gm) {

	static struct gui_vtbl static_vt = {
		.Render = render,
		.Reap = reap,
		.UpdatePos = updatePos,
		.HandleCommand = handleCommand,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyDown = keyDown,
//		.GainedFocus = gainedFocus,
//		.User = userEvent,
	};
	
	
	GUITerminal* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	w->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
//	w->header.cmdElementType = CUSTOM_ELEM_TYPE_FuzzyMatcher;

	GUITerminalLine* line = pcalloc(line);
	line->textAlloc = 128;
	line->text = malloc(line->textAlloc * sizeof(*line->text));	
	VEC_PUSH(&w->lines, line);

	char* args[] = {
		"/bin/bash",
		NULL,
	};
	
	w->cpi = exec_process("/bin/bash", args);
	
	
	
	
	return w;
}







// http://git.suckless.org/st/file/st.c.html#l786

// effectively a better, asynchronous version of system()
// redirects and captures the child process i/o
static struct child_pty_info* exec_process(char* execPath, char* args[]) {
	
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
		
		/*
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
		*/
		
		login_tty(slave);
		
		/*
		if(ioctl(slave, TIOCSCTTY, NULL) < 0) {
			fprintf(stderr, "ioctl TIOCSCTTY failed: %s, %d\n", execPath, errno);
		}
		*/
		
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
		
		char* b = "ls\r\e[A\e[B\e[A\r";
//		char* b = "printenv | grep TERM\r";
//		char* b = "\[A";
		write(master, b, strlen(b));
// 		tcsetattr(STDIN_FILENO, TCSANOW, &master);
// 		fcntl(master, F_SETFL, FNDELAY);
		
// 		int status;
		// returns 0 if nothing happened, -1 on error
// 		pid = waitpid(childPID, &status, WNOHANG);
		
		return cpi;
	}
	
	return NULL; // shouldn't reach here
	
}





static void writeChar(GUITerminal* w, int c) {

}











