#ifndef CGA_HI_PLOT_H
#define CGA_HI_PLOT_H

#include "cga_hi_colours.h"
#include <stdint.h>

/**
 * @brief   Plot one pixel in medium resolution (640x200, 4 colour).
 * @details The point is a generic packed uint32_t, low word x, high word y,
 * such that points can come from any domain.
 * However, the colour is a CGA domain specific type.
 * @param   p  packed point  i.e. y:x
 * @param   c  pre-shifted pixel value, CGA_HI_COLOUR or CGA_HI_BLACK
 */
void cga_hi_plot(uint32_t p, cga_hi_colour_t c);

#endif
