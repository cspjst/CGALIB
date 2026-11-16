/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#include "test_cga.h"
#include "test_cga_med.h"
#include "test_cga_hi.h"

#include <stdio.h>
#include <assert.h>

#include "../include/BIOS/bios_version.h"
#include "../include/BIOS/bios_keyboard_services.h"

#include "../include/ENV/env_version.h"
#include "../include/ENV/env_video_mode.h"

#include "../CGA/cga_vram.h"

volatile unsigned char far* const test_vram = (volatile unsigned char far*)0xB8000000L;

/* ---------------------------------------------------------------- helpers */

void test_wait_key(const char* msg) {
    bios_key_t key;
    printf("%s", msg);
    bios_wait_for_keystroke_and_read(&key);
}

/* 16-bit LCG: use the high bits, the low bits only alternate */
uint16_t test_rand(uint16_t* s) {
    *s = (uint16_t)(*s * 25173u + 13849u);
    return *s;
}

/* independent of the library, and leaves DS = B800h */
void test_prefill(unsigned char b) {
    unsigned off;
    for (off = 0; off < TEST_VRAM_SIZE; ++off) test_vram[off] = b;
}

/* independent row offset: no shared code with the library */
unsigned test_offset(unsigned row, unsigned col) {
    return ((row & 1) ? TEST_ODD_BANK : 0) + (row >> 1) * 80 + col;
}

/* counts visible bytes (both banks, gaps excluded) that are not b */
unsigned test_count_visible_not(unsigned char b) {
    unsigned off, bad = 0;
    for (off = 0; off < TEST_BANK_BYTES; ++off) {
        if (test_vram[off] != b) ++bad;
        if (test_vram[TEST_ODD_BANK + off] != b) ++bad;
    }
    return bad;
}

/* writes header then 16K of VRAM as 512 lines of 32 hex bytes, returns 1 on success */
int test_dump_vram(const char* filename, const char* header) {
    FILE* f;
    unsigned off;

    f = fopen(filename, "w");
    if (!f) return 0;
    fprintf(f, "%s\n", header);
    for (off = 0; off < TEST_VRAM_SIZE; ++off) {
        if ((off & 31) == 0) fprintf(f, "%04X:", off);
        fprintf(f, " %02X", test_vram[off]);
        if ((off & 31) == 31) fprintf(f, "\n");
    }
    fclose(f);
    return 1;
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

/* ---------------------------------------------------------------- general purpose VRAM */

/* fill_vram_banks / _word / _byte and poke / peek: mode independent, checked byte by byte */
static void test_cga_vram() {
    /* { row, col } - corners of both banks, the middle, and neighbours of the corners */
    static const uint8_t cells[][2] = {
        {   0,  0 }, {   0, 79 }, {   1,  0 }, {   1, 79 }, { 198,  0 },
        { 199, 79 }, { 100, 40 }, { 101, 40 }, {   2,  1 }, {   3, 78 }
    };
    const unsigned n_cells = sizeof(cells) / sizeof(cells[0]);
    bios_video_mode_t original;
    unsigned i, off;
    unsigned bad_banks_even = 0, bad_banks_odd = 0;
    unsigned bad_word = 0, bad_byte = 0, bad_poke = 0, bad_peek = 0;
    unsigned char want;

    test_wait_key("Mode 06h general purpose VRAM tests - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    /* 1. banks: byte order and bank order - low byte at the lower address */
    test_prefill(0x00);
    cga_fill_vram_banks(0x1234, 0x5678);
    for (off = 0; off < TEST_BANK_BYTES; off += 2) {
        if (test_vram[off] != 0x34 || test_vram[off + 1] != 0x12) ++bad_banks_even;
        if (test_vram[TEST_ODD_BANK + off] != 0x78 || test_vram[TEST_ODD_BANK + off + 1] != 0x56) ++bad_banks_odd;
    }
    test_wait_key("");                                  /* visual: different texture on alternate rows */

    /* 2. word: same word everywhere, byte order */
    test_prefill(0x00);
    cga_fill_vram_word(0x1234);
    for (off = 0; off < TEST_BANK_BYTES; off += 2) {
        if (test_vram[off] != 0x34 || test_vram[off + 1] != 0x12) ++bad_word;
        if (test_vram[TEST_ODD_BANK + off] != 0x34 || test_vram[TEST_ODD_BANK + off + 1] != 0x12) ++bad_word;
    }

    /* 3. byte: every visible byte */
    test_prefill(0x00);
    cga_fill_vram_byte(0xA5);
    bad_byte = test_count_visible_not(0xA5);
    test_wait_key("");                                  /* visual: even fine texture */

    /* 4. poke: each cell lands at its own address and nowhere else */
    test_prefill(TEST_DUMP_FILL);
    for (i = 0; i < n_cells; ++i)
        cga_poke_vram_byte(cells[i][0], cells[i][1], (uint8_t)(0x80 + i));
    for (off = 0; off < TEST_VRAM_SIZE; ++off) {
        want = TEST_DUMP_FILL;
        for (i = 0; i < n_cells; ++i)
            if (off == test_offset(cells[i][0], cells[i][1])) want = (unsigned char)(0x80 + i);
        if (test_vram[off] != want) ++bad_poke;
    }

    /* 5. peek: reads back bytes written independently through the far pointer */
    test_prefill(TEST_DUMP_FILL);
    for (i = 0; i < n_cells; ++i)
        test_vram[test_offset(cells[i][0], cells[i][1])] = (unsigned char)(0xC0 + i);
    for (i = 0; i < n_cells; ++i)
        if (cga_peek_vram_byte(cells[i][0], cells[i][1]) != (uint8_t)(0xC0 + i)) ++bad_peek;

    env_set_video_mode(original);

    printf("fill_vram_banks: even bank %u errors, odd bank %u errors\n", bad_banks_even, bad_banks_odd);
    printf("fill_vram_word:  %u errors\n", bad_word);
    printf("fill_vram_byte:  %u errors\n", bad_byte);
    printf("poke_vram_byte:  %u errors\n", bad_poke);
    printf("peek_vram_byte:  %u errors\n", bad_peek);
    assert(bad_banks_even == 0);
    assert(bad_banks_odd == 0);
    assert(bad_word == 0);
    assert(bad_byte == 0);
    assert(bad_poke == 0);
    assert(bad_peek == 0);
    printf("cga general purpose VRAM: PASS\n\n");
}

void test_cga() {
    printf("Test CGALIB...\n");
    //test_cga_versions();
    //test_cga_vram();
    test_cga_med();
    //test_cga_hi();
}
