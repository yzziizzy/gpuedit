#ifndef __EACSMB_common_math_h__
#define __EACSMB_common_math_h__



#include <stdio.h> 
#include <math.h> 
#include <stdint.h> 
#include <limits.h> 
#include <assert.h> 

#include "c3dlas/c3dlas.h" 
#include "c3dlas/meshgen.h"


// basic vertex formats for general use

typedef struct Vertex_PT {
	Vector3 p;
	struct { float u, v; } t;
} Vertex_PT;

typedef struct Vertex_PNT {
	Vector3 p, n;
	struct { float u, v; } t;
} Vertex_PNT;

#define LOGB(b,x) (log2(x)/log2(b))

#endif // __EACSMB_common_math_h__
