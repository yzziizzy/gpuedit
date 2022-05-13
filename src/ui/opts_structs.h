#ifndef __gputk__opts_structs_h__
#define __gputk__opts_structs_h__

/*
#define STATE_ACTIVE 2
#define STATE_HOT    1
#define STATE_NORMAL 0
*/




#define charp char*



#define GUI_CONTROL_OPS_STRUCT_LIST \
	V(GUIButtonOpts, \
		Y(colors, 3,\
			X(Color4, bg, C4(.1,.1,.1,1), C4(.1,.1,.1,1), C4(.1,.1,.1,1)),\
			X(Color4, border, C4(.7,.7,.7,1), C4(.7,.4,.4,1), C4(.9,.1,.1,1)), \
			X(Color4, text, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)) \
		), \
		XX(Vector2, size, ((Vector2){150,20})), \
		XX(float, borderWidth, 2), \
		XX(float, fontSize, 14), \
		XX(charp, fontName, "Arial") \
	) \
	\
	V(GUICheckboxOpts, \
		Y(colors, 3,\
			X(Color4, bg, C4(.1,.1,.1,1), C4(.1,.1,.1,1), C4(.1,.1,.1,1)),\
			X(Color4, border, C4(.7,.7,.7,1), C4(.7,.4,.4,1), C4(.9,.1,.1,1)), \
			X(Color4, text, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)) \
		), \
		XX(Vector2, labelSize, ((Vector2){150,20})), \
		XX(float, boxSize, 20), \
		XX(float, borderWidth, 2), \
		XX(float, fontSize, 14), \
		XX(charp, fontName, "Arial") \
	) \
	\
	V(GUIRadioBoxOpts, \
		Y(colors, 3,\
			X(Color4, bg, C4(.1,.1,.1,1), C4(.1,.1,.1,1), C4(.1,.1,.1,1)),\
			X(Color4, border, C4(.7,.7,.7,1), C4(.7,.4,.4,1), C4(.9,.1,.1,1)), \
			X(Color4, text, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)) \
		), \
		XX(Vector2, labelSize, ((Vector2){150,20})), \
		XX(float, boxSize, 20), \
		XX(float, borderWidth, 2), \
		XX(float, fontSize, 14), \
		XX(charp, fontName, "Arial") \
	) \
	\
	V(GUIFloatSliderOpts, \
		Y(colors, 3,\
			X(Color4, bg, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)),\
			X(Color4, bar, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)), \
			X(Color4, text, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)) \
		), \
		XX(Vector2, size, ((Vector2){150,20})), \
		XX(int, precision, 20), \
		XX(float, fontSize, 14), \
		XX(charp, fontName, "Arial") \
	) \
	\
	V(GUIIntSliderOpts, \
		Y(colors, 3,\
			X(Color4, bg, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)),\
			X(Color4, bar, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)), \
			X(Color4, text, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)) \
		), \
		XX(Vector2, size, ((Vector2){150,20})), \
		XX(float, fontSize, 14), \
		XX(charp, fontName, "Arial") \
	) \
	\
	V(GUIOptionSliderOpts, \
		Y(colors, 3,\
			X(Color4, bg, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)),\
			X(Color4, bar, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)), \
			X(Color4, text, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)) \
		), \
		XX(Vector2, size, ((Vector2){150,20})), \
		XX(float, fontSize, 14), \
		XX(charp, fontName, "Arial") \
	) \
	\
	V(GUISelectBoxOpts, \
		Y(colors, 3,\
			X(Color4, bg, C4(.1,.1,.1,1), C4(.1,.1,.1,1), C4(.1,.1,.1,1)),\
			X(Color4, border, C4(.7,.7,.7,1), C4(.7,.4,.4,1), C4(.9,.1,.1,1)), \
			X(Color4, text, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)) \
		), \
		XX(Vector2, labelSize, ((Vector2){150,20})), \
		XX(Vector2, size, ((Vector2){150,20})), \
		XX(float, borderWidth, 2), \
		XX(float, fontSize, 14), \
		XX(charp, fontName, "Arial") \
	) \
	\
	V(GUIEditOpts, \
		Y(colors, 3,\
			X(Color4, bg, C4(.1,.1,.1,1), C4(.1,.1,.1,1), C4(.1,.1,.1,1)),\
			X(Color4, border, C4(.7,.7,.7,1), C4(.7,.4,.4,1), C4(.9,.1,.1,1)), \
			X(Color4, text, C4(1,1,1,1), C4(1,1,1,1), C4(1,1,1,1)) \
		), \
		XX(Color4, cursorColor, C4(1,1,1,1)), \
		XX(Color4, selectionBgColor, C4(1,1,1,1)), \
		XX(Vector2, size, ((Vector2){150,20})), \
		XX(float, borderWidth, 2), \
		XX(float, fontSize, 14), \
		XX(charp, fontName, "Arial") \
	) \





#define ARG_N(_1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30,_31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, N, ...) N
#define RSEQ_N() 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0
#define NARG_(...)    ARG_N(__VA_ARGS__)    
#define NARG(...)     NARG_(__VA_ARGS__, RSEQ_N())

#define P_(a,b) a##b
#define P(a,b) P_(a,b)
#define O(o) o

#define EX(m, c, ...) P(EX_, NARG(__VA_ARGS__))(m, c,##__VA_ARGS__)
#define EX_1(m, c, x) O(m)(c,x)
#define EX_2(m, c, x, ...) O(m)(c,x), EX_1(m, c,##__VA_ARGS__)
#define EX_3(m, c, x, ...) O(m)(c,x), EX_2(m, c,##__VA_ARGS__)
#define EX_4(m, c, x, ...) O(m)(c,x), EX_3(m, c,##__VA_ARGS__)
#define EX_5(m, c, x, ...) O(m)(c,x), EX_4(m, c,##__VA_ARGS__)
#define EX_6(m, c, x, ...) O(m)(c,x), EX_5(m, c,##__VA_ARGS__)
#define EX_7(m, c, x, ...) O(m)(c,x), EX_6(m, c,##__VA_ARGS__)
#define EX_8(m, c, x, ...) O(m)(c,x), EX_7(m, c,##__VA_ARGS__)
#define EX_9(m, c, x, ...) O(m)(c,x), EX_8(m, c,##__VA_ARGS__)
#define EX_10(m, c, x, ...) O(m)(c,x), EX_9(m, c,##__VA_ARGS__)
#define EX_11(m, c, x, ...) O(m)(c,x), EX_10(m, c,##__VA_ARGS__)
#define EX_12(m, c, x, ...) O(m)(c,x), EX_11(m, c,##__VA_ARGS__)
#define EX_13(m, c, x, ...) O(m)(c,x), EX_12(m, c,##__VA_ARGS__)
#define EX_14(m, c, x, ...) O(m)(c,x), EX_13(m, c,##__VA_ARGS__)
#define EX_15(m, c, x, ...) O(m)(c,x), EX_14(m, c,##__VA_ARGS__)
#define EX_16(m, c, x, ...) O(m)(c,x), EX_15(m, c,##__VA_ARGS__)
#define EX_17(m, c, x, ...) O(m)(c,x), EX_16(m, c,##__VA_ARGS__)
#define EX_18(m, c, x, ...) O(m)(c,x), EX_17(m, c,##__VA_ARGS__)
#define EX_19(m, c, x, ...) O(m)(c,x), EX_18(m, c,##__VA_ARGS__)
#define EX_20(m, c, x, ...) O(m)(c,x), EX_19(m, c,##__VA_ARGS__)

#define EX2(m, c, ...) P(EX2_, NARG(__VA_ARGS__))(m, c, __VA_ARGS__)
#define EX2_1(m, c, x) O(m)(c,x)
#define EX2_2(m, c, x, ...) O(m)(c,x) EX2_1(m, c, __VA_ARGS__)
#define EX2_3(m, c, x, ...) O(m)(c,x) EX2_2(m, c, __VA_ARGS__)
#define EX2_4(m, c, x, ...) O(m)(c,x) EX2_3(m, c, __VA_ARGS__)
#define EX2_5(m, c, x, ...) O(m)(c,x) EX2_4(m, c, __VA_ARGS__)
#define EX2_6(m, c, x, ...) O(m)(c,x) EX2_5(m, c, __VA_ARGS__)
#define EX2_7(m, c, x, ...) O(m)(c,x) EX2_6(m, c, __VA_ARGS__)
#define EX2_8(m, c, x, ...) O(m)(c,x) EX2_7(m, c, __VA_ARGS__)
#define EX2_9(m, c, x, ...) O(m)(c,x) EX2_8(m, c, __VA_ARGS__)
#define EX2_10(m, c, x, ...) O(m)(c,x) EX2_9(m, c, __VA_ARGS__)
#define EX2_11(m, c, x, ...) O(m)(c,x) EX2_10(m, c, __VA_ARGS__)
#define EX2_12(m, c, x, ...) O(m)(c,x) EX2_11(m, c, __VA_ARGS__)
#define EX2_13(m, c, x, ...) O(m)(c,x) EX2_12(m, c, __VA_ARGS__)
#define EX2_14(m, c, x, ...) O(m)(c,x) EX2_13(m, c, __VA_ARGS__)
#define EX2_15(m, c, x, ...) O(m)(c,x) EX2_14(m, c, __VA_ARGS__)
#define EX2_16(m, c, x, ...) O(m)(c,x) EX2_15(m, c, __VA_ARGS__)
#define EX2_17(m, c, x, ...) O(m)(c,x) EX2_16(m, c, __VA_ARGS__)
#define EX2_18(m, c, x, ...) O(m)(c,x) EX2_17(m, c, __VA_ARGS__)
#define EX2_19(m, c, x, ...) O(m)(c,x) EX2_18(m, c, __VA_ARGS__)
#define EX2_20(m, c, x, ...) O(m)(c,x) EX2_19(m, c, __VA_ARGS__)




#define W(n, x) x
#define O2(n, x) x
#define V(n, ...) typedef struct n { EX2(W, n, __VA_ARGS__) } n;
#define Y(n, c, ...) struct { EX2(W, n, __VA_ARGS__) } n[c];
#define X(t, n, v,...) t n;
#define XX(t, n, v) t n;
	GUI_CONTROL_OPS_STRUCT_LIST
#undef W
#undef X
#undef XX
#undef Y
#undef Z
#undef V



#undef charp



/*
#define G(a,b) a.b
#define I(a,b) a.b
#define F(a,b) a = b;
#define XX(t,n,v) n = v;

#define H(a, b) I(a, O(F)b)
#define X(t,n,v) (n, v)
#define Y(a, b, ...) EX(H, a, __VA_ARGS__)
//#define V(m, ...) __VA_ARGS__
#define V(m, ...) EX2(G, m, __VA_ARGS__)
XLIST
*/


typedef GUIButtonOpts GUIToggleButtonOpts;
typedef struct GUIEditOpts GUIIntEditOpts;







#endif //__gputk__opts_structs_h__
