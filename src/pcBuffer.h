#ifndef __EACSMB_pcBuffer_h__
#define __EACSMB_pcBuffer_h__

/*************************************\
|                                     |
| Persistent Coherent Mapped Buffers  |
|                                     |
\*************************************/

#include <stdint.h>
#include "common_gl.h"


// shouldn't need to be higher than 3.
#define PC_BUFFER_DEPTH 3


typedef struct PCBuffer {
	size_t bufferSize; // per region. this is the usable space. 
	
	GLsync fences[PC_BUFFER_DEPTH];
	int nextRegion;
	
	GLenum type;
	GLuint bo;
	void* dataPtr;
	
	char* label;
	
} PCBuffer;


PCBuffer* PCBuffer_alloc(size_t size, GLenum type);
void PCBuffer_startInit(PCBuffer* b, size_t size, GLenum type);
void PCBuffer_finishInit(PCBuffer* b);

void* PCBuffer_beginWrite(PCBuffer* b);
void PCBuffer_afterDraw(PCBuffer* b);
void PCBuffer_bind(PCBuffer* b);

// Can only be used for: 
//   GL_ATOMIC_COUNTER_BUFFER
//   GL_TRANSFORM_FEEDBACK_BUFFER
//   GL_UNIFORM_BUFFER
//   GL_SHADER_STORAGE_BUFFER
void PCBuffer_bindActiveRange(PCBuffer* b);

size_t PCBuffer_getOffset(PCBuffer* b); // in bytes




#endif // __EACSMB_pcBuffer_h__
