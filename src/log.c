#include <stdio.h>
#include <stdarg.h>



int g_log_verbosity_level = 0;


void LOG(int verbosity, char* fmt, ...) {
	va_list ap;
	
	if(verbosity <= g_log_verbosity_level) {
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
	}
}



