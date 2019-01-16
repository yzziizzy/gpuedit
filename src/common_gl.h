#ifndef __common_gl_h__
#define __common_gl_h__


#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "common_math.h"

#include "utilities.h"
#include "shader.h"
#include "texture.h"



// GLEW lacks this for some reason
typedef  struct {
	GLuint  count;
	GLuint  instanceCount;
	GLuint  first;
	GLuint  baseInstance;
} DrawArraysIndirectCommand;

typedef  struct {
	GLuint  count;
	GLuint  instanceCount;
	GLuint  firstIndex;
	GLuint  baseVertex;
	GLuint  baseInstance;
} DrawElementsIndirectCommand;




#define USE_KHR_DEBUG
#define NO_GL_GET_ERR_DEBUG



#define getPrintGLEnum(e, m) _getPrintGLEnumMin(e, #e, m)
static int _getPrintGLEnumMin(GLenum e, char* name, char* message) {
	GLint i;
	
	glGetIntegerv(e, &i);
	printf("%s: %d\n", name, i);
	
	return i;
}




// manual GL debugging

static inline void _glexit(char* msg, const char* file, int line, const char* func) {
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		fprintf(stderr, "GL ERROR at %s:%d (%s): %s: %s \n", file, line, func, msg, gluErrorString(err));
		exit(-1);
	}
}


static inline char* _glerr(char* msg, const char* file, int line, const char* func) {
	char* errstr;
	GLenum err;
	
	err = glGetError();
	errstr = NULL;
	
	if (err != GL_NO_ERROR) {
		errstr = (char*)gluErrorString(err);
#ifndef NO_GL_GET_ERR_DEBUG
		fprintf(
			stderr,
				TERM_BOLD TERM_COLOR_RED "GL ERROR:" TERM_RESET TERM_COLOR_RED
				"GL ERROR at %s:%d (%s): %s: %s \n",
			file, line, func, msg, errstr);
#endif
	}
	
	return errstr;
}


// msg is ignored, largely
#define glexit(msg) _glexit(msg, __FILE__, __LINE__, __func__)
// returns NULL for no error, a human error string otherwise. the error is printed to stderr.
#define glerr(msg) _glerr(msg, __FILE__, __LINE__, __func__)









#endif // __common_gl_h__
