/*
 * Copyright 2002-2010 Guillaume Cottenceau.
 * Copyright 2015-2016 Paul Gardner-Stephen.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...)
{
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  fprintf(stderr, "\n");
  va_end(args);
  abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

void read_png_file(char* file_name)
{
  unsigned char header[8];    // 8 is the maximum size that can be checked

  /* open file and test for it being a png */
  FILE *fp = fopen(file_name, "rb");
  if (!fp)
    abort_("[read_png_file] File %s could not be opened for reading", file_name);
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8))
    abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


  /* initialize stuff */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr)
    abort_("[read_png_file] png_create_read_struct failed");

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    abort_("[read_png_file] png_create_info_struct failed");

  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[read_png_file] Error during init_io");

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  // Convert palette to RGB values
  png_set_expand(png_ptr);

  png_read_info(png_ptr, info_ptr);

  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  number_of_passes = png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);

  /* read file */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[read_png_file] Error during read_image");

  row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
  for (y=0; y<height; y++)
    row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

  png_read_image(png_ptr, row_pointers);

  fclose(fp);
}

struct mega65_tile {
  // [x][y]
  unsigned int pixels[8][8];
};

int extract_tile(png_bytep *row_pointers,int multiplier,
		 int tx,int ty,
		 struct mega65_tile *tile)
{  
  for (y=0; y<8; y++) {
    png_byte* row = row_pointers[ty+y];
    for (x=0; x<8; x++) {
      png_byte* ptr = &(row[(tx+x)*multiplier]);
      int r=ptr[0],g=ptr[1],b=ptr[2]; // a=ptr[3];

      // XXX - Allow selection between paletised and colour cube mode
      // Compute colour cube colour
      unsigned char c=(r&0xe0)|((g>>5)<<2)|(b>>6);

      tile->pixels[x][y]=c;
      
    }
  }
  
  return 0;
}

void process_file(int mode,char *outputfilename)
{
  int multiplier=-1;
  if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB)
    multiplier=3;
    
  if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGBA)
    multiplier=4;

  if (multiplier==-1) {
    fprintf(stderr,"Could not convert file to RGB or RGBA\n");
  }

  if (height&7||width&7) {
    fprintf(stderr,"Image must be a multiple of 8 pixels wide and high, so that it fits into an integer number of tiles.\n");
  }
  for(int ty=0;ty<height;ty+=8) {
    for(int tx=0;tx<width;tx+=8) {
      // Process a tile.
      struct mega65_tile tile;
      extract_tile(row_pointers,multiplier,tx,ty,&tile);
      
    }
  }
      
}


int main(int argc, char **argv)
{
  if (argc != 4) {
    fprintf(stderr,"Usage: program_name <logo|charrom> <file_in> <file_out>\n");
    exit(-1);
  }

  int mode=-1;

  if (!strcasecmp("logo",argv[1])) mode=0;
  if (!strcasecmp("charrom",argv[1])) mode=1;
  if (!strcasecmp("hires",argv[1])) mode=2;
  if (mode==-1) {
    fprintf(stderr,"Usage: program_name <logo|charrom> <file_in> <file_out>\n");
    exit(-1);
  }
  read_png_file(argv[2]);
  process_file(mode,argv[3]);

  return 0;
}
