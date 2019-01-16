#ifndef __EACSMB_SHADER_H__
#define __EACSMB_SHADER_H__

 
#include "hash.h"

 
 
struct ShaderSource;


typedef struct {
	HashTable(NewShaderSource*) sources;
	struct ShaderSource* shaders[6];
	
	char* name;
	
	GLuint id;
	
} ShaderProgram;
 

ShaderProgram* loadCombinedProgram(char* path);



#endif // __EACSMB_SHADER_H__

