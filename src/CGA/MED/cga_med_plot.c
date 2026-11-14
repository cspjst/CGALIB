#include "cga_med_plot.h"

#include "../cga_constants.h"
#include "../cga_row_offsets.h" // IWYU pragma: keep

void cga_med_plot(uint32_t p, cga_med_colour_t c) {
    // __watcall DX = y, AX = x, BL = c
    __asm {
        push    es

        mov     cx, CGA_VIDEO_RAM_SEGMENT   ; load the VRAM segment address
        mov     es, cx                      ; transer segment into es
        xchg    bx, dx                      ; DL = colour BX = y
        shl     bx, 1                       ; turn y into a word table index
        mov     bx, CGA_ROW_OFFSETS[bx]     ; load the VRAM row offset address
        mov     cx, ax                      ; copy of x in CX
        and     cx, 3h                      ; mask off 0011 lower bits i.e.mod 4 pixels per byte
        shl     cx, 1                       ; *2 as 2 colour bits per pixel
        mov     dh, 00111111b               ; load AH with pixel mask
        ror     dh, cl                      ; roll mask right (msb->lsb) x mod 4
        shr     dl, cl                      ; shift colour right similarly
        shr     ax, 1                       ; calculate column byte x / 4
        shr     ax, 1                       ; 8086 limited to single step shifts                       ;
        add     bx, ax                      ; add in column byte
        and		es:[bx], dh		            ; mask out the pixel bits
        or		es:[bx], dl		            ; or in the pixel colour

        pop     es
    }
}
