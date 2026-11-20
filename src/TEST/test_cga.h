/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief       CGALIB test harness: runner, shared helpers and mode independent tests.
 *  @details     Dump-and-diff tests draw, then write all 16K of VRAM as hex to a file that is
 *               diffed on the host against an independently generated reference image.
 *               Every test first writes VRAM through a far pointer. Watcom loads DS = B800h to do
 *               that and does not restore it, so every library call runs with a hostile DS.
 *               Helpers are prefixed test_ and never call the library, so a library bug
 *               cannot hide inside the harness.
 */
#ifndef TEST_CGA_H
#define TEST_CGA_H

#include <stdint.h>

#define TEST_DUMP_POINTS    256
#define TEST_DUMP_SEED      0x1234
#define TEST_DUMP_FILL      0x55        /* 01010101: hi res, every plot of either colour sets or clears something */
#define TEST_VRAM_SIZE      0x4000      /* physical: both banks including the 192-byte gaps */
#define TEST_BANK_BYTES     0x1F40      /* visible bytes per bank */
#define TEST_ODD_BANK       0x2000

extern volatile unsigned char* const test_vram;

void     test_wait_key(const char* msg);
uint16_t test_rand(uint16_t* s);
void     test_prefill(unsigned char b);
unsigned test_offset(unsigned row, unsigned col);
unsigned test_count_visible_not(unsigned char b);
int      test_dump_vram(const char* filename, const char* header);

void test_cga();

#endif
