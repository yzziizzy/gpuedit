#ifndef  __units_h__
#define  __units_h__

#include "sti/sti.h"


#define UNIT_BASE_LIST \
	X(s, time) \
	X(m, length) \
	X(kg, mass) \
	X(A, current) \
	X(K, heat) \
	X(mol, amount) \
	X(cd, brightness)
	
	
// unit  si       to si    to imp
//       base     base      base
#define UNIT_LIST \
	X(nm, m, .000000001) \
	X(um, m, .000001) \
	X(mm, m, .001) \
	X(cm, m, .01) \
	X(m,  m, 1) \
	X(km, m, 1000) \
	\
	X(mil,  m, .0000254) \
	X(in,   m, .0254) \
	X(ft,   m, .3048) \
	X(mile, m, 1609.344) \
	\
	X(s, s, 1) \
	X(kg, kg, 1) \
	X(A, A, 1) \
	X(K, K, 1) \
	X(mol, mol, 1) \
	X(cd, cd, 1) \
	



enum {
#define X(a,...) UNIT_BASE_##a,
	UNIT_BASE_LIST
#undef X
	UNIT_BASE_MAX_VALUE
};



enum {
#define X(a,...) UNIT_##a,
	UNIT_LIST
#undef X
	UNIT_MAX_VALUE
};

extern char* unit_name_list[UNIT_MAX_VALUE];


typedef struct {
	int type; // from the list
	int power; // the exponent
} DimComp;


typedef struct {
	VEC(DimComp) dims;
	double scalar;
} UnitValue;





#endif  __units_h__
