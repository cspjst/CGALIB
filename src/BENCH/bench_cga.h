/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief       CGALIB benchmarks: runner and shared helpers.
 *  @details     All timing uses the BIOS tick counter (INT 1Ah, 18.2065 ticks/s).
 *               Anything random or trigonometric is prepared before timing starts.
 *               Helpers are prefixed bench_ and never call the code being timed.
 */
#ifndef BENCH_CGA_H
#define BENCH_CGA_H

#include <stdint.h>

#define BENCH_SEED          0x1234
#define BENCH_TICKS_PER_DAY 0x1800B0UL      /* midnight rollover */
#define BENCH_US_PER_TICK   54925UL         /* 1 / 18.2065 Hz */

void     bench_wait_key(const char* msg);
uint16_t bench_rand(uint16_t* s);           /* 16-bit LCG: use the high bits, the low bits only alternate */
uint32_t bench_ticks();
uint32_t bench_elapsed(uint32_t start, uint32_t end);

/**
 * @brief   Ray circle end points around a centre, calculated once with sin / cos.
 * @param   cx, cy  centre
 * @param   rx, ry  radii: rx / ry = pixel aspect makes the circle look round
 * @param   centre  out: packed centre point
 * @param   ends    out: n packed end points, one per degree step of 360 / n
 */
void bench_rays_init(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry,
                     uint32_t* centre, uint32_t* ends, unsigned n);

void bench_cga();

#endif
