#ifndef __gpuedit__types_h__
#define __gpuedit__types_h__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "c3dlas/c3dlas.h"

struct json_value;
typedef struct json_value json;


#define C4H_SH(s,n) ((float)((s) >> n)/255.0)
#define C4H_HEX(s) { \
	.r = C4H_SH(s&0xff000000,24), \
	.g = C4H_SH(s&0xff0000, 16), \
	.b = C4H_SH(s&0xff00,8), \
	.a = C4H_SH(s&0xff,0)}


#define C4H_P(a,b) a##b
#define C4H(s) ((Color4)C4H_HEX(C4H_P(0x,s)))


#define C4S_H(s) (((s)[0] >= '0' && (s)[0] <= '9') ? (s)[0] - '0' : (\
    ((s)[0] >= 'a' && (s)[0] <= 'f') ? (s)[0] - 'a' + 10 : ( \
    ((s)[0] >= 'A' && (s)[0] <= 'F') ? (s)[0] - 'A' + 10 : 0)))

#define C4S_H2(s) (C4S_H(s) * 16 + C4S_H(s+1))
#define C4S(s) \
    (s[0] == '#' ? \
        (Color4){C4S_H2(s+1)/255.0, C4S_H2(s+3)/255.0, C4S_H2(s+5)/255.0, C4S_H2(s+7)/255.0 }: \
        (Color4){C4S_H2(s)/255.0, C4S_H2(s+2)/255.0, C4S_H2(s+4)/255.0, C4S_H2(s+6)/255.0 })


typedef struct Color4 {
	float r,g,b,a;
} Color4;

typedef struct Color3 {
	float r,g,b;
} Color3;

typedef uint8_t bool;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t szt;
typedef ssize_t sszt;

typedef Vector4 quat;
typedef Matrix Matrix4;
typedef Matrix4 mat4;
typedef Matrix3 mat3;

typedef Vector2 vec2;
typedef Vector3 vec3;
typedef Vector4 vec4;
typedef Vector2d vec2d;
typedef Vector3d vec3d;
typedef Vector4d vec4d;
typedef Vector2i vec2i;
typedef Vector3i vec3i;
typedef Vector4i vec4i;
typedef Vector2l vec2l;
typedef Vector3l vec3l;
typedef Vector4l vec4l;

typedef _Atomic u32 spinlock_t;

typedef struct BlitParams {
	vec2i dataSize;
	vec2i windowOffset;
	vec2i windowSize;
} BlitParams;


// because some macros need a typename without an asterisk
typedef bool*  boolp;
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
typedef vec2* vec2p;
typedef vec3* vec3p;
typedef vec4* vec4p;
typedef vec2d* vec2dp;
typedef vec3d* vec3dp;
typedef vec4d* vec4dp;
typedef vec2l* vec2lp;
typedef vec3l* vec3lp;
typedef vec4l* vec4lp;
typedef vec2i* vec2ip;
typedef vec3i* vec3ip;
typedef vec4i* vec4ip;
typedef char*  charp; 
typedef char** charpp;









#endif // __gpuedit__types_h__
