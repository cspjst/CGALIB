#include "cga_hi_bmp.h"

#include <errno.h>
#include <stdio.h>

void cga_hi_bmp_init(cga_hi_bmp_t* bmp, uint16_t w, uint16_t h, uint16_t sz, uint16_t n) {
    if(bmp) {
        bmp->width = w;
        bmp->height = h;
        bmp->size = sz;
        bmp->nblocks = n;
    }
    else {
        errno = EINVAL;
        perror(__FUNCTION__);
    }
}
