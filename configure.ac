#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(eacsmb, 1.0, no@spam.com)
AM_INIT_AUTOMAKE([-Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for required libraries.
AC_CHECK_LIB([GL], [glFlush])
AC_CHECK_LIB([GLEW], [glewInit])
AC_CHECK_LIB([GLU], [gluCheckExtension])
AC_CHECK_LIB([X11], [XOpenDisplay])
AC_CHECK_LIB([freetype], [FT_Init_FreeType])
AC_CHECK_LIB([fontconfig], [FcInitLoadConfigAndFonts])
AC_CHECK_LIB([m], [sin])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([ \
	X11/X.h \
	X11/Xlib.h \
	X11/keysym.h \
	alloca.h \
	ctype.h \
	dirent.h \
	errno.h \
	fontconfig/fontconfig.h \
	ft2build.h \
	libgen.h \
	limits.h \
	malloc.h \
	math.h \
	png.h \
	pthread.h \
	setjmp.h \
	stdatomic.h \
	stddef.h \
	stdint.h \
	stdio.h \
	stdlib.h \
	string.h \
	strings.h \
	sys/sysinfo.h \
	time.h \
	unistd.h \
])


#AC_CHECK_FUNCS( memset  \
#    snprintf \
#    sqrt strcasecmp strcasestr strchr strstr strtol strtoul)





#
# Check for optional dependencies
#

AC_MSG_CHECKING([for ALSA support])
AC_ARG_ENABLE(
	alsa,
[  --enable-alsa     ALSA support ([default=yes])],
	if test x$enableval = "xyes"; then
		AC_MSG_RESULT([yes])
		AC_CHECK_LIB([asound], [snd_pcm_open], 
			AC_DEFINE(HAVE_ALSA, 1, "ALSA support")
			LIBS="$LIBS -lasound")
	else
		AC_MSG_RESULT([no])
	fi,  
	AC_MSG_RESULT([yes])
	AC_CHECK_LIB([asound], [snd_pcm_open],  
		AC_DEFINE(HAVE_ALSA, 1, "ALSA support")
		LIBS="$LIBS -lasound")
)



AC_MSG_CHECKING([for OGG/Vorbis support])
AC_ARG_ENABLE(
	vorbis,
[  --enable-vorbis        OGG/Vorbis support ([default=yes])],
	if test x$enableval = "xyes"; then
		AC_MSG_RESULT([yes])
		AC_CHECK_LIB([vorbisfile], [ov_fopen], 
			AC_DEFINE(HAVE_VORBIS, 1, "libVorbis support")
			LIBS="$LIBS -lvorbis")
	else
		AC_MSG_RESULT([no])
	fi,  
	AC_MSG_RESULT([yes])
	AC_CHECK_LIB([vorbisfile], [ov_fopen],  
		AC_DEFINE(HAVE_VORBIS, 1, "libVorbis support")
		LIBS="$LIBS -lvorbisfile")
)







AC_CONFIG_FILES([Makefile
                 src/Makefile])


AC_OUTPUT
