/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#include "bench_hi_line.h"
#include "bench_cga.h"

#include <stdio.h>
#include <stdlib.h>

#include "../include/ENV/env_video_mode.h"

#include "../CGA/cga_types.h"
#include "../CGA/HI/cga_hi_plot.h"
#include "../CGA/HI/cga_hi_cls.h"
#include "../CGA/HI/cga_hi_line.h"

#define BENCH_RAYS      360
#define BENCH_SAMPLES   2                   /* passes over the ray circle per timing */
#define BENCH_CX        319                 /* screen centre */
#define BENCH_CY        99
#define BENCH_RX        238                 /* 640x200 pixels are ~2.4 times taller than wide: */
#define BENCH_RY        99                  /* RX = 2.4 * RY looks round on a 4:3 monitor */

typedef void (*bench_hi_line_fn)(uint32_t p1, uint32_t p2, cga_hi_colour_t c);

static uint32_t bench_centre;
static uint32_t bench_ends[BENCH_RAYS];

/**
 * Naive Bresenham baseline: textbook all-octant error form, cga_hi_plot per pixel.
 * Every pixel pays a call, a table lookup and a full address calculation.
 * NB tie-breaking differs from cga_hi_bresenham_line, so a few pixels may differ: timing only.
 */
static void bench_naive_line(uint32_t p1, uint32_t p2, cga_hi_colour_t c) {
    cga_point_t a, b;
    int dx, dy, sx, sy, err, e2;

    a.u = p1;
    b.u = p2;
    dx =  abs((int)b.x - (int)a.x);
    dy = -abs((int)b.y - (int)a.y);
    sx = (a.x < b.x) ? 1 : -1;
    sy = (a.y < b.y) ? 1 : -1;
    err = dx + dy;
    for (;;) {
        cga_hi_plot(a.u, c);
        if (a.x == b.x && a.y == b.y) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; a.x += sx; }
        if (e2 <= dx) { err += dx; a.y += sy; }
    }
}

/* draws BENCH_SAMPLES ray circles, returns elapsed ticks */
static unsigned long bench_run(bench_hi_line_fn line, cga_hi_colour_t c) {
    uint32_t start;
    int i, j;

    start = bench_ticks();
    for (j = 0; j < BENCH_SAMPLES; ++j)
        for (i = 0; i < BENCH_RAYS; ++i)
            line(bench_centre, bench_ends[i], c);
    return bench_elapsed(start, bench_ticks());
}

/* microseconds per line, from ticks */
static unsigned long bench_us(unsigned long ticks) {
    return ticks * BENCH_US_PER_TICK / ((unsigned long)BENCH_RAYS * BENCH_SAMPLES);
}

static void bench_print(const char* name, unsigned long ticks, unsigned long naive) {
    printf("%-22s %6lu  %8lu", name, ticks, bench_us(ticks));
    if (naive) printf("  %5lu%%", ticks * 100UL / naive);
    printf("\n");
}

void bench_hi_line() {
    bios_video_mode_t original;
    unsigned long t_naive_col, t_naive_blk, t_fast_col, t_fast_blk;

    printf("High line benchmark: %d rays x %d samples per timing\n", BENCH_RAYS, BENCH_SAMPLES);
    printf("Calculating ray circle...\n");
    bench_rays_init(BENCH_CX, BENCH_CY, BENCH_RX, BENCH_RY, &bench_centre, bench_ends, BENCH_RAYS);

    bench_wait_key("Mode 06h next - press a key...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    /* 1. naive: colour on black, then black on colour */
    cga_hi_cls();
    t_naive_col = bench_run(bench_naive_line, CGA_HI_COLOUR);
    bench_wait_key("");
    cga_hi_cls_colour(CGA_HI_COLOUR);
    t_naive_blk = bench_run(bench_naive_line, CGA_HI_BLACK);
    bench_wait_key("");

    /* 2. optimised: same again */
    cga_hi_cls();
    t_fast_col = bench_run(cga_hi_bresenham_line, CGA_HI_COLOUR);
    bench_wait_key("");
    cga_hi_cls_colour(CGA_HI_COLOUR);
    t_fast_blk = bench_run(cga_hi_bresenham_line, CGA_HI_BLACK);
    bench_wait_key("");

    env_set_video_mode(original);

    printf("\n%-22s %6s  %8s  %6s\n", "Line", "ticks", "us/line", "naive");
    bench_print("naive, colour",     t_naive_col, 0);
    bench_print("naive, black",      t_naive_blk, 0);
    bench_print("optimised, colour", t_fast_col,  t_naive_col);
    bench_print("optimised, black",  t_fast_blk,  t_naive_blk);
    printf("\n");
}
