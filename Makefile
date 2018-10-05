all:	freetype-test pngtotiles

freetype-test:	freetype-test.c Makefile
	gcc -g -Wall -o ft freetype-test.c -I/usr/local/opt/freetype/include/freetype2/ -L/usr/local/opt/freetype/lib -lfreetype

pngtotiles:	pngtotiles.c Makefile
	gcc -g -Wall -I/usr/local/include -L/usr/local/lib -o pngtotiles pngtotiles.c -lpng

