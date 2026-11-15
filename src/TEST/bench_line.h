/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief       Speed comparison of a naive Bresenham line (cga_hi_plot per pixel) and cga_hi_bresenham_line.
 *  @details     Draws a ray circle of 360 lines from the screen centre, colour on black then black on colour,
 *               for each line routine, timed with the BIOS tick counter (INT 1Ah).
 *               Ray end points are calculated once with sin / cos before any timing starts.
 */
#ifndef BENCH_LINE_H
#define BENCH_LINE_H

void bench_line();

#endif
