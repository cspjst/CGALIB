#include "cga_hi_bmp_pbm.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define PBM_MAGIC_P4  0x3450u

uint32_t cga_hi_bmp_load_pbm_raw(const char* file_path, cga_hi_bmp_t* bmp, int block) {
    errno = EINVAL;
    uint16_t magic;
    if(!file_path || !bmp) goto error;
    FILE* f = fopen(file_path, "rb");
    if(!f) goto error;
    if(!fread(&magic, sizeof magic, 1, f)) goto close;
    if(magic != PBM_MAGIC_P4) { fprintf(stderr, "file %s not PB4 raw binary raster\n", file_path); goto close; }

    //bmp->data[block] = malloc(bmp->size);
    //if(!bmp->data[block] || !fread(bmp->data[block], bmp->size, 1, f)) goto release;


    release:
        free(bmp->data[block]);
        bmp->data[block] = NULL;
    close:
        if(feof(f)) fprintf(stderr, "file %s is shorter than expected\n", file_path);
        fclose(f);
    error:
        perror(__FUNCTION__);
        return 0;
}
