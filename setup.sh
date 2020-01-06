#!/bin/bash


if [ ! -d src/c3dlas ] ; then
	echo "c3dlas not found... fetching"
	git clone https://github.com/yzziizzy/c3dlas src/c3dlas
fi

if [ ! -d src/c_json ] ; then
	echo "c_json not found... fetching"
	git clone https://github.com/yzziizzy/c_json src/c_json
fi

if [ ! -d src/sti ] ; then
	echo "sti not found... fetching"
	git clone https://github.com/yzziizzy/sti src/sti
fi


# the remainder at least...
echo "This setup file is for Ubuntu."


F=$(locate png.h | grep 'png.h' | tail -n 1)
echo $F
if [[ -z $F || ! -f $F ]] ; then
	echo "libpng dev files are missing..."
	sudo apt-get install libpng-dev
fi

F=$(locate freetype/freetype.h | grep 'freetype/freetype.h' | tail -n 1)
echo $F
if [[ -z $F || ! -f $F ]] ; then
	echo "freetype dev files are missing..."
	sudo apt-get install libfreetype6-dev
fi

F=$(locate GL/glew.h | grep 'GL/glew.h' | tail -n 1)
echo $F
if [[ -z $F || ! -f $F ]] ; then
	echo "glew is missing..."
	sudo apt-get install libglew-dev
fi

F=$(locate fontconfig/fontconfig.h | grep 'fontconfig/fontconfig.h' | tail -n 1)
echo $F
if [[ -z $F || ! -f $F ]] ; then
	echo "fontconfig dev files are missing..."
	sudo apt-get install libfontconfig1-dev
fi

F=$(locate X11/X.h | grep 'X11/X.h' | tail -n 1)
echo $F
if [[ -z $F || ! -f $F ]] ; then
	echo "xlib dev files are missing..."
	sudo apt-get install libx11-dev
fi

F=$(locate GL/gl.h | grep 'GL/gl.h' | tail -n 1)
echo $F
if [[ -z $F || ! -f $F ]] ; then
	echo "OpenGL dev files are missing. You may need Mesa header files, or your graphics drivers might be misinstalled or broken."
	exit 1
fi



./autogen.sh && make -j`nproc --ignore=1` && src/gpuedit bufferEditor.c


