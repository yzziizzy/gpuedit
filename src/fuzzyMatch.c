//fuzzy file opener

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fuzzyMatch.h"

//#define DBG printf
#define DBG(...)


int match_cmp(const void* a_in, const void* b_in) {
	fmatch* a = (fmatch*)a_in;
	fmatch* b = (fmatch*)b_in;
	if(a->len_c != b->len_c) {
		return a->len_c - b->len_c;
	} else if(a->end != b->end) {
		return b->end - a->end;
	}

	return 0;
}


int fuzzy_match_fmatch(
	fcandidate* candidates,
	const int n_candidates,
	fmatch** matches_out,
	int* n_out,
	const char* input,
	char case_sensitive
) {
	fmatch matches[n_candidates];
	int n_matches = 0;
	int i_match = 0;
	int len_i = strlen(input);

	if(len_i == 0) {
		DBG("empty input, no matches\n");
		return 1;
	}

	if(matches_out == 0) {
		return 2;
	}

	int i, j_c, j_i;
	for(i=0;i<n_candidates;i++) {
		matches[i].index = i;
		matches[i].start = -1;
		matches[i].end = -1;
		matches[i].len_m = 0;
		matches[i].len_c = strlen(candidates[i].filepath);

		if(len_i > matches[i].len_c) {
			DBG("input longer than candidate, no match [%s,%s]\n", candidates[i].filepath, input);
			continue;
		}

		j_c = matches[i].len_c - 1;
		j_i = len_i - 1;
		while(j_c >= 0 && j_i >= 0) {
//			DBG("while %d, %d\n", j_c, j_i);
			if(case_sensitive ? candidates[i].filepath[j_c] == input[j_i] : tolower(candidates[i].filepath[j_c]) == tolower(input[j_i])) {
				if(matches[i].end == -1) {
					matches[i].end = j_c;
				}
				if(j_i == 0) {
					matches[i].start = j_c;
				}
				j_i--;
			}
			j_c--;
		}
		if(j_i == -1) {
			matches[i].len_m = matches[i].end - matches[i].start + 1;
			n_matches++;
			DBG(
				"found match (%d, %d)[%d] for [%d:%s, %s]\n",
				matches[i].start,
				matches[i].end,
				matches[i].end - matches[i].start + 1,
				matches[i].index,
				candidates[i].filepath,
				input
			);
		}
	}

	DBG("have %d matches\n", n_matches);
	fmatch* out = malloc(sizeof(fmatch) * n_matches);
	*n_out = n_matches;
	*matches_out = out;

	if(out == 0) {
		return 3;
	}
	for(i=0;i<n_candidates;i++) {
		if((matches[i].len_m >= len_i) && (i_match < n_matches)) {
			DBG("pre write out %d => %d\n", i, i_match);
			out[i_match].index = matches[i].index;
			out[i_match].start = matches[i].start;
			out[i_match].end = matches[i].end;
			out[i_match].len_c = matches[i].len_c;
			
			// memcpy(out[i_match], &(matches[i]), sizeof(fmatch));
			DBG("post write out %d => %d\n", i, i_match);
			i_match++;
		}
	}

	qsort((void*)out, n_matches, sizeof(fmatch), &match_cmp);

	for(i=0;i<n_matches;i++) {
		DBG(
			"ordered match fmatch (%d, %d)[%d] for [%s]\n",
			out[i].start,
			out[i].end,
			out[i].end - out[i].start + 1,
			candidates[out[i].index].filepath
		);
	}

	return 0;
}


int fuzzy_match_fcandidate(
	fcandidate* candidates,
	const int n_candidates,
	fcandidate** matches_out,
	int* n_out,
	const char* input,
	char case_sensitive
) {
	fmatch* matches;
	fcandidate* out;

	int err = 0;

	err = fuzzy_match_fmatch(candidates, n_candidates, &matches, n_out, input, case_sensitive);
	DBG("fuzzy_match_fmatch exit code: %d\n", err);

	if(err) {
		return err;
	}

	out = malloc(sizeof(*out) * (*n_out));
	

	int i;
	for(i=0;i<*n_out;i++) {
		DBG(
			"ordered match charpp (%d, %d)[%d] for [%s]\n",
			matches[i].start,
			matches[i].end,
			matches[i].end - matches[i].start + 1,
			candidates[matches[i].index].filepath
		);
		out[i] = candidates[matches[i].index];
	}

	free(matches);

	*matches_out = out;

	return 0;
}

#undef DBG
