/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief       Speed comparison of cga_med_plot (table) and cga_med_plot_calc (calculated).
 *  @details     Runs a null plotter, then each real plotter, for ~10 s each in mode 04h.
 *               Reports time per plot and net time per plot after subtracting loop and call overhead.
 */
#ifndef BENCH_MED_PLOT_H
#define BENCH_MED_PLOT_H

void bench_med_plot();

#endif
