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

#endif
