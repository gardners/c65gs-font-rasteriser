all:	freetype-test pngtotiles

freetype-test:	freetype-test.c Makefile
	gcc -g -Wall -o ft freetype-test.c `freetype-config --cflags --libs`

pngtotiles:	pngtotiles.c Makefile
	gcc -g -Wall `libpng-config --cflags --libs` -o pngtotiles pngtotiles.c -lpng

