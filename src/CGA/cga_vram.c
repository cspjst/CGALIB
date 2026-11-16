#include "cga_vram.h"

#include "cga_constants.h"
#include "cga_row_offsets.h" // IWYU pragma: keep

void cga_fill_vram_banks(uint16_t even, uint16_t odd) {
    __asm {
        .8086
        push    es
        pushf

        cld                                 ; incremental STOSW
        mov     di, CGA_EVEN_VRAM_SEGMENT
        mov     es, di
        xor     di, di
        mov     cx, CGA_SCREEN_WORDS / 2    ; 4096 words
        mov     ax, even
        rep     stosw                       ; store AX full screen VRAM

        mov     di, CGA_ODD_VRAM_SEGMENT
        mov     es, di
        xor     di, di
        mov     cx, CGA_SCREEN_WORDS / 2    ; 4096 words
        mov     ax, odd
        rep     stosw                       ; store AX full screen VRAM

        popf
        pop     es
    }
}

void cga_fill_vram_word(uint16_t w) {
    __asm {
        .8086
        push    es
        pushf

        cld                                 ; incremental STOSW
        mov     di, CGA_VIDEO_RAM_SEGMENT
        mov     es, di
        xor     di, di
        mov     cx, CGA_VRAM_WORDS          ; 2000h words
        mov     ax, w
        rep     stosw                       ; store AX full screen VRAM

        popf
        pop     es
    }
}

void cga_fill_vram_byte(uint8_t b) {
    __asm {
        .8086
        push    es
        pushf

        cld                                 ; incremental STOSW
        mov     di, CGA_VIDEO_RAM_SEGMENT
        mov     es, di
        xor     di, di
        mov     cx, CGA_VRAM_WORDS          ; 2000h words
        mov     al, b
        mov     ah, al
        rep     stosw                       ; store AX full screen VRAM

        popf
        pop     es
    }
}

void cga_poke_vram_byte(uint16_t row, uint16_t col, uint8_t b) {
    // __watcall AX = row, DX = col, BL = b
    __asm {
        .8086
        push    es

        xchg    ax, dx                      ; AX = col, DX = row
        mov     cl, bl
        mov     bx, dx                      ; BX = row
        shl     bx, 1                       ; convert to a word index
        mov     di, seg CGA_ROW_OFFSETS     ; load the TABLE segment address
        mov     ds, di                      ; DS = TABLE segment
        mov     di, CGA_ROW_OFFSETS[bx]     ; ES:DI->VRAM row
        add     di, ax                      ; ES:DI->VRAM row, col
        mov     al, cl
        mov     si, CGA_VIDEO_RAM_SEGMENT
        mov     es, si                      ; ES = VRAM segment
        stosb

        pop     es
    }
}

uint8_t cga_peek_vram_byte(uint16_t row, uint16_t col) {
    // __watcall AX = row, DX = col
    __asm {
        .8086

        mov     bx, ax                      ; BX = row
        shl     bx, 1                       ; convert to a word index
        mov     si, seg CGA_ROW_OFFSETS     ; load the TABLE segment address
        mov     ds, si                      ; DS = TABLE segment
        mov     si, CGA_ROW_OFFSETS[bx]     ; SI->VRAM row
        add     si, dx                      ; SI->VRAM row, col
        mov     di, CGA_VIDEO_RAM_SEGMENT
        mov     ds, di                      ; DS:SI->VRAM row, col
        lodsb
    }
}
