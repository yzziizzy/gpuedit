
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

static struct child_pty_info* exec_process(char* execPath, char* args[]);






static void render(GUIHeader* w_, PassFrameParams* pfp) {
	GUITerminalControl* w = (GUITerminalControl*)w_;
	GUIManager* gm = w->header.gm;
	GUIUnifiedVertex* v;

	Vector2 tl = w->header.absTopLeft;
	
	
	
	GUIHeader_renderChildren(&w->header, pfp);
}


static void updatePos(GUIHeader* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUITerminalControl* w = (GUITerminalControl*)w_;
	
	size_t bufferLength = 512;
	char buffer[512];
	
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
			if(!isprint(buffer[i])) {
				printf("%d ", (int)buffer[i]);
			}
			else {
				printf("%c", (int)buffer[i]);
			}
		}
		printf("\n");
	}
	
	
	/*
	n_read = fread(buffer, 1, bufferLength, w->cpi->f_stderr);
	
	if(n_read > 0) {
		printf("err: ");
		for(int i = 0; i < n_read; i++) {
			printf("%c", (int)buffer[i]);
		}
		printf("\n");
	}
*/
	gui_defaultUpdatePos(&w->header, grp, pfp);
}



GUITerminalControl* GUITerminalControl_New(GUIManager* gm) {

	static struct gui_vtbl static_vt = {
		.Render = render,
		.UpdatePos = updatePos,
//		.HandleCommand = (void*)handleCommand,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
//		.KeyDown = keyDown,
//		.GainedFocus = gainedFocus,
//		.User = userEvent,
	};
	
	
	GUITerminalControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	w->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
//	w->header.cmdElementType = CUSTOM_ELEM_TYPE_FuzzyMatcher;
	

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
		
		char* b = "ls\r";
		write(master, b, 3);
// 		tcsetattr(STDIN_FILENO, TCSANOW, &master);
// 		fcntl(master, F_SETFL, FNDELAY);
		
// 		int status;
		// returns 0 if nothing happened, -1 on error
// 		pid = waitpid(childPID, &status, WNOHANG);
		
		return cpi;
	}
	
	return NULL; // shouldn't reach here
	
}

















