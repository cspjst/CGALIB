#ifndef CGA_MED_COLOURS_H
#define CGA_MED_COLOURS_H

/**
 * @brief   Medium resolution (320x200, 4 colour) pixel values.
 * @details Each value is pre-shifted into bits 7-6, the position of the
 * leftmost pixel in a video byte, so plot can bitshift rightward.
 */
typedef enum {
    CGA_MED_BKGD = 0x00,    // 00xx xxxx  background (colour select reg)
    CGA_MED_COL1 = 0x40,    // 01xx xxxx  colour 1 of the colour set
    CGA_MED_COL2 = 0x80,    // 10xx xxxx  colour 2 of the colour set
    CGA_MED_COL3 = 0xC0     // 11xx xxxx  colour 3 of the colour set
} cga_med_colour_t;

#endif
