#include <string.h>
#include <ctype.h>

// HACK
#include "../buffer.h"

#define CATS \
	X(Normal) \
	X(Identifier) \
	X(Keyword) \
	X(ControlFlow) \
	X(DataType) \
	X(Operator) \
	X(Number) \
	X(CharLiteral) \
	X(String) \
	X(CommentSingle) \
	X(CommentMulti) \
	X(LineContinuation) \
	X(Brackets) \
	X(Braces) \
	X(Parenthesis) \
	X(Semicolon) \
	X(Comma) \
	X(Preprocessor)



#define X_2(a,b)               X(a) X(a##b)
#define X_3(a,b,c)             X_2(a,b) X(a##b##c)
#define X_4(a,b,c,d)           X_3(a,b,c) X(a##b##c##d)
#define X_5(a,b,c,d,e)         X_4(a,b,c,d) X(a##b##c##d##e)
#define X_6(a,b,c,d,e,f)       X_5(a,b,c,d,e) X(a##b##c##d##e##f)
#define X_7(a,b,c,d,e,f,g)     X_6(a,b,c,d,e,f) X(a##b##c##d##e##f##g)
#define X_8(a,b,c,d,e,f,g,h)   X_7(a,b,c,d,e,f,g) X(a##b##c##d##e##f##g##h)
#define X_9(a,b,c,d,e,f,g,h,i) X_8(a,b,c,d,e,f,g,h) X(a##b##c##d##e##f##g##h##i)

#define Y_2(a,b, y,z)               X(a) Y(a##b, y,z)
#define Y_3(a,b,c, y,z)             X_2(a,b) Y(a##b##c, y,z)
#define Y_4(a,b,c,d, y,z)           X_3(a,b,c) Y(a##b##c##d, y,z)
#define Y_5(a,b,c,d,e, y,z)         X_4(a,b,c,d) Y(a##b##c##d##e, y,z)
#define Y_6(a,b,c,d,e,f, y,z)       X_5(a,b,c,d,e) Y(a##b##c##d##e##f, y,z)
#define Y_7(a,b,c,d,e,f,g, y,z)     X_6(a,b,c,d,e,f) Y(a##b##c##d##e##f##g, y,z)
#define Y_8(a,b,c,d,e,f,g,h, y,z)   X_7(a,b,c,d,e,f,g) Y(a##b##c##d##e##f##g##h, y,z)
#define Y_9(a,b,c,d,e,f,g,h,i, y,z) X_8(a,b,c,d,e,f,g,h) Y(a##b##c##d##e##f##g##h##i, y,z)

#define Y(a, b, c) X(a)

// X states are intermediaries
// Y states are (potentially) terminal

// ID,        name, category 
#define FINAL_TOKENS \
	Y(linebreak,  "", 0) \
	Y(id,         "", HLO_Identifier) \
	Y(num,        "", HLO_Number) \
	Y(string,     "", HLO_String) \
	Y(charlit,    "", HLO_CharLiteral) \
	Y(mlcomment,  "", HLO_CommentMulti) \
	Y(slcomment,  "", HLO_CommentSingle) \
	Y(pound,      "", HLO_Preprocessor) \
	Y(pound_pound,"", HLO_Preprocessor) \
	Y(preprocessor, "", HLO_Preprocessor) \
	Y(plus,       "Add", HLO_Operator) \
	Y(plus_plus,  "Increment", HLO_Operator) \
	Y(plus_eq,    "Add Assignment", HLO_Operator) \
	Y(minus,      "Minus", HLO_Operator) \
	Y(minus_minus,"Decrement", HLO_Operator) \
	Y(minus_eq,   "Minus Assignment", HLO_Operator) \
	Y(minus_gt,   "Arrow", HLO_Operator) \
	Y(star,       "Multiply", HLO_Operator) \
	Y(star_eq,    "Multiply Assignment", HLO_Operator) \
	Y(slash,      "Divide", HLO_Operator) \
	Y(slash_eq,   "Divide Assignment", HLO_Operator) \
	Y(eq,         "Assignment", HLO_Operator) \
	Y(eq_eq,      "Equality", HLO_Operator) \
	Y(bang,       "Not", HLO_Operator) \
	Y(bang_eq,    "Not Equals", HLO_Operator) \
	Y(quest,      "Ternary", HLO_Operator) \
	Y(colon,      "Ternary (colon)", HLO_Operator) \
	Y(colon_colon,"Blasphemy", HLO_Operator) \
	Y(amp,        "Bitwise AND", HLO_Operator) \
	Y(amp_amp,    "Logical AND", HLO_Operator) \
	Y(amp_eq,     "Bitwise AND Assignment", HLO_Operator) \
	Y(pipe,       "Bitwise OR", HLO_Operator) \
	Y(pipe_pipe,  "Logical OR", HLO_Operator) \
	Y(pipe_eq,    "Bitwise OR Assignment", HLO_Operator) \
	Y(lbracket,   "", HLO_Brackets) \
	Y(rbracket,   "", HLO_Brackets) \
	Y(lbrace,     "", HLO_Braces) \
	Y(rbrace,     "", HLO_Braces) \
	Y(lparen,     "", HLO_Parenthesis) \
	Y(rparen,     "", HLO_Parenthesis) \
	Y(gt,         "", HLO_Operator) \
	Y(gt_eq,      "", HLO_Operator) \
	Y(gt_gt,      "", HLO_Operator) \
	Y(gt_gt_eq,   "", HLO_Operator) \
	Y(lt,         "", HLO_Operator) \
	Y(lt_eq,      "", HLO_Operator) \
	Y(lt_lt,      "", HLO_Operator) \
	Y(lt_lt_eq,   "", HLO_Operator) \
	Y(comma,      "", HLO_Comma) \
	Y(dot,        "", HLO_Operator) \
	Y(dot_dot_dot,"", HLO_Operator) \
	Y(tilde,      "", HLO_Operator) \
	Y(tilde_eq,   "", HLO_Operator) \
	Y(pct,        "", HLO_Operator) \
	Y(pct_eq,     "", HLO_Operator) \
	Y(caret,      "", HLO_Operator) \
	Y(caret_eq,   "", HLO_Operator) \
	Y(semi,       "", HLO_Semicolon) \
	\
	Y(backslash,  "", HLO_LineContinuation) \
	X(mlcomment_star) \
	X(star_slash) \
	X(slash_slash) \
	X(slash_star) \
	X(dot_dot) \
	\
	/* keyword names */ \
	Y_4(_a,u,t,o, "", HLO_Keyword) \
	Y_5(_b,r,e,a,k, "", HLO_Keyword) \
	  X(_c) \
	Y_3(_ca,s,e, "", HLO_Keyword) \
	Y_3(_ch,a,r, "", HLO_DataType) \
	X_2(_co,n) \
	Y_2(_cons,t, "", HLO_DataType) \
	Y_5(_cont,i,n,u,e, "", HLO_Keyword) \
	  X(_d) \
	Y_6(_de,f,a,u,l,t, "", HLO_Keyword) \
	Y_5(_do,u,b,l,e, "", HLO_DataType) \
	  X(_e) \
	Y_3(_el,s,e, "", HLO_Keyword) \
	Y_3(_en,u,m, "", HLO_Keyword) \
	Y_5(_ex,t,e,r,n, "", HLO_DataType) \
	  X(_f) \
	Y_4(_fl,o,a,t, "", HLO_DataType) \
	Y_2(_fo,r, "", HLO_Keyword) \
	Y_4(_g,o,t,o, "", HLO_Keyword) \
	  X(_i) \
	Y_4(_inl,i,n,e, "", HLO_DataType) \
	Y_2(_in,t, "", HLO_DataType) \
	Y_3(_int8,_,t, "", HLO_DataType) \
	Y_4(_int1,6,_,t, "", HLO_DataType) \
	Y_4(_int3,2,_,t, "", HLO_DataType) \
	Y_4(_int6,4,_,t, "", HLO_DataType) \
	  Y(_if, "", HLO_Keyword) \
	Y_4(_l,o,n,g, "", HLO_DataType) \
	Y_9(_p,t,r,d,i,f,f,_,t, "", HLO_DataType) \
	X_2(_r,e) \
	Y_6(_reg,i,s,t,e,r, "", HLO_DataType) \
	Y_4(_ret,u,r,n, "", HLO_Keyword) \
	  X(_s) \
	Y_4(_sh,o,r,t, "", HLO_DataType) \
	  X(_si) \
	Y_4(_sig,n,e,d, "", HLO_DataType) \
	X_2(_siz,e) \
	Y_2(_size_,t, "", HLO_DataType) \
	Y_2(_sizeo,f, "", HLO_Keyword) \
	  X(_st) \
	Y_4(_sta,t,i,c, "", HLO_DataType) \
	Y_4(_str,u,c,t, "", HLO_Keyword) \
	Y_5(_sw,i,t,c,h, "", HLO_Keyword) \
	Y_7(_t,y,p,e,d,e,f, "", HLO_Keyword) \
	  X(_u) \
	X_3(_ui,n,t) \
	Y_3(_uint8,_,t, "", HLO_DataType) \
	Y_4(_uint1,6,_,t, "", HLO_DataType) \
	Y_4(_uint3,2,_,t, "", HLO_DataType) \
	Y_4(_uint6,4,_,t, "", HLO_DataType) \
	  X(_un) \
	Y_3(_uni,o,n, "", HLO_Keyword) \
	Y_6(_uns,i,g,n,e,d, "", HLO_DataType) \
	X_2(_v,o) \
	Y_2(_voi,d, "", HLO_DataType) \
	Y_6(_vol,a,t,i,l,e, "", HLO_DataType) \
	Y_5(_w,h,i,l,e, "", HLO_Keyword)


enum LexState {
	LST_NULL = 0,
	LST_INVALID,
	
	#define X(t) LST_##t,
		FINAL_TOKENS
	#undef X
	
	LST_MAXVALUE
};

char* stateNames[] = {
	[LST_NULL] = "<NULL State>",
	[LST_INVALID] = "<Invalid State>",
	
	#define X(t) [LST_##t] = #t,
		FINAL_TOKENS
	#undef X
};

enum HLOptions {
	HLO_None = 0,
	#define X(a) HLO_##a,
		CATS
	#undef X
	HLO_MAXVALUE,
};

int HLOLookup[] = {
	[LST_NULL] = 0,
	[LST_INVALID] = 0,
	#undef Y
	#define Y(a, b, c) [LST_##a] = c,
	#define X(t) [LST_##t] = 0,
		FINAL_TOKENS
	#undef X
	#undef Y
};

char* HLONames[] = {
	[HLO_None] = "None",
	#define X(a) [HLO_##a] = #a,
		CATS
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





void hlfn(Highlighter* h, hlinfo* hl) {
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
	ls.pastLeadingWS = 0;
	
	struct input_state is;
	
	
	int q = 0;
	while(hl->dirtyLines > 0) {
		char* line;
		size_t llen;
// 		printf("new line: -------------\n");
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
				
				int hlo = HLOLookup[ls.tokenState];
				
				hl->writeSection(hl, hlo, span);
				
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
	
	free(ls.buffer);
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


static int is_id_char(int c) {
	return isdigit(c) || isalpha(c) || c == '_';
}



static int eatchar(struct lexer_state* st, int c) {
	
#define push_char \
do { \
	st->buffer[st->blen] = c; \
	st->blen++; \
} while(0)

#define push_char_id_ret(_state) \
do { \
	st->state = _state; \
	goto LABEL_push_char_ret; \
} while(0)

#define if_push_char_id_ret(_char, _state) \
do { \
	if(c == _char) { \
		st->state = _state; \
		goto LABEL_push_char_ret; \
	} \
} while(0)

#define ipcir(a, b) if_push_char_id_ret(a, b)

#define push_char_ret goto LABEL_push_char_ret;
// do { \
// 	st->buffer[st->blen] = c; \
// 	st->blen++; \
// 	return 1; \
// } while(0)

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

#define retry_as(_state) \
do { \
	st->state = _state; \
	goto RETRY; \
} while(0);
	
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

RETRY:
	switch(st->state) {
		case LST_NULL:
			switch(c) {
				case 'a': push_char_id_ret(LST__a);
				case 'b': push_char_id_ret(LST__b);
				case 'c': push_char_id_ret(LST__c);
				case 'd': push_char_id_ret(LST__d);
				case 'e': push_char_id_ret(LST__e);
				case 'f': push_char_id_ret(LST__f);
				case 'g': push_char_id_ret(LST__g);
				case 'i': push_char_id_ret(LST__i);
				case 'l': push_char_id_ret(LST__l);
				case 'r': push_char_id_ret(LST__r);
				case 's': push_char_id_ret(LST__s);
				case 't': push_char_id_ret(LST__t);
				case 'u': push_char_id_ret(LST__u);
				case 'v': push_char_id_ret(LST__v);
				case 'w': push_char_id_ret(LST__w);
				
				case '"': discard_id_ret(LST_string);
				case '\'': discard_id_ret(LST_charlit);
				
				case '#': push_char_id_ret(LST_pound);
				case '+': push_char_id_ret(LST_plus);
				case '-': push_char_id_ret(LST_minus);
				case '*': push_char_id_ret(LST_star);
				case '/': push_char_id_ret(LST_slash);
				case '=': push_char_id_ret(LST_eq);
				case '!': push_char_id_ret(LST_bang);
				case '?': push_char_id_ret(LST_quest);
				case ':': push_char_id_ret(LST_quest);
				case '&': push_char_id_ret(LST_amp);
				case '|': push_char_id_ret(LST_pipe);
				case '{': push_char_id_ret(LST_lbrace);
				case '}': push_char_id_ret(LST_rbrace);
				case '[': push_char_id_ret(LST_lbracket);
				case ']': push_char_id_ret(LST_rbracket);
				case '(': push_char_id_ret(LST_lparen);
				case ')': push_char_id_ret(LST_rparen);
				case '>': push_char_id_ret(LST_gt);
				case '<': push_char_id_ret(LST_lt);
				case ',': push_char_id_ret(LST_comma);
				case '.': push_char_id_ret(LST_dot);
				case '~': push_char_id_ret(LST_tilde);
				case '^': push_char_id_ret(LST_caret);
				case '%': push_char_id_ret(LST_pct);
				case ';': push_char_id_ret(LST_semi);
				
				
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					push_char_id_ret(LST_num);
				
				case '_':
				default:
					if(isalpha(c)) push_char_id_ret(LST_id);
			}
			
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
		
		// keyword matching
		// not the prettiest names
			
		/* TODO: 
			__builtin_*
			__* keywords
			wchar_t
			*_t types?
			_Atomic and typedefs
			https://en.cppreference.com/w/c/types
		*/
		// ipcir == if_push_char_id_ret
#define ipcir_1(a,b) case LST__##a: ipcir(#b[0], LST__##a##b); retry_as(LST_id);  
#define ipcir_2(a,b,c) case LST__##a: ipcir(#b[0], LST__##a##b); ipcir(#c[0], LST__##a##c); retry_as(LST_id);  
#define ipcir_3(a,b,c,d) case LST__##a: ipcir(#b[0], LST__##a##b); ipcir(#c[0], LST__##a##c); ipcir(#d[0], LST__##a##d); retry_as(LST_id);  
#define ipcir_4(a,b,c,d,e) case LST__##a: ipcir(#b[0], LST__##a##b); ipcir(#c[0], LST__##a##c); ipcir(#d[0], LST__##a##d); ipcir(#e[0], LST__##a##e); retry_as(LST_id);  

// keywords must be ended with a final()
#define final(a) case LST__##a: if(is_id_char(c)) retry_as(LST_id); done;

// these are all final
#define _2_ipcir(a,b) ipcir_1(a, b); final(a##b);
#define _3_ipcir(a,b,c) ipcir_1(a, b); ipcir_1(a##b,c); final(a##b##c);
#define _4_ipcir(a,b,c,d) ipcir_1(a, b); _3_ipcir(a##b,c,d); 
#define _5_ipcir(a,b,c,d,e) ipcir_1(a, b); _4_ipcir(a##b,c,d,e); 
#define _6_ipcir(a,b,c,d,e,f) ipcir_1(a, b); _5_ipcir(a##b,c,d,e,f); 
#define _7_ipcir(a,b,c,d,e,f,g) ipcir_1(a, b); _6_ipcir(a##b,c,d,e,f,g); 
#define _8_ipcir(a,b,c,d,e,f,g,h) ipcir_1(a, b); _7_ipcir(a##b,c,d,e,f,g,h); 
#define _9_ipcir(a,b,c,d,e,f,g,h,i) ipcir_1(a, b); _8_ipcir(a##b,c,d,e,f,g,h,i); 

// #define _3_ipcir(a,b, c) ipcir_1(a, b); ipcir_1(a##b, c);
// #define _4_ipcir(a,b,c,d) _3_ipcir(a,b,c); ipcir_1(a##b##c, d);
// #define _5_ipcir(a,b,c,d,e) _4_ipcir(a,b,c,d); ipcir_1(a##b##c##d, e);
// #define _6_ipcir(a,b,c,d,e,f) _5_ipcir(a,b,c,d,e); ipcir_1(a##b##c##d##e, f);
// #define _7_ipcir(a,b,c,d,e,f,g) _6_ipcir(a,b,c,d,e,f); ipcir_1(a##b##c##d##e##f, g);
// 

		_4_ipcir(a,u,t,o);
		_5_ipcir(b,r,e,a,k);
		 ipcir_3(c, a, h, o);
		_3_ipcir(ca,s,e);
		_3_ipcir(ch,a,r);
		 ipcir_1(co, n); 
		 ipcir_2(con, s, t);
		_2_ipcir(cons, t);
		_5_ipcir(cont,i,n,u,e);
		ipcir_1(d, o);
		case LST__do: // tokens that are a prefix of other tokens need special handling
			ipcir('u', LST__dou); 
			if(is_id_char(c)) retry_as(LST_id);
			done;
		_4_ipcir(dou,b,l,e);
		
		 ipcir_3(e, l, n, x);
		_3_ipcir(el,s,e);
		_3_ipcir(en,u,m);
		_5_ipcir(ex,t,e,r,n);
		 ipcir_2(f, l, o);
		_2_ipcir(fo, r);
		_4_ipcir(fl,o,a,t);
		_4_ipcir(g,o,t,o); 
		 
		 ipcir_2(i, n, f);
		 ipcir_2(in, t, l);
		_4_ipcir(inl,i,n,e);
		case LST__int: // tokens that are a prefix of other tokens need special handling
			ipcir('8', LST__int8); 
			ipcir('1', LST__int1); 
			ipcir('3', LST__int3); 
			ipcir('6', LST__int6); 
			if(is_id_char(c)) retry_as(LST_id);
			done;
		_3_ipcir(int8,_,t);
		_4_ipcir(int1,6,_,t);
		_4_ipcir(int3,2,_,t);
		_4_ipcir(int6,4,_,t);
		_4_ipcir(l,o,n,g);
		
		_9_ipcir(p,t,r,d,i,f,f,_,t);
		
		 ipcir_4(s, h, i, t, w);
		_4_ipcir(sh,o,r,t);
		 ipcir_2(si, g, z);
		_4_ipcir(sig,n,e,d);
		 ipcir_1(siz, e);
		 ipcir_2(size, o, _);
		_2_ipcir(sizeo,f);
		_2_ipcir(size_,t);
		 ipcir_2(st, a, r);
		_4_ipcir(sta,t,i,c);
		_4_ipcir(str,u,c,t);
		_5_ipcir(sw,i,t,c,h);
		
		_7_ipcir(t,y,p,e,d,e,f);
		 
		 ipcir_2(u, i, n);
		 ipcir_1(ui, n);
		 ipcir_1(uin, t);
		case LST__uint: // tokens that are a prefix of other tokens need special handling
			ipcir('8', LST__uint8); 
			ipcir('1', LST__uint1); 
			ipcir('3', LST__uint3); 
			ipcir('6', LST__uint6); 
			if(is_id_char(c)) retry_as(LST_id);
			done;
		_3_ipcir(uint8,_,t);
		_4_ipcir(uint1,6,_,t);
		_4_ipcir(uint3,2,_,t);
		_4_ipcir(uint6,4,_,t);
		 ipcir_2(un, i, s);
		_3_ipcir(uni,o,n);
		_6_ipcir(uns,i,g,n,e,d);
		
		 ipcir_1(v,o)
		 ipcir_2(vo, i, l)
		 ipcir_1(voi,d);
		_6_ipcir(vol,a,t,i,l,e);
		 ipcir_1(r,e);
		 ipcir_2(re, t, g);
		 _6_ipcir(reg,i,s,t,e,r);
		 _4_ipcir(ret,u,r,n);
		_5_ipcir(w,h,i,l,e); 
		
		
		// terminal states
		default: 
			st->tokenFinished = 1;
			st->tokenState = st->state;
			return 0;
	}
	
	assert(0);
	// never gets here
	
LABEL_push_char_ret: 
	st->buffer[st->blen] = c; 
	st->blen++;
	return 1; 


}




void initCStyles(Highlighter* hl) {
	
	hl->numStyles = HLO_MAXVALUE;
	
	hl->styles = calloc(1, sizeof(*hl->styles) * hl->numStyles);
	
	for(int i = 0; i < HLO_MAXVALUE; i++) {
		hl->styles[i].index = i;
		hl->styles[i].category = 0;
		hl->styles[i].name = HLONames[i];
		hl->styles[i].fgColor = (Vector4){(i%5)*.1 + .5, (i%50)*.01 + .5, ((i+17)%30)*.3 + .5, 1};
		hl->styles[i].bgColor = (Vector4){0,0,0,0};
		
		hl->styles[i].fgSelColor = (Vector4){(i%5)*.1 + .2, (i%50)*.01 + .2, ((i+17)%30)*.3 + .2, 1};
		hl->styles[i].bgSelColor = (Vector4){1,1,1,1};
		
		hl->styles[i].underline = 0;
		hl->styles[i].bold = 0;
		hl->styles[i].italic = 0;
		hl->styles[i].useFGDefault = 0;
		hl->styles[i].useBGDefault = 1;
		hl->styles[i].useFGSelDefault = 0;
		hl->styles[i].useBGSelDefault = 0;
		
	}
}






