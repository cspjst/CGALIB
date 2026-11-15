#include "cga_hi_vram.h"

#include "../cga_constants.h"
#include "cga_hi_colours.h"

void cga_hi_cls() {
    __asm {
        .8086
        push    es
        pushf

        cld                                 ; incremental STOSW
        mov     di, CGA_VIDEO_RAM_SEGMENT
        mov     es, di
        xor     di, di
        mov     cx, CGA_VRAM_WORDS
        xor     ax, ax                      ; black
        rep     stosw                       ; store AX full screen VRAM

        popf
        pop     es
    }
}


void cga_hi_cls_colour(cga_hi_colour_t c)  {
    __asm {
        .8086
        push    es
        pushf

        mov     al, c
        test    al, al
        jz      CLS                         ; black
        mov     al, 0FFh                    ; colour
CLS:    mov     ah, al                      ; duplicate colour byte
        cld                                 ; incremental STOSW
        mov     di, CGA_VIDEO_RAM_SEGMENT
        mov     es, di
        xor     di, di
        mov     cx, CGA_VRAM_WORDS
        rep     stosw                       ; store AX full screen VRAM

        popf
        pop     es
    }
}

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
