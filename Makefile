all:	freetype-test pngtotiles

/usr/bin/freetype-config:
	echo "You need to install libfreetype. Try something like:"
	echo "    sudo apt-get install libfreetype6-dev"

/usr/bin/libpng-config:
	echo "You need to install libpng. Try something like:"
	echo "    sudo apt-get install libpng-dev"

freetype-test:	freetype-test.c Makefile
	gcc -g -Wall -o ft freetype-test.c `freetype-config --cflags --libs`

pngtotiles:	pngtotiles.c Makefile
	gcc -g -Wall `libpng-config --cflags --libs` -o pngtotiles pngtotiles.c -lpng

