

volatile int g_debugToggle = 0;

#include <execinfo.h>




void printbt(int fd) {

	void* btbuf[100];
	
	int btlen = backtrace(btbuf, 100);
	char** syms = backtrace_symbols(btbuf, btlen);
	
	for(int i = 1; i < btlen; i++) {
		dprintf(fd, "  %d: %s\n", i, syms[i]);
	}
	
	free(syms);
}





	
	

