/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#include "test_cga.h"

#include <stdio.h>
#include <assert.h>

#include "../include/BIOS/bios_version.h"
#include "../include/BIOS/bios_keyboard_services.h"

#include "../include/ENV/env_version.h"
#include "../include/ENV/env_video_mode.h"

#include "../CGA/cga_types.h"
#include "../CGA/MED/cga_med_plot.h"


// vendored headers and .LIBs must come from the same build
void test_cga_versions() {
    printf("BIOSLIB headers %s, BIOS.LIB %s\n", BIOS_VERSION_STRING, bios_version_string());
    printf("ENVLIB  headers %s, ENV.LIB  %s\n", ENV_VERSION_STRING,  env_version_string());
    assert(bios_version() == BIOS_VERSION);
    assert(env_version()  == ENV_VERSION);
    printf("Vendored libraries match their headers: PASS\n\n");
}

static unsigned char cga_med_peek(unsigned x, unsigned y) {
    unsigned char* vram = (unsigned char*)0xB8000000L;
    unsigned offset = ((y & 1) ? 0x2000 : 0) + (y >> 1) * 80 + (x >> 2);
    unsigned shift  = 6 - ((x & 3) << 1);
    return (vram[offset] >> shift) & 3;
}

static void wait_key_cga(const char* msg) {
    bios_key_t key;
    printf("%s", msg);
    bios_wait_for_keystroke_and_read(&key);
}

void test_cga_med_plot() {
    static const unsigned int colours[4] = {
           CGA_MED_BKGD, CGA_MED_COL1, CGA_MED_COL2, CGA_MED_COL3
       };
    static const uint16_t corners[4][2] = { {0,0}, {319,0}, {0,199}, {319,199} };
    bios_video_mode_t original;
    cga_point_t p;
    unsigned c, i;
    unsigned long bad_fill = 0;
    unsigned int bad_pack = 0, bad_corner = 0, bad_neighbour = 0, bad_overwrite = 0;

    /* 0. the union packs x into the low word, y into the high word */
    p.x = 0x1234;
    p.y = 0x5678;
    if (p.u != 0x56781234UL) ++bad_pack;

    wait_key_cga("Mode 04h next - press a key...\n");
    original = env_set_video_mode(0x04);

    /* 1. corners, every colour: catches x/y swaps and interlace errors */
    for (i = 0; i < 4; ++i) {
        p.x = corners[i][0];
        p.y = corners[i][1];
        for (c = 0; c < 4; ++c) {
            cga_med_plot(p.u, colours[c]);
            if (cga_med_peek(p.x, p.y) != c) ++bad_corner;
        }
    }

    /* 2. read-modify-write: one pixel changes, its 3 byte-mates don't */
    p.y = 100;
    for (c = 0; c < 4; ++c) {
        for (p.x = 0; p.x < 4; ++p.x) cga_med_plot(p.u, CGA_MED_COL2);
        p.x = c;
        cga_med_plot(p.u, CGA_MED_COL1);
        for (i = 0; i < 4; ++i)
            if (cga_med_peek(i, 100) != (i == c ? 1 : 2)) ++bad_neighbour;
    }

    /* 3. overwrite must clear bits, not OR them */
    p.x = 160;
    p.y = 101;
    cga_med_plot(p.u, CGA_MED_COL3);
    cga_med_plot(p.u, CGA_MED_BKGD);
    if (cga_med_peek(p.x, p.y) != 0) ++bad_overwrite;

    /* 4. exhaustive: every pixel, diagonal stripes, then verify all 64000 */
    for (p.y = 0; p.y < 200; ++p.y)
        for (p.x = 0; p.x < 320; ++p.x)
            cga_med_plot(p.u, colours[(p.x + p.y) & 3]);
    for (p.y = 0; p.y < 200; ++p.y)
        for (p.x = 0; p.x < 320; ++p.x)
            if (cga_med_peek(p.x, p.y) != ((p.x + p.y) & 3)) ++bad_fill;

    wait_key_cga("");   /* visual: even 4-colour diagonal stripes, no gaps or tearing */
    env_set_video_mode(original);

    printf("Point packing: %u errors\n", bad_pack);
    printf("Corners:       %u errors\n", bad_corner);
    printf("Neighbours:    %u errors\n", bad_neighbour);
    printf("Overwrite:     %u errors\n", bad_overwrite);
    printf("Full screen:   %lu errors\n", bad_fill);
    assert(bad_pack == 0);
    assert(bad_corner == 0);
    assert(bad_neighbour == 0);
    assert(bad_overwrite == 0);
    assert(bad_fill == 0);
    printf("cga_med_plot tests passed\n\n");
}

/* plots CGA_DUMP_POINTS pseudo-random pixels, then writes all 16K of CGA VRAM as hex to CGADUMP.TXT */
#define CGA_DUMP_POINTS 256
#define CGA_DUMP_SEED   0x1234

static uint16_t cga_dump_rand(uint16_t* s) {
    *s = (uint16_t)(*s * 25173u + 13849u);
    return *s;
}

void test_cga_med_plot_dump() {
    volatile unsigned char far* vram = (volatile unsigned char far*)0xB8000000L;
    static const cga_med_colour_t colours[4] = {
        CGA_MED_BKGD, CGA_MED_COL1, CGA_MED_COL2, CGA_MED_COL3
    };
    bios_video_mode_t original;
    cga_point_t p;
    uint16_t s = CGA_DUMP_SEED;
    unsigned i, off;
    FILE* f;

    wait_key_cga("Mode 04h next - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_4_COLOUR_320X200);

    for (i = 0; i < CGA_DUMP_POINTS; ++i) {
        p.x = cga_dump_rand(&s) % 320;
        p.y = cga_dump_rand(&s) % 200;
        cga_med_plot(p.u, colours[cga_dump_rand(&s) >> 14]);
    }

    f = fopen("CGADUMP.TXT", "w");
    if (f) {
        fprintf(f, "seed %04X points %u\n", CGA_DUMP_SEED, CGA_DUMP_POINTS);
        for (off = 0; off < 0x4000; ++off) {
            if ((off & 31) == 0) fprintf(f, "%04X:", off);
            fprintf(f, " %02X", vram[off]);
            if ((off & 31) == 31) fprintf(f, "\n");
        }
        fclose(f);
    }

    wait_key_cga("");
    env_set_video_mode(original);
    printf("CGADUMP.TXT %s\n\n", f ? "written" : "FAILED");
}

void test_cga() {
    printf("Test CGALIB...\n");
    //test_cga_versions();
    test_cga_med_plot_dump();
    // test_cga_hi_plot();
    // test_cga_med_cls();
    // test_cga_hi_cls();
}
