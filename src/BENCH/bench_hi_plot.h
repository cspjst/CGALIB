/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief       Speed of cga_hi_plot against a null plotter.
 *  @details     Runs a null plotter, then cga_hi_plot, for ~10 s each in mode 06h.
 *               Reports time per plot and net time per plot after subtracting loop and call overhead.
 */
#ifndef BENCH_HI_PLOT_H
#define BENCH_HI_PLOT_H

void bench_hi_plot();

#endif
