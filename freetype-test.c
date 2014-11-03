#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define MAX_POINTS 1024
int unicode_points[MAX_POINTS]={
  0
};
int glyph_count=0;

int
main( int     argc,
      char**  argv )
{
  // Initialise unicode points we want to render.
  int i;
  int cn=0;
  // normal ASCII characters
  for(i=0x20;i<=0x7e;i++) unicode_points[cn++]=i;
  // Latin extended characters
  for(i=0xa1;i<=0xac;i++) unicode_points[cn++]=i;
  for(i=0xae;i<=0xff;i++) unicode_points[cn++]=i;
  // Hebrew alphabet
  for(i=0x5d0;i<=0x5f4;i++) unicode_points[cn++]=i;
  // Maths
  for(i=0x2200;i<=0x22ff;i++) unicode_points[cn++]=i;
  printf("Defined %d unicode points\n",cn);
  glyph_count=cn;


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

  for ( n = 0; n < glyph_count; n++ )
  {
    /* load glyph image into the slot (erase previous one) */
    // error = FT_Load_Char( face, text[n], FT_LOAD_RENDER );

    // Convert unicode point to glyph index
    int glyph_index = FT_Get_Char_Index( face, unicode_points[n] );
    // read glyph by index
    error = FT_Load_Glyph( face, glyph_index, FT_LOAD_RENDER );

    if ( error )
      continue;                 /* ignore errors */

    /* now, draw to our target surface (convert position) */
    printf("bitmap_left=%d, bitmap_top=%d\n", slot->bitmap_left, slot->bitmap_top);
    printf("bitmap_width=%d, bitmap_rows=%d\n", slot->bitmap.width, slot->bitmap.rows);
    int blank_pixels_to_left=slot->bitmap_left;
    if (blank_pixels_to_left<0) blank_pixels_to_left=0;
    if (slot->bitmap_top>=0) {
      int char_rows=slot->bitmap_top/8;
      if (slot->bitmap_top%8) char_rows++;
      int char_columns=(slot->bitmap_left+slot->bitmap.width)/8;
      if ((slot->bitmap_left+slot->bitmap.width)%8) char_columns++;
      printf("Character is %dx%d cards above, and includes %d pixels to the left.\n",
	     char_columns,char_rows,blank_pixels_to_left);
    }
    if (slot->bitmap_top-slot->bitmap.rows<0) {
      // Character has underhang as well
      int underhang=slot->bitmap.rows-slot->bitmap_top;
      int under_rows=underhang/8;
      if (underhang%8) under_rows++;
      int under_columns=(slot->bitmap_left+slot->bitmap.width)/8;
      if ((slot->bitmap_left+slot->bitmap.width)%8) under_columns++;
      printf("Character is %dx%d cards under, and includes %d pixels to the left.\n",
	     under_columns,under_rows,blank_pixels_to_left);
    }
    

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
