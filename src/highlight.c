#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dlfcn.h>

#include "highlight.h"
#include "common_math.h"
#include "common_gl.h"





void HighlighterManager_Init(HighlighterManager* hm, Settings* s) {
	VEC_INIT(&hm->modules);
	VEC_INIT(&hm->plugins);
	
	HT_init(&hm->extLookup, 16);
	
	hm->s = s;
	hm->gs = Settings_GetSection(s, SETTINGS_General);
	
	
	// add a default hightlighter that does nothing
	Highlighter* h = pcalloc(h);
	h->plugin = NULL;
	h->numStyles = 1;
	h->stylesDark = calloc(1, sizeof(*h->stylesDark) * 1);
	h->stylesLight = calloc(1, sizeof(*h->stylesLight) * 1);
	
	h->stylesDark[0] = (StyleInfo){
		.fgColor = {.9,.9,.9, 1.0},
		.bgColor = {.1,.1,.1, 1.0},
		.fgSelColor = {.2,.2,.2, 1.0},
		.bgSelColor = {.5,.5,.8, 1.0},
	};
	
	h->stylesLight[0] = (StyleInfo){
		.fgColor = {.1,.1,.1, 1.0},
		.bgColor = {1,1,1, 1.0},
		.fgSelColor = {.3,.3,.3, 1.0},
		.bgSelColor = {.5,.5,.8, 1.0},
	};
	
	VEC_PUSH(&hm->plugins, h);
}

void HighlighterManager_Destroy(HighlighterManager* hm) {
	VEC_FREE(&hm->modules);
	VEC_FREE(&hm->plugins);
	
	HT_destroy(&hm->extLookup);
}







static void* a_malloc(Allocator* a, size_t sz) {
	return malloc(sz);
}
static void* a_calloc(Allocator* a, size_t sz) {
	return calloc(1, sz);
}
static void* a_realloc(Allocator* a, void* p, size_t sz) {
	return realloc(p, sz);
}
static void a_free(Allocator* a, void* p) {
	free(p);
}


typedef void (*entryFn)(Allocator*, HighlighterPluginInfo** /*list*/, uint64_t* /*count*/);


HighlighterModule* Highlighter_LoadModule(HighlighterManager* hm, char* path) {
	
	int flags = RTLD_LAZY;
	
	void* lib = dlopen(path, flags);
	if(!lib) {
		printf("Failed to open highlighter library: '%s'\n", path);
		return NULL;
	}
	
	entryFn getList = dlsym(lib, "gpuedit_list_highlighters");
	if(!getList) {
		printf("Invalid highlighter library: '%s'\n", path);
		dlclose(lib);
		return NULL;
	}
	
	Allocator al = {
		.malloc = a_malloc,
		.calloc = a_calloc,
		.realloc = a_realloc,
		.free = a_free,
	};
	
	HighlighterPluginInfo* list;
	uint64_t cnt;
	
	getList(&al, &list, &cnt);
	
	for(int i = 0; i < cnt; i++) {
		HighlighterPluginInfo* hpi = list + i;
		Highlighter* h = pcalloc(h);
		h->plugin = hpi;
		h->numStyles = hpi->getStyleCount();
		h->stylesDark = calloc(1, sizeof(*h->stylesDark) * h->numStyles);
		h->stylesLight = calloc(1, sizeof(*h->stylesLight) * h->numStyles);
		
		hpi->getStyleDefaults(h->stylesDark, h->numStyles);
		hpi->getStyleDefaults(h->stylesLight, h->numStyles);
		
		char* tmp = sprintfdup("%s/%s_colors.txt", hm->gs->highlightStylesPath, hpi->name);
		Highlighter_LoadStyles(h, tmp);
		free(tmp);

		
		
		// parse and cache the supported file extensions
		char* s = hpi->extensions;
		while(s && *s) {
			char* e = strchr(s, ';');
			if(!e && *s != 0) {
				e = s + strlen(s);
			}
			char* ext = strndup(s, e - s);
			
			HT_set(&hm->extLookup, ext, h);
			
			s = e;
			while(*s == ';') s++;
		}
		
		
		VEC_PUSH(&hm->plugins, h);
	}
	
	HighlighterModule* mod = pcalloc(mod);
	mod->numHighlighters = cnt;
	mod->highlighters = (void*)list;
	mod->path = strdup(path);
	mod->libHandle = lib;
	
	VEC_PUSH(&hm->modules, mod);
	
	return mod;
}

// returns number of .so's loaded
int Highlighter_ScanDirForModules(HighlighterManager* hm, char* dir) {
	int files_read = 0;
	
	char* path_2 = resolve_path(dir);
	if(is_path_a_dir(path_2)) {
		size_t num_files = 0;
		char* so_glob = path_join(path_2, "*.so");
		char** files;
		
		files = multi_wordexp_dup(so_glob, &num_files);
		
		for(size_t i = 0; i < num_files; i++) {
			files_read += !!Highlighter_LoadModule(hm, files[i]);
			free(files[i]);
		}
		
		free(so_glob);
		free(files);
	}
	free(path_2);
	
	return files_read;
}




Highlighter* HighlighterManager_ProbeExt(HighlighterManager* hm, char* filename) {
	Highlighter* h;
	
	char* ext = pathExt(filename);
	if(!ext || !*ext) {
		printf("File '%s' does not have an extension\n", filename);
		return NULL;
	}
	
	if(HT_get(&hm->extLookup, ext, &h)) {
		printf("No highlighter registered for extension '%s'\n", ext);
		return NULL;
	}

	return h;
}









 
#define CHECK_ALLOC(buf, allocsz, len, inc, extra) \
do { \
	if((buf) == NULL) { \
		(allocsz) = MAX(32, nextPOT((inc) + (extra))); \
		(buf) = malloc(sizeof(*(buf)) * (allocsz)); \
	} \
	else if((allocsz) < (len) + (inc) + (extra)) { \
		(allocsz) = nextPOT((len) + (inc) + (extra));  \
		(buf) = realloc((buf), sizeof(*(buf)) * (allocsz)); \
	} \
} while(0);



static StyleInfo* get_style(Highlighter* h, char* name, char prefix) {
	for(int i = 0; i < h->numStyles; i++) {
		switch(prefix) {
			case 'd':
				if(0 == strcmp(h->stylesDark[i].name, name)) {
					return &h->stylesDark[i];
				}
				break;
			case 'l':
				if(0 == strcmp(h->stylesLight[i].name, name)) {
					return &h->stylesLight[i];
				}
				break;
		}
	}
	
	return NULL;
}



void Highlighter_PrintStyles(Highlighter* h) {
	
	for(int i = 0; i < h->numStyles; i++) {
		printf("%d (dark): %s\n", i, h->stylesDark[i].name);
		printf("%d (light): %s\n", i, h->stylesLight[i].name);
	}
	
}



void Highlighter_LoadStyles(Highlighter* h, char* path) {
	size_t len;
	
	char* src = readWholeFile(path, &len);
	if(!src) return;
	
	char** lines = strsplit_inplace(src, '\n', NULL);
	
	char** lines2 = lines;
	for(int ln = 1; *lines2; lines2++, ln++) {
		char prefix[2];
		char name[128];
		char value[128];
		StyleInfo* style;
		
		if(3 != sscanf(*lines2, " %1[dl] %127[_a-zA-Z0-9] = %127s ", prefix, name, value)) {
			// don't warn about comments and empty lines
			if(*lines2[0] != '#' && *lines2[0] != '\n' && *lines2[0] != '\0') {
				printf("Invalid highlighting color line %s:%d: '%s'\n", path, ln, *lines2);
			}
			continue;
		}
		
// 		printf("line: '%s' = '%s'\n", name, value);
		style = get_style(h, name, prefix[0]);
		if(!style) {
//			fprintf(stderr, "Unknown style name '%s' in %s:%d\n", name, path, ln);
			continue;
		}
		
		if(value[0] == '#') { // hex code
			decodeHexColorNorm(value, (float*)&style->fgColor);
		}
		
		// TODO: rgba()
		// TODO: backgrounds, formats, fonts, etc
		
	}
	
	
	free(lines);
	free(src);
	
	
}








