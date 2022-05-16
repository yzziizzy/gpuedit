
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>



#include <X11/X.h>
#include <X11/Xlib.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "c3dlas/c3dlas.h"
#include "c3dlas/meshgen.h"

#include "utilities.h"
#include "fbo.h"

#include "json_gl.h"



//
GLuint* initFBOTextures(int w, int h, FBOTexConfig* cfg) {
	int len, i;
	GLuint* texids;
	
	// calc length of the config array
	for(len = 0; 
		cfg[len].internalType != 0 
		&& cfg[len].format != 0 
		&& cfg[len].size != 0; len++);
	
	texids = calloc(1, len + 1);
	
	glGenTextures(len, texids);
	
	// allocate storage for each texture
	for(i = 0; i < len; i++) {
		
		glBindTexture(GL_TEXTURE_2D, texids[i]);
		
		glexit(" -- tex buffer creation");

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		// just depth textures for this one
		if(cfg[i].format == GL_DEPTH_COMPONENT) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		}
		glexit("pre tex buffer creation");
		
		glTexImage2D(GL_TEXTURE_2D, 0, cfg[i].internalType, w, h, 0, cfg[i].format, cfg[i].size, NULL);
		glexit("tex buffer creation");
	}
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glexit("end of fb tex creation");
	
	return texids;
}

// don't do dumb things like pass in null pointers or empty lists. you get what you deserve if you do.
void destroyFBOTextures(GLuint* texids) {
	int len = 0;
	
	while(texids[len] != 0) len++;
	
	glDeleteTextures(len, texids);
	glexit("delete fbo textures");
}



Framebuffer* allocFBO() {
	return calloc(1, sizeof(Framebuffer));
}
 
void initFBO(Framebuffer* fb, FBOConfig* cfg) {
	int i, dblen = 0;
	GLenum status;
	GLenum DrawBuffers[32]; // nonsense beyond this point...
	
	
	glGenFramebuffers(1, &fb->fb);
	glBindFramebuffer(GL_FRAMEBUFFER, fb->fb);
	glexit("fbo creation");
	
	// The depth buffer
	for(i = 0; cfg[i].texture != 0; i++) {
		GLenum att = cfg[i].attachment;
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, att, GL_TEXTURE_2D, cfg[i].texture, 0);
		if(att != GL_DEPTH_ATTACHMENT && att != GL_STENCIL_ATTACHMENT) {
			DrawBuffers[dblen++] = att;
		}
	}
	glexit("fbo texture attach");
	

	glDrawBuffers(dblen, DrawBuffers);
	glexit("fbo drawbuffers");
	
	
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE) {
		printf("fbo status invalid\n");
		exit(1);
	}
	//printf("FBO created.\n");
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glexit("unbind fb");
}


void destroyFBO(Framebuffer* fb) {
	
	glDeleteFramebuffers(1, &fb->fb);
	glexit("delete fb");
	
	fb->fb = 0;
}



void Framebuffer_bind(Framebuffer* fb) {
	if(!fb->isBound) {
		glBindFramebuffer(GL_FRAMEBUFFER, fb->fb);
		glexit("");
	}
}

void Framebuffer_unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glexit("");
}


void unpack_fbo(json_value_t* p, char* key, FBOTexConfig* cfg) {
	char* a, *b, *c;
	json_value_t* o, *v1, *v2, *v3;
	
	json_obj_get_key(p, key, &o);
	
	json_obj_get_key(o, "internalType", &v1); a = v1->s;
	json_as_GLenum(v1, &cfg->internalType);
	
	json_obj_get_key(o, "format", &v2); b = v2->s;
	json_as_GLenum(v2, &cfg->format);
	
	json_obj_get_key(o, "size", &v3); c = v3->s;
	json_as_GLenum(v3, &cfg->size);
	
	//printf("fbo cfg from json: %s: %x, %s: %x, %s: %x\n", a, cfg->internalType, b, cfg->format, c, cfg->size);
}
