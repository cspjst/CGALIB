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
#include "../CGA/MED/cga_med_cls.h"
#include "../CGA/MED/cga_med_plot.h"
#include "../CGA/MED/cga_med_line.h"

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

/* cls and cls_colour: every visible byte checked, each over a 1Bh prefill so a no-op fails */
static void test_cga_med_cls() {
    static const cga_med_colour_t colours[4] = {
        CGA_MED_BKGD, CGA_MED_COL1, CGA_MED_COL2, CGA_MED_COL3
    };
    /* colour repeated across all 4 pixels of a byte: 00 / 01010101 / 10101010 / 11111111 */
    static const unsigned char fills[4] = { 0x00, 0x55, 0xAA, 0xFF };
    bios_video_mode_t original;
    unsigned bad_colour[4], bad_cls, off, i;
    unsigned bad_cls_gaps = 0;

    test_wait_key("Mode 04h cls tests - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_4_COLOUR_320X200);

    for (i = 0; i < 4; ++i) {
        test_prefill(TEST_MED_FILL);
        cga_med_cls_colour(colours[i]);
        bad_colour[i] = test_count_visible_not(fills[i]);
        test_wait_key("");                              /* visual: whole screen in colour i */
    }

    test_prefill(TEST_MED_FILL);
    cga_med_cls();
    bad_cls = test_count_visible_not(0x00);
    for (off = TEST_BANK_BYTES; off < TEST_ODD_BANK; ++off) {     /* cls clears the gaps too */
        if (test_vram[off] != 0x00) ++bad_cls_gaps;
        if (test_vram[TEST_ODD_BANK + off] != 0x00) ++bad_cls_gaps;
    }
    test_wait_key("");                                  /* visual: background */

    env_set_video_mode(original);

    for (i = 0; i < 4; ++i)
        printf("cls_colour(%u): %u errors\n", i, bad_colour[i]);
    printf("cls:           %u errors, gaps %u errors\n", bad_cls, bad_cls_gaps);
    for (i = 0; i < 4; ++i)
        assert(bad_colour[i] == 0);
    assert(bad_cls == 0);
    assert(bad_cls_gaps == 0);
    printf("cga_med_cls: PASS\n\n");
}

#define MED_HLINE_RANDOM        64
#define MED_HLINE_FIXED_BYTES   286     /* bytes the fixed cases change over a 1Bh prefill (host reference) */

/* CGAMHLIN.TXT: fixed edge cases then random lines over a 1Bh prefill */
static void test_cga_med_hline_dump() {
    static const cga_med_colour_t colours[4] = {
        CGA_MED_BKGD, CGA_MED_COL1, CGA_MED_COL2, CGA_MED_COL3
    };
    /*
     * { x, y, len, colour index 0 .. 3 }
     * 1Bh prefill = 00 01 10 11: pixel position p in every byte starts as colour p,
     * so each single pixel case uses a colour different from its position's start colour.
     */
    static const uint16_t cases[][4] = {
        {   0,   0,   1, 1 },   /* single pixel, position 0 */
        {   1,   2,   1, 2 },   /* single pixel, position 1 */
        {   2,   4,   1, 3 },   /* single pixel, position 2 */
        {   3,   6,   1, 0 },   /* single pixel, position 3, background */
        { 319, 199,   1, 1 },   /* single pixel, bottom right */
        {   4,   8,   4, 2 },   /* exactly one byte */
        {   5,  10,   2, 1 },   /* inside one byte, positions 1 - 2 */
        {   6,  12,   4, 3 },   /* straddles a byte boundary, 2 + 2 */
        {   8,  14,  13, 1 },   /* starts on a byte boundary, ends mid-byte */
        {   3,  16,  13, 2 },   /* starts mid-byte, ends on a byte boundary */
        {   1,  18,  50, 3 },   /* both ends mid-byte */
        {   1,  20,  50, 0 },   /* both ends mid-byte, background */
        {   0,  22, 320, 1 },   /* full width, even row */
        {   0,  23, 320, 2 },   /* full width, odd row */
        {   0,  24, 320, 0 },   /* full width, background */
        { 100,  26,   0, 3 },   /* len 0: draws nothing */
        { 316,  28,   4, 1 },   /* last byte exactly */
        { 317,  30,   3, 2 },   /* right edge, mid-byte start */
        {   7,  25,   2, 3 }    /* straddles a byte boundary, 2 pixels, odd row */
    };
    const unsigned n_cases = sizeof(cases) / sizeof(cases[0]);
    bios_video_mode_t original;
    cga_point_t p;
    uint16_t s = TEST_DUMP_SEED;
    uint16_t len;
    unsigned i, off;
    unsigned fixed_changed = 0;
    char header[80];
    int ok;

    test_wait_key("Mode 04h hline dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_4_COLOUR_320X200);

    /* 1. prefill: all 4 colours in every byte, and DS = B800h for the calls below */
    test_prefill(TEST_MED_FILL);

    /* 2. fixed edge cases, one row each */
    for (i = 0; i < n_cases; ++i) {
        p.x = cases[i][0];
        p.y = cases[i][1];
        cga_med_hline(p.u, cases[i][2], colours[cases[i][3]]);
    }

    /* 3. checkpoint: did the fixed cases draw? */
    for (off = 0; off < TEST_VRAM_SIZE; ++off)
        if (test_vram[off] != TEST_MED_FILL) ++fixed_changed;

    /* 4. random lines */
    for (i = 0; i < MED_HLINE_RANDOM; ++i) {
        p.x = (test_rand(&s) >> 4) % 320;
        p.y = (test_rand(&s) >> 4) % 200;
        len = (test_rand(&s) >> 4) % (321 - p.x);     /* 0 .. 320 - x, never past the edge */
        cga_med_hline(p.u, len, colours[test_rand(&s) >> 14]);
    }

    /* 5. dump */
    sprintf(header, "med hline seed %04X cases %u random %u fill %02X gen hi",
            TEST_DUMP_SEED, n_cases, MED_HLINE_RANDOM, TEST_MED_FILL);
    ok = test_dump_vram("CGAMHLIN.TXT", header);

    test_wait_key("");
    env_set_video_mode(original);

    printf("Fixed cases changed %u bytes (expect %u): %s\n",
           fixed_changed, MED_HLINE_FIXED_BYTES,
           fixed_changed == MED_HLINE_FIXED_BYTES ? "ok" : "FAIL");
    printf("CGAMHLIN.TXT %s\n\n", ok ? "written" : "FAILED");
    assert(fixed_changed == MED_HLINE_FIXED_BYTES);
}

#define MED_VLINE_RANDOM        64
#define MED_VLINE_FIXED_BYTES   610     /* bytes the fixed cases change over a 1Bh prefill (host reference) */

/* CGAMVLIN.TXT: fixed edge cases then random lines over a 1Bh prefill */
static void test_cga_med_vline_dump() {
    static const cga_med_colour_t colours[4] = {
        CGA_MED_BKGD, CGA_MED_COL1, CGA_MED_COL2, CGA_MED_COL3
    };
    /*
     * { x, y, len, colour index 0 .. 3 }
     * 1Bh prefill = 00 01 10 11: pixel position p (x mod 4) starts as colour p,
     * so every case draws a colour different from its column's start colour.
     */
    static const uint16_t cases[][4] = {
        {   0,   0,   1, 1 },   /* single pixel, top left */
        { 319, 199,   1, 0 },   /* single pixel, bottom right, background */
        {  20,   3,  20, 1 },   /* position 0, odd start */
        {  25,   4,  21, 2 },   /* position 1, even start */
        {  30,   5,  22, 3 },   /* position 2 */
        {  35,   6,  23, 0 },   /* position 3, background */
        {  40,   7,  24, 2 },   /* position 0 */
        {  45,   8,  25, 3 },   /* position 1 */
        {  50,   9,  26, 0 },   /* position 2, background */
        {  55,  10,  27, 1 },   /* position 3 */
        {  60,   0, 200, 3 },   /* full height */
        {  61,   0, 200, 0 },   /* full height, background */
        { 100,  50,   0, 2 },   /* len 0: draws nothing */
        { 104, 199,   1, 2 },   /* last row only */
        { 108,  99,   2, 1 },   /* odd then even row: crosses banks */
        { 113,  10,  50, 3 },   /* even start, 50 rows */
        { 118, 150,  50, 1 },   /* ends exactly on row 199 */
        { 123,   1,   2, 2 },   /* odd start, len 2 */
        { 124,   2,   3, 3 },   /* even start, len 3 */
        { 316,   7,  13, 2 },   /* last byte of the row, odd start */
        { 319, 100, 100, 1 }    /* right edge, bottom half, overlaps case 2 */
    };
    const unsigned n_cases = sizeof(cases) / sizeof(cases[0]);
    bios_video_mode_t original;
    cga_point_t p;
    uint16_t s = TEST_DUMP_SEED;
    uint16_t len;
    unsigned i, off;
    unsigned fixed_changed = 0;
    char header[80];
    int ok;

    test_wait_key("Mode 04h vline dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_4_COLOUR_320X200);

    /* 1. prefill: all 4 colours in every byte, and DS = B800h for the calls below */
    test_prefill(TEST_MED_FILL);

    /* 2. fixed edge cases, one column each */
    for (i = 0; i < n_cases; ++i) {
        p.x = cases[i][0];
        p.y = cases[i][1];
        cga_med_vline(p.u, cases[i][2], colours[cases[i][3]]);
    }

    /* 3. checkpoint: did the fixed cases draw? */
    for (off = 0; off < TEST_VRAM_SIZE; ++off)
        if (test_vram[off] != TEST_MED_FILL) ++fixed_changed;

    /* 4. random lines */
    for (i = 0; i < MED_VLINE_RANDOM; ++i) {
        p.x = (test_rand(&s) >> 4) % 320;
        p.y = (test_rand(&s) >> 4) % 200;
        len = (test_rand(&s) >> 4) % (201 - p.y);     /* 0 .. 200 - y, never past the bottom */
        cga_med_vline(p.u, len, colours[test_rand(&s) >> 14]);
    }

    /* 5. dump */
    sprintf(header, "med vline seed %04X cases %u random %u fill %02X gen hi",
            TEST_DUMP_SEED, n_cases, MED_VLINE_RANDOM, TEST_MED_FILL);
    ok = test_dump_vram("CGAMVLIN.TXT", header);

    test_wait_key("");
    env_set_video_mode(original);

    printf("Fixed cases changed %u bytes (expect %u): %s\n",
           fixed_changed, MED_VLINE_FIXED_BYTES,
           fixed_changed == MED_VLINE_FIXED_BYTES ? "ok" : "FAIL");
    printf("CGAMVLIN.TXT %s\n\n", ok ? "written" : "FAILED");
    assert(fixed_changed == MED_VLINE_FIXED_BYTES);
}

#define MED_BLINE_RANDOM        64
#define MED_BLINE_FIXED_BYTES   910     /* bytes the fixed cases change over a 1Bh prefill (host reference) */

/* CGAMBLIN.TXT: fixed cases covering every path, then random lines, over a 1Bh prefill */
static void test_cga_med_bresenham_dump() {
    static const cga_med_colour_t colours[4] = {
        CGA_MED_BKGD, CGA_MED_COL1, CGA_MED_COL2, CGA_MED_COL3
    };
    /* { x0, y0, x1, y1, colour index 0 .. 3 } */
    static const uint16_t cases[][5] = {
        {   5,   5,   5,   5, 1 },  /* single point: hline path, len 1 */
        {  10,  20, 100,  20, 2 },  /* horizontal */
        { 100,  23,  10,  23, 0 },  /* horizontal, reversed, background */
        { 200,  10, 200,  60, 3 },  /* vertical */
        { 203,  60, 203,  10, 1 },  /* vertical, reversed */
        { 150,   0, 249,  99, 3 },  /* 45 degrees down-right */
        { 150, 199, 249, 100, 2 },  /* 45 degrees up-right */
        {   0, 100, 319, 150, 1 },  /* x-major, y++, full width */
        {   0, 199, 319, 160, 2 },  /* x-major, y--, full width */
        { 260,   0, 280, 199, 3 },  /* y-major, x++, full height */
        { 300,   0, 290, 199, 0 },  /* y-major, x--, full height, background */
        {  20, 120,  30, 131, 2 },  /* just steeper than 45 */
        {  40, 120,  51, 130, 1 },  /* just shallower than 45 */
        {  60,  30,  90,  90, 3 },  /* reverse twins: a second pass must add nothing */
        {  90,  90,  60,  30, 3 },
        { 130, 180, 100, 120, 2 },
        { 100, 120, 130, 180, 2 },
        { 310,   5, 315,  40, 1 },  /* right edge, steep */
        { 315,  40, 310,   5, 1 }
    };
    const unsigned n_cases = sizeof(cases) / sizeof(cases[0]);
    bios_video_mode_t original;
    cga_point_t p1, p2;
    uint16_t s = TEST_DUMP_SEED;
    unsigned i, off;
    unsigned fixed_changed = 0;
    char header[80];
    int ok;

    test_wait_key("Mode 04h Bresenham dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_4_COLOUR_320X200);

    /* 1. prefill: all 4 colours in every byte, and DS = B800h for the calls below */
    test_prefill(TEST_MED_FILL);

    /* 2. fixed cases: every path, all colours, both directions */
    for (i = 0; i < n_cases; ++i) {
        p1.x = cases[i][0];
        p1.y = cases[i][1];
        p2.x = cases[i][2];
        p2.y = cases[i][3];
        cga_med_bresenham_line(p1.u, p2.u, colours[cases[i][4]]);
    }

    /* 3. checkpoint: did the fixed cases draw? */
    for (off = 0; off < TEST_VRAM_SIZE; ++off)
        if (test_vram[off] != TEST_MED_FILL) ++fixed_changed;

    /* 4. random lines */
    for (i = 0; i < MED_BLINE_RANDOM; ++i) {
        p1.x = (test_rand(&s) >> 4) % 320;
        p1.y = (test_rand(&s) >> 4) % 200;
        p2.x = (test_rand(&s) >> 4) % 320;
        p2.y = (test_rand(&s) >> 4) % 200;
        cga_med_bresenham_line(p1.u, p2.u, colours[test_rand(&s) >> 14]);
    }

    /* 5. dump */
    sprintf(header, "med bline seed %04X cases %u random %u fill %02X gen hi",
            TEST_DUMP_SEED, n_cases, MED_BLINE_RANDOM, TEST_MED_FILL);
    ok = test_dump_vram("CGAMBLIN.TXT", header);

    test_wait_key("");
    env_set_video_mode(original);

    printf("Fixed cases changed %u bytes (expect %u): %s\n",
           fixed_changed, MED_BLINE_FIXED_BYTES,
           fixed_changed == MED_BLINE_FIXED_BYTES ? "ok" : "FAIL");
    printf("CGAMBLIN.TXT %s\n\n", ok ? "written" : "FAILED");
    assert(fixed_changed == MED_BLINE_FIXED_BYTES);
}

void test_cga_med() {
    //test_cga_med_plot();
    //test_cga_med_plot_dump();
    //test_cga_med_cls();
    //test_cga_med_hline_dump();
    //test_cga_med_vline_dump();
    test_cga_med_bresenham_dump();
}
