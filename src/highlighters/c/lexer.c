#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "../../sti/sti.h"
#include "lexer.h"
#include "../../highlighterAPI.h"




char* state_names[] = {
	[LST_INVALID] = "LST_INVALID",
	#define PARSER_INCLUDE_ENUM_NAMES
	#include "./parser_generated.h"
	#undef PARSER_INCLUDE_ENUM_NAMES
	[LST_MAXVALUE] = "LST_MAXVALUE",
};


#define PARSER_INCLUDE_TERMINAL_DATA_DEFS
#include "./parser_generated.h"
#undef PARSER_INCLUDE_TERMINAL_DATA_DEFS

char** state_data[] = {
	#define PARSER_INCLUDE_TERMINAL_DATA
	#include "./parser_generated.h"
	#undef PARSER_INCLUDE_TERMINAL_DATA
};

enum terminal_cats {
	#define TDX_block(k, v)
	#define TDX_type(k, v)
	#define TDX_cat(k, v) CAT_##v,
	#define TDX(k, v) TDX_##k(k, v)
	#define PARSER_INCLUDE_UNIQUE_TERMINAL_PAIRS
	#include "./parser_generated.h"
	#undef PARSER_INCLUDE_UNIQUE_TERMINAL_PAIRS
	#undef TDX
	#undef TDX_cat
	#undef TDX_type
	#undef TDX_block
	CAT_MAX_VALUE,
};

char* cat_names[] = {
	#define TDX_block(k, v)
	#define TDX_type(k, v)
	#define TDX_cat(k, v) [CAT_##v] = #v,
	#define TDX(k, v) TDX_##k(k, v)
	#define PARSER_INCLUDE_UNIQUE_TERMINAL_PAIRS
	#include "./parser_generated.h"
	#undef PARSER_INCLUDE_UNIQUE_TERMINAL_PAIRS
	#undef TDX
	#undef TDX_cat
	#undef TDX_type
	#undef TDX_block
};

int terminal_lookup[] = {
	#define TDX_block(s, k, v)
	#define TDX_type(s, k, v)
	#define TDX_cat(s, k, v) [s] = CAT_##v,
	#define TDX(s, k, v) TDX_##k(s, k, v)
	#define PARSER_INCLUDE_TERMINAL_TRIPLETS
	#include "./parser_generated.h"
	#undef PARSER_INCLUDE_TERMINAL_TRIPLETS
	#undef TDX
	#undef TDF_cat
	#undef TDF_type
	#undef TDF_block
};



// this is for the incremental lexing of each token, not the whole stream
struct lexer_state {
	enum LexState state;
	char* buffer;
	int blen;
	int balloc;
	
	int linenum;
	int charnum;
	
	enum LexState tokenState;
	int tokenFinished; // buffer should be consumed and cleaned at this point 
};





static int eatchar(struct lexer_state* st, int c, char suppress) {
	#define PARSER_INCLUDE_CSETS
	#include "./parser_generated.h"
	#undef PARSER_INCLUDE_CSETS
	
#define push_char_id(_state) \
do { \
	st->state = _state; \
	goto PUSH_CHAR_RET; \
} while(0)


#define discard_char_id(_state) \
do { \
	st->state = _state; \
	return 1; \
} while(0)


#define retry_as(_state) \
do { \
	st->state = _state; \
	goto RETRY; \
} while(0);

#define done_zero_move(_state) \
do { \
	st->state = _state; \
	goto TOKEN_DONE; \
} while(0);

#define push_char_done(_state) \
do { \
	st->state = _state; \
	goto PUSH_CHAR_DONE; \
} while(0);

#define charset_has(cs, c) (c <= cs##_len && !!cs[c])

	// hopefully this works
	st->charnum++;
	if(c == '\n') {
		st->linenum++;
		st->charnum = 0;
	}
	

RETRY:
	switch(st->state) {
		#define PARSER_INCLUDE_SWITCH
		#include "./parser_generated.h"
		#undef PARSER_INCLUDE_SWITCH
		
		default: 
			printf("Lexer reached default: %d\n", st->state);
			st->state = LST_NULL; 
			return 0;
	}
	
	assert(0);
	// never gets here
ERROR:
	printf("Lexer error at line %d:%d: state %d(%s) %d='%c' \n", st->linenum, st->charnum, st->state, state_names[st->state], c, c);
	st->state = LST_NULL; 
	st->blen = 0;
	return 1;

TOKEN_DONE:
	st->tokenFinished = 1;
	st->tokenState = st->state;
	return 0;
	
PUSH_CHAR_RET: 
	if(!suppress) {
		if(st->blen >= st->balloc) {
			st->balloc *= 2;
			st->buffer = realloc(st->buffer, sizeof(*st->buffer) * st->balloc);
		}
		st->buffer[st->blen] = c; 
		st->blen++;
	}
	return 1;
	
PUSH_CHAR_DONE: 
	if(!suppress) {
		if(st->blen >= st->balloc) {
			st->balloc *= 2;
			st->buffer = realloc(st->buffer, sizeof(*st->buffer) * st->balloc);
		}
		st->buffer[st->blen] = c; 
		st->blen++;
	}
	st->tokenFinished = 1;
	st->tokenState = st->state;
	return 1; 


}




void hlfn(HLContext* hl) {
//	printf("\n");
//	TokenStream* ts = calloc(1, sizeof(*ts));
	
	struct lexer_state ls = {
		.state = 0,
		.balloc = 256,
		.blen = 0,
		.buffer = calloc(1, 256),
		
		.state = LST_NULL,
		.linenum = 0,
		.charnum = 0,
		
		.tokenState = 0,
		.tokenFinished = 0,
	};


	struct input_state is;
	
	int span = 0; // ls.blen does not track ingored characters, like whitespace
	              // span does

	int q = 0;
	while(hl->dirtyLines > 0) {
		char* line;
		size_t llen;
		
		if(hl->getNextLine(hl, &line, &llen)) {
			break;
		};

		is.buffer = line;
		is.length = llen;
		is.cursor = 0;
		
		int flop = 0;
		
		for(int i = 0; i <= llen;) {
			int ret;
						
			// newlines are not given, nor written back
			// the 3rd param to eatchar() suppresses output
			if(i == llen) {
				ret = eatchar(&ls, '\n', 1);
//				printf("  i: %d, char: \\n, span: %d (ret=%d) state: %s\n", i, span, ret, state_names[ls.state]);
			
			}
			else {
				ret = eatchar(&ls, line[i], 0);
				if(ret) span++;
//				printf("  i: %d, char: %d, span: %d (ret=%d) state: %s\n",
//					 i, line[i], span, ret, state_names[ls.state]);
			
			}
			
			
			if(ls.tokenFinished) { 
				// token is ready
 				
				int hlo = terminal_lookup[ls.tokenState];
				
//				printf(" token: %d %d, '%.*s' (ret=%d)\n", 
//					hlo, span, ls.blen, ls.buffer, ret);
				
				
				hl->writeSection(hl, hlo, span);
				
				// reset the lex state when done reading
				ls.tokenFinished = 0;
				ls.state = LST_NULL;
				ls.blen = 0;
				
				span = 0;
			}
			
			
			if(ret || i == llen) {
				i++; // advance on ret == 1
			}
		}
		//printf("line ending\n");
		
		
		
		//hl->writeSection(hl, 0, span); // the +1 is to eat the implicit linebreak
		
		q++;
	}
	
	// finish off the last token
	hl->writeSection(hl, terminal_lookup[ls.tokenState], span);
	
	free(ls.buffer);
}



	/*
	for(int i = 0; source[i];) {
		int ret;
		ret = eatchar(&ls, source[i]);
		
		if(ls.tokenFinished) { 
			// token is ready
			VEC_INC(&ts->tokens);
			LexerToken* t = &VEC_TAIL(&ts->tokens);
			
			t->tokenState = ls.tokenState;
			t->tokenText = strndup(ls.buffer, ls.blen);
			t->line = ls.linenum;
			t->character = ls.charnum;
			t->sourceFile = NULL;
			
// 			printf("got token: #%d (%s) '%.*s'\n", ls.tokenState, state_names[ls.tokenState], ls.blen, ls.buffer);
			
			// reset the lex state when done reading
			ls.tokenFinished = 0;
			ls.state = LST_NULL;
			ls.blen = 0;
		}
		
		if(ret) {
			i++; // advance on ret == 1
		}
	}

// 	printf("last token: #%d (%s) '%.*s'\n", ls.tokenState, state_names[ls.tokenState], ls.blen, ls.buffer);
	
	free(source);
	
	return ts;
}
*/




static uint64_t get_style_count() { return CAT_MAX_VALUE; };
static void get_style_names(char** nameList, uint64_t maxNames) {};
static void get_style_defaults(StyleInfo* styles, uint64_t maxStyles);
// static void refresh_style(struct Highlighter*, hlinfo*);
static void hl_init() {};
static void hl_cleanup() {};


#define MIN(a,b) a > b ? b : a

void get_style_defaults(StyleInfo* styles, uint64_t maxStyles) {
	
	int max = MIN(maxStyles, CAT_MAX_VALUE);
	
	for(int i = 0; i < max; i++) {
		styles[i].index = i;
		styles[i].category = 0;
		styles[i].name = cat_names[i];
		styles[i].fgColor = (Color4f){(i%5)*.1 + .5, (i%50)*.01 + .5, ((i+17)%30)*.3 + .5, 1};
		styles[i].bgColor = (Color4f){0,0,0,0};
		
		styles[i].fgSelColor = (Color4f){(i%5)*.1 + .2, (i%50)*.01 + .2, ((i+17)%30)*.3 + .2, 1};
		styles[i].bgSelColor = (Color4f){1,1,1,1};
		
		styles[i].underline = 0;
		styles[i].bold = 0;
		styles[i].italic = 0;
		styles[i].useFGDefault = 0;
		styles[i].useBGDefault = 1;
		styles[i].useFGSelDefault = 0;
		styles[i].useBGSelDefault = 0;
		
	}
}




void __attribute__((used)) gpuedit_list_highlighters(Allocator* al, HighlighterPluginInfo** hllist, uint64_t* count) {
	
	HighlighterPluginInfo* list = al->malloc(al, sizeof(*list));
	
	list->majorVersion = 0;
	list->minorVersion = 0;
	list->abiVersion = 0;
	list->name = "c";
	list->description = "C syntax highlighter";
	list->author = "yzziizzy";
	list->extensions = "c;h";
	
	list->getStyleCount = get_style_count;
	list->getStyleNames = get_style_names;
	list->getStyleDefaults = get_style_defaults;
	list->refreshStyle = hlfn;
	list->init = hl_init;
	list->cleanup = hl_cleanup;
	
	
	*hllist = list;
	*count = 1;
}


