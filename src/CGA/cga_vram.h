#ifndef CGA_VRAM_H
#define CGA_VRAM_H

#include <stdint.h>

void cga_fill_vram_banks(uint16_t even, uint16_t odd);

void cga_fill_vram_word(uint16_t w);

void cga_fill_vram_byte(uint8_t b);

void    cga_poke_vram_byte(uint8_t row, uint8_t col, uint8_t b);

uint8_t cga_peek_vram_byte(uint8_t row, uint8_t col);

#endif
