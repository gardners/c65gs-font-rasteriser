c65gs-font-rasteriser
=====================

Rasterise TrueType fonts ready for use on the C65GS retro-computer

Take a TTF file, open it using freetype, render each of the ASCII
characters, convert them to one or more 8x8 anti-aliased char cards
and write these together with control information into a simple
binary format that the C65GS can easily handle.
