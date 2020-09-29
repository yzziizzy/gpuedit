#ifndef __EACSMB_FBO_H__
#define __EACSMB_FBO_H__


#include "common_gl.h"

#include "c_json/json.h"


typedef struct FBOConfig {
	GLenum attachment;
	GLuint texture;
	
} FBOConfig;

typedef struct FBOTexConfig {
	GLenum internalType;
	GLenum format;
	GLenum size;
} FBOTexConfig;


typedef struct FramebufferTexture {
	GLuint tex_id;
	
	char* name;
	uint32_t width, height;
	
} FramebufferTexture;

typedef struct Framebuffer {
	GLuint fb;
	
	char isBound;
	
	uint32_t width, height;
	char* name;
	
} Framebuffer;


// not the best name. this struct holds various screen-related resources
typedef struct GPUResources {
	
	HT(FramebufferTexture*) fbTextures;
	HT(Framebuffer*) fbs;
	
} GPUResources;


GLuint* initFBOTextures(int w, int h, FBOTexConfig* cfg);
void destroyFBOTextures(GLuint* texids);
Framebuffer* allocFBO();
void initFBO(Framebuffer* fb, FBOConfig* cfg);
void destroyFBO(Framebuffer* fb);

void Framebuffer_bindRead(Framebuffer* fb);
void Framebuffer_bindReadName(char* name);
void Framebuffer_bind(Framebuffer* fb);
void Framebuffer_bindName(char* name);
void Framebuffer_unbind();
void Framebuffer_unbindRead();


void unpack_fbo(json_value_t* p, char* key, FBOTexConfig* cfg);


#endif // __EACSMB_FBO_H__
