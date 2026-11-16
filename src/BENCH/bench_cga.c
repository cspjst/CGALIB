/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#include "bench_cga.h"
#include "bench_med_plot.h"
#include "bench_hi_plot.h"
#include "bench_med_line.h"
#include "bench_hi_line.h"

#include <stdio.h>
#include <math.h>

#include "../include/BIOS/bios_keyboard_services.h"
#include "../include/BIOS/bios_clock_services.h"

#include "../CGA/cga_types.h"

/* ---------------------------------------------------------------- helpers */

void bench_wait_key(const char* msg) {
    bios_key_t key;
    printf("%s", msg);
    bios_wait_for_keystroke_and_read(&key);
}

uint16_t bench_rand(uint16_t* s) {
    *s = (uint16_t)(*s * 25173u + 13849u);
    return *s;
}

uint32_t bench_ticks() {
    bios_ticks_since_midnight_t t;
    bios_read_system_clock(&t);
    return (uint32_t)t;
}

uint32_t bench_elapsed(uint32_t start, uint32_t end) {
    if (end < start) end += BENCH_TICKS_PER_DAY;
    return end - start;
}

void bench_rays_init(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry,
                     uint32_t* centre, uint32_t* ends, unsigned n) {
    cga_point_t p;
    float angle;
    unsigned a;

    p.x = cx;
    p.y = cy;
    *centre = p.u;
    for (a = 0; a < n; ++a) {
        angle = (a * 2.0f * 3.14159265f) / n;
        p.x = (uint16_t)(cx + rx * cos(angle));
        p.y = (uint16_t)(cy + ry * sin(angle));
        ends[a] = p.u;
    }
}

/* ---------------------------------------------------------------- runner */

void bench_cga() {
    printf("Bench CGALIB...\n");
    bench_med_plot();
    bench_hi_plot();
    bench_med_line();
    bench_hi_line();
}
