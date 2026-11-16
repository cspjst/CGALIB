#include "../cga_constants.h"
#include "cga_med_colours.h"

void cga_med_cls() {
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

void cga_med_cls_colour(cga_med_colour_t c)  {
    // __watcall(AL = c)
    __asm {
        .8086
        push    es

        // 1. smear colour bits across AX
        mov     ah, al                      ; AH = AL = ccxxxxxx
        shr     ah, 1
        shr     ah, 1                       ; AH = xxccxxxx
        or      al, ah                      ; AL = ccccxxxx
        mov     ah, al                      ; AH = ccccxxxx
        shr     ah, 1
        shr     ah, 1
        shr     ah, 1
        shr     ah, 1                       ; AH = xxxxcccc
        or      al, ah                      ; AL = cccccccc
        mov     ah, al                      ; duplicate colour byte
        // 2. repeat store AX
        cld                                 ; incremental STOSW
        mov     di, CGA_VIDEO_RAM_SEGMENT
        mov     es, di
        xor     di, di
        mov     cx, CGA_VRAM_WORDS
        rep     stosw                       ; store AX full screen VRAM

        pop     es
    }
}
