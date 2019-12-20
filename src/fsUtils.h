#ifndef __yzziizzy_fsUtils_h__
#define __yzziizzy_fsUtils_h__

// return 0 to continue, nonzero to stop all directory scanning
typedef int (*readDirCallbackFn)(char* /*fullPath*/, char* /*fileName*/, void* /*data*/);

#define FSU_EXCLUDE_HIDDEN     (1<<0)
#define FSU_NO_FOLLOW_SYMLINKS (1<<1)
#define FSU_INCLUDE_DIRS       (1<<2)
#define FSU_EXCLUDE_FILES      (1<<3)

// returns nonzero if scanning was halted by the callback
int recurseDirs(
	char* path, 
	readDirCallbackFn fn, 
	void* data, 
	int depth, 
	unsigned int flags
);


char* pathJoin(char* a, char* b); 

// gets a pointer to the first character of the file extension, or to the null terminator if none
char* pathExt(const char* path);

// gets a pointer to the first character of the file extension, or to the null terminator if none
// also provides the length of the path without the period and extension
char* pathExt2(const char* path, int* end);



#endif // __yzziizzy_fsUtils_h__
