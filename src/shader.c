
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include <libgen.h>

#include "utilities.h"
#include "shader.h"
#include "sti/sti.h"


/*
TODO:
clean up debug file/line info
capture compile errors and translat to actual file and line
*/

const char* SHADER_BASE_PATH = "/usr/lib64/gpuedit/shaders/";


typedef VEC(char*) stringlist;

typedef struct {
	char* src;
	char* file_path;
	int file_line;
} LineInfo;

typedef struct ShaderSource {
	VEC(LineInfo) lines;
	VEC(char*) strings; // only used in final separated shaders
	char* path;
	
	GLenum type;
	uint32_t version;
	GLuint id;
	
} ShaderSource;



void printLogOnFail(GLuint id) {
	
	GLint success, logSize;
	GLsizei len;
	GLchar* log;
	
	if(!glIsShader(id)) {
		fprintf(stderr, "id is not a shader!\n");
		return;
	}
	
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if(success) return;
	
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logSize);
	log = (GLchar*)malloc(logSize);
	
	glGetShaderInfoLog(id, logSize, &len, log);
	fprintf(stderr, "Shader Log:\n%s", (char*)log);
	
	free(log);
}

void printProgLogOnFail(GLuint id) {
	
	GLint success, logSize;
	GLsizei len;
	GLchar* log;
	
	if(!glIsProgram(id)) {
		fprintf(stderr, "id is not a program!\n");
		return;
	}
	
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if(success) return;
	
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logSize);
	log = (GLchar*)malloc(logSize);
	
	glGetProgramInfoLog(id, logSize, &len, log);
	fprintf(stderr, "Program Log:\n%s", (char*)log);
	
	free(log);
}


GLenum indexToEnum(int index) {
	GLenum a[] = {
		GL_VERTEX_SHADER,
		GL_TESS_CONTROL_SHADER,
		GL_TESS_EVALUATION_SHADER,
		GL_GEOMETRY_SHADER,
		GL_FRAGMENT_SHADER,
		GL_COMPUTE_SHADER
	};
	
	if(index < 0 || index > 5) return -1;
	
	return a[index];
}


int nameToIndex(char* name) {
	
	if(0 == strcasecmp(name, "VERTEX")) return 0;
	if(0 == strcasecmp(name, "TESS_CONTROL")) return 1;
	if(0 == strcasecmp(name, "TESS_EVALUATION")) return 2;
	if(0 == strcasecmp(name, "GEOMETRY")) return 3;
	if(0 == strcasecmp(name, "FRAGMENT")) return 4;
	if(0 == strcasecmp(name, "COMPUTE")) return 5;
	
	return -1;
}

GLenum nameToEnum(char* name) {
	
	if(0 == strcasecmp(name, "VERTEX")) return GL_VERTEX_SHADER;
	if(0 == strcasecmp(name, "TESS_CONTROL")) return GL_TESS_CONTROL_SHADER;
	if(0 == strcasecmp(name, "TESS_EVALUATION")) return GL_TESS_EVALUATION_SHADER;
	if(0 == strcasecmp(name, "GEOMETRY")) return GL_GEOMETRY_SHADER;
	if(0 == strcasecmp(name, "FRAGMENT")) return GL_FRAGMENT_SHADER;
	if(0 == strcasecmp(name, "COMPUTE")) return GL_COMPUTE_SHADER;
	
	return -1;
}





// does not remove \n chars. gl wants them.
void internal_strsplit(char* source, stringlist* out) { 
	char* s = source;
	char* lstart = source;
	
	while(*s) {
		if(*s == '\n') {
			VEC_PUSH(out, strndup(lstart, s - lstart + 1));
			lstart = s + 1;
		}
		s++;
	}
	
	// handle the last line
	if(s > lstart) {
		VEC_PUSH(out, strndup(lstart, s - lstart));
	}
}

ShaderSource* makeShaderSource() {
	ShaderSource* n;
	
	n = calloc(1, sizeof(*n));
	CHECK_OOM(n);
	
	VEC_INIT(&n->lines);
	VEC_INIT(&n->strings);
	
	return n;
}

ShaderSource* loadShaderSource(char* path) {
	ShaderSource* ss;
	stringlist l;
	int i;
	char* source;

	
	source = readFile(path, NULL);
	if(!source) {
		// TODO copypasta, fix this
		fprintf(stderr, "failed to load shader file '%s'\n", path);
		return NULL;
	}
	
	VEC_INIT(&l);
	ss = makeShaderSource();
	
	ss->path = path;
	internal_strsplit(source, &l);
	
	for(i = 0; i < VEC_LEN(&l); i++) {
		LineInfo* li;
		VEC_INC(&ss->lines);
		
		li = &VEC_ITEM(&ss->lines, i);
		li->src = VEC_ITEM(&l, i);
		li->file_path = path;
		li->file_line = i + 1;
	}
	
	VEC_FREE(&l);
	
	return ss;
}


char* extractFileName(char* src) {
	char* path, *s, *e;
	int delim;
	
	s = src;
	
	// skip spaces
	while(*s && *s == ' ') s++;
	
	delim = *s++;
	e = strchr(s, delim);
	path = strndup(s, e - s);
	
	return path;
}

char* realFromSiblingPath(char* sibling, char* file) {
	char* falsePath, *realPath;
	
	char* fuckdirname = strdup(sibling);
	char* dir;
	
	dir = dirname(fuckdirname);
	
	falsePath = path_join(dir, file);
	
	realPath = realpath(falsePath, NULL);
	if(!realPath) {
		// handle errno
	}
	
	free(fuckdirname);
	free(falsePath);
	
	return realPath;
}

void processIncludes(ShaderProgram* sp, ShaderSource* ss) {
	int i;
	char* s, *includeFileName, *includeFilePath;
	
	for(i = 0; i < VEC_LEN(&ss->lines); i++) {
		ShaderSource* iss;
		LineInfo* li = &VEC_ITEM(&ss->lines, i);
		
		
		if(0 == strncmp("#include", li->src, strlen("#include"))) {
			s = li->src + strlen("#include");
			
			includeFileName = extractFileName(s);
			includeFilePath = realFromSiblingPath(ss->path, includeFileName);
			
			// TODO: recursion detection
			iss = loadShaderSource(includeFilePath);
			
			//HT_set(&sp->sources, includeFilePath, iss);
			
			// insert the included lines into this file's lines
			VEC_SPLICE(&ss->lines, &iss->lines, i+1);
			
			// comment out this line
			li->src[0] = '/';
			li->src[1] = '/';
			
			// the spliced in lines are ahead of the loop counter.
			// recursion is not necessary
		}
		
	}
}


void extractShaders(ShaderProgram* sp, ShaderSource* raw) {
	int i;
	char* s;
	char typeName[24];
	int commonVersion = 0;
	
	ShaderSource* curShader = NULL;
	
	/*
	ShaderSource* common;
	common = makeShaderSource();
	*/
	
	for(i = 0; i < VEC_LEN(&raw->lines); i++) {
		int cnt, index;
		int version;
		LineInfo* li = &VEC_ITEM(&raw->lines, i);
		
		
		// extract the GLSL version to be prepended to the shader
		if(0 == strncmp("#version", li->src, strlen("#version"))) {
			s = li->src + strlen("#version");
			
			sscanf(s, " %d", &version);
			
			if(!curShader) {
				commonVersion = version;
			}
			else {
				curShader->version = version;
			}
			
			// comment out this line
			li->src[0] = '/';
			li->src[1] = '/';
		}
		else if(0 == strncmp("#shader", li->src, strlen("#shader"))) {
			s = li->src + strlen("#shader");
			
			cnt = sscanf(s, " %23s", typeName); // != 1 for failure
			if(cnt == EOF || cnt == 0) {
				printf("failure scanning shader type name\n");
				continue;
			}
			
			index = nameToIndex(typeName);
			
			sp->shaders[index] = curShader = makeShaderSource();
			curShader->type = indexToEnum(index);
			
			if(commonVersion) curShader->version = commonVersion;
			
			// make room for the version directive, added later
			VEC_INC(&curShader->lines);
			VEC_INC(&curShader->strings);
			
			// comment out this line
			li->src[0] = '/';
			li->src[1] = '/';
		}
		else {
			if(curShader) {
				// copy line 
				VEC_PUSH(&curShader->lines, *li);
				VEC_PUSH(&curShader->strings, li->src);
			}
		}
		
	}
	
	
	// fill in version info
	for(i = 0; i < 6; i++) {
		ShaderSource* ss = sp->shaders[i];
		char* s;
		
		if(!ss) continue;
		
		s = malloc(20);
		sprintf(s, "#version %.3u", ss->version);
		VEC_ITEM(&ss->strings, 0) = s;
		VEC_ITEM(&ss->lines, 0).src = s;
		VEC_ITEM(&ss->lines, 0).file_path = NULL;
		VEC_ITEM(&ss->lines, 0).file_line = -1;
	}
}



ShaderProgram* makeShaderProgram() {
	ShaderProgram* sp;
	
	sp = calloc(1, sizeof(*sp));
	CHECK_OOM(sp);
	
	HT_init(&sp->sources, 0);
	
	return sp;
}

void compileShader(ShaderSource* ss) {
	GLuint id;
	glerr("pre shader create error");
	ss->id = glCreateShader(ss->type);
	glerr("shader create error");
	
	glShaderSource(ss->id, VEC_LEN(&ss->strings), (const GLchar**)VEC_DATA(&ss->strings), NULL);
	glerr("shader source error");
	
	//printf("compiling\n");
	glCompileShader(ss->id);
	printLogOnFail(ss->id);
	glerr("shader compile error");
}

ShaderProgram* loadCombinedProgram(char* path) {
	ShaderProgram* sp;
	ShaderSource* ss;
	char* spath;
	int bplen = strlen(SHADER_BASE_PATH);
	

	// grab the source
	spath = (char*)malloc(bplen + strlen(path) + 6);
	sprintf(spath, "%s%s.glsl", SHADER_BASE_PATH, path);
	
	sp = makeShaderProgram();
	
	ss = loadShaderSource(spath);
	
	processIncludes(sp, ss);
	extractShaders(sp, ss);
	
	sp->id = glCreateProgram();
	
	int i;
	for(i = 0; i < 6; i++) {
		int j;
		ShaderSource* sss = sp->shaders[i];
		
		if(!sss) continue;
		
// 		for(j = 0; j < VEC_LEN(&sss->lines); j++)
// 		printf("%s:%d: '%s'\n", 
// 			VEC_ITEM(&sp->shaders[i]->lines, j).file_path,
// 			VEC_ITEM(&sp->shaders[i]->lines, j).file_line,
// 			VEC_ITEM(&sp->shaders[i]->lines, j).src
// 		);
		
		compileShader(sss);
		
		glAttachShader(sp->id, sss->id);
		glerr("Could not attach shader");

	}
	
	
	glLinkProgram(sp->id);
	printProgLogOnFail(sp->id);
	glerr("linking program");
	
	
	return sp;
}







