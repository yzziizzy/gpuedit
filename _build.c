// gcc -lutil build.c -o hacer && ./hacer

#include "_build.inc.c"

// link with -lutil

char* build_dir;

char* sources[] = { "main.c",
	"app.c",
	"buffer.c",
	"buffer_drawing.c",
	"buffer_range.c",
	"buffer_raw.c",
	"buffer_settings.c",
	"buffer_undo.c",
	"bufferEditor.c",
	"bufferEditControl.c",
	"bufferLine.c",
	"c3dlas/c3dlas.c",
	"c3dlas/meshgen.c",
	"c_json/json.c",
//	"calcControl.c",
	"clipboard.c",
	"dumpImage.c",
	"fbo.c",
	"fcfg.c",
	"fileBrowser.c",
	"font.c",
	"fuzzyMatch.c",
	"fuzzyMatchControl.c",
	"grepOpenControl.c",
	"hexedit.c",
	"highlight.c",
	"input.c",
	"json_gl.c",
	"log.c",
	"mainControl.c",
	"mdi.c",
	"msg.c",
	"pass.c",
	"pcBuffer.c",
	"qsort_r.c",
	"settings.c",
	"shader.c",
	"statusBar.c",
	"sti/sti.c",
	"sti/debug.c",
	"texture.c",
	"textureAtlas.c",
	"ui/buttons.c",
	"ui/commands.c",
	"ui/edits.c",
	"ui/gui.c",
	"ui/gui_settings.c",
	"ui/guiManager.c",
	"ui/imgui.c",
	"ui/opts_structs.c",
	"ui/scrollbars.c",
//	"ui/selects.c",
//	"ui/sliders.c",
	"ui/text.c",
	"units.c",
	"utilities.c",
	"window.c", 
	NULL,
};

// these are run through pkg-config
char* lib_headers_needed[] = {
	// freetype and fontconfig are dynamically loaded only when needed
	"freetype2", "fontconfig",
	
	"gl", "glu", "glew",
	"libpcre2-8", 
	"libpng", 
	"x11", "xfixes", "xrender",
	NULL
};

// these are run through pkg-config
char* libs_needed[] = {
	"gl", "glu", "glew",
	"libpcre2-8", 
	"libpng",
	"x11", "xfixes", "xrender",
	NULL,
};

char* ld_add[] = {
	"-lm", "-ldl", "-lutil",
	NULL,
};


char* debug_cflags[] = {
	"-ggdb",
	"-DDEBUG",
	"-O0",
	NULL
};

char* profiling_cflags[] = {
	"-pg",
	NULL
};

char* release_cflags[] = {
	"-DRELEASE",
	"-O3",
	"-Wno-array-bounds", // temporary, until some shit in sti gets fixed. only happens with -O3
	NULL
};

// -ffast-math but without reciprocal approximations 
char* cflags[] = {
	"-std=gnu11", 
	"-ffunction-sections", "-fdata-sections",
	"-DLINUX",
	"-D_GNU_SOURCE",
	"-march=native",
	"-mtune=native", 
	"-DSTI_C3DLAS_NO_CONFLICT",
	"-fno-math-errno", 
	"-fexcess-precision=fast", 
	"-fno-signed-zeros",
	"-fno-trapping-math", 
	"-fassociative-math", 
	"-ffinite-math-only", 
	"-fno-rounding-math", 
	"-fno-signaling-nans", 
	"-include signal.h", 
	"-include ./src/global.h",
	"-pthread", 
	"-Wall", 
	"-Werror", 
	"-Wextra", 
	"-Wno-unused-result", 
	"-Wno-unused-variable", 
	"-Wno-unused-but-set-variable", 
	"-Wno-unused-function", 
	"-Wno-unused-label", 
	"-Wno-unused-parameter",
	"-Wno-pointer-sign", 
	"-Wno-missing-braces", 
	"-Wno-maybe-uninitialized", 
	"-Wno-implicit-fallthrough", 
	"-Wno-sign-compare", 
	"-Wno-char-subscripts", 
	"-Wno-int-conversion", 
	"-Wno-int-to-pointer-cast", 
	"-Wno-unknown-pragmas",
	"-Wno-sequence-point",
	"-Wno-switch",
	"-Wno-parentheses",
	"-Wno-comment",
	"-Wno-strict-aliasing",
	"-Wno-endif-labels",
	"-Wno-calloc-transposed-args", // just stupid
	"-Werror=implicit-function-declaration",
	"-Werror=uninitialized",
	"-Werror=return-type",
	NULL,
};


void gen_optsstructs() {
	time_t exe_mtime;
	time_t cfg_mtime;
	time_t gen1_mtime;
	time_t gen2_mtime;
	char* exefile = "tools/settings/gen_opts_structs";
	char* cfgfile = "src/ui/opts_structs.sex";
	char* genfile1 = "src/ui/opts_structs.h";
	char* genfile2 = "src/ui/opts_structs.c";
	resolve_path(exefile, &exe_mtime);
	resolve_path(cfgfile, &cfg_mtime);
	resolve_path(genfile1, &gen1_mtime);
	resolve_path(genfile2, &gen2_mtime);
	if(cfg_mtime > gen1_mtime || cfg_mtime > gen2_mtime || !exe_mtime) {
		printf("Regenerating src/ui/opts_structs.[ch]\n");
		
		system("tools/settings/build.sh");
		system("tools/settings/gen_opts_structs src/ui/opts_structs.sex src/ui/opts_structs.h src/ui/opts_structs.c");
	}
}


int compile_source(char* src_path, char* obj_path) {
	char* cmd = sprintfdup("gcc -c -o %s %s %s", obj_path, src_path, g_gcc_opts_flat);
//	printf("%s\n", cmd);
	strlist_push(&compile_cache, cmd);
//	exit(1);
	return 0;
}



void check_source(char* raw_src_path, strlist* objs) {
	time_t src_mtime, obj_mtime = 0, dep_mtime = 0;
	
	char* src_path = resolve_path(raw_src_path, &src_mtime);
	char* src_dir = dir_name(raw_src_path);
	char* base = base_name(src_path);
	
//	char* build_base = "debug";
	char* src_build_dir = path_join(build_dir, src_dir);
	char* obj_path = path_join(src_build_dir, base);
	
	// cheap and dirty
	size_t olen = strlen(obj_path);
	obj_path[olen-1] = 'o';
	
	
	strlist_push(objs, obj_path);
	
	char* dep_path = strcatdup(src_build_dir, "/", base, ".d");
	
	mkdirp_cached(src_build_dir, 0755);
	
	char* real_obj_path = resolve_path(obj_path, &obj_mtime);
	if(obj_mtime < src_mtime) {
//		printf("  objtime compile\n");
		compile_source(src_path, real_obj_path);
		return;
	}
	
	
	if(gen_deps(src_path, dep_path, src_mtime, obj_mtime)) {
//		printf("  deep dep compile\n");
		compile_source(src_path, real_obj_path);
	}
	
	
	
	//gcc -c -o $2 $1 $CFLAGS $LDADD
}


struct {
	int debug;
	int profiling;
	int release;
	int clean;
} g_options;


int main(int argc, char* argv[]) {
	string_cache_init(2048);
	realname_cache_init();
	strlist_init(&compile_cache);
	hash_init(&mkdir_cache, 128);
	g_nprocs = get_nprocs();
	
	// defaults
	g_options.debug = 2;
	char* exe_path = "gpuedit";
	char* base_build_dir = "build";
	
	gen_optsstructs();
	
	char* tmp;
	int mode = 0;
	
	for(int a = 1; a < argc; a++) {
		if(argv[a][0] == '-') {
			for(int i = 0; argv[a][i]; i++) {
				
				switch(argv[a][i]) {
					case 'd': // debug: -ggdb
						g_options.debug = 1;
						if(g_options.release == 1) {
							fprintf(stderr, "Debug and Release set at the same time.\n");
						}
						break;
						
					case 'p': // profiling: -pg
						g_options.profiling = 1;
						break;
						
					case 'r': // release: -O3
						g_options.release = 1;
						g_options.debug = 0;
						break;
						
					case 'c': // clean
						g_options.clean = 1;
						break;
				}
			}		
		}
	
	}
	
	
	// delete the old executable
	unlink(exe_path);
	
	char build_subdir[20] = {0};
	
	if(g_options.debug) strcat(build_subdir, "d");
	if(g_options.profiling) strcat(build_subdir, "p");
	if(g_options.release) strcat(build_subdir, "r");

	build_dir = path_join(base_build_dir, build_subdir);
	
	// delete old build files if needed 
	if(g_options.clean) {
		printf("Cleaning directory %s/\n", build_dir);
		system(sprintfdup("rm -rf %s/*", build_dir));
	}
	
	mkdirp_cached(build_dir, 0755);
	
	// create the test files
	system("cp src/buffer.h ./testfile.h");
	system("cp src/buffer.c ./testfile.c");
	
	g_gcc_opts_list = concat_lists(ld_add, cflags);
	
	if(g_options.debug) g_gcc_opts_list = concat_lists(g_gcc_opts_list, debug_cflags);
	if(g_options.profiling) g_gcc_opts_list = concat_lists(g_gcc_opts_list, profiling_cflags);
	if(g_options.release) g_gcc_opts_list = concat_lists(g_gcc_opts_list, release_cflags);
	
	g_gcc_opts_flat = join_str_list(g_gcc_opts_list, " ");
	g_gcc_include = pkg_config(lib_headers_needed, "I");
	g_gcc_libs = pkg_config(libs_needed, "L");
	tmp = g_gcc_opts_flat;
	g_gcc_opts_flat = strjoin(" ", g_gcc_opts_flat, g_gcc_include);
	free(tmp);
	
	//printf("%s\n\n\n\n",g_gcc_opts_flat);
//	rglob src;
	//recursive_glob("src", "*.[ch]", 0, &src);
	
	strlist objs;
	strlist_init(&objs);
	
	float source_count = list_len(sources);
	
	for(int i = 0; sources[i]; i++) {
//		printf("%i: checking %s\n", i, sources[i]);
		char* t = path_join("src", sources[i]);
		check_source(t, &objs);
		free(t);
		
		printf("\rChecking dependencies...  %s", printpct((i * 100) / source_count));
	}
	printf("\rChecking dependencies...  \e[32mDONE\e[0m\n");
	fflush(stdout);
	
	
	
	if(compile_cache_execute()) {
		printf("\e[1;31mBuild failed.\e[0m\n");
		return 1;
	}
	
	char* objects_flat = join_str_list(objs.entries, " ");
	
	
	printf("Creating archive...      "); fflush(stdout);
	if(system(sprintfdup("ar rcs %s/tmp.a %s", build_dir, objects_flat))) {
		printf(" \e[1;31mFAIL\e[0m\n");
		return 1;
	}
	else {
		printf(" \e[32mDONE\e[0m\n");
	}
	
	
	printf("Linking executable...    "); fflush(stdout);
	char* cmd = sprintfdup("gcc -Wl,--gc-sections %s %s/tmp.a -o %s %s %s", g_options.profiling ? "-pg" : "", build_dir, exe_path, g_gcc_libs, g_gcc_opts_flat);
	if(system(cmd)) {
		printf(" \e[1;31mFAIL\e[0m\n");
		return 1;
	}
	else {
		printf(" \e[32mDONE\e[0m\n");
	}
	
	// erase the build output if it succeeded
	printf("\e[F\e[K");
	printf("\e[F\e[K");
	printf("\e[F\e[K");
	printf("\e[F\e[K");
	
	printf("\e[32mBuild successful:\e[0m %s\n\n", exe_path);
	
	return 0;
}


