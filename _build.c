// gcc -lutil build.c -o hacer && ./hacer

#include "_build.inc.c"

// link with -lutil


char* sources[] = { "main.c",
	"app.c",
	"buffer.c",
	"buffer_drawing.c",
	"buffer_raw.c",
	"buffer_settings.c",
	"bufferEditor.c",
	"bufferEditControl.c",
	"bufferLine.c",
	"c3dlas/c3dlas.c",
	"c3dlas/meshgen.c",
	"c_json/json.c",
	"clipboard.c",
	"dumpImage.c",
	"fbo.c",
	"fcfg.c",
	"fileBrowser.c",
	"font.c",
	"fuzzyMatch.c",
	"fuzzyMatchControl.c",
	"highlight.c",
	"input.c",
	"json_gl.c",
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
	"texture.c",
	"textureAtlas.c",
	"ui/commands.c",
	"ui/gui.c",
	"ui/gui_settings.c",
	"ui/guiManager.c",
	"ui/imgui.c",
	"utilities.c",
	"window.c", 
	NULL,
};

// these are run through pkg-config
char* libs_needed[] = {
	"gl", "glu", "glew",
	"freetype2", "fontconfig", 
	"libpcre2-8", 
	"libpng", 
	"x11", "xfixes",
	NULL,
};

char* ld_add[] = {
	"-lm", "-ldl", "-lutil",
	NULL,
};


// -ffast-math but without reciprocal approximations 
char* cflags[] = {
	"-std=gnu11", 
	"-ggdb", 
	"-DLINUX",
	"-march=native",
	"-mtune=native", 
	"-DSTI_C3DLAS_NO_CONFLICT",
	"-DEACSMB_USE_SIMD",
	"-DEACSMB_HAVE_SSE4",
	"-DEACSMB_HAVE_AVX",
	"-fno-math-errno", 
	"-fexcess-precision=fast", 
	"-fno-signed-zeros",
	"-fno-trapping-math", 
	"-fassociative-math", 
	"-ffinite-math-only", 
	"-fno-rounding-math", 
	"-fno-signaling-nans", 
	"-include signal.h", 
	"-include ./config.h",
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
	"-Werror=implicit-function-declaration",
	"-Werror=uninitialized",
	"-Werror=return-type",
	NULL,
};



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
	
	char* build_dir = path_join("build", src_dir);
	char* obj_path = path_join(build_dir, base);
	
	// cheap and dirty
	size_t olen = strlen(obj_path);
	obj_path[olen-1] = 'o';
	
	
	strlist_push(objs, obj_path);
	
	char* dep_path = strcatdup(build_dir, "/", base, ".d");
	
	mkdirp_cached(build_dir, 0755);
	
	char* real_obj_path = resolve_path(obj_path, &obj_mtime);
	if(obj_mtime < src_mtime) {
		printf("  objtime compile\n");
		compile_source(src_path, real_obj_path);
		return;
	}
	
	
	if(gen_deps(src_path, dep_path, src_mtime, obj_mtime)) {
		printf("  deep dep compile\n");
		compile_source(src_path, real_obj_path);
	}
	
	
	//gcc -c -o $2 $1 $CFLAGS $LDADD
}



int main(int argc, char* argv[]) {
	string_cache_init(2048);
	realname_cache_init();
	strlist_init(&compile_cache);
	hash_init(&mkdir_cache, 128);
	
	char* tmp;
	
	char* exe_path = "gpuedit";
	unlink(exe_path);
	
	mkdirp_cached("build", 0755);
	
	g_gcc_opts_list = concat_lists(ld_add, cflags);
	g_gcc_opts_flat = join_str_list(g_gcc_opts_list, " ");
	g_gcc_include = pkg_config(libs_needed, "I");
	g_gcc_libs = pkg_config(libs_needed, "L");
	tmp = g_gcc_opts_flat;
	g_gcc_opts_flat = strjoin(" ", g_gcc_opts_flat, g_gcc_include);
	free(tmp);
	
//	rglob src;
	//recursive_glob("src", "*.[ch]", 0, &src);
	
	strlist objs;
	strlist_init(&objs);
	
	for(int i = 0; sources[i]; i++) {
		printf("%i: checking %s\n", i, sources[i]);
		char* t = path_join("src", sources[i]);
		check_source(t, &objs);
		free(t);
	}
	printf("Executing compile cache..."); fflush(stdout);
	if(compile_cache_execute()) {
		printf("Build halted due to errors.\n");
		return 1;
	}
	printf(" DONE.\n");
	
	
	printf("Linking final executable..."); fflush(stdout);
	char* objects_flat = join_str_list(objs.entries, " ");
//	gcc -o imcalc $objlist $CFLAGS $LDADD
	char* cmd = sprintfdup("gcc -o %s %s %s %s", exe_path, objects_flat, g_gcc_libs, g_gcc_opts_flat);
//	printf("cmd: %s\n", cmd );
	system(cmd);
	printf(" DONE.\n");
	
	
//	printf("%d: %s\n", err, strerror(errno));
	return 0;
}


