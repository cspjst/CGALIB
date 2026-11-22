#ifndef CGA_HI_BMP_PBM_H
#define CGA_HI_BMP_PBM_H

#include <stdint.h>

#include "cga_hi_bmp.h"

/**
 * The PBM (P4) bitstream file format is a close match to the 1-bit-per-pixel (1bpp)
 * 8pixels per byte similarly LSB..MSB left..right pixel CGA mode 6 "hi res" layout.
 * However, and rather bizarrely, the PBM format pixel 1 is black and 0 is white.
 */
uint32_t cga_hi_bmp_load_pbm_raw(const char* file_path, cga_hi_bmp_t* bmp, int block);

#endif
