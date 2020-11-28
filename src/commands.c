#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <X11/keysymdef.h>

#include "sti/sti.h"

#include "commands.h"
#include "gui.h"



static struct {
	char* name;
	uint64_t val;
} enum_table[] = { 
#define X(a, b) {#a "_" #b, a##_##b},
	COMMANDTYPE_LIST
#undef X
};



static struct {
	char* name;
	uint64_t val;
} keysym_lookup[] = {
	#include "cmd_keysym_lookup.c"
	{NULL, NULL},
};



static struct { 
	char* name;
	uint64_t key;
} raw_flags[] = {
	{"scrollToCursor",   1 << 0},
	{"rehighlight",      1 << 1},
	{"resetCursorBlink", 1 << 2},
	{"undoSeqBreak",     1 << 3},
	{"hideMouse",        1 << 4},
	{NULL, 0},
};



static HT(uint64_t) syms;
static HT(uint64_t) flag_lookup;
static HT(uint64_t) cmd_enums;

static void init_words() {
	HT_init(&flag_lookup, 16);
	HT_init(&syms, 2100);
	HT_init(&cmd_enums, 120);
	

	for(int i = 0; keysym_lookup[i].name != 0; i++) {
		HT_set(&syms, keysym_lookup[i].name, keysym_lookup[i].val);
	}
	
	for(int i = 0; enum_table[i].name != 0; i++) {
		HT_set(&cmd_enums, enum_table[i].name, enum_table[i].val);
	}
	
	for(int i = 0; raw_flags[i].name != 0; i++) {
		HT_set(&flag_lookup, raw_flags[i].name, raw_flags[i].key);
	}
}


int Commands_ProbeCommand(GUIEvent* gev, Cmd* list, unsigned int mode, Cmd* out, unsigned int* iter) {
	
	unsigned int ANY = (GUIMODKEY_SHIFT | GUIMODKEY_CTRL | GUIMODKEY_ALT | GUIMODKEY_TUX);
	unsigned int ANY_MASK = ~ANY;
	
	unsigned int i = *iter;
	
	for(; list[i].cmd != 0; i++) {
// 			printf("%d, '%c', %x \n", gev->keycode, gev->keycode, gev->modifiers);
		if(list[i].mode != mode) continue;
		if(list[i].keysym != tolower(gev->keycode)) continue;
		if((list[i].mods & ANY) != (gev->modifiers & ANY)) continue;
		// TODO: specific mods
		
		// found
		*out = list[i];
		
		i++;
		*iter = i;
		return 1;
	}
	
	*iter = i;
	return 0; // no match
}

enum {
	has_ctl = 1<<0,
	has_alt = 1<<1,
	has_shift = 1<<2,
	has_tux = 1<<3,
};


static unsigned int get_index(unsigned int mods) {
	unsigned int o = 0;
	if(mods & GUIMODKEY_CTRL) o |= has_ctl;
	if(mods & GUIMODKEY_ALT) o |= has_alt;
	if(mods & GUIMODKEY_SHIFT) o |= has_shift;
	if(mods & GUIMODKEY_TUX) o |= has_tux;
	return o;
}


// split up a single command list by the modifiers on the commands
CmdList* Commands_SeparateCommands(Cmd* in) {
	CmdList* cl = pcalloc(cl);
	
	int counts[16] = {};
	int lens[16] = {};
	
	for(int i = 0; in[i].cmd != 0; i++) {
		counts[get_index(in[i].mods)]++;
	}
	
	for(int j = 0; j < 16; j++) {
		if(counts[j] <= 0) continue;
		
		cl->mods[j] = calloc(1, sizeof(*cl->mods[0]) * (counts[j] + 1));
	}
	
	for(int i = 0; in[i].cmd != 0; i++) {
		int j = get_index(in[i].mods);
		
		cl->mods[j][lens[j]] = in[i];
		
		lens[j]++;
	}
	
	return cl;
}


Cmd* CommandList_loadJSONFile(char* path) {
	json_file_t* jsf;
	
	jsf = json_load_path(path);
	Cmd* c = CommandList_loadJSON(jsf->root);
	json_file_free(jsf);
	
	return c;
}

Cmd* CommandList_loadJSON(json_value_t* root) {
	
	static int is_init = 0;
	if(!is_init) {
		is_init = 1;
		init_words();
	}
	
	if(root->type != JSON_TYPE_ARRAY) {
		fprintf(stderr, "Command List json root must be an array.\n");
		return NULL;
	}
	
	
	int cmd_count = root->len;
	Cmd* cmds = calloc(1, sizeof(*cmds) * (cmd_count + 1));
	
	
	int i = 0;
	uint64_t n;
	json_link_t* link = root->arr.head;
	json_value_t* v;
	for(;link; link = link->next) {
		
		// command enum
		char* s = json_obj_get_str(link->v, "cmd");
		if(s == NULL) {
			fprintf(stderr, "Command List entry missing cmd name\n");
			continue;
		}
		
		if(HT_get(&cmd_enums, s, &n)) {
			fprintf(stderr, "Unknown command enum: '%s'\n", s);
			continue;
		}
		cmds[i].cmd = n;
		
		// key
		s = json_obj_get_str(link->v, "key");
		if(s == NULL) {
			fprintf(stderr, "Command List entry missing key\n");
			continue;
		}
		
		if(*s == 'X' && *(s+1) == 'K') {
			// X11 key macro
			// cat keysymdef.h | grep '#define' | egrep -o 'XK_[^ ]* *[x0-9a-f]*' | sed 's/  */", /g;s/^/{"/;s/$/},/'
			if(HT_get(&syms, s, &n)) {
				fprintf(stderr, "Invalid X11 keysym name: '%s'\n", s);
				continue;
			}
			
			cmds[i].keysym = n;
		}
		else { // regular character literal
			cmds[i].keysym = s[0];
		}
		
		
		// optional modifiers
		s = json_obj_get_str(link->v, "mods");
		if(s) {
			unsigned int m = 0;
			
			for(; *s; s++) {
				if(*s == ' ') { 
					s++;
					break;
				}
				else if(*s == 'L') {
					s++;
						if(*s == 'C') m |= GUIMODKEY_LCTRL | GUIMODKEY_CTRL;
					else if(*s == 'A') m |= GUIMODKEY_LALT | GUIMODKEY_ALT;
					else if(*s == 'S') m |= GUIMODKEY_LSHIFT | GUIMODKEY_SHIFT;
					else if(*s == 'T') m |= GUIMODKEY_LTUX | GUIMODKEY_TUX;
					else if(*s == 'W') m |= GUIMODKEY_LTUX | GUIMODKEY_TUX;
				}
				else if(*s == 'R') {
					s++;
						if(*s == 'C') m |= GUIMODKEY_RCTRL | GUIMODKEY_CTRL;
					else if(*s == 'A') m |= GUIMODKEY_RALT | GUIMODKEY_ALT;
					else if(*s == 'S') m |= GUIMODKEY_RSHIFT | GUIMODKEY_SHIFT;
					else if(*s == 'T') m |= GUIMODKEY_RTUX | GUIMODKEY_TUX;
					else if(*s == 'W') m |= GUIMODKEY_RTUX | GUIMODKEY_TUX;
				}
				else if(*s == 'C') m |= GUIMODKEY_CTRL;
				else if(*s == 'A') m |= GUIMODKEY_ALT;
				else if(*s == 'S') m |= GUIMODKEY_SHIFT;
				else if(*s == 'T') m |= GUIMODKEY_TUX;
				else if(*s == 'W') m |= GUIMODKEY_TUX;
				else {
					printf("Unknown character looking for command modifiers: \"%c\"\n", *s);
					s++;
				}
			}
			
			cmds[i].mods = m;
		}
		
		// optional amt value (default 0)
		if(!json_obj_get_key(link->v, "amt", &v)) {
			if(v->type == JSON_TYPE_STRING) {
				cmds[i].str = strdup(v->s);
			}
			else if(v->type == JSON_TYPE_ARRAY) {
				// ONLY supports array of strings
				char** z = malloc(sizeof(*z) * (v->len + 1));
				
				int j = 0;
				json_link_t* link = v->arr.head;
				while(link) {
					z[j++] = strdup(link->v->s);
					
					link = link->next;
				}
				z[j] = NULL;
				
				cmds[i].pstr = z;
			}
			else {
				cmds[i].amt = json_as_int(v);
			}
		}
		
		// optional mode value (default 0)
		if(!json_obj_get_key(link->v, "mode", &v)) {
			cmds[i].mode = json_as_int(v);
		}
		
		// optional flag list
		if(!json_obj_get_key(link->v, "flags", &v)) {
			unsigned int flags = 0;
			
			if(v->type == JSON_TYPE_ARRAY) {
			
				json_link_t* l2 = v->arr.head;
				for(;l2; l2 = l2->next) {
					
					if(l2->v->type == JSON_TYPE_STRING) {
						uint64_t x;
						if(!HT_get(&flag_lookup, l2->v->s, &x)) {
							flags |= x;
						}
					}
					else {
						fprintf(stderr, "Invalid flag format in command list.\n");
					}
				}
				
				cmds[i].flags = flags;
			}
		}
		
		i++;
	}
	
	return cmds;
}

