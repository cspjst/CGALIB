#include "cga_med_plot.h"

#include "../cga_constants.h"
#include "../cga_row_offsets.h" // IWYU pragma: keep

void cga_med_plot(uint32_t p, cga_med_colour_t c) {
    // __watcall DX = y, AX = x, BL = c
    __asm {
        push    es

        mov     di, CGA_VIDEO_RAM_SEGMENT   ; load the VRAM segment address
        mov     es, di                      ; transer segment into es
        xchg    bx, dx                      ; DL = colour BX = y
        shl     bx, 1                       ; turn y into a word table index
        mov     di, seg CGA_ROW_OFFSETS     ; load the TABLE segment address
        mov     ds, di                      ; transer segment into DS
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

void cga_med_plot_calc(uint32_t p, cga_med_colour_t c) {
    __asm {
        mov     ax, word ptr p          ; AX = x (0..319)
        mov     dx, word ptr p+2        ; DX = y (0..199), so DH = 0
        mov     bl, c                   ; BL = colour, pre-shifted to bits 7-6

        ; row offset = (y & 1) * 2000h + (y >> 1) * 80
        shr     dx, 1                   ; CF = odd row, DX = y >> 1 (max 99, DH still 0)
        sbb     di, di                  ; DI = 0000h or FFFFh
        and     di, 2000h               ; DI = bank
        xchg    dh, dl                  ; DX = (y >> 1) * 256
        shr     dx, 1
        shr     dx, 1                   ; DX = (y >> 1) * 64
        add     di, dx
        shr     dx, 1
        shr     dx, 1                   ; DX = (y >> 1) * 16
        add     di, dx                  ; DI = bank + (y >> 1) * 80

        ; byte = x >> 2, shift = (x & 3) * 2
        mov     cl, al
        shr     ax, 1
        shr     ax, 1
        add     di, ax                  ; DI = byte offset in VRAM
        and     cl, 3
        shl     cl, 1                   ; CL = 0, 2, 4, 6
        mov     al, 3Fh
        ror     al, cl                  ; AL = mask that clears the target pixel
        shr     bl, cl                  ; BL = colour moved into position

        mov     dx, 0B800h
        mov     es, dx
        mov     ah, es:[di]             ; one VRAM read
        and     ah, al
        or      ah, bl
        mov     es:[di], ah             ; one VRAM write
    }
}
