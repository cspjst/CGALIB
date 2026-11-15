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

#include "../CGA/HI/cga_hi_plot.h"
#include "../CGA/HI/cga_hi_vram.h"
#include "../CGA/HI/cga_hi_line.h"

/*
 * Dump-and-diff tests: draw, then write all 16K of VRAM as hex to a file that is
 * diffed on the host against an independently generated reference image.
 *
 * Every test first writes VRAM through a far pointer. Watcom loads DS = B800h to do
 * that and does not restore it, so every library call below runs with a hostile DS.
 */
#define CGA_DUMP_POINTS 256
#define CGA_DUMP_SEED   0x1234
#define CGA_DUMP_FILL   0x55        /* 01010101: every plot of any colour sets or clears something */
#define CGA_VRAM_SIZE   0x4000

#define HLINE_RANDOM        64
#define HLINE_FIXED_BYTES   267     /* bytes the fixed cases change over a 55h prefill (host reference) */

static volatile unsigned char far* const vram = (volatile unsigned char far*)0xB8000000L;

/* ---------------------------------------------------------------- helpers */

static void wait_key_cga(const char* msg) {
    bios_key_t key;
    printf("%s", msg);
    bios_wait_for_keystroke_and_read(&key);
}

/* 16-bit LCG: use the high bits, the low bits only alternate */
static uint16_t cga_dump_rand(uint16_t* s) {
    *s = (uint16_t)(*s * 25173u + 13849u);
    return *s;
}

static void cga_fill_vram_bytes(unsigned char b) {
    unsigned off;
    for (off = 0; off < CGA_VRAM_SIZE; ++off) vram[off] = b;
}

/* writes header then 16K of VRAM as 512 lines of 32 hex bytes, returns 1 on success */
static int cga_dump_vram(const char* filename, const char* header) {
    FILE* f;
    unsigned off;

    f = fopen(filename, "w");
    if (!f) return 0;
    fprintf(f, "%s\n", header);
    for (off = 0; off < CGA_VRAM_SIZE; ++off) {
        if ((off & 31) == 0) fprintf(f, "%04X:", off);
        fprintf(f, " %02X", vram[off]);
        if ((off & 31) == 31) fprintf(f, "\n");
    }
    fclose(f);
    return 1;
}

/* independent mode 04h pixel read: no shared code with the library */
static unsigned cga_med_peek(unsigned x, unsigned y) {
    unsigned offset = ((y & 1) ? 0x2000 : 0) + (y >> 1) * 80 + (x >> 2);
    unsigned shift  = 6 - ((x & 3) << 1);
    return (vram[offset] >> shift) & 3;
}

/* ---------------------------------------------------------------- versions */

/* vendored headers and .LIBs must come from the same build */
static void test_cga_versions() {
    printf("BIOSLIB headers %s, BIOS.LIB %s\n", BIOS_VERSION_STRING, bios_version_string());
    printf("ENVLIB  headers %s, ENV.LIB  %s\n", ENV_VERSION_STRING,  env_version_string());
    assert(bios_version() == BIOS_VERSION);
    assert(env_version()  == ENV_VERSION);
    printf("Vendored libraries match their headers: PASS\n\n");
}

/* ---------------------------------------------------------------- medium res */

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

    wait_key_cga("Mode 04h plot checks - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_4_COLOUR_320X200);
    cga_fill_vram_bytes(0);                             /* hostile DS, clean screen */

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
            if (cga_med_peek(i, 100) != (i == c ? 1u : 2u)) ++bad_neighbour;
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
    printf("cga_med_plot checks: PASS\n\n");
}

/* CGAMDUMP.TXT: CGA_DUMP_POINTS random pixels over a 55h prefill */
static void test_cga_med_plot_dump() {
    static const cga_med_colour_t colours[4] = {
        CGA_MED_BKGD, CGA_MED_COL1, CGA_MED_COL2, CGA_MED_COL3
    };
    bios_video_mode_t original;
    cga_point_t p;
    uint16_t s = CGA_DUMP_SEED;
    unsigned i;
    char header[80];
    int ok;

    wait_key_cga("Mode 04h plot dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_4_COLOUR_320X200);
    cga_fill_vram_bytes(CGA_DUMP_FILL);

    for (i = 0; i < CGA_DUMP_POINTS; ++i) {
        p.x = (cga_dump_rand(&s) >> 4) % 320;
        p.y = (cga_dump_rand(&s) >> 4) % 200;
        cga_med_plot(p.u, colours[cga_dump_rand(&s) >> 14]);
    }

    sprintf(header, "med plot seed %04X points %u fill %02X gen hi",
            CGA_DUMP_SEED, CGA_DUMP_POINTS, CGA_DUMP_FILL);
    ok = cga_dump_vram("CGAMDUMP.TXT", header);

    wait_key_cga("");
    env_set_video_mode(original);
    printf("CGAMDUMP.TXT %s\n\n", ok ? "written" : "FAILED");
}

/* ---------------------------------------------------------------- high res */

/* CGAHDUMP.TXT: CGA_DUMP_POINTS random pixels over a 55h prefill */
static void test_cga_hi_plot_dump() {
    static const cga_hi_colour_t colours[2] = { CGA_HI_BLACK, CGA_HI_COLOUR };
    bios_video_mode_t original;
    cga_point_t p;
    uint16_t s = CGA_DUMP_SEED;
    unsigned i;
    char header[80];
    int ok;

    wait_key_cga("Mode 06h plot dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);
    cga_fill_vram_bytes(CGA_DUMP_FILL);

    for (i = 0; i < CGA_DUMP_POINTS; ++i) {
        p.x = (cga_dump_rand(&s) >> 4) % 640;
        p.y = (cga_dump_rand(&s) >> 4) % 200;
        cga_hi_plot(p.u, colours[cga_dump_rand(&s) >> 15]);
    }

    sprintf(header, "hi plot seed %04X points %u fill %02X gen hi",
            CGA_DUMP_SEED, CGA_DUMP_POINTS, CGA_DUMP_FILL);
    ok = cga_dump_vram("CGAHDUMP.TXT", header);

    wait_key_cga("");
    env_set_video_mode(original);
    printf("CGAHDUMP.TXT %s\n\n", ok ? "written" : "FAILED");
}

static void test_cga_hi_vram() {
    bios_video_mode_t original;
    unsigned off;
    unsigned bad_even = 0, bad_odd = 0;

    wait_key_cga("Mode 06h VRAM tests - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    cga_hi_cls_colour(CGA_HI_COLOUR);
    wait_key_cga("");
    cga_hi_cls_colour(CGA_HI_BLACK);
    wait_key_cga("");
    cga_hi_cls_colour(CGA_HI_COLOUR);
    wait_key_cga("");
    cga_hi_cls();
    wait_key_cga("");
    cga_hi_fill_vram(0x5555, 0xAAAA);
    wait_key_cga("");

    /* byte order: little-endian, so low byte at the lower address */
    cga_hi_fill_vram(0x1234, 0x5678);
    for (off = 0; off < 0x1F40; off += 2) {
        if (vram[off] != 0x34 || vram[off + 1] != 0x12) ++bad_even;
        if (vram[0x2000 + off] != 0x78 || vram[0x2000 + off + 1] != 0x56) ++bad_odd;
    }
    wait_key_cga("");   /* visual: fine texture, different on alternate rows */

    env_set_video_mode(original);

    printf("fill_vram byte order: even bank %u errors, odd bank %u errors\n", bad_even, bad_odd);
    assert(bad_even == 0);
    assert(bad_odd == 0);
    printf("cga_hi_fill_vram byte order: PASS\n\n");
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
    uint16_t s = CGA_DUMP_SEED;
    uint16_t len;
    unsigned i, off;
    unsigned fixed_changed = 0;
    char header[80];
    int ok;

    wait_key_cga("Mode 06h hline dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    /* 1. prefill: every line of either colour now sets or clears something */
    cga_fill_vram_bytes(CGA_DUMP_FILL);

    /* 2. fixed edge cases, one row each */
    for (i = 0; i < n_cases; ++i) {
        p.x = cases[i][0];
        p.y = cases[i][1];
        cga_hi_hline(p.u, cases[i][2], colours[cases[i][3]]);
    }

    /* 3. checkpoint: did the fixed cases draw? */
    for (off = 0; off < CGA_VRAM_SIZE; ++off)
        if (vram[off] != CGA_DUMP_FILL) ++fixed_changed;

    /* 4. random lines */
    for (i = 0; i < HLINE_RANDOM; ++i) {
        p.x = (cga_dump_rand(&s) >> 4) % 640;
        p.y = (cga_dump_rand(&s) >> 4) % 200;
        len = (cga_dump_rand(&s) >> 4) % (641 - p.x);     /* 0 .. 640 - x, never past the edge */
        cga_hi_hline(p.u, len, colours[cga_dump_rand(&s) >> 15]);
    }

    /* 5. dump */
    sprintf(header, "hline seed %04X cases %u random %u fill %02X gen hi",
            CGA_DUMP_SEED, n_cases, HLINE_RANDOM, CGA_DUMP_FILL);
    ok = cga_dump_vram("CGAHLINE.TXT", header);

    wait_key_cga("");
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
    uint16_t s = CGA_DUMP_SEED;
    uint16_t len;
    unsigned i, off;
    unsigned fixed_changed = 0;
    char header[80];
    int ok;

    wait_key_cga("Mode 06h vline dump - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    /* 1. prefill: also leaves DS = B800h for the calls below */
    cga_fill_vram_bytes(CGA_DUMP_FILL);

    /* 2. fixed edge cases, one column each */
    for (i = 0; i < n_cases; ++i) {
        p.x = cases[i][0];
        p.y = cases[i][1];
        cga_hi_vline(p.u, cases[i][2], colours[cases[i][3]]);
    }

    /* 3. checkpoint: did the fixed cases draw? */
    for (off = 0; off < CGA_VRAM_SIZE; ++off)
        if (vram[off] != CGA_DUMP_FILL) ++fixed_changed;

    /* 4. random lines */
    for (i = 0; i < VLINE_RANDOM; ++i) {
        p.x = (cga_dump_rand(&s) >> 4) % 640;
        p.y = (cga_dump_rand(&s) >> 4) % 200;
        len = (cga_dump_rand(&s) >> 4) % (201 - p.y);     /* 0 .. 200 - y, never past the bottom */
        cga_hi_vline(p.u, len, colours[cga_dump_rand(&s) >> 15]);
    }

    /* 5. dump */
    sprintf(header, "vline seed %04X cases %u random %u fill %02X gen hi",
            CGA_DUMP_SEED, n_cases, VLINE_RANDOM, CGA_DUMP_FILL);
    ok = cga_dump_vram("CGAVLINE.TXT", header);

    wait_key_cga("");
    env_set_video_mode(original);

    printf("Fixed cases changed %u bytes (expect %u): %s\n",
           fixed_changed, VLINE_FIXED_BYTES,
           fixed_changed == VLINE_FIXED_BYTES ? "ok" : "FAIL");
    printf("CGAVLINE.TXT %s\n\n", ok ? "written" : "FAILED");
    assert(fixed_changed == VLINE_FIXED_BYTES);
}

void test_cga() {
    printf("Test CGALIB...\n");
    //test_cga_versions();
    //test_cga_med_plot();
    //test_cga_med_plot_dump();
    //test_cga_hi_plot_dump();
    //test_cga_hi_vram();
    //test_cga_hi_hline_dump();
    test_cga_hi_vline_dump();
}
