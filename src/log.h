#ifndef __gpuedit__log_h__
#define __gpuedit__log_h__


void LOG(int verbosity, char* fmt, ...);

// preset verbosity levels

/*
#define L1(fmt, ...) LOG(1, fmt __VA_OPT__(,) __VA_ARGS__)
#define L2(fmt, ...) LOG(2, fmt __VA_OPT__(,) __VA_ARGS__)
#define L3(fmt, ...) LOG(3, fmt __VA_OPT__(,) __VA_ARGS__)
#define L4(fmt, ...) LOG(4, fmt __VA_OPT__(,) __VA_ARGS__)
#define L5(fmt, ...) LOG(5, fmt __VA_OPT__(,) __VA_ARGS__)
*/

// no __VA_OPT__(,) on old GCC (7.5.0)
#define L1(fmt, ...) LOG(1, fmt, ##__VA_ARGS__)
#define L2(fmt, ...) LOG(2, fmt, ##__VA_ARGS__)
#define L3(fmt, ...) LOG(3, fmt, ##__VA_ARGS__)
#define L4(fmt, ...) LOG(4, fmt, ##__VA_ARGS__)
#define L5(fmt, ...) LOG(5, fmt, ##__VA_ARGS__)

#define L_ERROR(fmt, ...) fprintf(stderr, "ERROR: " fmt __VA_OPT__(,) __VA_ARGS__);
#define L_FATAL(fmt, ...) fprintf(stderr, "FATAL: " fmt __VA_OPT__(,) __VA_ARGS__);

#ifdef DEBUG
	#define dbg(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__);
#else
	#define dbg(...) ((void)0);
#endif

extern int g_log_verbosity_level;

#endif
