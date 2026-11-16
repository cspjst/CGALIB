/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#ifndef CGA_MED_PLOT_H
#define CGA_MED_PLOT_H

#include "cga_med_colours.h"
#include <stdint.h>

/**
 * @brief   Plot one pixel in medium resolution (320x200, 4 colour).
 * @details The point is a generic packed uint32_t, low word x, high word y,
 * such that points can come from any domain.
 * However, the colour is a CGA domain specific type.
 * @param   p  packed point  i.e. y:x
 * @param   c  pre-shifted pixel value, CGA_MED_BKGD .. CGA_MED_COL3
 */
void cga_med_plot(uint32_t p, cga_med_colour_t c);

/**
 * @brief   Plot one pixel in medium resolution, calculating the row offset instead of using the table.
 * @details Same contract as cga_med_plot. The offset is (y & 1) * 2000h + (y >> 1) * 80,
 * computed with shifts and no MUL, which saves the 400-byte row table.
 * Slower on an 8088: 76.1 us net per plot versus 59.9 us for the table (127%, 86Box XT).
 * The extra code bytes cost more to fetch over the 8-bit bus than one table read.
 * @param   p  packed point  i.e. y:x
 * @param   c  pre-shifted pixel value, CGA_MED_BKGD .. CGA_MED_COL3
 */
void cga_med_plot_calc(uint32_t p, cga_med_colour_t c);

#endif
