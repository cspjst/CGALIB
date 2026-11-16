/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#include "test_cga_med.h"
#include "test_cga.h"

#include <stdio.h>
#include <assert.h>

#include "../include/ENV/env_video_mode.h"

#include "../CGA/cga_types.h"
#include "../CGA/MED/cga_med_plot.h"

/* independent mode 04h pixel read: no shared code with the library */
static unsigned test_med_peek(unsigned x, unsigned y) {
    unsigned shift = 6 - ((x & 3) << 1);
    return (test_vram[test_offset(y, x >> 2)] >> shift) & 3;
}

/* in-program checks via an independent peek; failed before the DS fix */
static void test_cga_med_plot() {
    static const cga_med_colour_t colours[4] = {
        CGA_MED_BKGD, CGA_MED_COL1, CGA_MED_COL2, CGA_MED_COL3
    };
    static const uint16_t corners[4][2] = { {0,0}, {319,0}, {0,199}, {319,199} };
    bios_video_mode_t original;
    cga_point_t p;
    unsigned c, i;
    unsigned long bad_fill = 0;
    unsigned bad_pack = 0, bad_corner = 0, bad_neighbour = 0, bad_overwrite = 0;

    /* 0. the union packs x into the low word, y into the high word */
    p.x = 0x1234;
    p.y = 0x5678;
    if (p.u != 0x56781234UL) ++bad_pack;

    test_wait_key("Mode 04h plot checks - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_4_COLOUR_320X200);
    test_prefill(0);                                    /* hostile DS, clean screen */

    /* 1. corners, every colour: catches x/y swaps and interlace errors */
    for (i = 0; i < 4; ++i) {
        p.x = corners[i][0];
        p.y = corners[i][1];
        for (c = 0; c < 4; ++c) {
            cga_med_plot(p.u, colours[c]);
            if (test_med_peek(p.x, p.y) != c) ++bad_corner;
        }
    }

    /* 2. read-modify-write: one pixel changes, its 3 byte-mates don't */
    p.y = 100;
    for (c = 0; c < 4; ++c) {
        for (p.x = 0; p.x < 4; ++p.x) cga_med_plot(p.u, CGA_MED_COL2);
        p.x = c;
        cga_med_plot(p.u, CGA_MED_COL1);
        for (i = 0; i < 4; ++i)
            if (test_med_peek(i, 100) != (i == c ? 1u : 2u)) ++bad_neighbour;
    }

    /* 3. overwrite must clear bits, not OR them */
    p.x = 160;
    p.y = 101;
    cga_med_plot(p.u, CGA_MED_COL3);
    cga_med_plot(p.u, CGA_MED_BKGD);
    if (test_med_peek(p.x, p.y) != 0) ++bad_overwrite;

    /* 4. exhaustive: every pixel, diagonal stripes, then verify all 64000 */
    for (p.y = 0; p.y < 200; ++p.y)
        for (p.x = 0; p.x < 320; ++p.x)
            cga_med_plot(p.u, colours[(p.x + p.y) & 3]);
    for (p.y = 0; p.y < 200; ++p.y)
        for (p.x = 0; p.x < 320; ++p.x)
            if (test_med_peek(p.x, p.y) != ((p.x + p.y) & 3)) ++bad_fill;

    test_wait_key("");  /* visual: even 4-colour diagonal stripes, no gaps or tearing */
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
    printf("cga_med_plot checks: PASS\n\n");
}

/* CGAMDUMP.TXT: TEST_DUMP_POINTS random pixels over a 55h prefill */
static void test_cga_med_plot_dump() {
    static const cga_med_colour_t colours[4] = {
        CGA_MED_BKGD, CGA_MED_COL1, CGA_MED_COL2, CGA_MED_COL3
    };
    bios_video_mode_t original;
    cga_point_t p;
    uint16_t s = TEST_DUMP_SEED;
    unsigned i;
    char header[80];
    int ok;

    test_wait_key("Mode 04h plot dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_4_COLOUR_320X200);
    test_prefill(TEST_DUMP_FILL);

    for (i = 0; i < TEST_DUMP_POINTS; ++i) {
        p.x = (test_rand(&s) >> 4) % 320;
        p.y = (test_rand(&s) >> 4) % 200;
        cga_med_plot(p.u, colours[test_rand(&s) >> 14]);
    }

    sprintf(header, "med plot seed %04X points %u fill %02X gen hi",
            TEST_DUMP_SEED, TEST_DUMP_POINTS, TEST_DUMP_FILL);
    ok = test_dump_vram("CGAMDUMP.TXT", header);

    test_wait_key("");
    env_set_video_mode(original);
    printf("CGAMDUMP.TXT %s\n\n", ok ? "written" : "FAILED");
}

/* ---------------------------------------------------------------- runner */

void test_cga_med() {
    //test_cga_med_plot();
    //test_cga_med_plot_dump();
    //test_cga_med_cls();             to come: all 4 colours, 1Bh prefill
    //test_cga_med_hline_dump();      to come: 1Bh prefill, all 4 colours, every pixel position in a byte
    //test_cga_med_vline_dump();      to come
    //test_cga_med_bresenham_dump();  to come
}
