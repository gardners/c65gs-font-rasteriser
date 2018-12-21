#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#define STRINGIFY(x) #x
#define STRINGIFY_EX(x) STRINGIFY(x)
#define TEST_POINT(x, y) printf(STRINGIFY_EX(__FILE__)":"STRINGIFY_EX(__LINE__)":"x"\n", y)

uint8_t magic_header[0x80] = {
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

FILE *ifile = NULL;
FILE *ofile = NULL;

void usage()
{
    if (ifile) fclose(ifile);
    if (ofile) fclose(ofile);
    fprintf(stderr, "usage:\nf65patch -f <f65 file> -o <output file> [-r <RAM location>]\nf65patch -F <FPK file> -o <output file> [-r <RAM location>]\n-r defaults to 0x40000 if flag no set\n");
    exit(1);
}

// size_t base_offset = 0;
size_t current_font = 0x40000;

uint32_t readint(uint8_t *file, size_t offset, size_t size)
{
    uint32_t rtn = 0;
    for (size_t i = 0; i < size; ++i)
        rtn |= file[(offset++) - current_font] << (8 * i);
    return rtn;
}

void writeint(uint8_t *file, size_t offset, uint32_t val, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        file[(offset++) - current_font] = (val >> (8 * i)) & 0xFF;
}

int main(int argc, char **argv)
{
    int opt;
    enum { F65, FPK } type;
    // size_t current_font = 0x40000;

    while ((opt = getopt(argc, argv, "f:F:o:r:")) != -1)
    {
        switch(opt)
        {
            case 'f': {
                if (ifile) usage();
                type = F65;
                ifile = fopen(optarg, "rb");
                if (!ifile)
                {
                    fprintf(stderr, "Could not open '%s'\n", optarg);
                    usage();
                }
                fpos_t pos;
                fgetpos(ifile, &pos);
                if (fgetc(ifile) != 0x01)
                {
                    fprintf(stderr, "File doesn't start with 0x01, not an F65 file");
                    usage();
                }
                fsetpos(ifile, &pos);
            } break;
            case 'F': {
                if (ifile) usage();
                type = FPK;
                ifile = fopen(optarg, "rb");
                if (!ifile)
                {
                    fprintf(stderr, "Could not open '%s'\n", optarg);
                    usage();
                }
                fpos_t pos;
                fgetpos(ifile, &pos);
                if (fgetc(ifile) != 0x2D)
                {
                    fprintf(stderr, "File doesn't start with 0x2D, not an FPK file");
                    usage();
                }
                fsetpos(ifile, &pos);
            } break;
            case 'o': {
                ofile = fopen(optarg, "wb");
                if (!ofile)
                {
                    fprintf(stderr, "Could not open '%s' for writing\n", optarg);
                    usage();
                }
            } break;
            case 'r': {
                current_font = strtoull(optarg, NULL, 0);
            } break;
        }
    }
    if (ifile == NULL || ofile == NULL) usage();
    // base_offset = current_font;
    printf("Patching for RAM location '%#0zX'\n", current_font);

    size_t font_size = 0;
    uint8_t header[0x90];
    uint8_t *mem = NULL;
    // if (type == F65)
    //     fseek(ifile, 2, SEEK_CUR); // skip 01 08
    size_t loop = 0;
    do
    {
        int c = fgetc(ifile);
        if (c == EOF) break;
        ungetc(c, ifile);
        if (type == F65)
        {
            uint16_t v;
            fread(&v, 2, 1, ifile);
            fwrite(&v, 2, 1, ofile);
        }

        fread(header, 1, sizeof(header), ifile);

        uint16_t tile_map_start = readint(header, current_font + 0x82, 2);
        uint32_t point_list_size = tile_map_start - 0x100;
        uint32_t point_list_address = current_font + 0x100;

        uint32_t tile_array_start = readint(header, current_font + 0x84, 3);
        uint32_t tile_map_address = current_font + tile_map_start;

        font_size = readint(header, current_font + 0x8C, 4); // 0x8C
        // 0x90...

        if (font_size < sizeof(header))
        {
            fwrite(header, 1, sizeof(header), ofile);
            int c = fgetc(ifile);
            while (c != EOF)
            {
                fputc(c, ofile);
                c = fgetc(ifile);
            }
            break;
        }
        if (header[0] != 0x2D)
        {
            fprintf(stderr, "Malformed F65. Header code '%i' in loop '%zu'\n", header[0], loop);
            usage();
        }
        header[0] = 0x3D; // mark this file as patched

        mem = (uint8_t*)malloc(font_size);
        assert(mem);
        memset(mem, 0, font_size);
        memcpy(mem, header, sizeof(header));
        fread(mem + sizeof(header), 1, font_size - sizeof(header), ifile);

        // start patching

        tile_array_start += current_font;
        writeint(mem, current_font + 0x84, tile_array_start, 3);
        tile_array_start /= 0x40;

        for (size_t i = 0; i < point_list_size; i += 5)
        {
            uint32_t tile_address = 0;
            uint32_t point_tile = point_list_address + i + 2;
            tile_address = readint(mem, point_tile, 3) + tile_map_address;
            writeint(mem, point_tile, tile_address, 3);
            uint8_t rows_above = readint(mem, tile_address, 1);
            uint8_t rows_below = readint(mem, tile_address + 1, 1);
            uint8_t bytes_per_row = readint(mem, tile_address + 2, 1);
            size_t tile_cards = tile_address + 4;
            int16_t glyph_size = (int16_t)(rows_above + rows_below) * bytes_per_row;
            for (size_t j = 0; j < glyph_size; ++j)
            {
                uint16_t card_address = readint(mem, tile_cards, 2) + tile_array_start;
                writeint(mem, tile_cards, card_address, 2);
                tile_cards += 2;
            }
        }

        printf("Writing %zu bytes to out file\n", font_size);
        fflush(stdout);
        fwrite(mem, 1, font_size, ofile);
        printf("Wrote %zu bytes to out file\n", font_size);
        fflush(stdout);

        free(mem);
        loop++;
        current_font += font_size;
    } while (font_size > 0);

    printf("Successfully patched\n");
    if (ifile) fclose(ifile);
    if (ofile) fclose(ofile);
    return 0;
}