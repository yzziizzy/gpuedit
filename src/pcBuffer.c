
#include <stdlib.h>
#include <string.h>

#include "utilities.h"
#include "pcBuffer.h"



static int waitSync(GLsync id, char* label); 


PCBuffer* PCBuffer_alloc(size_t size, GLenum type) {
	PCBuffer* b;
	
	b = malloc(sizeof(*b));
	CHECK_OOM(b);
	
	PCBuffer_startInit(b, size, type);
	
	return b;
}


void PCBuffer_startInit(PCBuffer* b, size_t size, GLenum type) {
	GLbitfield flags;

	flags =  GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	
	b->type = type;
	b->bufferSize = size;
	
	memset(b->fences, 0, sizeof(b->fences));
	b->nextRegion= 0;
	
	
	glGenBuffers(1, &b->bo);
	glexit("PCBuffer GenBuffers");
	
	glBindBuffer(b->type, b->bo);
	glBufferStorage(b->type, size * PC_BUFFER_DEPTH, NULL, flags);
	glexit("PCBuffer storage alloc");
	
	// the buffer is left bound
	// do any buffer config then call  PCBuffer_finishInit()
}



void PCBuffer_finishInit(PCBuffer* b) {
	GLbitfield flags;

	flags =  GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	
	b->dataPtr = glMapBufferRange(b->type, 0, b->bufferSize * PC_BUFFER_DEPTH, flags);
	glexit("PCBuffer persistent map");
	
	glBindBuffer(b->type, 0);
}

// returns the offset of the current region in bytes
size_t PCBuffer_getOffset(PCBuffer* b) {
	return (b->nextRegion * b->bufferSize);
}


void* PCBuffer_beginWrite(PCBuffer* b) {
	// the fence at index n protects from writing to index n.
	// it is set after commands for n - 1;
	waitSync(b->fences[b->nextRegion], b->label);
	
	return b->dataPtr + (b->nextRegion * b->bufferSize);
}


void PCBuffer_afterDraw(PCBuffer* b) {
	
	if(b->fences[b->nextRegion]) glDeleteSync(b->fences[b->nextRegion]);
	glexit("");
	
	b->fences[b->nextRegion] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glexit("");
	
	b->nextRegion = (b->nextRegion + 1) % PC_BUFFER_DEPTH; 
	
	
}



void PCBuffer_bind(PCBuffer* b) {
	glBindBuffer(b->type, b->bo);
}

// Can only be used for: 
//   GL_ATOMIC_COUNTER_BUFFER
//   GL_TRANSFORM_FEEDBACK_BUFFER
//   GL_UNIFORM_BUFFER
//   GL_SHADER_STORAGE_BUFFER
void PCBuffer_bindActiveRange(PCBuffer* b) {
	glBindBufferRange(b->type, 0, b->bo, (GLintptr)PCBuffer_getOffset(b), (GLintptr)b->bufferSize);
}


// terrible code, but use for now
static int waitSync(GLsync id, char* label) {
	GLenum ret;
	if(!id || !glIsSync(id)) return 1;
	int n = 0;
	while(1) {
		ret = glClientWaitSync(id, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
		glexit("");
		if(ret == GL_ALREADY_SIGNALED || ret == GL_CONDITION_SATISFIED)
			break;
		
		n++;
	}
	
	// check for stalls
	if(n > 6) {
		printf("\n\nserious pipeline stall in '%s' pcBuffer! (%d)\n\n", label, n);
		//int z = *((int*)0); 
	}
	
	// normal healthy return
	return 0;
}



