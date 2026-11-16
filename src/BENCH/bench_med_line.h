/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief       Speed comparison of a naive Bresenham line (cga_med_plot per pixel) and cga_med_bresenham_line.
 *  @details     Draws a ray circle of 360 lines from the screen centre, colour 3 on background then
 *               background on colour 3, for each line routine, timed with the BIOS tick counter (INT 1Ah).
 *               Ray end points are calculated once with sin / cos before any timing starts.
 *               Set BENCH_HAVE_MED_BRESENHAM to 1 once cga_med_bresenham_line exists.
 */
#ifndef BENCH_MED_LINE_H
#define BENCH_MED_LINE_H

void bench_med_line();

#endif
