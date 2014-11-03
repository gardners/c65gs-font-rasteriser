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

#define MAX_CARDS 65536
int card_count=0;
unsigned char cards[MAX_CARDS][64];
int reuses=0;

int encode_card(FT_GlyphSlot  slot,int card_x, int card_y);

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
    printf("Rendering U+%04x\n",unicode_points[n]);
    printf("bitmap_left=%d, bitmap_top=%d\n", slot->bitmap_left, slot->bitmap_top);
    printf("bitmap_width=%d, bitmap_rows=%d\n", slot->bitmap.width, slot->bitmap.rows);
    int char_rows=0,char_columns=0;
    int under_rows=0,under_columns=0;
    int blank_pixels_to_left=slot->bitmap_left;
    if (blank_pixels_to_left<0) blank_pixels_to_left=0;
    if (slot->bitmap_top>=0) {
      char_rows=slot->bitmap_top/8;
      if (slot->bitmap_top%8) char_rows++;
      char_columns=(slot->bitmap_left+slot->bitmap.width)/8;
      if ((slot->bitmap_left+slot->bitmap.width)%8) char_columns++;
      printf("Character is %dx%d cards above, and includes %d pixels to the left.\n",
	     char_columns,char_rows,blank_pixels_to_left);
    }
    if (slot->bitmap_top-slot->bitmap.rows<0) {
      // Character has underhang as well
      int underhang=slot->bitmap.rows-slot->bitmap_top;
      under_rows=underhang/8;
      if (underhang%8) under_rows++;
      under_columns=(slot->bitmap_left+slot->bitmap.width)/8;
      if ((slot->bitmap_left+slot->bitmap.width)%8) under_columns++;
      printf("Character is %dx%d cards under, and includes %d pixels to the left.\n",
	     under_columns,under_rows,blank_pixels_to_left);
    }

    int x,y;

    printf("y range = %d..%d\n",char_rows-1,-under_rows);
    for(y=-under_rows;y<char_rows;y++)
      for(x=0;x<char_columns;x++)
	{
	  printf("  encoding card (%d,%d)\n",x,y);
	  int card_number=encode_card(slot,x,y);
	}
    printf("done\n");
  }

  FT_Done_Face    ( face );
  FT_Done_FreeType( library );

  printf("%d Unique cards were used %d times to encode %d glyphs\n",
	 card_count,card_count+reuses,glyph_count);

  return 0;
}

int encode_card(FT_GlyphSlot  slot,int card_x, int card_y)
{
  int min_x=slot->bitmap_left;
  if (min_x<0) min_x=0;
  int max_x=slot->bitmap.width+min_x;

  int max_y=slot->bitmap_top-1;
  int min_y=slot->bitmap_top-1-slot->bitmap.rows;

  int base_x=card_x*8;
  int base_y=card_y*8;

  printf("x=%d..%d, y=%d..%d, base=(%d,%d)\n",
	 min_x,max_x,min_y,max_y,base_x,base_y);
  
  unsigned char card[64];

  int x,y;
  for(y=0;y<8;y++) {
      for(x=0;x<8;x++)
	{
	  int x_pos=x+base_x-min_x;
	  int y_pos=slot->bitmap_top-((7-y)+base_y);
	  //	  printf("pixel (%d,%d) will be in bitmap (%d,%d)\n",
	  //             x,y,x_pos,y_pos);
	  if ((x_pos>=0&&x_pos<slot->bitmap.width)
	      &&(y_pos>=0&&y_pos<slot->bitmap.rows)) {
	    // Pixel is in bitmap
	    card[y*8+x]=slot->bitmap.buffer[x_pos+
					    y_pos*slot->bitmap.width];
	  } else {
	    // Pixel is not in bitmap, so blank pixel
	    card[y*8+x]=0x00;
	  }
	}
  }

  printf("card (%d,%d) is:\n",card_x,card_y);
  for(y=0;y<8;y++) {
    for(x=0;x<8;x++)
      {
	if (card[x+y*8]==0) 
	  printf(".");
	else if (card[x+y*8]<128) 
	  printf("+");
	else
	  printf("*");
      }
    printf("\n");
  }

  // Now compare card with all previous ones
  int c;
  for(c=0;c<card_count;c++)
    {
      if (!bcmp(card,cards[c],64)) {
	printf("Re-using card #%d\n",c);
	reuses++;
	return c;
      }
    }

  // Store card if necessary
  if (card_count>=MAX_CARDS) return -1;
  printf("Creating card #%d\n",card_count);
  bcopy(card,cards[card_count++],64);
  return card_count-1;
}

/* EOF */
