/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#ifndef CGA_ROW_OFFSETS_H
#define CGA_ROW_OFFSETS_H

#define CGA_ROWS_PER_SCREEN 200

/**
 * @brief   Byte offset of each scanline in CGA graphics memory (B800h).
 * @details Shared by modes 4/5 (320x200) and 6 (640x200)
 * Both use 80-byte rows with even rows at 0000h, odd rows at 2000h.
 */
extern const unsigned short CGA_ROW_OFFSETS[CGA_ROWS_PER_SCREEN];

#endif
