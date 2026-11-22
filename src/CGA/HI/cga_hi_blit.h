#ifndef CGA_HI_BLIT_H
#define CGA_HI_BLIT_H

#include "cga_hi_bmp.h"

void cga_hi_blit_paste(cga_hi_bmp_t* bmp, uint16_t x, uint16_t y);

void cga_hi_blit_grab(cga_hi_bmp_t* bmp, uint16_t x, uint16_t y);

void cga_hi_blit(cga_hi_bmp_t* bmp, cga_hi_bmp_t* mask, cga_hi_bmp_t* grab, uint16_t x, uint16_t y);

#endif
