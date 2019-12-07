

// HACK
#include "../buffer.h"

/*

void classify_token(char* start, char** end) {
	
	int state = 0;
	
	char* p = start;
	char c = *p;
	
#define ONE   { *end = p + 1; return; }
#define TWO   { *end = p + 2; return; }
#define THREE { *end = p + 3; return; }
	
	if(isdigit(c)) {
		// must be a number
		
// 		printf("error\n");
		printf("number\n");
		return;
	}
	else if(isalpha(c)) {
		// might be an identifier or keyword
		
	}
	else if(c == '#') ONE;
	else if(c == '"') {
		// string
	}
	else if(c == '\'') {
		// char
	}
	else if(c == '/') {
		if(*(p+1) == '/') TWO;
		else if(*(p+1) == '*') TWO;
		else if(*(p+1) == '=') TWO; // operator
		else ONE; // division operator
	}
	else if(c == '+') {
		if(*(p+1) == '=') TWO;
		else if(*(p+1) == '+') TWO;
		else ONE;
	}
	else if(c == '-') {
		if(*(p+1) == '=') TWO;
		else if(*(p+1) == '-') TWO;
		else if(*(p+1) == '>') TWO;
		else ONE;
	}
	else if(c == '*') {
		if(*(p+1) == '=') TWO;
		else ONE;
	}
	else if(c == '&') {
		if(*(p+1) == '&') TWO;
		if(*(p+1) == '=') TWO;
		else ONE;
	}
	else if(c == '|') {
		if(*(p+1) == '|') TWO;
		if(*(p+1) == '=') TWO;
		else ONE;
	}
	else if(c == '^') {
		if(*(p+1) == '=') TWO;
		else ONE;
	}
	else if(c == '%') {
		if(*(p+1) == '=') TWO;
		else ONE;
	}
	else if(c == '!') ONE;
	else if(c == '=') {
		if(*(p+1) == '=') TWO;
		else ONE;
	}
	else if(c == '?') ONE;
	else if(c == '\\') {
		// TODO: save state and continue later
	}
	else if(c == '~') ONE;
	else if(c == ':') ONE;
	else if(c == '[') ONE;
	else if(c == ']') ONE;
	else if(c == '{') ONE;
	else if(c == '}') ONE;
	else if(c == ')') ONE;
	else if(c == '(') ONE;
	else if(c == '.') ONE;
	else if(c == ';') ONE;
	else if(c == ',') ONE;
	else if(c == '>') ONE;
		if(*(p+1) == '=') TWO;
		else if(*(p+1) == '>') {
			if(*(p+2) == '=') THREE;
			else TWO;
		}
		else ONE;
	}
	else if(c == '<') {
		if(*(p+1) == '=') TWO;
		else if(*(p+1) == '<') {
			if(*(p+2) == '=') THREE
			else TWO;
		}
		else ONE;
	}
	else {
		printf("token not identified\n");
		*end = p + 1;
	}
	
	
	
}




void parse_c(char* txt, size_t len) {
	
	char* s, *e;
	
	s = txt;
	
	while(e - s < len) {
		
		
		
	} 
	
	
	
	
}
*/


void hlfn(hlinfo* hl) {
	
	int q = 0;
	while(hl->dirtyLines > 0) {
		char* line;
		size_t llen;
		
		if(hl->getNextLine(hl, &line, &llen)) {
			printf("highlighter ran out of input early \n");
			break;
		};
		
// 		printf("%d: (%ld)'%s'\n", q, llen, line);
		
		int span = 0;
		for(int i = 0; i < llen; i++) {
			 if(line[i] == 'e') {
				 hl->writeSection(hl, 0, span);
				 span = 0;
				 hl->writeSection(hl, 1, 1);
			}
			else {
				span++;
			}
		}
		hl->writeSection(hl, 0, span + 1); // the +1 is to eat the implicit linebreak
		
		q++;
	}
	
	
}





