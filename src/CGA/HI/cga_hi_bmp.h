#ifndef CGA_HI_BMP_H
#define CGA_HI_BMP_H

#include <stdint.h>

#define CGA_BMP_HI_OFFSET_WIDTH     0
#define CGA_BMP_HI_OFFSET_HEIGHT    2
#define CGA_BMP_HI_OFFSET_SIZE      4
#define CGA_BMP_HI_OFFSET_NBLOCKS   6
#define CGA_BMP_HI_OFFSET_DATA      8

typedef struct {
    uint16_t width;     // 0    W   mode 6 = 0..639, mode 4 = 0..319
    uint16_t height;    // 2    W   modes 4 and 6 = 0..199
    uint16_t size;      // 4    W   data block size bytes max 64K
    uint16_t nblocks;   // 6    W   number of data blocks 1 or 8 shift optimised or other combination
    char* data[8];      // 8    DW x 4
} cga_hi_bmp_t;         // 40 bytes

void cga_hi_bmp_init(cga_hi_bmp_t* bmp, uint16_t w, uint16_t h, uint16_t sz, uint16_t n);

cga_hi_bmp_t* cga_hi_bmp_new(uint16_t w, uint16_t h, uint16_t sz, uint16_t n);

void cga_hi_bmp_free_data(cga_hi_bmp_t* bmp, int i);

void cga_hi_bmp_free(cga_hi_bmp_t* bmp);

void cga_hi_bmp_shift_right(char* src, char* dst, uint16_t w, uint16_t h);

void cga_hi_bmp_top(const cga_hi_bmp_t* bmp);

uint32_t cga_hi_bmp_save_raw(const char* file_path, const cga_hi_bmp_t* bmp);

uint32_t cga_hi_bmp_load_raw(const char* file_path, cga_hi_bmp_t* bmp);

#endif
