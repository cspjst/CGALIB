/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#include "test_cga_bmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "../CGA/HI/cga_hi_bmp.h"

#define BMP_FILE    "TBMP.CGA"          /* 8.3 names for DOS */
#define BMP_BAD     "TBMPBAD.CGA"

/* ---------------------------------------------------------------- helpers */

static int test_file_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if(!f) return 0;
    fclose(f);
    return 1;
}

static void test_bmp_report(const char* name, int ok, unsigned* failed) {
    printf("  %-34s %s\n", name, ok ? "PASS" : "FAIL");
    if(!ok) ++*failed;
}

/* ---------------------------------------------------------------- high res bitmap */

static unsigned test_hi_bmp_capacity(const cga_hi_bmp_t* b) {
    return sizeof b->data / sizeof b->data[0];
}

/* allocate any missing blocks and fill every block with a pattern unique to block and offset */
static int test_hi_bmp_fill(cga_hi_bmp_t* b) {
    uint16_t i, j;
    for(i = 0; i < b->nblocks; ++i) {
        if(!b->data[i]) b->data[i] = malloc(b->size);
        if(!b->data[i]) return 0;
        for(j = 0; j < b->size; ++j)
            ((unsigned char*)b->data[i])[j] = (unsigned char)(j * 7 + i * 31 + 1);
    }
    return 1;
}

/* counts differences: header fields, then every data byte */
static unsigned test_hi_bmp_diff(const cga_hi_bmp_t* a, const cga_hi_bmp_t* b) {
    unsigned bad = 0;
    uint16_t i, j;
    if(a->nblocks != b->nblocks) ++bad;
    if(a->size != b->size) ++bad;
    if(bad) return bad;
    for(i = 0; i < a->nblocks; ++i)
        for(j = 0; j < a->size; ++j)
            if(((unsigned char*)a->data[i])[j] != ((unsigned char*)b->data[i])[j]) ++bad;
    return bad;
}

/* writes header bytes of b then n bytes of its first block: for the truncation tests */
static void test_hi_bmp_write_partial(const char* path, const cga_hi_bmp_t* b, unsigned header, unsigned n) {
    FILE* f = fopen(path, "wb");
    assert(f);
    fwrite(b, header, 1, f);
    if(n) fwrite(b->data[0], n, 1, f);
    fclose(f);
}

static void test_cga_hi_bmp() {
    cga_hi_bmp_t* out = malloc(sizeof(*out));
    cga_hi_bmp_t* in  = malloc(sizeof(*in));
    unsigned cap, n, failed = 0;
    uint32_t expect, got;
    uint16_t saved;

    printf("Testing cga_hi_bmp save / load...\n");
    assert(out && in);
    cap = test_hi_bmp_capacity(out);
    n = cap < 3 ? cap : 3;

    /* 1. round trip, 1 block */
    cga_hi_bmp_init(out, 32, 8, 256, 1);
    assert(test_hi_bmp_fill(out));
    expect = sizeof(cga_hi_bmp_t) + (uint32_t)out->nblocks * out->size;
    got = cga_hi_bmp_save(BMP_FILE, out);
    test_bmp_report("save 1 block: size returned", got == expect, &failed);
    got = cga_hi_bmp_load(BMP_FILE, in);
    test_bmp_report("load 1 block: size returned", got == expect, &failed);
    test_bmp_report("load 1 block: identical", got && test_hi_bmp_diff(out, in) == 0, &failed);
    if(got) cga_hi_bmp_free_data(in, in->nblocks - 1);
    cga_hi_bmp_free_data(out, out->nblocks - 1);

    /* 2. round trip, several blocks */
    cga_hi_bmp_init(out, 32, 8, 256, n);
    assert(test_hi_bmp_fill(out));
    expect = sizeof(cga_hi_bmp_t) + (uint32_t)out->nblocks * out->size;
    got = cga_hi_bmp_save(BMP_FILE, out);
    test_bmp_report("save several blocks: size", got == expect, &failed);
    got = cga_hi_bmp_load(BMP_FILE, in);
    test_bmp_report("load several blocks: size", got == expect, &failed);
    test_bmp_report("load several blocks: identical", got && test_hi_bmp_diff(out, in) == 0, &failed);
    if(got) cga_hi_bmp_free_data(in, in->nblocks - 1);

    printf("  (library error messages below are expected)\n");

    /* 3. NULL bitmap */
    test_bmp_report("save NULL bmp", cga_hi_bmp_save(BMP_BAD, NULL) == 0, &failed);
    test_bmp_report("load NULL bmp", cga_hi_bmp_load(BMP_FILE, NULL) == 0, &failed);

    /* 4. NULL block: refused, and no file left behind */
    free(out->data[0]);
    out->data[0] = NULL;
    test_bmp_report("save NULL block refused", cga_hi_bmp_save(BMP_BAD, out) == 0, &failed);
    test_bmp_report("save NULL block: file removed", !test_file_exists(BMP_BAD), &failed);
    assert(test_hi_bmp_fill(out));

    /* 5. bad nblocks on save: refused before anything is written */
    saved = out->nblocks;
    out->nblocks = 0;
    test_bmp_report("save nblocks 0 refused", cga_hi_bmp_save(BMP_BAD, out) == 0, &failed);
    out->nblocks = cap + 1;
    test_bmp_report("save nblocks > capacity refused", cga_hi_bmp_save(BMP_BAD, out) == 0, &failed);
    test_bmp_report("bad nblocks: no file", !test_file_exists(BMP_BAD), &failed);

    /* 6. load a header claiming too many blocks */
    test_hi_bmp_write_partial(BMP_BAD, out, sizeof(cga_hi_bmp_t), 0);
    out->nblocks = saved;
    test_bmp_report("load nblocks > capacity refused", cga_hi_bmp_load(BMP_BAD, in) == 0, &failed);

    /* 7. missing file */
    remove(BMP_BAD);
    test_bmp_report("load missing file", cga_hi_bmp_load(BMP_BAD, in) == 0, &failed);

    /* 8. truncated header */
    test_hi_bmp_write_partial(BMP_BAD, out, 3, 0);
    test_bmp_report("load truncated header", cga_hi_bmp_load(BMP_BAD, in) == 0, &failed);

    /* 9. truncated data: the partly read block is released */
    test_hi_bmp_write_partial(BMP_BAD, out, sizeof(cga_hi_bmp_t), out->size / 2);
    test_bmp_report("load truncated data", cga_hi_bmp_load(BMP_BAD, in) == 0, &failed);
    test_bmp_report("truncated data: block released", in->data[0] == NULL, &failed);

    cga_hi_bmp_free_data(out, out->nblocks - 1);
    remove(BMP_FILE);
    remove(BMP_BAD);
    free(out);
    free(in);

    printf("cga_hi_bmp save / load: %u failed\n\n", failed);
    assert(failed == 0);
}

/* ---------------------------------------------------------------- runner */

void test_cga_bmp() {
    test_cga_hi_bmp();
    //test_cga_med_bmp();   to come: same tests with the medium resolution bitmap type
}
