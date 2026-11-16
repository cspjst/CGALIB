/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#ifndef CGA_HI_LINE_H
#define CGA_HI_LINE_H

#include "cga_hi_colours.h"

#include <stdint.h>

/**
 * @brief   Draw a horizontal line in high resolution (640x200, 2 colour).
 * @details Draws len pixels rightwards from p, along row y. len == 0 draws nothing.
 * @param   p    packed start point  i.e. y:x
 * @param   len  length in pixels
 * @param   c    CGA_HI_BLACK or CGA_HI_COLOUR
 * @note    ABSOLUTELY NO BOUNDS CHECKING! x + len must not exceed 640.
 */
void cga_hi_hline(uint32_t p, uint16_t len, cga_hi_colour_t c);

/**
 * @brief   Draw a vertical line in high resolution (640x200, 2 colour).
 * @details Draws len pixels downwards from p, in column x. len == 0 draws nothing.
 * @param   p    packed start point  i.e. y:x
 * @param   len  length in pixels
 * @param   c    CGA_HI_BLACK or CGA_HI_COLOUR
 * @note    ABSOLUTELY NO BOUNDS CHECKING! y + len must not exceed 200.
 */
void cga_hi_vline(uint32_t p, uint16_t len, cga_hi_colour_t c);

/**
 * @brief   Draw a line between two points in high resolution (640x200, 2 colour), using Bresenham's algorithm.
 * @details Both endpoints are drawn, in any direction (all 8 octants). p1 == p2 draws one pixel.
 * @param   p1  packed start point  i.e. y:x
 * @param   p2  packed end point    i.e. y:x
 * @param   c   CGA_HI_BLACK or CGA_HI_COLOUR
 * @note    ABSOLUTELY NO BOUNDS CHECKING! Both points must be on screen.
 */
void cga_hi_bresenham_line(uint32_t p1, uint32_t p2, cga_hi_colour_t c);

#endif
