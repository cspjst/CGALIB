/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#include "test_cga_hi.h"
#include "test_cga.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "../include/ENV/env_video_mode.h"

#include "../CGA/cga_types.h"

#include "../CGA/HI/cga_hi_plot.h"
#include "../CGA/HI/cga_hi_cls.h"
#include "../CGA/HI/cga_hi_line.h"
#include "../CGA/HI/cga_hi_bmp.h"

#define HLINE_RANDOM        64
#define HLINE_FIXED_BYTES   267     /* bytes the fixed cases change over a 55h prefill (host reference) */

/* CGAHDUMP.TXT: TEST_DUMP_POINTS random pixels over a 55h prefill */
static void test_cga_hi_plot_dump() {
    static const cga_hi_colour_t colours[2] = { CGA_HI_BLACK, CGA_HI_COLOUR };
    bios_video_mode_t original;
    cga_point_t p;
    uint16_t s = TEST_DUMP_SEED;
    unsigned i;
    char header[80];
    int ok;

    test_wait_key("Mode 06h plot dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);
    test_prefill(TEST_DUMP_FILL);

    for (i = 0; i < TEST_DUMP_POINTS; ++i) {
        p.x = (test_rand(&s) >> 4) % 640;
        p.y = (test_rand(&s) >> 4) % 200;
        cga_hi_plot(p.u, colours[test_rand(&s) >> 15]);
    }

    sprintf(header, "hi plot seed %04X points %u fill %02X gen hi",
            TEST_DUMP_SEED, TEST_DUMP_POINTS, TEST_DUMP_FILL);
    ok = test_dump_vram("CGAHDUMP.TXT", header);

    test_wait_key("");
    env_set_video_mode(original);
    printf("CGAHDUMP.TXT %s\n\n", ok ? "written" : "FAILED");
}

/* cls and cls_colour: every visible byte checked, each over a 55h prefill so a no-op fails */
static void test_cga_hi_cls() {
    bios_video_mode_t original;
    unsigned bad_colour, bad_black, bad_cls, off;
    unsigned bad_cls_gaps = 0;

    test_wait_key("Mode 06h cls tests - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    test_prefill(TEST_DUMP_FILL);
    cga_hi_cls_colour(CGA_HI_COLOUR);
    bad_colour = test_count_visible_not(0xFF);
    test_wait_key("");                                  /* visual: whole screen foreground colour */

    test_prefill(TEST_DUMP_FILL);
    cga_hi_cls_colour(CGA_HI_BLACK);
    bad_black = test_count_visible_not(0x00);

    test_prefill(TEST_DUMP_FILL);
    cga_hi_cls();
    bad_cls = test_count_visible_not(0x00);
    for (off = TEST_BANK_BYTES; off < TEST_ODD_BANK; ++off) {     /* cls clears the gaps too */
        if (test_vram[off] != 0x00) ++bad_cls_gaps;
        if (test_vram[TEST_ODD_BANK + off] != 0x00) ++bad_cls_gaps;
    }
    test_wait_key("");                                  /* visual: black */

    env_set_video_mode(original);

    printf("cls_colour(COLOUR): %u errors\n", bad_colour);
    printf("cls_colour(BLACK):  %u errors\n", bad_black);
    printf("cls:                %u errors, gaps %u errors\n", bad_cls, bad_cls_gaps);
    assert(bad_colour == 0);
    assert(bad_black == 0);
    assert(bad_cls == 0);
    assert(bad_cls_gaps == 0);
    printf("cga_hi_cls: PASS\n\n");
}

/* CGAHLINE.TXT: fixed edge cases then random lines over a 55h prefill */
static void test_cga_hi_hline_dump() {
    static const cga_hi_colour_t colours[2] = { CGA_HI_BLACK, CGA_HI_COLOUR };
    /* { x, y, len, colour index 0 = black, 1 = colour } */
    static const uint16_t cases[][4] = {
        {   0,   0,   1, 1 },   /* single pixel, top left */
        { 639, 199,   1, 1 },   /* single pixel, bottom right */
        {  10,   2,   3, 1 },   /* inside one byte */
        {  10,   4,   3, 0 },   /* inside one byte, black */
        {  16,   6,   8, 1 },   /* exactly one byte */
        {  16,   8,  20, 1 },   /* starts on a byte boundary, ends mid-byte */
        {   5,  10,  11, 1 },   /* starts mid-byte, ends on a byte boundary */
        {   3,  12,  50, 1 },   /* both ends mid-byte */
        {   3,  14,  50, 0 },   /* both ends mid-byte, black */
        {   0,  16, 640, 1 },   /* full width, even row */
        {   0,  17, 640, 1 },   /* full width, odd row */
        {   0,  18, 640, 0 },   /* full width, black */
        { 100,  20,   0, 1 },   /* len 0: draws nothing */
        { 632,  22,   8, 1 },   /* last byte exactly */
        { 633,  24,   7, 0 },   /* right edge, mid-byte start, black */
        {   7,  21,   2, 1 },   /* straddles a byte boundary, 2 pixels, odd row */
        {   8,  23,   1, 1 },   /* leftmost pixel of a byte */
        {  15,  25,   1, 1 }    /* rightmost pixel of a byte */
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

    test_wait_key("Mode 06h hline dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    /* 1. prefill: every line of either colour now sets or clears something */
    test_prefill(TEST_DUMP_FILL);

    /* 2. fixed edge cases, one row each */
    for (i = 0; i < n_cases; ++i) {
        p.x = cases[i][0];
        p.y = cases[i][1];
        cga_hi_hline(p.u, cases[i][2], colours[cases[i][3]]);
    }

    /* 3. checkpoint: did the fixed cases draw? */
    for (off = 0; off < TEST_VRAM_SIZE; ++off)
        if (test_vram[off] != TEST_DUMP_FILL) ++fixed_changed;

    /* 4. random lines */
    for (i = 0; i < HLINE_RANDOM; ++i) {
        p.x = (test_rand(&s) >> 4) % 640;
        p.y = (test_rand(&s) >> 4) % 200;
        len = (test_rand(&s) >> 4) % (641 - p.x);     /* 0 .. 640 - x, never past the edge */
        cga_hi_hline(p.u, len, colours[test_rand(&s) >> 15]);
    }

    /* 5. dump */
    sprintf(header, "hline seed %04X cases %u random %u fill %02X gen hi",
            TEST_DUMP_SEED, n_cases, HLINE_RANDOM, TEST_DUMP_FILL);
    ok = test_dump_vram("CGAHLINE.TXT", header);

    test_wait_key("");
    env_set_video_mode(original);

    printf("Fixed cases changed %u bytes (expect %u): %s\n",
           fixed_changed, HLINE_FIXED_BYTES,
           fixed_changed == HLINE_FIXED_BYTES ? "ok" : "FAIL");
    printf("CGAHLINE.TXT %s\n\n", ok ? "written" : "FAILED");
    assert(fixed_changed == HLINE_FIXED_BYTES);
}

#define VLINE_RANDOM        64
#define VLINE_FIXED_BYTES   610     /* bytes the fixed cases change over a 55h prefill (host reference) */

/* CGAVLINE.TXT: fixed edge cases then random lines over a 55h prefill */
static void test_cga_hi_vline_dump() {
    static const cga_hi_colour_t colours[2] = { CGA_HI_BLACK, CGA_HI_COLOUR };
    /*
     * { x, y, len, colour index 0 = black, 1 = colour }
     * 55h prefill = 01010101: even pixel columns start black, odd start coloured,
     * so colour goes on even columns and black on odd - every case changes pixels.
     */
    static const uint16_t cases[][4] = {
        {   0,   0,   1, 1 },   /* single pixel, top left */
        { 639, 199,   1, 0 },   /* single pixel, bottom right */
        {  40,   3,  20, 1 },   /* bit 0 of a byte, odd start  */
        {  49,   4,  21, 0 },   /* bit 1, even start           */
        {  58,   5,  22, 1 },   /* bit 2                       */
        {  67,   6,  23, 0 },   /* bit 3                       */
        {  76,   7,  24, 1 },   /* bit 4                       */
        {  85,   8,  25, 0 },   /* bit 5                       */
        {  94,   9,  26, 1 },   /* bit 6                       */
        { 103,  10,  27, 0 },   /* bit 7                       */
        { 120,   0, 200, 1 },   /* full height                 */
        { 121,   0, 200, 0 },   /* full height, black          */
        { 200,  50,   0, 1 },   /* len 0: draws nothing        */
        { 210, 199,   1, 1 },   /* last row only               */
        { 220,  99,   2, 1 },   /* odd then even row: crosses banks */
        { 231,  10,  50, 0 },   /* black run, even start       */
        { 240, 150,  50, 1 },   /* ends exactly on row 199     */
        { 250,   1,   2, 1 },   /* odd start, len 2            */
        { 261,   2,   3, 0 },   /* even start, len 3, black    */
        { 632,   7,  13, 1 },   /* last byte of the row, odd start */
        { 639, 100, 100, 0 }    /* right edge, bottom half, overlaps case 2 */
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

    test_wait_key("Mode 06h vline dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    /* 1. prefill: also leaves DS = B800h for the calls below */
    test_prefill(TEST_DUMP_FILL);

    /* 2. fixed edge cases, one column each */
    for (i = 0; i < n_cases; ++i) {
        p.x = cases[i][0];
        p.y = cases[i][1];
        cga_hi_vline(p.u, cases[i][2], colours[cases[i][3]]);
    }

    /* 3. checkpoint: did the fixed cases draw? */
    for (off = 0; off < TEST_VRAM_SIZE; ++off)
        if (test_vram[off] != TEST_DUMP_FILL) ++fixed_changed;

    /* 4. random lines */
    for (i = 0; i < VLINE_RANDOM; ++i) {
        p.x = (test_rand(&s) >> 4) % 640;
        p.y = (test_rand(&s) >> 4) % 200;
        len = (test_rand(&s) >> 4) % (201 - p.y);     /* 0 .. 200 - y, never past the bottom */
        cga_hi_vline(p.u, len, colours[test_rand(&s) >> 15]);
    }

    /* 5. dump */
    sprintf(header, "vline seed %04X cases %u random %u fill %02X gen hi",
            TEST_DUMP_SEED, n_cases, VLINE_RANDOM, TEST_DUMP_FILL);
    ok = test_dump_vram("CGAVLINE.TXT", header);

    test_wait_key("");
    env_set_video_mode(original);

    printf("Fixed cases changed %u bytes (expect %u): %s\n",
           fixed_changed, VLINE_FIXED_BYTES,
           fixed_changed == VLINE_FIXED_BYTES ? "ok" : "FAIL");
    printf("CGAVLINE.TXT %s\n\n", ok ? "written" : "FAILED");
    assert(fixed_changed == VLINE_FIXED_BYTES);
}

#define BLINE_RANDOM        64
#define BLINE_FIXED_BYTES   689     /* bytes the fixed cases change over a 55h prefill (host reference) */

/* CGABLINE.TXT: fixed cases covering every path, then random lines, over a 55h prefill */
static void test_cga_hi_bresenham_dump() {
    static const cga_hi_colour_t colours[2] = { CGA_HI_BLACK, CGA_HI_COLOUR };
    /* { x0, y0, x1, y1, colour index 0 = black, 1 = colour } */
    static const uint16_t cases[][5] = {
        {   5,   5,   5,   5, 1 },  /* single point: hline path, len 1 */
        {  10,  20, 100,  20, 1 },  /* horizontal */
        { 100,  23,  10,  23, 0 },  /* horizontal, reversed, black */
        { 200,  10, 200,  60, 1 },  /* vertical */
        { 203,  60, 203,  10, 0 },  /* vertical, reversed, black */
        { 300,   0, 399,  99, 1 },  /* 45 degrees down-right */
        { 300, 199, 399, 100, 0 },  /* 45 degrees up-right, black */
        {   0, 100, 639, 150, 1 },  /* x-major, y++, full width */
        {   0, 199, 639, 160, 0 },  /* x-major, y--, full width, black */
        { 450,   0, 500, 199, 1 },  /* y-major, x++, full height */
        { 600,   0, 550, 199, 0 },  /* y-major, x--, full height, black */
        {  20, 120,  30, 131, 1 },  /* just steeper than 45 */
        {  40, 120,  51, 130, 0 },  /* just shallower than 45, black */
        { 120,  30, 180,  90, 1 },  /* reverse twins: a second pass must add nothing */
        { 180,  90, 120,  30, 1 },
        { 250, 180, 200, 120, 1 },
        { 200, 120, 250, 180, 1 },
        { 620,   5, 630,  40, 1 },  /* right edge, steep */
        { 630,  40, 620,   5, 1 }
    };
    const unsigned n_cases = sizeof(cases) / sizeof(cases[0]);
    bios_video_mode_t original;
    cga_point_t p1, p2;
    uint16_t s = TEST_DUMP_SEED;
    unsigned i, off;
    unsigned fixed_changed = 0;
    char header[80];
    int ok;

    test_wait_key("Mode 06h Bresenham dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    /* 1. prefill: also leaves DS = B800h for the calls below */
    test_prefill(TEST_DUMP_FILL);

    /* 2. fixed cases: every path, both colours, both directions */
    for (i = 0; i < n_cases; ++i) {
        p1.x = cases[i][0];
        p1.y = cases[i][1];
        p2.x = cases[i][2];
        p2.y = cases[i][3];
        cga_hi_bresenham_line(p1.u, p2.u, colours[cases[i][4]]);
    }

    /* 3. checkpoint: did the fixed cases draw? */
    for (off = 0; off < TEST_VRAM_SIZE; ++off)
        if (test_vram[off] != TEST_DUMP_FILL) ++fixed_changed;

    /* 4. random lines */
    for (i = 0; i < BLINE_RANDOM; ++i) {
        p1.x = (test_rand(&s) >> 4) % 640;
        p1.y = (test_rand(&s) >> 4) % 200;
        p2.x = (test_rand(&s) >> 4) % 640;
        p2.y = (test_rand(&s) >> 4) % 200;
        cga_hi_bresenham_line(p1.u, p2.u, colours[test_rand(&s) >> 15]);
    }

    /* 5. dump */
    sprintf(header, "bline seed %04X cases %u random %u fill %02X gen hi",
            TEST_DUMP_SEED, n_cases, BLINE_RANDOM, TEST_DUMP_FILL);
    ok = test_dump_vram("CGABLINE.TXT", header);

    test_wait_key("");
    env_set_video_mode(original);

    printf("Fixed cases changed %u bytes (expect %u): %s\n",
           fixed_changed, BLINE_FIXED_BYTES,
           fixed_changed == BLINE_FIXED_BYTES ? "ok" : "FAIL");
    printf("CGABLINE.TXT %s\n\n", ok ? "written" : "FAILED");
    assert(fixed_changed == BLINE_FIXED_BYTES);
}

void test_bmp() {
    cga_hi_bmp_t* bmp = malloc(sizeof(*bmp));
    cga_hi_bmp_init(bmp, 32, 8, 256, 1);
    cga_hi_bmp_top(bmp);
    printf("%li bytes out\n", cga_hi_bmp_save("test1.cga", bmp));
    printf("%li bytes in\n", cga_hi_bmp_load("test2.cga", bmp));
}

void test_cga_hi() {
    //test_cga_hi_plot_dump();
    //test_cga_hi_cls();
    //test_cga_hi_hline_dump();
    //test_cga_hi_vline_dump();
    //test_cga_hi_bresenham_dump();
    test_bmp();
}
