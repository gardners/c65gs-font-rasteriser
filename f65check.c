#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

#define MAX_POINTS 0x4000
#define MAX_CARDS 0x10000

#define STRINGIFY_EX(x) STRINGIFY(x)
#define STRINGIFY(x) #x
#define TEST_POINT(x, y) printf(STRINGIFY_EX(__FILE__)":"STRINGIFY_EX(__LINE__)":"x"\n", y)

unsigned char magic_header[128] = {
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

void usage()
{
    fprintf(stderr, "useage: f65check -f <f65 file>\n");
    exit(1);
}

uint32_t readint(FILE *file, size_t size)
{
    uint8_t buffer[4] = {0, 0, 0, 0};
    fread(buffer, size, 1, file);
    return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
}

int main(int argc, char **argv)
{
    int opt;
    char *filename = NULL;
    FILE *file = NULL;

    while ((opt = getopt(argc, argv, "f:")) != -1)
    {
        switch(opt)
        {
            case 'f': {
                file = fopen(optarg, "r");
                if (!file)
                {
                    fprintf(stderr, "Could not open '%s'\n", optarg);
                    usage();
                }
            } break;
        }
    }
    if (file == NULL) usage();
    uint8_t oh8oh1[2]; // $0000 $0001
    fread(oh8oh1, sizeof(oh8oh1), 1, file);
    printf("oh8oh1: 0x%zX 0x%zX\n", (size_t)oh8oh1[1], (size_t)oh8oh1[0]);

    uint8_t header[sizeof(magic_header)]; // $0002-$0081
    fread(header, sizeof(magic_header), 1, file);
    int header_diff = bcmp(header, magic_header, sizeof(magic_header));
    if (header_diff) printf("Bad magic header\n");
    else printf("Magic header good\n");

    uint16_t glyph_count = readint(file, 2); // $0082 $0083
    printf("Glyph count: %zu\n", (size_t)glyph_count);

    uint16_t tile_map_start = readint(file, 2); // $0084 $0085
    printf("Tile map start: 0x%zX\n", (size_t)tile_map_start);

    uint32_t tile_array_start = readint(file, 3); // $0086 $0087 $0088
    printf("Tile array start: 0x%zX\n", (size_t)tile_array_start);

    uint16_t font_size = readint(file, 2); // $0089 $008A
    printf("Font size: %zu\n", (size_t)font_size);

    uint8_t bits_per_pixel = readint(file, 1); // $008B
    printf("Bits per pixel: %zu\n", (size_t)bits_per_pixel);

    uint16_t style_flags = readint(file, 2); // $008C $008D
    printf("Style flags: 0x%zX\n", (size_t)style_flags);

    uint8_t unknown[0x14]; // $008E-$00A1
    fread(unknown, sizeof(unknown), 1, file);

    char style_name[0x20]; // $00A2-$00C1
    fread(style_name, sizeof(style_name), 1, file);
    printf("Font style name: %s\n", style_name);

    char font_name[0x40]; // $00C2-$0101
    fread(font_name, sizeof(font_name), 1, file);
    printf("Font name: %s\n", font_name);

    // $0102-(tile_map_start-1) unicode point list
    const size_t unicode_points_size = tile_map_start - 0x100;
    printf("Unicode points size: 0x%zX\n", unicode_points_size);
    uint8_t *unicode_points = malloc(unicode_points_size);
    if (unicode_points == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for unicode_points\n");
        exit(1);
    }
    fread(unicode_points, unicode_points_size, 1, file);

    // tile_map_start-(tile_array_start-1)
    const size_t tile_map_size = tile_array_start - tile_map_start;
    printf("Tile map size: 0x%zX\n", tile_map_size);
    uint8_t *tile_map = malloc(tile_map_size);
    if (tile_map == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for tile_map\n");
        exit(1);
    }
    fread(tile_map, tile_map_size, 1, file);
    // for (size_t i = 0; i < tile_map_size; ++i)
    //     printf("0x%zX ", tile_map[i]);

    size_t _tile_array_size = ftell(file);
    fseek(file, 0, SEEK_END);
    size_t end_pos = ftell(file);
    fseek(file, _tile_array_size, SEEK_SET);
    const size_t tile_array_size = end_pos - _tile_array_size;

    printf("Tile array size: 0x%zX\n", tile_array_size);
    uint8_t *tile_array = malloc(tile_array_size);
    if (tile_array == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for tile_array\n");
        exit(1);
    }
    fread(tile_array, tile_array_size, 1, file);
    // for (size_t i = 0; i < tile_array_size; ++i)
    //     printf("0x%zX ", tile_array[i]);

    char character[21];
    printf("Type a character to test: ");
    scanf("%20s", character);
    while (character[0] != 27 && character[0] != 0)
    {
        printf("Character: 0x%zX\n", (size_t)character[0]);
        size_t point;
        for (point = 0; point < unicode_points_size; point += 5)
            if (unicode_points[point] == character[0]) break;
        printf("Character offset: 0x%zX\n", point);
        if (point < unicode_points_size)
        {
            size_t tile_map_offset = unicode_points[point + 2]
                | (unicode_points[point + 3] << 8)
                | (unicode_points[point + 4] << 16);
            printf("Tile map offset: 0x%zX\n", tile_map_offset);
            size_t offset = &tile_map[tile_map_offset] - &tile_map[0];
            printf("Offset: 0x%zX\n", offset);
            uint8_t *tile = &tile_map[tile_map_offset];
            uint8_t char_rows = *tile;
            printf("Char rows: %zu\n", (size_t)char_rows);
            uint8_t under_rows = *++tile;
            printf("Under rows: %zu\n", (size_t)under_rows);
            uint8_t char_columns = *++tile;
            printf("Char columns: %zu\n", (size_t)char_columns);
            uint8_t display_width = *++tile;
            printf("Display width: %zu\n", (size_t)display_width);
            ++tile;

            size_t glyph_height = 8 * (char_rows + under_rows);
            printf("Glyph height: %zu\n", glyph_height);
            size_t glyph_width = 8 * char_columns;
            printf("Glyph width: %zu\n", glyph_width);
            printf("Renderd glyph width: %zu\n", glyph_width-display_width);
            size_t glyph_size = glyph_height * glyph_width;
            printf("Glyph size: %zu\n", glyph_size);
            uint8_t *glyph = malloc(glyph_size);
            if (!glyph)
            {
                printf("Failed to allocate memory for glyph\n");
                exit(1);
            }
            for (size_t y = 0; y < glyph_size; y += glyph_width * 8)
                for (size_t x = 0; x < glyph_width; x += 8)
                {
                    uint16_t card_number = tile[0] | (tile[1] << 8);
                    // trim_bits = card_number >> 12;
                    card_number &= 0x0FFF;
                    tile += 2;
                    for (size_t y2 = 0; y2 < 8; ++y2)
                        bcopy(&tile_array[(card_number * 64) + (y2 * 8)],
                            &glyph[x + y + (y2 * glyph_width)], 8);
                }
            printf("Glyph:\n");
            for (size_t y = 0; y < glyph_size; y += glyph_width)
            {
                for (size_t x = 0; x < glyph_width; ++x)
                {
                    if (glyph[x+y])
                        printf("*");
                    else
                        printf(" ");
                }
                printf("\n");
            }

            free(glyph);
        }
        else printf("Character not in unicode point list\n");
        printf("Type a character to test: ");
        scanf("%20s", character);
    }
    free(unicode_points);
    free(tile_map);
    free(tile_array);
    return 0;
}