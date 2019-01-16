

bin_PROGRAMS = gpuedit

# add modscan to the list below for automatic scanning
#  it kills rebuild performance
gpuedit_SOURCES = main.c \
	app.c \
	c3dlas/c3dlas.c \
	c3dlas/meshgen.c \
	c_json/json.c \
	ds.c \
	dumpImage.c \
	fbo.c \
	fcfg.c \
	font.c \
	gui.c \
	hash.c \
	input.c \
	json_gl.c \
	mdi.c \
	mempool.c \
	MurmurHash3.c \
	pass.c \
	pcBuffer.c \
	qsort_r.c \
	renderLoop.c \
	settings.c \
	shader.c \
	texture.c \
	textureAtlas.c \
	ui/animations/pulse.c \
	ui/columnLayout.c \
	ui/configLoader.c \
	ui/edit.c \
	ui/gridLayout.c \
	ui/image.c \
	ui/monitors.c \
	ui/scrollWindow.c \
	ui/simpleWindow.c \
	ui/text.c \
	ui/window.c \
	utilities.c \
	window.c 

gpuedit_LDADD =  \
	-lm \
	-lX11 -lfreetype -lfontconfig -lpng \
	-lGL -lGLU -lGLEW 
	 
AM_CFLAGS = "-I/usr/include/freetype2" -std=gnu11 -g -DLINUX \
	-include "../config.h" \
	-pthread \
	-Wno-unused-result \
	-Werror-implicit-function-declaration
	
AM_CXXFLAGS = $(AM_CFLAGS)




