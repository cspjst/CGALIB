/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 *  @brief   CGA video memory layout and I/O ports for the graphics modes.
 *  @details Values use C hex notation, valid both in C and inside Watcom __asm blocks.
 */
#ifndef CGA_CONSTANTS_H
#define CGA_CONSTANTS_H

/**
 *  @name    Video memory
 *  @details CGA VRAM is 16K at B8000h, 736K into the 8086's 1MB address space.
 *  Medium res 320x200 4 colour (modes 04h, 05h) and high res 640x200 2 colour (mode 06h)
 *  share one layout: packed pixels, scan lines interleaved across two 8K banks.
 *  - Even rows 0, 2 .. 198 are in the even bank at B800:0000h.
 *  - Odd rows  1, 3 .. 199 are in the odd bank at B800:2000h (= BA00:0000h).
 *  Each bank holds 100 rows of 80 bytes (8000 bytes). The last 192 bytes of each bank are never displayed.
 *  @{
 */
#define CGA_VIDEO_RAM_SEGMENT   0xB800
#define CGA_EVEN_VRAM_SEGMENT   0xB800
#define CGA_ODD_VRAM_SEGMENT    0xBA00
#define CGA_BANK_BIT            0x2000      // odd bank offset: bit 13 set selects the odd bank
/** @} */

/**
 *  @name    Geometry
 *  @details A row is 80 bytes in both modes: 4 pixels per byte at 320x200, 8 pixels per byte at 640x200.
 *  @{
 */
#define CGA_BYTES_PER_ROW       80
#define CGA_WORDS_PER_ROW       40
#define CGA_ROWS_PER_SCREEN     200
#define CGA_ROWS_PER_BANK       100
#define CGA_BYTES_PER_BANK      0x1F40      // 8000: visible bytes per bank, 100 rows x 80
#define CGA_WORDS_PER_BANK      0x0FA0      // 4000: rep stosw count for one bank
/** @} */

/**
 *  @name    Sizes
 *  @details SCREEN is the visible pixels only. VRAM is the whole 16K, including the two 192-byte gaps.
 *  @{
 */
#define CGA_SCREEN_BYTES        0x3E80      // 16000: 200 rows x 80
#define CGA_SCREEN_WORDS        0x1F40      //  8000
#define CGA_VRAM_BYTES          0x4000      // 16384: 2 banks x 8K
#define CGA_VRAM_WORDS          0x2000      //  8192
/** @} */

/**
 *  @name    I/O ports
 *  @{
 */
#define CGA_CONTROL_REG         0x3D8       // Mode Control Register, write only
#define CGA_PALETTE_REG         0x3D9       // Colour Select Register: background/foreground, intensity, colour set
#define CGA_STATUS_REG          0x3DA       // Status Register, read only
/** @} */

/**
 *  @name    Status register bits (read from CGA_STATUS_REG, 3DAh)
 *  @{
 */
#define CGA_CTRL_BIT0           0x01        // set: display disabled (horizontal or vertical retrace), VRAM access without snow
#define CGA_CTRL_BIT3           0x08        // set: vertical retrace in progress
/** @} */

#endif
