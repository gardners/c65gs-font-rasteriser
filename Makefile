all:	ttftof65 pngtotiles f65check

/usr/bin/freetype-config:
	echo "You need to install libfreetype. Try something like:"
	echo "    sudo apt-get install libfreetype6-dev"

/usr/bin/libpng-config:
	echo "You need to install libpng. Try something like:"
	echo "    sudo apt-get install libpng-dev"

ttftof65:	ttftof65.c utf8_decode.c utf8_decode.h Makefile
	gcc -g -Wall -o ttftof65 ttftof65.c utf8_decode.c `freetype-config --cflags --libs`

pngtotiles:	pngtotiles.c Makefile
	gcc -g -Wall `libpng-config --cflags --libs` -o pngtotiles pngtotiles.c -lpng

f65check:	f65check.c Makefile
	gcc -g -Wall -o f65check f65check.c

f65patch:	f65patch.c Makefile
	gcc -g -Wall -o $@ $<