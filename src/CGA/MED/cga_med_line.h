/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#ifndef CGA_MED_LINE_H
#define CGA_MED_LINE_H

#include "cga_med_colours.h"

#include <stdint.h>

void cga_med_hline(uint32_t p, uint16_t len, cga_med_colour_t c);

void cga_med_vline(uint32_t p, uint16_t len, cga_med_colour_t c);

void cga_med_bresenham_line(uint32_t p1, uint32_t p2, cga_med_colour_t c);

#endif
