all:	freetype-test

freetype-test:	freetype-test.c Makefile
	gcc -g -Wall -o ft freetype-test.c -I/usr/local/opt/freetype/include/freetype2/ -L/usr/local/opt/freetype/lib -lfreetype
