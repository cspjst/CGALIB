#include "cga_hi_bmp_pbm.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define PBM_MAGIC_P4    0x3450u
#define PBM_LINE_SIZE   80

uint32_t cga_hi_bmp_load_pbm_raw(const char* file_path, cga_hi_bmp_t* bmp, int block) {
    errno = EINVAL;
    uint16_t magic;
    char line[PBM_LINE_SIZE];
    if(!file_path || !bmp) goto error;
    FILE* f = fopen(file_path, "rb");
    if(!f) goto error;
    if(!fgets((char*)&line, PBM_LINE_SIZE, f)) goto close;
    if(*(uint16_t*)line  != PBM_MAGIC_P4) { fprintf(stderr, "file %s not PB4 raw binary raster\n", file_path); goto close; }
    while (fgets(line, sizeof(line), f) && (line[0] == '#')) {}
    if (sscanf(line, "%hu %hu", &bmp->width, &bmp->height) != 2) goto close;
    bmp->size = (bmp->width >> 3) * bmp->height;
    bmp->nblocks++;
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
