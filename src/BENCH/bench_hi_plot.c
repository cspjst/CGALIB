/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#include "bench_hi_plot.h"
#include "bench_cga.h"

#include <stdio.h>

#include "../include/ENV/env_video_mode.h"

#include "../CGA/cga_types.h"
#include "../CGA/HI/cga_hi_colours.h"
#include "../CGA/HI/cga_hi_plot.h"

#define BENCH_TICKS     182                 /* ~10 s per plotter */
#define BENCH_POINTS    1024                /* power of 2 */

typedef void (*bench_hi_plot_fn)(uint32_t p, cga_hi_colour_t c);

static uint32_t bench_points[BENCH_POINTS];
static const cga_hi_colour_t bench_colours[2] = { CGA_HI_BLACK, CGA_HI_COLOUR };

/* same call shape as a real plotter: measures loop + call overhead */
static void bench_null(uint32_t p, cga_hi_colour_t c) {
    (void)p;
    (void)c;
}

/* plots for BENCH_TICKS, returns plot count, actual ticks in *ticks */
static unsigned long bench_run(bench_hi_plot_fn plot, unsigned long* ticks) {
    unsigned long n = 0;
    uint32_t start, now;
    unsigned i;

    env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200); /* clears VRAM */
    start = bench_ticks();
    while ((now = bench_ticks()) == start) ;                  /* align to a tick edge */
    start = now;
    do {
        for (i = 0; i < BENCH_POINTS; ++i)
            plot(bench_points[i], bench_colours[i & 1]);
        n += BENCH_POINTS;
        now = bench_ticks();
    } while (bench_elapsed(start, now) < BENCH_TICKS);
    *ticks = bench_elapsed(start, now);
    return n;
}

/* time per plot in tenths of a microsecond: 1 tick = 54925.4 us */
static unsigned long bench_tenth_us(unsigned long n, unsigned long ticks) {
    return ticks * 549254UL / n;
}

static void bench_print(const char* name, unsigned long n, unsigned long t,
                        unsigned long us, unsigned long us_null) {
    printf("%-6s %8lu  %5lu  %4lu.%lu", name, n, t, us / 10, us % 10);
    if (us_null) printf("  %4lu.%lu", (us - us_null) / 10, (us - us_null) % 10);
    printf("\n");
}

void bench_hi_plot() {
    bios_video_mode_t original;
    cga_point_t p;
    uint16_t s = BENCH_SEED;
    unsigned long n_null, n_plot;
    unsigned long t_null, t_plot;
    unsigned long us_null, us_plot;
    unsigned i;

    for (i = 0; i < BENCH_POINTS; ++i) {
        p.x = (bench_rand(&s) >> 4) % 640;
        p.y = (bench_rand(&s) >> 4) % 200;
        bench_points[i] = p.u;
    }

    printf("High plot benchmark: null, plot - ~10 s each in mode 06h...\n");
    original = env_set_video_mode(BIOS_CGA_GRAPHICS_MONOCHROME_640X200);

    n_null = bench_run(bench_null,  &t_null);
    n_plot = bench_run(cga_hi_plot, &t_plot);

    env_set_video_mode(original);

    us_null = bench_tenth_us(n_null, t_null);
    us_plot = bench_tenth_us(n_plot, t_plot);

    printf("\nPlotter   plots  ticks  us/plot  net us/plot\n");
    bench_print("null", n_null, t_null, us_null, 0);
    bench_print("plot", n_plot, t_plot, us_plot, us_null);
    printf("\n");
}
