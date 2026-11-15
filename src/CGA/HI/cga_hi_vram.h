#ifndef CGA_HI_VRAM_H
#define CGA_HI_VRAM_H

#include "cga_hi_colours.h"

#include <stdint.h>

void cga_hi_cls();

void cga_hi_cls_colour(cga_hi_colour_t c);

void cga_hi_fill_vram(uint16_t pattern1, uint16_t pattern2);

#endif
