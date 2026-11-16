/**
 *  @author      Jeremy Simon Thornton
 *  @copyright   2026 Jeremy Simon Thornton
 */
#ifndef CGA_VRAM_H
#define CGA_VRAM_H

#include <stdint.h>

/**
 * @brief   Fill CGA VRAM with one word pattern per bank.
 * @details Mode independent: works in 320x200 and 640x200. The even bank (B800:0000h) holds
 * rows 0, 2 .. 198 and the odd bank (B800:2000h) holds rows 1, 3 .. 199, so different
 * patterns give alternating rows. Words are little-endian: 1234h fills the bank 34h 12h 34h 12h ...
 * @param   even  word pattern for the even rows
 * @param   odd   word pattern for the odd rows
 */
void cga_fill_vram_banks(uint16_t even, uint16_t odd);

/**
 * @brief   Fill CGA VRAM with one word pattern.
 * @details Mode independent. Little-endian: 1234h fills VRAM 34h 12h 34h 12h ...
 * @param   w  word pattern
 */
void cga_fill_vram_word(uint16_t w);

/**
 * @brief   Fill CGA VRAM with one byte pattern.
 * @details Mode independent. 00h clears to background (320x200) or black (640x200).
 * @param   b  byte pattern
 */
void cga_fill_vram_byte(uint8_t b);

/**
 * @brief   Write one byte of CGA VRAM by row and byte column.
 * @details Mode independent. A byte holds 4 pixels at 320x200 and 8 pixels at 640x200.
 * @param   row  0 .. 199
 * @param   col  byte column 0 .. 79, not a pixel x
 * @param   b    byte to write
 * @note    ABSOLUTELY NO BOUNDS CHECKING!
 * @note    Leaves DS = segment of CGA_ROW_OFFSETS (DS is scratch under Watcom).
 */
void    cga_poke_vram_byte(uint16_t row, uint16_t col, uint8_t b);

/**
 * @brief   Read one byte of CGA VRAM by row and byte column.
 * @details Mode independent. A byte holds 4 pixels at 320x200 and 8 pixels at 640x200.
 * @param   row  0 .. 199
 * @param   col  byte column 0 .. 79, not a pixel x
 * @return  the byte at row, col
 * @note    ABSOLUTELY NO BOUNDS CHECKING!
 * @note    Leaves DS = VRAM segment B800h (DS is scratch under Watcom).
 */
uint8_t cga_peek_vram_byte(uint16_t row, uint16_t col);

#endif
