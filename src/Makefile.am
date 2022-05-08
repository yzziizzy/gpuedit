

bin_PROGRAMS = gpuedit

# add modscan to the list below for automatic scanning
#  it kills rebuild performance
gpuedit_SOURCES = main.c \
	app.c \
	buffer.c \
	buffer_drawing.c \
	buffer_raw.c \
	bufferEditor.c \
	bufferEditControl.c \
	bufferLine.c \
	c3dlas/c3dlas.c \
	c3dlas/meshgen.c \
	c_json/json.c \
	clipboard.c \
	dumpImage.c \
	fbo.c \
	fcfg.c \
	font.c \
	fuzzyMatch.c \
	fuzzyMatchControl.c \
	highlight.c \
	input.c \
	json_gl.c \
	mainControl.c \
	mdi.c \
	pass.c \
	pcBuffer.c \
	qsort_r.c \
	settings.c \
	shader.c \
	sti/sti.c \
	texture.c \
	textureAtlas.c \
	ui/commands.c \
	ui/gui.c \
	ui/gui_settings.c \
	ui/guiManager.c \
	ui/imgui.c \
	utilities.c \
	window.c 

gpuedit_LDADD =  \
	-ldl -lutil \
	-lXfixes -lpng -lpcre2-8

# -ffast-math but without reciprocal approximations 
AM_CFLAGS = "-I/usr/include/freetype2" -std=gnu11 -g -DLINUX \
	-DSTI_C3DLAS_NO_CONFLICT \
	-DEACSMB_USE_SIMD \
	-DEACSMB_HAVE_SSE4 \
	-DEACSMB_HAVE_AVX \
	-march=native -mtune=native \
	-fno-math-errno \
	-fexcess-precision=fast \
	-fno-signed-zeros -fno-trapping-math -fassociative-math \
	-ffinite-math-only -fno-rounding-math \
	-fno-signaling-nans \
	-include "../config.h" \
	-include "signal.h" \
	-pthread \
	-Wall \
	-Werror \
	-Wextra \
	-Wno-unused-result \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable \
	-Wno-unused-function \
	-Wno-unused-label \
	-Wno-unused-parameter \
	-Wno-pointer-sign \
	-Wno-missing-braces \
	-Wno-maybe-uninitialized \
	-Wno-implicit-fallthrough \
	-Wno-sign-compare \
	-Wno-char-subscripts \
	-Wno-int-conversion \
	-Wno-int-to-pointer-cast \
	-Wno-unknown-pragmas \
	-Wno-sequence-point \
	-Wno-switch \
	-Wno-parentheses \
	-Wno-comment \
	-Wno-strict-aliasing \
	-Werror=implicit-function-declaration \
	-Werror=uninitialized \
	-Werror=return-type 
	
AM_CXXFLAGS = $(AM_CFLAGS)





