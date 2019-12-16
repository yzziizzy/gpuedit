#include <string.h>
#include <ctype.h>

// HACK
#include "../buffer.h"



#define FINAL_TOKENS \
	X(linebreak) \
	X(id) \
	X(num) \
	X(string) \
	X(charlit) \
	X(mlcomment) \
	X(slcomment) \
	X(pound) \
	X(pound_pound) \
	X(preprocessor) \
	X(plus) \
	X(plus_plus) \
	X(plus_eq) \
	X(minus) \
	X(minus_minus) \
	X(minus_eq) \
	X(minus_gt) \
	X(star) \
	X(star_eq) \
	X(slash) \
	X(slash_eq) \
	X(eq) \
	X(eq_eq) \
	X(bang) \
	X(bang_eq) \
	X(quest) \
	X(colon) \
	X(colon_colon) \
	X(amp) \
	X(amp_amp) \
	X(amp_eq) \
	X(pipe) \
	X(pipe_pipe) \
	X(pipe_eq) \
	X(lbracket) \
	X(rbracket) \
	X(lbrace) \
	X(rbrace) \
	X(lparen) \
	X(rparen) \
	X(gt) \
	X(gt_eq) \
	X(gt_gt) \
	X(gt_gt_eq) \
	X(lt) \
	X(lt_eq) \
	X(lt_lt) \
	X(lt_lt_eq) \
	X(comma) \
	X(dot) \
	X(dot_dot_dot) \
	X(tilde) \
	X(tilde_eq) \
	X(pct) \
	X(pct_eq) \
	X(caret) \
	X(caret_eq) \
	X(semi) \
	\
	X(backslash) \
	X(mlcomment_star) \
	X(star_slash) \
	X(slash_slash) \
	X(slash_star) \
	X(dot_dot)



enum LexState {
	LST_NULL = 0,
	LST_INVALID,
	
	#define X(t) LST_##t,
		FINAL_TOKENS
	#undef X
};

char* stateNames[] = {
	[LST_NULL] = "<NULL State>",
	[LST_INVALID] = "<Invalid State>",
	
	#define X(t) [LST_##t] = #t,
		FINAL_TOKENS
	#undef X
};


// this is for the processing of the input stream overall
struct input_state {
	char* buffer;
	int length;
	int cursor;
	
	enum LexState tokenState;
	int tokenFinished;
};

// this is for the incremental lexing of each token, not the whole stream
struct lexer_state {
	enum LexState state;
	char* buffer;
	int blen;
	int balloc;
	
	int linenum;
	int charnum;
	
	size_t pastLeadingWS; // flag set to 1 upton first non-whitespace character on each line 
	char priorEscape; 
	char priorBackslash;
	
	enum LexState tokenState;
	int tokenFinished; // buffer should be consumed and cleaned at this point 
};



static int isKeyword(char* in);
static int eatchar(struct lexer_state* st, int c);





void hlfn(hlinfo* hl) {
	
	struct lexer_state ls;
	ls.state = LST_NULL;
	ls.buffer = malloc(9*4096); // shame on you for longer tokens :P
	ls.blen = 0;
	ls.balloc = 4096;
	ls.linenum = 0;
	ls.charnum = 0;
	ls.priorEscape = 0;
	ls.tokenState = LST_NULL;
	ls.tokenFinished = 0;
	
	struct input_state is;
	
	
	int q = 0;
	while(hl->dirtyLines > 0) {
		char* line;
		size_t llen;
		printf("new line: -------------\n");
		if(hl->getNextLine(hl, &line, &llen)) {
			printf("highlighter ran out of input early \n");
			break;
		};
		
		is.buffer = line;
		is.length = llen;
		is.cursor = 0;
		
// 		printf("%d: (%ld)'%s'\n", q, llen, line);
		int span = 0;
		// 			eat_token(&is, &ls);
		int flop = 0;
		
		for(int i = 0; i <= llen;) {
			int ret;
			
			
// 			printf("pre-state: %s\n", stateNames[ls.state]);
			// 0 if a new token is ready
			if(i == llen) {
// 				printf(" eating newline\n");
				ret = eatchar(&ls, '\n');
			}
			else {
// 				printf(" eating char: %c\n", line[i]);
				ret = eatchar(&ls, line[i]);
			}
			
// 			printf(" post-state: %s\n", stateNames[ls.state]);
			
			if(ls.tokenFinished) { 
				// token is ready
// 				printf(" token: '%.*s'\n", ls.blen, ls.buffer);
				
				if(ls.tokenState != LST_linebreak) {
					flop++;
				}
				
				hl->writeSection(hl, flop%2, span);
				
				// reset the lex state when done reading
				ls.tokenFinished = 0;
				ls.state = LST_NULL;
				ls.blen = 0;
				
				span = 0;
			}
			
			if(ret) {
				i++; // advance on ret == 1
				/*if( i < llen)*/ span++;
			}
		}
		
		
// 		hl->writeSection(hl, 0, span);
	
		/*
		for(int i = 0; i < llen; i++) {
			 if(line[i] == 'e') { // e is an important letter
				 hl->writeSection(hl, 0, span);
				 span = 0;
				 hl->writeSection(hl, 1, 1);
			}
			else {
				span++;
			}
		}*/
		hl->writeSection(hl, 0, span + 1); // the +1 is to eat the implicit linebreak
		
		q++;
	}
	
	
}


// must be sorted
static char* preprocessors[] = {
	"define",
	"elif",
	"else",
	"endif",
	"error",
	"if",
	"ifdef",
	"ifndef",
	"include",
	"line",
	"pragma",
	"undef",
	"warning",
	NULL,
};

static char* keywords[] = {
	"auto",
	"break",
	"case",
	"char",
	"const",
	"continue",
	"default",
	"do",
	"double",
	"else",
	"enum",
	"extern",
	"float",
	"for",
	"goto",
	"if",
	"int",
	"int16_t",
	"int32_t",
	"int64_t",
	"int8_t",
	"long",
	"register",
	"return",
	"short",
	"signed",
	"sizeof",
	"static",
	"struct",
	"switch",
	"typedef",
	"uint16_t",
	"uint32_t",
	"uint64_t",
	"uint8_t",
	"union",
	"unsigned",
	"void",
	"volatile",
	"while",
	NULL,
};


static int isKeyword(char* in) {
	char** s = keywords;
	while(*s) {
		if(0 == strcmp(*s, in)) return 1;
	}
	return 0;
}




static int eatchar(struct lexer_state* st, int c) {
	
#define push_char \
do { \
	st->buffer[st->blen] = c; \
	st->blen++; \
} while(0)

#define push_char_id_ret(_state) \
do { \
	st->buffer[st->blen] = c; \
	st->blen++; \
	st->state = _state; \
	return 1; \
} while(0)

#define push_char_ret \
do { \
	st->buffer[st->blen] = c; \
	st->blen++; \
	return 1; \
} while(0)

#define discard_id_ret(_state) \
do { \
	st->state = _state; \
	return 1; \
} while(0)

#define done \
do { \
	st->tokenFinished = 1; \
	st->tokenState = st->state; \
	return 0; \
} while(0)
	
	// hopefully this works
	st->charnum++;
	if(c == '\n') {
		st->linenum++;
		st->charnum = 0;
	}
	
// START:
	if(c == '\r' && st->priorBackslash) {
		return 1;
	}
	if(c == '\n' && st->priorBackslash) {
		st->priorBackslash = 0;
		return 1;
	}
	
	if(c == '\\') {
		st->priorBackslash = 1;
	}
	else {
		st->priorBackslash = 0;
	}
	
	if(!isspace(c) || st->pastLeadingWS > 0) {
		st->pastLeadingWS++;
	}
	if(c == '\n') {
		st->pastLeadingWS = 0;
	}


	switch(st->state) {
		case LST_NULL:
			if(isdigit(c)) push_char_id_ret(LST_num);
			if(isalpha(c) || c == '_') push_char_id_ret(LST_id);
			if(c == '"') discard_id_ret(LST_string);
			if(c == '\'') discard_id_ret(LST_charlit);
			
			if(c == '#') push_char_id_ret(LST_pound);
			if(c == '+') push_char_id_ret(LST_plus);
			if(c == '-') push_char_id_ret(LST_minus);
			if(c == '*') push_char_id_ret(LST_star);
			if(c == '/') push_char_id_ret(LST_slash);
			if(c == '=') push_char_id_ret(LST_eq);
			if(c == '!') push_char_id_ret(LST_bang);
			if(c == '?') push_char_id_ret(LST_quest);
			if(c == ':') push_char_id_ret(LST_quest);
			if(c == '&') push_char_id_ret(LST_amp);
			if(c == '|') push_char_id_ret(LST_pipe);
			if(c == '{') push_char_id_ret(LST_lbrace);
			if(c == '}') push_char_id_ret(LST_rbrace);
			if(c == '[') push_char_id_ret(LST_lbracket);
			if(c == ']') push_char_id_ret(LST_rbracket);
			if(c == '(') push_char_id_ret(LST_lparen);
			if(c == ')') push_char_id_ret(LST_rparen);
			if(c == '>') push_char_id_ret(LST_gt);
			if(c == '<') push_char_id_ret(LST_lt);
			if(c == ',') push_char_id_ret(LST_comma);
			if(c == '.') push_char_id_ret(LST_dot);
			if(c == '~') push_char_id_ret(LST_tilde);
			if(c == '^') push_char_id_ret(LST_caret);
			if(c == '%') push_char_id_ret(LST_pct);
			if(c == ';') push_char_id_ret(LST_semi);
			
// 			if(c == '\n') {
// 				push_char;
// 				st->state = LST_linebreak;
// 				done;
// 			}
			// TODO whitespace is eaten here, and all sorts of garbage
			
			return 1;
		
		case LST_id:
			if(isdigit(c) || isalpha(c) || c == '_') push_char_ret;
			done;
		
		case LST_preprocessor:
			if(c == '\n') {
				done;
			}
			push_char_ret;
		
		case LST_pound:
			if(st->pastLeadingWS == 2) {
				
				push_char_id_ret(LST_preprocessor);
				return 1;
			}
			
			if(c == '#') push_char_id_ret(LST_pound_pound);
			done;
			
		case LST_num:
			if(isdigit(c) || isalpha(c) || c == '.' || c == 'x' || c == 'e') push_char_ret;
			done;
			
		case LST_string:
			// BUG: need proper escaping
			if(st->priorEscape) { // process escape sequences
				st->priorEscape = 0;
				push_char_ret;
			}
			
			if(c == '\\') { 
				st->priorEscape = 1;
				return 1;
			}
			
			if(c == '"') { // string is closed
				st->tokenFinished = 1;
				st->tokenState = st->state;
				return 1;
			}
			
			// add a normal character to the string
			push_char_ret;
			
		case LST_charlit:
			if(st->priorEscape) { // process escape sequences
				st->priorEscape = 0;
				push_char_ret;
			}
			
			if(c == '\\') { 
				st->priorEscape = 1;
				return 1;
			}
			
			if(c == '\'') { // literal is closed
				st->tokenFinished = 1;
				st->tokenState = st->state;
				return 1;
			}
			
			// add a normal character to the character literal
			push_char_ret;
			
		case LST_slcomment:
			if(c == '\n') {
				push_char;
				st->tokenFinished = 1;
				st->tokenState = st->state;
			}
			push_char_ret;
		
		case LST_mlcomment:
			if(c == '*') push_char_id_ret(LST_mlcomment_star);
			push_char_ret;
			
		case LST_mlcomment_star:
			if(c == '/') {
				st->tokenFinished = 1;
				st->tokenState = LST_mlcomment;
				return 1;
			}
			push_char_id_ret(LST_mlcomment);
			
		case LST_plus:
			if(c == '+') push_char_id_ret(LST_plus_plus);
			if(c == '=') push_char_id_ret(LST_plus_eq);
			done;
			
		case LST_minus:
			if(c == '-') push_char_id_ret(LST_minus_minus);
			if(c == '=') push_char_id_ret(LST_minus_eq);
			if(c == '>') push_char_id_ret(LST_minus_gt);
			done;

		case LST_star:
			if(c == '/') push_char_id_ret(LST_star_slash);
			if(c == '=') push_char_id_ret(LST_star_eq);
			done;

		case LST_slash:
			if(c == '/') push_char_id_ret(LST_slcomment);
			if(c == '=') push_char_id_ret(LST_slash_eq);
			if(c == '*') push_char_id_ret(LST_mlcomment);
			done;
		
		case LST_eq:
			if(c == '=') push_char_id_ret(LST_eq_eq);
			done;
			
		case LST_dot:
			if(c == '.') push_char_id_ret(LST_dot_dot);
			if(isdigit(c)) push_char_id_ret(LST_num);
			done;
			
		case LST_dot_dot:
			if(c == '.') push_char_id_ret(LST_dot_dot_dot);
			st->tokenState = LST_INVALID;
			st->tokenFinished = 1;
			return 0;
			
		case LST_bang:
			if(c == '=') push_char_id_ret(LST_bang_eq);
			done;

		case LST_amp:
			if(c == '=') push_char_id_ret(LST_amp_eq);
			if(c == '&') push_char_id_ret(LST_amp_amp);
			done;
			
		case LST_pipe:
			if(c == '=') push_char_id_ret(LST_pipe_eq);
			if(c == '|') push_char_id_ret(LST_pipe_pipe);
			done;
		
		case LST_gt:
			if(c == '=') push_char_id_ret(LST_gt_eq);
			if(c == '>') push_char_id_ret(LST_gt_gt);
			done;
			
		case LST_gt_gt:
			if(c == '=') push_char_id_ret(LST_gt_gt_eq);
			done;
		
		case LST_lt:
			if(c == '=') push_char_id_ret(LST_lt_eq);
			if(c == '<') push_char_id_ret(LST_lt_lt);
			done;
			
		case LST_lt_lt:
			if(c == '=') push_char_id_ret(LST_lt_lt_eq);
			done;
		
		case LST_pct:
			if(c == '=') push_char_id_ret(LST_pct_eq);
			done;
			
		case LST_tilde:
			if(c == '=') push_char_id_ret(LST_tilde_eq);
			done;
			
		case LST_caret:
			if(c == '=') push_char_id_ret(LST_caret_eq);
			done;
		
		
		// terminal states
		default: 
			st->tokenFinished = 1;
			st->tokenState = st->state;
			return 0;
	}
	
}


