#ifndef __EACSMB_SHADER_H__
#define __EACSMB_SHADER_H__

 
#include "sti/sti.h"

 
 
struct ShaderSource;


typedef struct {
	HT(struct ShaderSource*) sources;
	struct ShaderSource* shaders[6];
	
	char* name;
	
	GLuint id;
	
} ShaderProgram;
 

ShaderProgram* loadCombinedProgram(char* path);



#endif // __EACSMB_SHADER_H__

