#ifndef __gpuedit_fuzzy_match_h__
#define __gpuedit_fuzzy_match_h__

typedef struct {
	int index; // in candidates
	int start;
	int end;
	int len_m;
	int len_c;
} fmatch;

int fuzzy_match_fmatch(
	char** candidates,
	const int n_candidates,
	fmatch** matches_out,
	int* n_out,
	const char* input
);

// the strings inside matches_out point into candidates
int fuzzy_match_charpp(
	char** candidates,
	const int n_candidates,
	char*** matches_out,
	int* n_out,
	const char* input
);

#endif //__gpuedit_fuzzy_match_h__
