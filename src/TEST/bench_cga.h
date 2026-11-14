/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief       Speed comparison of cga_med_plot (table) and cga_med_plot_calc (calculated).
 *  @details     Runs a null plotter, then each real plotter, for ~10 s each in mode 04h,
 *               timed with the BIOS tick counter (INT 1Ah). Reports time per plot and
 *               net time per plot after subtracting loop and call overhead.
 */
#ifndef TEST_CGA_BENCH_H
#define TEST_CGA_BENCH_H

void test_cga_bench();

#endif
