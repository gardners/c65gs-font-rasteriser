c65gs-font-rasteriser
=====================

Rasterise TrueType fonts ready for use on the MEGA65 retro-computer

Take a TTF file, open it using freetype, render each of the ASCII
characters, convert them to one or more 8x8 anti-aliased char cards
and write these together with control information into a simple
binary format that the MEGA65 can easily handle.

The file format consists of a 128-byte C64 BASIC header that simply
tells the user that it is a font, not a program, if they try to run
it on a C64.

This is followed by the following fields:

$0080-$0081 - Number of unicode points
$0082-$0083 - Start location of the tile map in the file.
$0084-$0086 - Tile array starting location (24 bit address). Must be a multiple of 64 due to hardware limitations of the MEGA65.  Before loading, this holds the offset of the tile array relative to the start of the file.  After loading, it is usually patched to point to the physical address.
$0087-$0088 - Size of font (is it in points or pixels?)
$0089 - Mono / 8-bit greyscale flag. 8 = 8-bit, 1= mono.
$008A-$008B - Font flags from Freetype.
$008C-$009F - RESERVED
$00A0-$00BF - Text description of font style, e.g., "bold condensed"
$00C0-$00FF - Font family name, e.g., "Prehistoric Sans".

$0100 onwards - Unicode point list, consisting of 5 bytes per point:: two bytes for unicode point (we support only the first 65535 unicode points), followed by the 24-bit offset into the tile map for this code point, i.e., relative to the start of the tile map.

Each tile map consists of the following:
Offset $0 - The number of 8-pixel rows above the baseline.
Offset $1 - The number of 8-pixel rows below the baseline.
Offset $2 - The number of 8-pixel columns.
Offset $3 - The number of pixels (0 - 7) to trim from the rightmost column.
This is then followed by a grid of (rows above baseline + rows below baseline) * columns 16-bit tile numbers that are used to assemble the glyph.

The final section of the file is the tile array.  Each tile consists of 64 bytes if using 8-bit greyscale, arranged in an 8x8 tile, following the convention of the C64 VIC-II, but with 8 bytes forming a row of pixels, followed by the 8 bytes for the next row and so on.

