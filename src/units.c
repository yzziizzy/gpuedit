
#include  <stdio.h>
#include  <ctype.h>

#include "units.h"




char* unit_name_list[UNIT_MAX_VALUE] = {
#define X(a,...) [UNIT_##a] = #a,
	UNIT_LIST
#undef X
};


char* unit_base_name_list[UNIT_BASE_MAX_VALUE] = {
#define X(a,...) [UNIT_BASE_##a] = #a,
	UNIT_BASE_LIST
#undef X
};

int unit_bases[UNIT_MAX_VALUE] = {
#define X(a, b,...) [UNIT_##a] = UNIT_BASE_##b,
	UNIT_LIST
#undef X
};

int unit_bases_units[UNIT_MAX_VALUE] = {
#define X(a, b,...) [UNIT_##a] = UNIT_##b,
	UNIT_LIST
#undef X
};

double unit_conv_factor_to_SI_base[UNIT_MAX_VALUE] = {
#define X(a, b, c,...) [UNIT_##a] = c,
	UNIT_LIST
#undef X
};



static int best_unit(char* s) {
	int longest = 0;
	int best_enum = -1;

	for(int i = 0; i < UNIT_MAX_VALUE; i++) {
		int nl = strlen(unit_name_list[i]);
		if(0 == strncasecmp(s, unit_name_list[i], nl)) {
			if(nl > longest) {
				best_enum = i;
				longest = nl;
			}
		}
	}
	
	return best_enum;
}


static void add_dim(UnitValue* uv, int u, int power) {
	VEC_EACHP(&uv->dims, i, d) {
		if(d->type == u) {
			d->power += power;
			return;
		}
	}
	
	VEC_PUSH(&uv->dims, ((DimComp){.type = u, .power = power}));
}


static double conv_factor(int to, int from) {
	if(to == from) return 1;

	// only through SI base unit for now
	return unit_conv_factor_to_SI_base[from] / unit_conv_factor_to_SI_base[to];
}


UnitValue* UV_ParseString(char* s) {
	char* e;
	
	double d = strtod(s, &e);
	if(!e) return NULL;
	
	UnitValue* uv = calloc(1, sizeof(*uv));
	uv->scalar = d;
	
	while(*e) {
		while(isspace(*e)) e++;
	
		int u = best_unit(e);
		if(u < 0) break;
		
		// todo: parse ^N
		
		add_dim(uv, u, 1);
		
		e += strlen(unit_name_list[u]);
	}
	
	return uv;
}


// returns 1 if both args have exactly equal powers of exactly identical units
int UV_DimEqualExact(UnitValue* a, UnitValue* b) {
	if(VEC_LEN(&a->dims) != VEC_LEN(&b->dims)) return 0;
	
	VEC_EACHP(&a->dims, ia, da) {
		
		VEC_EACHP(&b->dims, ib, db) {
			if(da->type == db->type && da->power == db->power) {
				goto FOUND;
			}
		}
		
		return 0; // unmatched dimension
		
	FOUND:
	}
	
	return 1;
}


// returns 1 if both args have equal powers of respective base units
int UV_DimEquivalent(UnitValue* a, UnitValue* b) {
	
	int powers_a[UNIT_BASE_MAX_VALUE] = {0};
	int powers_b[UNIT_BASE_MAX_VALUE] = {0};
	
	
	VEC_EACHP(&a->dims, ia, da) {
		powers_a[unit_bases[da->type]] += da->power;
	}
	
	VEC_EACHP(&b->dims, ib, db) {
		powers_b[unit_bases[db->type]] += db->power;
	}
	
	
	return 0 == memcmp(powers_a, powers_b, sizeof(powers_a));
}


// does no unit conversion. 
UnitValue* UV_MulRaw(UnitValue* a, UnitValue* b) {
	
	UnitValue* uv = calloc(1, sizeof(*uv));
	uv->scalar = a->scalar * b->scalar;
	
	VEC_EACHP(&a->dims, ia, da) {
		add_dim(uv, da->type, da->power);
	}
	VEC_EACHP(&b->dims, ib, db) {
		add_dim(uv, db->type, db->power);
	}
	
	return uv;
}



UnitValue* UV_ConvertTo(UnitValue* v, UnitValue* dims) {
	
	if(!UV_DimEquivalent(v, dims)) return NULL;
	
	
	UnitValue* uv = calloc(1, sizeof(*uv));
	uv->scalar = v->scalar;

	int powers_v[UNIT_BASE_MAX_VALUE] = {0};
	int vd[UNIT_MAX_VALUE] = {0};
	
	VEC_EACHP(&v->dims, iv, dv) {
		powers_v[unit_bases[dv->type]] += dv->power;
		vd[dv->type] += dv->power;
	}
	
	double factor = 1;

	VEC_EACHP(&dims->dims, id, dd) {
//		if(powers_v[unit_bases[dd->type]] == 0) continue; 
		
		int dpower = dd->power;
		
		// try to copy dimensions exactly if possible
		if(vd[dd->type] > 0) {
			int amt = MIN(dpower, vd[dd->type]);
			
			add_dim(uv, dd->type, amt);
			
			vd[dd->type] -= amt;
			dpower -= amt;
		}
		
		if(dpower == 0) continue;
		
		// find an equivalent base to convert from
		for(int i = 0; i < UNIT_MAX_VALUE && dpower > 0; i++) {
			if(vd[i] == 0) continue;
			if(unit_bases[i] != unit_bases[dd->type]) continue;
		
			double sf = conv_factor(dd->type, i);
		
			int amt = MIN(dpower, vd[i]);
			for(int j = labs(amt); j; j--) factor *= sf;
		
			vd[i] -= amt;
			dpower -= amt;
		}
		
		if(dpower > 0) {
			fprintf(stderr, "leftover dpower in UV_ConvertTo: %s^%d\n", unit_name_list[dd->type], dpower);
		}
		
	}
	
	uv->scalar *= factor;

	return uv;
}






