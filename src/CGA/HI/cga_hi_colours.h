/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#ifndef CGA_HI_COLOURS_H
#define CGA_HI_COLOURS_H

/**
 * @brief   High resolution (640x200, 2 colour) pixel values.
 * @details Each value is pre-shifted into bit 7, the position of the
 * leftmost pixel in a video byte, so plot can bitshift rightward.
 */
typedef enum {
    CGA_HI_BLACK    = 0x00,     // 0xxx xxxx  black
    CGA_HI_COLOUR   = 0x80      // 1xxx xxxx  colour (colour select reg)
} cga_hi_colour_t;

#endif
