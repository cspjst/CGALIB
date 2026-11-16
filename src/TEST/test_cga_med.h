/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief       CGALIB medium resolution (320x200, 4 colour, mode 04h) tests.
 *  @details     NB 55h is colour 1 on every pixel in mode 04h, so a colour 1 draw over a 55h
 *               prefill changes nothing. Line and fill tests use TEST_MED_FILL = 1Bh (00 01 10 11):
 *               every byte holds all 4 colours, so any colour changes 3 pixels in 4.
 */
#ifndef TEST_CGA_MED_H
#define TEST_CGA_MED_H

#define TEST_MED_FILL   0x1B

void test_cga_med();

#endif
