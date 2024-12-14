

#include "sti/sti.h"

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define h(...) dprintf(ofh, __VA_ARGS__);
#define d(...) dprintf(ofd, __VA_ARGS__);


#define OPTION_TYPE_LIST \
	X(invalid, NULL, 0) \
	X(float, float, sizeof(float) ) \
	X(vec2, vec2, (sizeof(float)*2) ) \
	X(Color4, Color4, sizeof(char*) ) \
	X(string, char*, sizeof(char*) ) \
	X(bool, bool, sizeof(char) ) \
	X(int, int, sizeof(int) ) \
	
	
enum {
#define X(a, ...) OT_##a,
	OPTION_TYPE_LIST
#undef X
	OT_MAX_VALUE,
};

const char* OT_name_list[] = {
#define X(a, b, ...) [OT_##a] = #a,
	OPTION_TYPE_LIST
#undef X
};

const char* OT_type_list[] = {
#define X(a, b, ...) [OT_##a] = #b,
	OPTION_TYPE_LIST
#undef X
};

const int OT_size_list[] = {
#define X(a, b, c, ...) [OT_##a] = c,
	OPTION_TYPE_LIST
#undef X
};


typedef struct Option {
	int type;
	char* name;
	
	char rescale;
	
	int value_cnt;
	void* values;
} Option;


typedef struct {
	char* name;
	int max_values;

	VEC(Option*) members;
} OptsStruct;





static int extract_type(sexp* x) {
	
	char* name = sexp_str(x, 0);
	
	for(int i = 0; i < OT_MAX_VALUE; i++) {
		if(!strcmp(OT_type_list[i], name)) {
			return i;
		}
	}
	
	return OT_invalid;
}

static char* extract_name(sexp* x) {
	return sexp_str(x, 1);
}







static void copy_invalid(sexp* x, void* dst) {}


static void copy_float(sexp* x, void* dst) {
	float* f = (float*)dst;
	f[0] = sexp_float(x, 0);
}

static void copy_vec2(sexp* x, void* dst) {
	float* f = (float*)dst;
	f[0] = sexp_float(x, 0);
	f[1] = sexp_float(x, 1);
}

static void copy_Color4(sexp* x, void* dst) {
	*(char**)dst = x->str;
	/*
	float* f = (float*)dst;
	f[0] = sexp_float(x, 0);
	f[1] = sexp_float(x, 1);
	f[2] = sexp_float(x, 2);
	f[3] = sexp_float(x, 3);*/
}

static void copy_string(sexp* x, void* dst) {
	*(char**)dst = x->str;
}

static void copy_bool(sexp* x, void* dst) {
	*(char*)dst = !!sexp_uint(x, 0);
}

static void copy_int(sexp* x, void* dst) {
	*(int*)dst = sexp_int(x, 0);
}



static void si_invalid(void* src, int ofd) {
	d("lol, fix your shit\n");
}


static void si_float(void* src, int ofd) {
	float* f = (float*)src;
	d("%f", *f)
}

static void si_vec2(void* src, int ofd) {
	float* f = (float*)src;
	d("((vec2){%f, %f})", f[0], f[1])
}

static void si_Color4(void* src, int ofd) {
	d("C4H(%s)", *(char**)src)
}

static void si_string(void* src, int ofd) {
	d("strdup(\"%s\")", *(char**)src)
}

static void si_bool(void* src, int ofd) {
	char* b = (char*)src;
	d("%d", !!*b)
}

static void si_int(void* src, int ofd) {
	int* b = (int*)src;
	d("%d", *b)
}


static void copyfn_invalid(char* srcptr, char* srcprop, int ofd) {
	d("lol, fix your shit\n");
}

static void copyfn_float(char* srcptr, char* srcprop, int ofd) {
	d("%s->%s", srcptr, srcprop);
}

static void copyfn_vec2(char* srcptr, char* srcprop, int ofd) {
	d("%s->%s", srcptr, srcprop);
}

static void copyfn_Color4(char* srcptr, char* srcprop, int ofd) {
	d("%s->%s", srcptr, srcprop);
}

static void copyfn_string(char* srcptr, char* srcprop, int ofd) {
	d("strdup(%s->%s)", srcptr, srcprop);
}

static void copyfn_bool(char* srcptr, char* srcprop, int ofd) {
	d("%s->%s", srcptr, srcprop);
}

static void copyfn_int(char* srcptr, char* srcprop, int ofd) {
	d("%s->%s", srcptr, srcprop);
}


// these functions need to output complete lines, including leading tabs and trailing newlines
static void freefn_invalid(char* ptr, char* prop, int ofd) {
	d("lol, fix your shit\n");
}
static void freefn_float(char* ptr, char* prop, int ofd) {}
static void freefn_vec2(char* ptr, char* prop, int ofd) {}
static void freefn_Color4(char* ptr, char* prop, int ofd) {}
static void freefn_bool(char* ptr, char* prop, int ofd) {}
static void freefn_int(char* ptr, char* prop, int ofd) {}

static void freefn_string(char* ptr, char* prop, int ofd) {
	d("\tfree(%s->%s);\n", ptr, prop);
}

static void rescalefn_invalid(char* raw, char* prop, char* scale, char* out, int ofd) {
	d("lol, fix your shit\n");
}
static void rescalefn_float(char* raw, char* prop, char* scale, char* out, int ofd) {
	d("\t%s->%s = %s->%s * %s;\n", out, prop, raw, prop, scale);
}
static void rescalefn_vec2(char* raw, char* prop, char* scale, char* out, int ofd) {
	d("\t%s->%s.x = %s->%s.x * %s;\n", out, prop, raw, prop, scale);
	d("\t%s->%s.y = %s->%s.y * %s;\n", out, prop, raw, prop, scale);
}
static void rescalefn_Color4(char* raw, char* prop, char* scale, char* out, int ofd) {}
static void rescalefn_bool(char* raw, char* prop, char* scale, char* out, int ofd) {}
static void rescalefn_int(char* raw, char* prop, char* scale, char* out, int ofd) {}

static void rescalefn_string(char* raw, char* prop, char* scale, char* out, int ofd) {}




OptsStruct* parse_optstruct(sexp* xos) {
	OptsStruct* os = calloc(1, sizeof(*os));
	
	os->name = VEC_ITEM(&xos->args, 0)->str;
	
	os->max_values = 0;
	
	for(int oi = 1; oi < VEC_LEN(&xos->args); oi++) {
		sexp* xo = VEC_ITEM(&xos->args, oi);
		
		Option* o = calloc(1, sizeof(*o));
		VEC_PUSH(&os->members, o);
		
		o->type = extract_type(xo);
		o->name = extract_name(xo);
		o->rescale = xo->brace == '<';
		
		o->value_cnt = VEC_LEN(&xo->args) - 2;
		os->max_values = MAX(os->max_values, o->value_cnt);
		
		o->values = calloc(1, OT_size_list[o->type] * o->value_cnt);
		for(int a = 0; a < o->value_cnt; a++) {
			sexp* xa = VEC_ITEM(&xo->args, a + 2);
			
			switch(o->type) {
				#define X(n, t, sz, ...) case OT_##n: copy_##n(xa, o->values + a * sz); break;
					OPTION_TYPE_LIST
				#undef X
			}
		}
	
	}
	
	return os;
}



int main(int argc, char* argv[]) {
	
	if(argc < 4) {
		fprintf(stderr, "Usage: %s <cfg.sex> <output_header.h> <output_source.c>\n", argv[0]);
		return 2;
	}
	
	sexp* root = sexp_parse_file(argv[1]);

	
//	sexp_print(1, root);
	
	VEC(OptsStruct*) structs;
	VEC_INIT(&structs);
	
	VEC_EACH(&root->args, i, a) {
		VEC_PUSH(&structs, parse_optstruct(a));
	}
	
	
	unlink(argv[3]);
	unlink(argv[2]);
	
	int ofd = open(argv[3], O_WRONLY | O_CREAT | O_EXCL, 0644);
	if(ofd == -1) {
		fprintf(stderr, "Could not create file '%s': %s\n", argv[3], strerror(errno));
		return 1;
	}
	
	int ofh = open(argv[2], O_WRONLY | O_CREAT | O_EXCL, 0644);
	if(ofh == -1) {
		fprintf(stderr, "Could not create file '%s': %s\n", argv[2], strerror(errno));
		return 1;
	}
	
	h("#ifndef __gputk__opts_structs_h__\n#define __gputk__opts_structs_h__\n")
	h("\n\n")
	h("/***********************************************\n")
	h("/  THIS FILE IS AUTOGENERATED. DO NOT EDIT.    /\n")
	h("/     see '%s' for details\n", argv[1]);
	h("***********************************************/\n")
	h("\n\n")
	
	
	h("#define GUI_CONTROL_OPTS_STRUCT_LIST \\\n");
	VEC_EACH(&structs, i, st) {
		h("\tX(GUI%sOpts, %s) \\\n", st->name, st->name)
	}
	h("\n\n");
	
	
	VEC_EACH(&structs, i, st) {
		h("typedef struct GUI%sOpts {\n", st->name);
		
		VEC_EACH(&st->members, mi, m) {
			h("\t%s %s;\n", OT_type_list[m->type], m->name);
		}
		
		h("} GUI%sOpts;\n\n", st->name);
	}
	
	h("\n\n")
	VEC_EACH(&structs, i, st) {
		h("int GUI%sOpts_LoadDefaults(GUI%sOpts* o, int version);\n", st->name, st->name);
		h("int GUI%sOpts_LoadScaledDefaults(GUI%sOpts* o, int maxVersion, float scale);\n", st->name, st->name);
		h("void GUI%sOpts_Copy(GUI%sOpts* dst, GUI%sOpts* src);\n", st->name, st->name, st->name);
		h("void GUI%sOpts_Free(GUI%sOpts* o);\n", st->name, st->name);
		h("void GUI%sOpts_UIRescale(GUI%sOpts* raw, float scale, GUI%sOpts* out);\n", st->name, st->name, st->name);
		h("\n")
	}
	
	
	d("\n\n")
	d("/***********************************************\n")
	d("/  THIS FILE IS AUTOGENERATED. DO NOT EDIT.    /\n")
	d("/     see '%s' for details\n", argv[1]);
	d("***********************************************/\n")
	d("\n\n")
	d("#include <stdlib.h>\n")
	d("\n\n")
	d("#include \"opts_structs.h\"\n")
	d("\n\n")
	
		
	VEC_EACH(&structs, i, st) {
		d("int GUI%sOpts_LoadDefaults(GUI%sOpts* o, int version) {\n", st->name, st->name);
		d("\n\tif(version < 0) return %d;\n\n", st->max_values);
		d("\tversion = version > %d ? %d : version;\n", st->max_values - 1, st->max_values - 1);
		d("\tswitch(version) {\n")
		
		for(int v = 0; v < st->max_values; v++) {
			d("\t\tcase %d:\n", v);
			
			VEC_EACH(&st->members, mi, m) {
				d("\t\t\to->%s = ", m->name);
				
				int mv = MIN(v, m->value_cnt - 1);
				
				switch(m->type) {
					#define X(n, t, sz, ...) case OT_##n: si_##n(m->values + sz * mv, ofd); break;
						OPTION_TYPE_LIST
					#undef X
				}
				
				d(";\n")
			}
			
			d("\t\t\tbreak;\n")
		}
		
		d("\t};\n");
		d("\treturn 0;\n};\n");
	}
	
	VEC_EACH(&structs, i, st) {
		d("int GUI%sOpts_LoadScaledDefaults(GUI%sOpts o[], int maxVersion, float scale) {\n", st->name, st->name);
		d("\tfor(int v = 0; v < maxVersion; v++) {\n")
		d("\t\tGUI%sOpts tmp;\n", st->name);
		d("\t\tGUI%sOpts_LoadDefaults(&tmp, v);\n", st->name);
		d("\t\tGUI%sOpts_UIRescale(&tmp, scale, &o[v]);\n", st->name);
		d("\t};\n");
		d("\treturn 0;\n};\n");
	}
		
	VEC_EACH(&structs, i, st) {
		d("void GUI%sOpts_Copy(GUI%sOpts* dst, GUI%sOpts* src) {\n", st->name, st->name, st->name);
		
		
		VEC_EACH(&st->members, mi, m) {
			d("\tdst->%s = ", m->name);
			
			switch(m->type) {
				#define X(n, t, sz, ...) case OT_##n: copyfn_##n("src", m->name, ofd); break;
					OPTION_TYPE_LIST
				#undef X
			}
			
			d(";\n")
		}
		
		d("};\n");
	}
		
	VEC_EACH(&structs, i, st) {
		d("void GUI%sOpts_Free(GUI%sOpts* o) {\n", st->name, st->name);
		
		VEC_EACH(&st->members, mi, m) {
			switch(m->type) {
				#define X(n, t, sz, ...) case OT_##n: freefn_##n("o", m->name, ofd); break;
					OPTION_TYPE_LIST
				#undef X
			}
		}
		
		d("};\n");
	}
	
	VEC_EACH(&structs, i, st) {
		d("void GUI%sOpts_UIRescale(GUI%sOpts* raw, float scale, GUI%sOpts* out) {\n", st->name, st->name, st->name);
		
		VEC_EACH(&st->members, mi, m) {
			if(!m->rescale) continue;
			
			switch(m->type) {
				#define X(n, t, sz, ...) case OT_##n: rescalefn_##n("raw", m->name, "scale", "out", ofd); break;
					OPTION_TYPE_LIST
				#undef X
			}
		}
		
		d("};\n");
	}
	
	
	
	h("\n\n\n")
	h("#endif // __gputk__opts_structs_h__")
	
	
	return 0;
}







