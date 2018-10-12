#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <unistd.h>

#include "utf8_decode.h"

#define MAX_POINTS 16384
int unicode_points[MAX_POINTS]={
  0
};
int glyph_count=0;
int rendered=0;

#define MAX_CARDS 65536
int card_count=0;
unsigned char cards[MAX_CARDS][64];
int card_reused[MAX_CARDS]={0};
int reuses=0;

unsigned char magic_header[128]=
  {
    0x2d, 0x08, 0x0a, 0x00, 0x99, 0x22, 0x54, 
    0x48, 0x49, 0x53, 0x20, 0x49, 0x53, 0x20, 0x41,
    0x20, 0x43, 0x36, 0x35, 0x47, 0x53, 0x20, 0x38, 
    0x42, 0x50, 0x50, 0x20, 0x46, 0x4f, 0x4e, 0x54,
    0x20, 0x46, 0x49, 0x4c, 0x45, 0x2c, 0x20, 0x56, 
    0x31, 0x2e, 0x30, 0x22, 0x00, 0x55, 0x08, 0x14,
    0x00, 0x99, 0x22, 0x53, 0x45, 0x45, 0x20, 0x48, 
    0x54, 0x54, 0x50, 0x3a, 0x2f, 0x2f, 0x43, 0x36,
    0x35, 0x47, 0x53, 0x2e, 0x42, 0x4c, 0x4f, 0x47, 
    0x53, 0x50, 0x4f, 0x54, 0x2e, 0x43, 0x4f, 0x4d,
    0x2e, 0x41, 0x55, 0x22, 0x00, 0x72, 0x08, 0x1e, 
    0x00, 0x99, 0x22, 0x46, 0x4f, 0x52, 0x20, 0x4d,
    0x4f, 0x52, 0x45, 0x20, 0x49, 0x4e, 0x46, 0x4f, 
    0x52, 0x4d, 0x41, 0x54, 0x49, 0x4f, 0x4e, 0x2e,
    0x22, 0x00, 0x78, 0x08, 0x28, 0x00, 0x80, 0x00, 
    0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff
  };

unsigned char font_data[16*1024*1024];
unsigned char tile_map_buffer[16*1024*1024];
int tile_map_offset=0;

int encode_card(FT_GlyphSlot  slot,int card_x, int card_y);

void usage(void)
{
  fprintf ( stderr,
	    "usage: ttftof65 [-A] [-8 <utf8 text file>] [-T <TTF file>] [-P <point size for rasterised font>] [-o <output file>]\n"
	    "    -A - Include all normal ASCII characters in the rasterised font.\n"
	    "    -8 - Include all code points used in the indicated UTF8 text file.\n"
	    "    -T - Specify the TTF source file.\n"
	    "    -P - Point size of rasterised output.\n"
	    "    -o - Name of rasterised MEGA65 font file for output.\n");
  exit( 1 );
}

int add_code_point(int point)
{
  // Don't insert duplicates
  for(int i=0;i<glyph_count;i++) {
    if (point==unicode_points[i]) return 0;
  }

  if (glyph_count>=MAX_POINTS) {
    fprintf(stderr,"ERROR: Too many unicode points specified. Consider increaseing MAX_POINTS, or reduce number of points required in font.\n");
    exit(-1);
  }

  unicode_points[glyph_count++]=point;
  return 0;
}

int
main( int     argc,
      char**  argv )
{
  int opt;
  char*         filename = NULL;
  char*         output_file = NULL;
  int           font_size=8;
  
  while ((opt = getopt(argc, argv, "A8:T:P:o:")) != -1) {
    switch (opt) {
    case 'A':
      // Add ascii code points
      for(int i=0x20;i<0x7f;i++) add_code_point(i);
      break;
    case 'P':
      font_size=atoi(optarg);
      if (font_size<1||font_size>200) {
	fprintf(stderr,"Illegal point size. Must be between 1 and 200.\n");
	usage();
      }
      break;
    case 'T':
      filename = optarg;
      break;
    case 'o':
      output_file = optarg;
      break;
    case '8':
      /* Read UTF8 text file to get a set of points to read it from */
      {
	FILE *f=fopen(optarg,"r");
	char line[1024];
	if (!f) {
	  fprintf(stderr,"Could not open '%s' for reading UTF8 text: Assuming it is text\n",optarg);
	  utf8_decode_init(optarg,strlen(optarg));
	  int code_point=utf8_decode_next();
	  while ((code_point!=UTF8_END)&&(code_point!=UTF8_ERROR))
	    {
	      add_code_point(code_point);
	      code_point=utf8_decode_next();
	    }
	} else {
	  line[0]=0; fgets(line,1024,f);
	  while (line[0]) {
	    utf8_decode_init(line,strlen(line));
	    int code_point=utf8_decode_next();
	    while ((code_point!=UTF8_END)&&(code_point!=UTF8_ERROR))
	      {
		add_code_point(code_point);
		code_point=utf8_decode_next();
	      }
	    line[0]=0; fgets(line,1024,f);
	  }
	  fclose(f);
	}
      }
      break;
    default: /* '?' */
      fprintf(stderr,"Unknown option character '%c'\n",opt);
      usage();
    }
  }  

  if (!filename) { fprintf(stderr,"No TTF file specified.\n"); usage(); }
  if (!output_file) { fprintf(stderr,"No output file specified.\n"); usage(); }
  if (!glyph_count) { fprintf(stderr,"No Unicode points specified.\n"); usage(); }
  

  // Initialise unicode points we want to render.
  printf("Defined %d unicode points\n",glyph_count);


  FT_Library    library;
  FT_Face       face;

  FT_GlyphSlot  slot;
  FT_Error      error;

  int           n;

  error = FT_Init_FreeType( &library );              /* initialize library */
  /* error handling omitted */

  error = FT_New_Face( library, filename, 0, &face );/* create face object */
  /* error handling omitted */

  error = FT_Set_Pixel_Sizes( face, font_size, font_size);
  /* error handling omitted */

  slot = face->glyph;

  for ( n = 0; n < glyph_count; n++ )
  {
    // Convert unicode point to glyph index
    int glyph_index = FT_Get_Char_Index( face, unicode_points[n] );
    // read glyph by index
    error = FT_Load_Glyph( face, glyph_index, FT_LOAD_RENDER );

    if ( error )
      continue;                 /* ignore errors */

    /* now, draw to our target surface (convert position) */
    printf("Rendering U+%04x\n",unicode_points[n]);

    // Work out horizontal width of the glyph
    int glyph_display_width=slot->bitmap_left+slot->bitmap.width;
    if (glyph_display_width==0)
      glyph_display_width=(slot->metrics.horiAdvance/64);
    
    //    printf("bitmap_left=%d, bitmap_top=%d\n", slot->bitmap_left, slot->bitmap_top);
    //    printf("bitmap_width=%d, bitmap_rows=%d\n", slot->bitmap.width, slot->bitmap.rows);
    int char_rows=0,char_columns=0;
    int under_rows=0,under_columns=0;
    int blank_pixels_to_left=slot->bitmap_left;
    if (blank_pixels_to_left<0) blank_pixels_to_left=0;
    if (slot->bitmap_top>=0) {
      char_rows=(slot->bitmap_top+1)/8;
      if ((slot->bitmap_top+1)%8) char_rows++;
      char_columns=glyph_display_width/8;
      if (glyph_display_width&7) char_columns++;
      if (!char_columns) { char_columns=1; char_rows=1; }
      if (0) printf("Character is %dx%d cards above, and includes %d pixels to the left. bitmap_top=%d\n",
		    char_columns,char_rows,blank_pixels_to_left,slot->bitmap_top);
    }
    if (0) printf("bitmap_top=%d, bitmap.rows=%d\n",slot->bitmap_top,slot->bitmap.rows);
    if (slot->bitmap_top<slot->bitmap.rows) {
      // Character has underhang as well
      int underhang=slot->bitmap.rows-slot->bitmap_top;
      under_rows=underhang/8;
      if (underhang%8) under_rows++;
      under_columns=(slot->bitmap_left+slot->bitmap.width+1)/8;
      if ((slot->bitmap_left+slot->bitmap.width)%8) under_columns++;
      if (0) printf("Character is %dx%d cards under (underhang=%d), and includes %d pixels to the left.\n",
		    under_columns,under_rows,underhang,blank_pixels_to_left);
    }

    int x,y;

    // printf("y range = %d..%d\n",char_rows-1,-under_rows);
    
    unsigned char glyph_tile_map[3+(2*(256*256))];
    int gtm_len=0;

    // Record size of character map
    glyph_tile_map[gtm_len++]=char_rows;
    glyph_tile_map[gtm_len++]=under_rows;
    glyph_tile_map[gtm_len++]=char_columns;

    printf("glyph display width = %d\n",glyph_display_width);
    printf("horiAdvance = %d, char_columns=%d\n",
	   (int)slot->metrics.horiAdvance,char_columns);
    
    // Record number of pixels to trim from right-most tile
    int trim_pixels=7-(glyph_display_width&7);
    if (!(glyph_display_width&7)) trim_pixels=0;
    glyph_tile_map[gtm_len++]=trim_pixels;
    printf("glyph_display_width=%d, trim_pixels=%d\n",
	   glyph_display_width,trim_pixels);
    
    // Now build the glyph map

    for(y=char_rows-1;y>=-under_rows;y--)
      for(x=0;x<char_columns;x++)
	{
	  int card_number=encode_card(slot,x,y);
	  if (card_number<0||card_number>4095) {
	    printf("Ran out of tiles.\n");
	    exit(-1);
	  } else {
	    printf("  encoding tile (%d,%d) using card #%d\n",x,y,card_number);	  
	    glyph_tile_map[gtm_len++]=card_number&0xff;
	    // Also encode trim pixels for right-most column, so that we don't have
	    // to apply them during rendering
	    if (x==(char_columns-1))
	      glyph_tile_map[gtm_len++]=((card_number>>8)&0xff)|(trim_pixels<<5);
	    else
	      glyph_tile_map[gtm_len++]=(card_number>>8)&0xff;	  
	  }
	}
    printf("  %d bytes used for glyph map.\n",gtm_len);

    // Write unicode point into list
    font_data[0x100 + rendered*5 + 0] = unicode_points[n]&0xff;
    font_data[0x100 + rendered*5 + 1] = (unicode_points[n]>>8)&0xff;
    // Followed by address of tile map
    font_data[0x100 + rendered*5 + 2] = tile_map_offset&0xff;
    font_data[0x100 + rendered*5 + 3] = (tile_map_offset>>8)&0xff;
    font_data[0x100 + rendered*5 + 4] = (tile_map_offset>>16)&0xff;
    // Now append tile map to temporary buffer until we write the whole thing out
    if (tile_map_offset+gtm_len>sizeof tile_map_buffer) {
      printf("Glyph tile map too large.\n");
    }
    bcopy(&glyph_tile_map[0],&tile_map_buffer[tile_map_offset],gtm_len);
    tile_map_offset+=gtm_len;
    
    rendered++;
  }

  // Now that we know how many glyphs are in the font, write this and other 
  // info into the header

  bcopy(magic_header,font_data,sizeof magic_header);

  // Glyph count at $0080-$0081
  font_data[0x80]=rendered&0xff;
  font_data[0x81]=(rendered>>8)&0xff;
  // Tile map start at $0082-$0083.
  // 16-bit offset limits number of glyphs in a font to (64K-256)/5 = quite a few
  int tile_map_start=0x100+5*rendered;
  font_data[0x82]=tile_map_start&0xff;
  font_data[0x83]=(tile_map_start>>8)&0xff;
  // Copy tile map into place
  bcopy(&tile_map_buffer[0],&font_data[tile_map_start],tile_map_offset);
  // Tile array (must be on a 64 byte boundary so that glyphs can be used in-place
  // if font file is loaded at a page boundary).
  int tile_array_start=tile_map_start + tile_map_offset;
  if (tile_array_start&63) { tile_array_start+=64; tile_array_start&=0xffffffc0; }
  font_data[0x84]=tile_array_start&0xff;
  font_data[0x85]=(tile_array_start>>8)&0xff;
  font_data[0x86]=(tile_array_start>>16)&0xff;
  // Copy tiles into place
  for(int i=0;i<card_count;i++) bcopy(cards[i],&font_data[tile_array_start+i*64],64);
  int font_file_size=tile_array_start+64*card_count;
  font_data[0x87]=font_size&0xff;
  font_data[0x88]=(font_size>>8)&0xff;
  // 8 bits per pixel
  font_data[0x89]=8;
  // slant, bold, underline and other flags (2 bytes allowed)
  font_data[0x8a]=face->style_flags&0xff;
  font_data[0x8b]=(face->style_flags>>8)&0xff;
  
  // $00A0-$00BF - style (eg bold, italic, condensed) of font
  if (face->style_name) {
    for(int i=0;i<32;i++)
      if (face->style_name[i]) font_data[0xa0+i]=face->style_name[i];
      else break;
  }

  // $00c0 - $00ff - name of font (upto 64 bytes)
  if (face->family_name) {
    for(int i=0;i<64;i++)
      if (face->family_name[i]) font_data[0xc0+i]=face->family_name[i];
      else break;
  }

  int unique_reused=0;
  for(int i=0;i<card_count;i++) if (card_reused[i]) unique_reused++;
  printf("%d Unique cards were used %d times (%d more than once) to encode %d of %d requested glyphs\n",
	 card_count,card_count+reuses,unique_reused,rendered,glyph_count);
  printf("Tile map start = %d ($%x)\n",tile_map_start,tile_map_start);
  printf("Tile array start = %d ($%x)\n",tile_array_start,tile_array_start);
  printf("Tile array size = %d ($%x)\n",64*card_count,64*card_count);
  printf("Total font file size = %d ($%x)\n",font_file_size,font_file_size);

  if (output_file) {
    FILE *f=fopen(output_file,"w");
    if (!f) {
      printf("Failed to open font output file '%s'\n",output_file);
      exit(-1);
    }
    unsigned char oh8oh1[2]={0x01,0x08};
    fwrite(oh8oh1,2,1,f);
    int r=fwrite(font_data,font_file_size,1,f);
    if (r!=1) {
      printf("Failed to write font data to '%s'\n",output_file);
      exit(-1);
    }
    fclose(f);    
  } else {
    fprintf(stderr,"WARNING: No output file specified, so no output written.\n");
    exit(0);
  }

  FT_Done_Face    ( face );
  FT_Done_FreeType( library );

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

  if (0) printf("x=%d..%d, y=%d..%d, base=(%d,%d)\n",
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

  if (0) {
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
  }

  // Now compare card with all previous ones
  int c;
  for(c=0;c<card_count;c++)
    {
      if (!bcmp(card,cards[c],64)) {
	// printf("Re-using card #%d\n",c);
	reuses++;
	card_reused[c]++;
	return c;
      }
    }

  // Store card if necessary
  if (card_count>=MAX_CARDS) return -1;
  //  printf("Creating card #%d\n",card_count);
  card_reused[card_count]=0;
  bcopy(card,cards[card_count++],64);
  return card_count-1;
}

/* EOF */
