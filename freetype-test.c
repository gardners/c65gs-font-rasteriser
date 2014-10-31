#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

int
main( int     argc,
      char**  argv )
{
  FT_Library    library;
  FT_Face       face;

  FT_GlyphSlot  slot;
  FT_Error      error;

  char*         filename;
  char*         text;

  int           n, num_chars;


  if ( argc < 3 )
  {
    fprintf ( stderr, "usage: %s <font.ttf> <size>\n", argv[0] );
    exit( 1 );
  }

  filename      = argv[1];                           /* first argument     */
  text          = (argc==3)?"Q":argv[3];
  num_chars     = strlen( text );

  error = FT_Init_FreeType( &library );              /* initialize library */
  /* error handling omitted */

  error = FT_New_Face( library, filename, 0, &face );/* create face object */
  /* error handling omitted */

  error = FT_Set_Pixel_Sizes( face, atoi(argv[2]), atoi(argv[2]));
  /* error handling omitted */

  slot = face->glyph;

  for ( n = 0; n < num_chars; n++ )
  {
    /* load glyph image into the slot (erase previous one) */
    error = FT_Load_Char( face, text[n], FT_LOAD_RENDER );
    if ( error )
      continue;                 /* ignore errors */

    /* now, draw to our target surface (convert position) */
    printf("bitmap_left=%d, bitmap_top=%d\n", slot->bitmap_left, slot->bitmap_top);
    printf("bitmap_width=%d, bitmap_rows=%d\n", slot->bitmap.width, slot->bitmap.rows);
    printf("bitmap=%p\n",&slot->bitmap);

    int x,y;
    for(y=0;y<slot->bitmap.rows;y++) {
      for(x=0;x<slot->bitmap.width;x++)
	{
	  if (slot->bitmap.buffer[x+y*slot->bitmap.width]==0) 
	    printf(" ");
	  else if (slot->bitmap.buffer[x+y*slot->bitmap.width]<128) 
	    printf("+");
	  else
	    printf("*");
	}
      printf("\n");
    }

  }

  FT_Done_Face    ( face );
  FT_Done_FreeType( library );

  return 0;
}

/* EOF */
