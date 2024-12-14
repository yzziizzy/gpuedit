#ifndef __gpuedit__global_h__
#define __gpuedit__global_h__


#include <math.h> 
#include <float.h>
#include <limits.h>
#include <ctype.h>

#define STRUCT(x) struct x; typedef struct x x;

#define PACKED __attribute__((packed))


#define clz(x) __builtin_clz(x)
#define popcount(x) __builtin_popcount(x)

#define countof(x) (sizeof(x) / sizeof((x)[0]))

#define CAT(a, b) a##b


#ifndef PP_NARG
	#define PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, N, ...) N
	#define PP_RSEQ_N() 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
	#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
	#define PP_NARG(...)  PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#endif


#define SZ2D(a) ((a).x * (a).y)
#define SZ3D(a) ((a).x * (a).y * (a).z)



#include <assert.h> 
#include <string.h> 
#include <stdatomic.h> 
#include "types.h"



static inline void spin_lock(_Atomic u32* sl) {
	do {
		int zero = 0;
		int res = atomic_compare_exchange_strong_explicit(sl, &zero, 1, memory_order_acq_rel, memory_order_relaxed);
		if(res) return;
		
		for(volatile int i = 0; i < 50; i++); // waste some time; TODO: make sure -O3 doesn't optimize out this whole statement
	} while(1);
}

static inline void spin_unlock(_Atomic u32* sl) {
	atomic_store_explicit(sl, 0, memory_order_release);
}





// variadic min/max macros

#ifdef MAX
	#undef MAX
#endif

#define MAX(a, ...) MAX_N(PP_NARG(__VA_ARGS__), a __VA_OPT__(,) __VA_ARGS__)
#define MAX_N(narg, a, ...) CAT(MAX_, narg)(a, __VA_ARGS__) 
#define MAX_1(a, b) ({ \
	__typeof__ (a) __MAX_a = (a); \
	__typeof__ (b) __MAX_b = (b); \
	__MAX_a > __MAX_b ? __MAX_a : __MAX_b; \
})
#define MAX_2(a, b, ...) MAX_1(MAX_1(a, b), __VA_ARGS__) 
#define MAX_3(a, b, ...) MAX_2(MAX_1(a, b), __VA_ARGS__) 
#define MAX_4(a, b, ...) MAX_3(MAX_1(a, b), __VA_ARGS__) 
#define MAX_5(a, b, ...) MAX_4(MAX_1(a, b), __VA_ARGS__) 
#define MAX_6(a, b, ...) MAX_5(MAX_1(a, b), __VA_ARGS__) 
#define MAX_7(a, b, ...) MAX_6(MAX_1(a, b), __VA_ARGS__) 
#define MAX_8(a, b, ...) MAX_7(MAX_1(a, b), __VA_ARGS__) 
#define MAX_9(a, b, ...) MAX_8(MAX_1(a, b), __VA_ARGS__) 
#define MAX_10(a, b, ...) MAX_9(MAX_1(a, b), __VA_ARGS__) 
#define MAX_11(a, b, ...) MAX_10(MAX_1(a, b), __VA_ARGS__) 
#define MAX_12(a, b, ...) MAX_11(MAX_1(a, b), __VA_ARGS__) 
#define MAX_13(a, b, ...) MAX_12(MAX_1(a, b), __VA_ARGS__) 
#define MAX_14(a, b, ...) MAX_13(MAX_1(a, b), __VA_ARGS__) 
#define MAX_15(a, b, ...) MAX_14(MAX_1(a, b), __VA_ARGS__) 
#define MAX_16(a, b, ...) MAX_15(MAX_1(a, b), __VA_ARGS__) 


#ifdef MIN
	#undef MIN
#endif

#define MIN(a, ...) MIN_N(PP_NARG(__VA_ARGS__), a __VA_OPT__(,) __VA_ARGS__)
#define MIN_N(narg, a, ...) CAT(MIN_, narg)(a, __VA_ARGS__) 
#define MIN_1(a, b) ({ \
	__typeof__ (a) __MIN_a = (a); \
	__typeof__ (b) __MIN_b = (b); \
	__MIN_a < __MIN_b ? __MIN_a : __MIN_b; \
})
#define MIN_2(a, b, ...) MIN_1(MIN_1(a, b), __VA_ARGS__) 
#define MIN_3(a, b, ...) MIN_2(MIN_1(a, b), __VA_ARGS__) 
#define MIN_4(a, b, ...) MIN_3(MIN_1(a, b), __VA_ARGS__) 
#define MIN_5(a, b, ...) MIN_4(MIN_1(a, b), __VA_ARGS__) 
#define MIN_6(a, b, ...) MIN_5(MIN_1(a, b), __VA_ARGS__) 
#define MIN_7(a, b, ...) MIN_6(MIN_1(a, b), __VA_ARGS__) 
#define MIN_8(a, b, ...) MIN_7(MIN_1(a, b), __VA_ARGS__) 
#define MIN_9(a, b, ...) MIN_8(MIN_1(a, b), __VA_ARGS__) 
#define MIN_10(a, b, ...) MIN_9(MIN_1(a, b), __VA_ARGS__) 
#define MIN_11(a, b, ...) MIN_10(MIN_1(a, b), __VA_ARGS__) 
#define MIN_12(a, b, ...) MIN_11(MIN_1(a, b), __VA_ARGS__) 
#define MIN_13(a, b, ...) MIN_12(MIN_1(a, b), __VA_ARGS__) 
#define MIN_14(a, b, ...) MIN_13(MIN_1(a, b), __VA_ARGS__) 
#define MIN_15(a, b, ...) MIN_14(MIN_1(a, b), __VA_ARGS__) 
#define MIN_16(a, b, ...) MIN_15(MIN_1(a, b), __VA_ARGS__) 


// this algorithm appears to be the fastest judging by godbolt output
#define ROUND_UP(v, gran) \
	((v) + ((v) % (gran) ? (gran) - ((v) % (gran)) : 0))

#define ROUND_DOWN(v, gran) \
	((v) - ((v) % (gran)))



// JSON loop macros
#define JSON_ARR_LOOP_VAL_DECL(obj, valname)       json_value_t* valname
#define JSON_ARR_LOOP_ITER_DECL(obj, index)        json_link_t* index = (obj) ? (obj)->arr.head : 0
#define JSON_ARR_LOOP_ITER_INC(obj, index)         index = index->next
#define JSON_ARR_LOOP_DONE(obj, index)             index == NULL
#define JSON_ARR_LOOP_SET_VAL(obj, index, valname) valname = index->v
	
#define JSON_ARR_EACH(cat_j, i, val) \
	LOOP_HELPER_EACH(JSON_ARR_LOOP, cat_j, i, val)

/* this macro already exists inside of c_json, but the example is left here for the future
#define JSON_OBJ_LOOP_KEY_DECL(obj, keyname)       char* keyname
#define JSON_OBJ_LOOP_VAL_DECL(obj, valname)       json_value_t* valname
#define JSON_OBJ_LOOP_ITER_DECL(obj, iter)        void* iter = NULL
#define JSON_OBJ_LOOP_NEXT_CALL(obj, iter, keyname, valname) json_obj_next(obj, &iter, &keyname, &valname)
	
#define JSON_OBJ_EACH(cat_j, i, val) \
	LOOP_HT_HELPER_EACH(JSON_OBJ_LOOP, cat_j, i, val)
*/



// debugging tools



typedef s8*  s8p;
typedef s16* s16p;
typedef s32* s32p;
typedef s64* s64p;
typedef u8*  u8p;
typedef u16* u16p;
typedef u32* u32p;
typedef u64* u64p;
typedef f32* f32p;
typedef f64* f64p;
typedef mat3* mat3p;
typedef mat4* mat4p;
typedef const char* ccharp;
typedef char** charpp;
typedef const char** ccharpp;


//   type        float  str   vlen
//           signed  int   size  ptrlvl
#define DEBUG_PANEL_TYPE_LIST(X) \
	X(s8,       1, 0, 1, 0, 1, 1, 0) \
	X(s16,      1, 0, 1, 0, 2, 1, 0) \
	X(s32,      1, 0, 1, 0, 4, 1, 0) \
	X(s64,      1, 0, 1, 0, 8, 1, 0) \
	X(u8,       0, 0, 1, 0, 1, 1, 0) \
	X(u16,      0, 0, 1, 0, 2, 1, 0) \
	X(u32,      0, 0, 1, 0, 4, 1, 0) \
	X(u64,      0, 0, 1, 0, 8, 1, 0) \
	X(f32,      1, 1, 0, 0, 4, 1, 0) \
	X(f64,      1, 1, 0, 0, 8, 1, 0) \
	X(vec2i,    1, 0, 1, 0, 4, 2, 0) \
	X(vec3i,    1, 0, 1, 0, 4, 3, 0) \
	X(vec4i,    1, 0, 1, 0, 4, 4, 0) \
	X(vec2l,    1, 0, 1, 0, 8, 2, 0) \
	X(vec3l,    1, 0, 1, 0, 8, 3, 0) \
	X(vec4l,    1, 0, 1, 0, 8, 4, 0) \
	X(vec2,     1, 1, 0, 0, 4, 2, 0) \
	X(vec3,     1, 1, 0, 0, 4, 3, 0) \
	X(vec4,     1, 1, 0, 0, 4, 4, 0) \
	X(vec2d,    1, 1, 0, 0, 8, 2, 0) \
	X(vec3d,    1, 1, 0, 0, 8, 3, 0) \
	X(vec4d,    1, 1, 0, 0, 8, 4, 0) \
	X(mat4,     1, 1, 0, 0, 4,16, 0) \
	X(mat3,     1, 1, 0, 0, 4, 9, 0) \
	X(charp,    0, 0, 0, 1, 8, 1, 0) \
	X(ccharp,   0, 0, 0, 1, 8, 1, 0) \
	X(s8p,      1, 0, 1, 0, 1, 1, 1) \
	X(s16p,     1, 0, 1, 0, 2, 1, 1) \
	X(s32p,     1, 0, 1, 0, 4, 1, 1) \
	X(s64p,     1, 0, 1, 0, 8, 1, 1) \
	X(u8p,      0, 0, 1, 0, 1, 1, 1) \
	X(u16p,     0, 0, 1, 0, 2, 1, 1) \
	X(u32p,     0, 0, 1, 0, 4, 1, 1) \
	X(u64p,     0, 0, 1, 0, 8, 1, 1) \
	X(f32p,     1, 1, 0, 0, 4, 1, 1) \
	X(f64p,     1, 1, 0, 0, 8, 1, 1) \
	X(vec2ip,   1, 0, 1, 0, 4, 2, 1) \
	X(vec3ip,   1, 0, 1, 0, 4, 3, 1) \
	X(vec4ip,   1, 0, 1, 0, 4, 4, 1) \
	X(vec2lp,   1, 0, 1, 0, 8, 2, 1) \
	X(vec3lp,   1, 0, 1, 0, 8, 3, 1) \
	X(vec4lp,   1, 0, 1, 0, 8, 4, 1) \
	X(vec2p,    1, 1, 0, 0, 4, 2, 1) \
	X(vec3p,    1, 1, 0, 0, 4, 3, 1) \
	X(vec4p,    1, 1, 0, 0, 4, 4, 1) \
	X(vec2dp,   1, 1, 0, 0, 8, 2, 1) \
	X(vec3dp,   1, 1, 0, 0, 8, 3, 1) \
	X(vec4dp,   1, 1, 0, 0, 8, 4, 1) \
	X(mat4p,    1, 1, 0, 0, 4,16, 1) \
	X(mat3p,    1, 1, 0, 0, 4, 9, 1) \
	X(charpp,   0, 0, 0, 1, 8, 1, 1) \
	X(ccharpp,  0, 0, 0, 1, 8, 1, 1) \


#define X(a, ...) union DBG_TYPE_##a {};
	DEBUG_PANEL_TYPE_LIST(X)
#undef X
//
//enum {
//	DBG_TYPE_NONE = 0,
//#define X(a, ...) DBG_TYPE_##a,
//	DEBUG_PANEL_TYPE_LIST(X)
//#undef X
//	DBG_TYPE_MAX_VALUE,
//};
//
//


// simple debug printf macros


#define dbg(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", strstr(__FILE__, "src/") ? (strstr(__FILE__, "src/") + 4) : (strstr(__FILE__, "baced/") + 6), __LINE__ __VA_OPT__(,) __VA_ARGS__);

#define dbgv(a) debug_print_var(__FILE__, __LINE__, __func__, #a, (void*)&a, WATCH_TYPE_TO_ENUM(a));
void debug_print_var(char* filename, long linenum, const char* funcname, char* name, void* var, int type);



#define debug_break *((char*)0) = "debug break";

// prints a backtrace to the provided file descriptor
void printbt(int fd);


#endif // __gpuedit__global_h__
