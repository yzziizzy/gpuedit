#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <X11/keysymdef.h>

#include "sti/sti.h"

#include "commands.h"
#include "gui.h"



static struct {
	char* name;
	int val;
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
	unsigned int key;
} raw_keys[] = {
	{"down", XK_Down},
	{"up", XK_Up},
	{"tab", XK_Tab},
	{"enter", XK_Return},
	{"return", XK_Return},
	{NULL, 0},
};


static HashTable words;
static HashTable syms;
static HashTable cmd_enums;

static void init_words() {
	HT_init(&words, 16);
	HT_init(&syms, 2100);
	HT_init(&cmd_enums, 120);
	
	for(int i = 0; raw_keys[i].name != 0; i++) {
		HT_set(&words, raw_keys[i].name, raw_keys[i].key);
	}
	
	for(int i = 0; keysym_lookup[i].name != 0; i++) {
		HT_set(&syms, keysym_lookup[i].name, keysym_lookup[i].val);
	}
	
	for(int i = 0; enum_table[i].name != 0; i++) {
		HT_set(&cmd_enums, enum_table[i].name, enum_table[i].val);
	}
}


static int get_word(char* w) {
	int64_t n;
	if(HT_get(&words, w, &n)) return -1;
	return n;
} 

static void after(char** s, char* search) {
	char* e = strstr(*s, search);
	
	if(e) *s = e + strlen(search);
}


Cmd* CommandList_loadFile(char* path) {
	
	char buf[128];
	size_t len = 0;
	int64_t n = 0;
	
	char* src = readWholeFile(path, NULL);
	char** olines = strsplit_inplace(src, '\n', NULL);
	char** lines = olines; // keep original for freeing
	
	int cmdalloc = 32;
	int cmdlen = 0;
	Cmd* commands = calloc(1, sizeof(*commands) * cmdalloc);
	
	while(*lines) {
		char* s = *lines;
		
		
		// first, modifiers
		unsigned int m = 0;
		unsigned int key = 0;
		
		for(; *s; s++) {
			if(*s == ' ') { 
				s++;
				break;
			}
			else if(*s == 'L') {
				s++;
				     if(*s == 'C') m |= GUIMODKEY_LCTRL | GUIMODKEY_CTRL;
				else if(*s == 'A') m |= GUIMODKEY_LALT | GUIMODKEY_ALT;
				else if(*s == 'T') m |= GUIMODKEY_LTUX | GUIMODKEY_TUX;
				else if(*s == 'W') m |= GUIMODKEY_LTUX | GUIMODKEY_TUX;
			}
			else if(*s == 'R') {
				s++;
				     if(*s == 'C') m |= GUIMODKEY_RCTRL | GUIMODKEY_CTRL;
				else if(*s == 'A') m |= GUIMODKEY_RALT | GUIMODKEY_ALT;
				else if(*s == 'T') m |= GUIMODKEY_RTUX | GUIMODKEY_TUX;
				else if(*s == 'W') m |= GUIMODKEY_RTUX | GUIMODKEY_TUX;
			}
			else if(*s == 'C') m |= GUIMODKEY_CTRL;
			else if(*s == 'A') m |= GUIMODKEY_ALT;
			else if(*s == 'T') m |= GUIMODKEY_TUX;
			else if(*s == 'W') m |= GUIMODKEY_TUX;
			else {
				printf("Unknown character looking for command modifiers: '%c'\n", *s);
				s++;
			}
		}
		
		// next the main key
		for(; *s; s++) {
			if(*s == '\'') { // a key literal
				s++;
				
				if(*s == '\\') { // escape sequence
					s++;
					     if(*s == 'n') key = '\n'; 
					else if(*s == 'r') key = '\r'; 
					else if(*s == 'v') key = '\v'; 
					else if(*s == 't') key = '\t'; 
					
					continue;
				}
				
				key = *s;
				s += 2; // skip the closing quote too
				
				break;
			}
			
			
			if(*s == 'X' && *(s+1) == 'K' && *(s+2) == '_') {
				// X11 key macro
				// cat keysymdef.h | grep '#define' | egrep -o 'XK_[^ ]* *[x0-9a-f]*' | sed 's/  */", /g;s/^/{"/;s/$/},/'
				len = sscanf("%.*s ", 127, buf); 
				buf[len] = 0;
				s += len;
				
				if(HT_get(&syms, buf, &n)) {
					printf("invalid X11 keysym name: '%s'\n", buf);
					continue;
				}
				
				key = n;
				
				break;
			}
			
			// normal words:
			
			
		}
		
		// find an equals
		after(&s, "=");
		
		// the command enum
		int64_t n = 0;
		len = sscanf("%.*s ", 127, buf); 
		buf[len] = 0;
		s += len;
		
		if(HT_get(&cmd_enums, buf, &n)) {
			printf("unknown command enum: '%s'\n", buf);
			continue;
		}
		
		
		printf("cmd_enum: %d\n", n);
		
		if(cmdlen + 1 >= cmdalloc) {
			cmdalloc *= 2;
			commands = realloc(commands, sizeof(*commands) * cmdalloc);
		}
		
		Cmd* c = commands + cmdlen;
		cmdlen++;
		
		c->mods = m;
		c->keysym = key;
		c->cmd = n;
		
		c->amt = 0;
		c->flags = 0;
		
	}
	
	// terminate the list
	commands[cmdlen] = (Cmd){};
	
	return commands;
}


int Commands_ProbeCommand(GUIEvent* gev, Cmd* list, Cmd* out, unsigned int* iter) {
	
	unsigned int ANY = (GUIMODKEY_SHIFT | GUIMODKEY_CTRL | GUIMODKEY_ALT | GUIMODKEY_TUX);
	unsigned int ANY_MASK = ~ANY;
	
	unsigned int i = *iter;
	
	for(; list[i].cmd != 0; i++) {
// 			printf("%d, '%c', %x \n", gev->keycode, gev->keycode, gev->modifiers);
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


static get_index(unsigned int mods) {
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
