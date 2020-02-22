

bin_PROGRAMS = gpuedit

# add modscan to the list below for automatic scanning
#  it kills rebuild performance
gpuedit_SOURCES = main.c \
	app.c \
	buffer.c \
	buffer_drawing.c \
	buffer_raw.c \
	bufferEditor.c \
	bufferLine.c \
	c3dlas/c3dlas.c \
	c3dlas/meshgen.c \
	c_json/json.c \
	clipboard.c \
	commands.c \
	dumpImage.c \
	fbo.c \
	fcfg.c \
	fileBrowser.c \
	font.c \
	gui.c \
	highlight.c \
	highlighters/c.c \
	input.c \
	json_gl.c \
	mainControl.c \
	mdi.c \
	pass.c \
	pcBuffer.c \
	qsort_r.c \
	renderLoop.c \
	settings.c \
	shader.c \
	sti/sti.c \
	texture.c \
	textureAtlas.c \
	ui/animations/pulse.c \
	ui/columnLayout.c \
	ui/configLoader.c \
	ui/debugAdjuster.c \
	ui/edit.c \
	ui/gridLayout.c \
	ui/image.c \
	ui/imgButton.c \
	ui/monitors.c \
	ui/performanceGraph.c \
	ui/scrollWindow.c \
	ui/simpleWindow.c \
	ui/slider.c \
	ui/structAdjuster.c \
	ui/tabBar.c \
	ui/tabControl.c \
	ui/text.c \
	ui/tree.c \
	ui/window.c \
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
	-DEACSMB_HAVE_SSE41 \
	-DEACSMB_HAVE_AVX \
	-DEACSMB_HAVE_AVX2 \
	-msse4.1 -mavx -mavx2\
	-fno-math-errno \
	-fexcess-precision=fast \
	-fno-signed-zeros -fno-trapping-math -fassociative-math \
	-ffinite-math-only -fno-rounding-math \
	-fno-signaling-nans \
	-include "../config.h" \
	-include "signal.h" \
	-pthread \
	-Wall \
	-Wno-unused-result \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable \
	-Wno-unused-function \
	-Wno-pointer-sign \
	-Wno-missing-braces \
	-Wno-char-subscripts \
	-Wno-int-conversion \
	-Wno-int-to-pointer-cast \
	-Wno-unknown-pragmas \
	-Wno-sequence-point \
	-Wno-switch \
	-Werror-implicit-function-declaration \
	-Werror=uninitialized \
	-Werror=return-type 
	
AM_CXXFLAGS = $(AM_CFLAGS)




