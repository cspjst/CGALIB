#include "cga_hi_bmp.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void cga_hi_bmp_init(cga_hi_bmp_t* bmp, uint16_t w, uint16_t h, uint16_t sz, uint16_t n) {
    if(bmp) {
        bmp->width = w;
        bmp->height = h;
        bmp->size = sz;
        bmp->nblocks = n;
        for(int i = 0; i < n; ++i) bmp->data[i] = NULL;
    }
    else {
        errno = EINVAL;
        perror(__FUNCTION__);
    }
}

cga_hi_bmp_t* cga_hi_bmp_new(uint16_t w, uint16_t h, uint16_t sz, uint16_t n) {
    cga_hi_bmp_t* bmp = malloc(sizeof(*bmp));
    cga_hi_bmp_init(bmp, w, h, sz, n);
    return bmp;
}

void cga_hi_bmp_top( const cga_hi_bmp_t* bmp) {
    errno = EINVAL;
    if(!bmp) { perror(__FUNCTION__);  return; }
    printf("cga_hi_bmp_t @%p\n%i x %i pixels, %i bytes x %i block(s)\n", (void*)bmp, bmp->width, bmp->height, bmp->size, bmp->nblocks);
    for(int i = 0; i < bmp->nblocks; ++i) {
        printf("[%i] %p ",i ,(void*)bmp->data[i]);
        for(int j = 0; j < 16; ++j)
            (bmp->data[i]) ?printf("%02X ", (unsigned char)bmp->data[i][j]) :printf("?? ");
        printf("\n");
    }
}

uint32_t cga_hi_bmp_save(const char* file_path, const cga_hi_bmp_t* bmp) {
    errno = EINVAL;
    if(!bmp) { perror(__FUNCTION__);  return 0; }
    FILE* f = fopen(file_path, "wb");
    if(!f) { perror(__FUNCTION__); return 0; };
    uint32_t size = sizeof(cga_hi_bmp_t);
    if(!fwrite(bmp, size, 1, f)) { perror(__FUNCTION__); return 0; };

    return size;
}
