#ifndef CGA_HI_BMP_H
#define CGA_HI_BMP_H

#include <stdint.h>

typedef struct {
    uint16_t width;     // 0    W   mode 6 = 0..639, mode 4 = 0..319
    uint16_t height;    // 2    W   modes 4 and 6 = 0..199
    uint16_t size;      // 4    W   data block size bytes max 64K
    uint16_t nblocks;   // 6    W   number of data blocks 1 or 8 shift optimised or other combination
    char* data[8];
} cga_hi_bmp_t;

void cga_hi_bmp_init(cga_hi_bmp_t* bmp, uint16_t w, uint16_t h, uint16_t sz, uint16_t n);

void cga_hi_bmp_top(cga_hi_bmp_t* bmp);

#endif
