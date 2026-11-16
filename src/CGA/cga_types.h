/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#ifndef CGA_TYPES_H
#define CGA_TYPES_H

#include <stdint.h>

typedef union {
    uint32_t u;        // packed: 0xXXXXYYYY
    struct {
        uint16_t x;
        uint16_t y;
    };
} cga_point_t;

#endif
