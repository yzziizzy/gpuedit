#ifndef __gpuedit_fuzzy_match_h__
#define __gpuedit_fuzzy_match_h__

typedef struct {
	char* basepath;
	char* filepath;
	char* projname;
	int excluded;
} fcandidate;

typedef struct {
	int index; // in candidates
	int start;
	int end;
	int len_m;
	int len_c;
} fmatch;

int fuzzy_match_fmatch(
	fcandidate* candidates,
	const int n_candidates,
	fmatch** matches_out,
	int* n_out,
	const char* input,
	char case_sensitive
);

// the strings inside matches_out point into candidates
int fuzzy_match_fcandidate(
	fcandidate* candidates,
	const int n_candidates,
	fcandidate** matches_out,
	int* n_out,
	const char* input,
	char case_sensitive
);

#endif //__gpuedit_fuzzy_match_h__
