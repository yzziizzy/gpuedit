#ifndef __EACSMB_JSON_GL_H__
#define __EACSMB_JSON_GL_H__


#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "c_json/json.h"
#include "c3dlas/c3dlas.h"


typedef enum json_type_gl {
	
	JSON_TYPE_GL_ENUM = JSON__type_enum_tail,
	
	JSON_TYPE_GL_MAXVALUE
	
} json_type_gl_t;

#undef JSON__type_enum_tail
#define JSON__type_enum_tail JSON_TYPE_GL_MAXVALUE

int json_as_GLenum(struct json_value* v, GLenum* out);
int json_as_vector(struct json_value* v, int max_len, float* out);
int json_vector3_minmax(struct json_value* v, Vector3* min, Vector3* max);
int json_double_minmax(struct json_value* v, double* min, double* max);


int json_as_type_gl(struct json_value* v, enum json_type_gl t, void* out);

void json_gl_init_lookup();


#endif // __EACSMB_JSON_GL_H__
