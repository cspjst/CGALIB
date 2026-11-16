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
