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

void cga_hi_bmp_free_data(cga_hi_bmp_t* bmp, int i) {
    for(; i >= 0; --i) {
        free(bmp->data[i]);
        bmp->data[i] = NULL;
    }
}

void cga_hi_bmp_free(cga_hi_bmp_t* bmp) {
    cga_hi_bmp_free_data(bmp, bmp->nblocks);
    free(bmp);
}

cga_hi_bmp_t* cga_hi_bmp_new(uint16_t w, uint16_t h, uint16_t sz, uint16_t n) {
    cga_hi_bmp_t* bmp = malloc(sizeof(*bmp));
    cga_hi_bmp_init(bmp, w, h, sz, n);
    return bmp;
}

void cga_hi_bmp_shift_right(char* src, char* dst, uint16_t w, uint16_t h) {
    // rcr
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

uint32_t cga_hi_bmp_save_raw(const char* file_path, const cga_hi_bmp_t* bmp) {
    uint16_t i;
    errno = EINVAL;
    if(!bmp) goto error;
    if(bmp->nblocks == 0 || bmp->nblocks > sizeof bmp->data / sizeof bmp->data[0]) goto error;
    FILE* f = fopen(file_path, "wb");
    if(!f) goto error;
    uint32_t size = sizeof(cga_hi_bmp_t);
    if(!fwrite(bmp, size, 1, f)) goto corrupt;

    for(i = 0; i < bmp->nblocks; ++i) {
        if(!bmp->data[i] || !fwrite(bmp->data[i], bmp->size, 1, f)) goto corrupt;
        size += bmp->size;
    }

    if(fclose(f) == EOF) goto discard;
    return size;
corrupt:
    fprintf(stderr, "write failed, %s removed\n", file_path);
    fclose(f);
discard:
    remove(file_path);
error:
    perror(__FUNCTION__);
    return 0;
}

uint32_t cga_hi_bmp_load_raw(const char* file_path, cga_hi_bmp_t* bmp) {
    int i = 0;
    errno = EINVAL;
    if(!bmp) goto error;
    FILE* f = fopen(file_path, "rb");
    if(!f) goto error;
    uint32_t size = sizeof(cga_hi_bmp_t);
    if(!fread(bmp, size, 1, f)) goto close;
    if(bmp->nblocks > sizeof bmp->data / sizeof bmp->data[0]) goto close;

    for(; i < bmp->nblocks; ++i) {
        bmp->data[i] = malloc(bmp->size);
        if(!bmp->data[i] || !fread(bmp->data[i], bmp->size, 1, f)) goto release;
        size += bmp->size;
    }

     if(fclose(f) == EOF) goto error;
     return size;
 release:
    cga_hi_bmp_free_data(bmp, i);
 close:
    if(feof(f)) fprintf(stderr, "file %s is shorter than expected\n", file_path);
    fclose(f);
 error:
    perror(__FUNCTION__);
    return 0;
 }
