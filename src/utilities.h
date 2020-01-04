#ifndef __gpuedit_utilities_h__
#define __gpuedit_utilities_h__

#include <stdio.h> // fprintf
#include <stdlib.h> // exit
#include <strings.h> // strcasecmp. yes, it's "strings" with an s at the end.

#include "common_gl.h"


#define MAX(a,b) ({ \
	__typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; \
})
#define MIN(a,b) ({ \
	__typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a < _b ? _a : _b; \
})
#define MAXE(a,b) ({ \
	__typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a >= _b ? _a : _b; \
})
#define MINE(a,b) ({ \
	__typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a <= _b ? _a : _b; \
})


#define CHECK_OOM(p) \
if(!(p)) { \
	fprintf(stderr, "OOM for %s at %s:%d. Buy more ram\n", #p, __FILE__, __LINE__); \
	exit(2); \
}

#define GOTO_OOM(p, label) \
if(!(p)) { \
	fprintf(stderr, "OOM for %s at %s:%d. Buy more ram\n", #p, __FILE__, __LINE__); \
	goto label; \
}


#define pcalloc(x) x = calloc(1, sizeof(*(x)))

#ifndef NO_TERM_COLORS
	#define TERM_COLOR_BLACK   "\x1b[30m"
	#define TERM_COLOR_RED     "\x1b[31m"
	#define TERM_COLOR_GREEN   "\x1b[32m"
	#define TERM_COLOR_YELLOW  "\x1b[33m"
	#define TERM_COLOR_BLUE    "\x1b[34m"
	#define TERM_COLOR_MAGENTA "\x1b[35m"
	#define TERM_COLOR_CYAN    "\x1b[36m"
	#define TERM_COLOR_GRAY    "\x1b[37m"
	#define TERM_RESET         "\x1b[0m"
	#define TERM_BOLD          "\x1b[1m"
	#define TERM_NORMAL        "\x1b[22m"
	#define TERM_BK_BLACK      "\x1b[40m"
	#define TERM_BK_RED        "\x1b[41m"
	#define TERM_BK_GREEN      "\x1b[42m"
	#define TERM_BK_YELLOW     "\x1b[43m"
	#define TERM_BK_BLUE       "\x1b[44m"
	#define TERM_BK_MAGENTA    "\x1b[45m"
	#define TERM_BK_CYAN       "\x1b[46m"
	#define TERM_BK_GRAY       "\x1b[47m"
#else
#endif





#include "sti/sti.h"



typedef void (*progess_fn_t)(float*); 


// cpu clock stuff
double getCurrentTime();
double timeSince(double past);


// gpu timer queries
typedef struct QueryQueue {
	GLuint qids[6];
	int head, used;
} QueryQueue;

void query_queue_init(QueryQueue* q);
void query_queue_start(QueryQueue* q);
void query_queue_stop(QueryQueue* q);
int query_queue_try_result(QueryQueue* q, uint64_t* time);


// TODO BUG: fix prepending a \n everywhere
char* readFile(char* path, int* srcLen);
char* readFileRaw(char* path, int* srcLen);

int glGenBindTexture(GLuint* tex, GLenum type);
void texParams2D(GLenum type, GLenum filter, GLenum wrap);


typedef struct VAOConfig {
	int bufferIndex; // for instancing
	
	int sz;
	GLenum type;
	
	int divisor;
	
	int normalized;
	
} VAOConfig;

GLuint makeVAO(VAOConfig* details);
size_t updateVAO(int bufferIndex, VAOConfig* details); 
size_t calcVAOStride(int bufferIndex, VAOConfig* details);



#define streq(a, b) (0 == strcmp(a, b))
#define strcaseeq(a, b) (0 == strcasecmp(a, b))



#endif // __gpuedit_utilities_h__
