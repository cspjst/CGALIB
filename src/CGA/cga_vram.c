#include "cga_vram.h"


void cga_hi_fill_vram(uint16_t pattern1, uint16_t pattern2) {
    __asm {
        .8086
        push    es
        pushf

        cld                                 ; incremental STOSW
        mov     di, CGA_EVEN_VRAM_SEGMENT
        mov     es, di
        xor     di, di
        mov     cx, CGA_SCREEN_WORDS / 2    ; 1000h words
        mov     ax, pattern1
        rep     stosw                       ; store AX full screen VRAM

        mov     di, CGA_ODD_VRAM_SEGMENT
        mov     es, di
        xor     di, di
        mov     cx, CGA_SCREEN_WORDS / 2    ; 1000h words
        mov     ax, pattern2
        rep     stosw                       ; store AX full screen VRAM

        popf
        pop     es
    }
}
