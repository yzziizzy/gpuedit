

# kills build caching atm
modscan:
	./scanMods.sh --quiet
#BUILT_SOURCES = modscan

testfile:
	cp src/buffer.h ./testfile.h
	cp src/buffer.c ./testfile.c

BUILT_SOURCES = testfile

SUBDIRS = src


ACLOCAL_AMFLAGS = -I m4

