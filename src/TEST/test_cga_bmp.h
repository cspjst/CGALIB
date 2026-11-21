/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief       CGALIB bitmap save / load tests.
 *  @details     Round trips with one and several blocks, checked byte for byte, then every failure path:
 *               NULL bitmap, NULL block, bad nblocks on save and load, missing file, truncated header
 *               and truncated data. Failure cases must return 0, leave no file behind on save, and
 *               release any block a failed load had allocated.
 *               test_cga_hi_bmp now, test_cga_med_bmp to follow with the medium resolution bitmap type.
 */
#ifndef TEST_CGA_BMP_H
#define TEST_CGA_BMP_H

void test_cga_bmp();

#endif
