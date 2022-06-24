#ifndef __gpuedit__log_h__
#define __gpuedit__log_h__


void LOG(int verbosity, char* fmt, ...);

// preset verbosity levels

#define L1(fmt, ...) LOG(1, fmt __VA_OPT__(,) __VA_ARGS__)
#define L2(fmt, ...) LOG(2, fmt __VA_OPT__(,) __VA_ARGS__)
#define L3(fmt, ...) LOG(3, fmt __VA_OPT__(,) __VA_ARGS__)
#define L4(fmt, ...) LOG(4, fmt __VA_OPT__(,) __VA_ARGS__)
#define L5(fmt, ...) LOG(5, fmt __VA_OPT__(,) __VA_ARGS__)


extern int g_log_verbosity_level;

#endif
