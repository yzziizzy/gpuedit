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

static struct { 
	char* name;
	unsigned int key;
} raw_flags[] = {
	{"scrollToCursor",   1 << 0},
	{"rehighlight",      1 << 1},
	{"resetCursorBlink", 1 << 2},
	{"undoSeqBreak",     1 << 3},
	{"hideMouse",        1 << 4},
	{NULL, 0},
};



static HashTable words;
static HashTable syms;
static HashTable flag_lookup;
static HashTable cmd_enums;

static void init_words() {
	HT_init(&words, 16);
	HT_init(&flag_lookup, 16);
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
	
	for(int i = 0; raw_flags[i].name != 0; i++) {
		HT_set(&flag_lookup, raw_flags[i].name, raw_flags[i].key);
	}
}


static int get_word(char* w) {
	int64_t n;
	if(HT_get(&words, w, &n)) return -1;
	return n;
} 
static unsigned int get_flag(char* w) {
	int64_t n;
	if(HT_get(&flag_lookup, w, &n)) return -1;
	return n;
} 

static void after(char** s, char* search) {
	char* e = strstr(*s, search);
	
	if(e) *s = e + strlen(search);
}



Cmd* CommandList_loadFile(char* path) {
	
	static is_init = 0;
	if(!is_init) {
		is_init = 1;
		init_words();
	}
	
	char buf[128];
	size_t len = 0;
	int64_t n = 0;
	
	char* src = readWholeFile(path, NULL);
	if(!src) {
		fprintf(stderr, "Could not read file '%s'\n", path);
		return NULL;
	}
	
	char** olines = strsplit_inplace(src, '\n', NULL);
	char** lines = olines; // keep original for freeing
	
	int cmdalloc = 32;
	int cmdlen = 0;
	Cmd* commands = calloc(1, sizeof(*commands) * cmdalloc);
	
	int lineNum = 1;
	
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
				printf("Unknown character looking for command modifiers: \"%c\" (%s:%d) \n", *s, path, lineNum);
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
				
// 				printf("found key literal: %c (%d)\n", key, key);
				
				break;
			}
			
			
			if(*s == 'X' && *(s+1) == 'K' && *(s+2) == '_') {
				// X11 key macro
				// cat keysymdef.h | grep '#define' | egrep -o 'XK_[^ ]* *[x0-9a-f]*' | sed 's/  */", /g;s/^/{"/;s/$/},/'
// 				len = sscanf(s, "%.*s ", 127, buf); 
				while(*s == ' ') s++;
				
				// the command enum
				char* e = s;
				while(isalnum(*e) || *e == '_') e++;
				
				len = e - s;
				strncpy(buf, s, len);
				buf[len] = 0;
				s += len;
				
				if(HT_get(&syms, buf, &n)) {
					printf("invalid X11 keysym name: '%s'\n", buf);
					continue;
				}
				
				key = n;
				
// 				printf("found X11 key: 0x%x\n", key);
				
				break;
			}
			
			// normal words:
			
			
		}
		
		// find an equals
		after(&s, "=");
		while(*s == ' ') s++;
		
		// the command enum
		char* e = s;
		while(isalnum(*e) || *e == '_') e++;
		
// 		int64_t n = 0;
// 		printf(s);
// 		sscanf(s, "%.*s%n", 127, buf, &len); 
		len = e - s;
		strncpy(buf, s, len);
		buf[len] = 0;
		s += len;
// 		printf("b: '%s'\n", buf);
		if(HT_get(&cmd_enums, buf, &n)) {
			printf("unknown command enum: '%s'\n", buf);
			
			lines++;
			lineNum++;
			continue;
		}
		
		// get the amount integer
		while(*s == ' ') s++;
		int amt = 0;
		amt = strtol(s, NULL, 10);
		e = strpbrk(s, "\n\r\t ");
		if(e) s = e;
		
		
		unsigned int flags = 0;
		// get the flags
		while(*s != '\n' && *s != '\r') {
			while(*s == ' ') s++;
			e = strpbrk(s, "\n\r\t ");
			char oc;
			if(e) {
				oc = *e;
				*e = 0;
			}
			
			
			int64_t x;
			if(!HT_get(&flag_lookup, s, &x)) {
				flags |= x;
			}
			
			if(!e) break;
			*e = oc;
			s = e;
		}
		
// 		printf("cmd_enum: %d\n", n);
// 		printf("mods: %x\n", m);
		
		if(cmdlen + 1 >= cmdalloc) {
			cmdalloc *= 2;
			commands = realloc(commands, sizeof(*commands) * cmdalloc);
		}
		
		Cmd* c = commands + cmdlen;
		cmdlen++;
		
		c->mods = m;
		c->keysym = key;
		c->cmd = n;
		
		c->amt = amt;
		c->flags = flags;
		
		lineNum++;
		lines++;
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
