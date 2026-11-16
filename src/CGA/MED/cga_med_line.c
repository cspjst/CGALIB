#include "cga_med_line.h"

#include "../cga_constants.h"
#include "../cga_row_offsets.h" // IWYU pragma: keep

void cga_med_hline(uint32_t p, uint16_t len, cga_med_colour_t c) {
    // __watcall DX = y, AX = x, BX = len, CL = c
    __asm {
        .8086
        cld                                                 ; clear direction flag
        // 0. len 0 draws nothing
        test    bx, bx                                      ; len == 0?
        jnz     HL0                                         ; no: carry on
        jmp     END                                         ; yes: draw nothing (near jmp, any distance)
HL0:    // 1. set up VRAM segment in ES
        mov     di, CGA_VIDEO_RAM_SEGMENT
        mov     es, di
        // 2. lookup y and setup ES:DI point to target row
        xchg    bx, dx                                      ; BX = y DX = len
        shl     bx, 1                                       ; convert BX word pointer
        mov     di, seg CGA_ROW_OFFSETS                     ; DS is scratch in Watcom compiler
        mov     ds, di                                      ; DS = table's segment
        mov     di, CGA_ROW_OFFSETS[bx]                     ; lookup y offset
        mov     si, cx                                      ; SI = colour, CX is free
        mov     bx, ax                                      ; BX = x0
        add     ax, dx                                      ; AX = x1
        dec     ax                                          ; AX = x1 = x0 + len - 1
        // 3. build lhs & rhs proto-masks, CX and DX are now free to use
        mov     dx, 0FFFFh                                  ; DL lhs DH rhs proto-masks (little endian)
        mov     cl, bl                                      ; CL = x0
        and     cl, 3                                       ; CL is x0 mod 4
        shl     cl, 1                                       ; double so shift 0, 2, 4 or 6 bits
        shr     dl, cl                                      ; shift lhs proto mask to starting pixel
        mov     cl, al                                      ; CL = x1
        and     cl, 3                                       ; CL is x1 mod 4
        xor     cl, 3                                       ; convert to bits to shift left
        shl     cl, 1                                       ; double so shift 0, 2, 4 or 6 bits
        shl     dh, cl                                      ; shift rhs proto mask to ending pixel
        // 4. reduce x0 and x1 to column bytes
        shr     bx, 1                                       ; calculate column byte x0 / 4
        shr     bx, 1                                       ; poor old 8086 only has opcodes shifts by an implicit 1 or CL
        shr     ax, 1                                       ; calculate column byte x1 / 4
        shr     ax, 1
        // 5. CX = rhs byte - lhs byte: 0 same byte, 1 same word, else general case
        mov     cx, ax                                      ; CX = x1
        sub     cx, bx                                      ; CX = x1 - x0
        // 6.0.0 smear colour across AX fill word and lhs/rhs colour bits
        mov     ax, si                                      ; AL = colour
        mov     ah, al                                      ; AH = AL = ccxxxxxx
        shr     ah, 1
        shr     ah, 1                                       ; AH = xxccxxxx
        or      al, ah                                      ; AL = ccccxxxx
        mov     ah, al                                      ; AH = ccccxxxx
        shr     ah, 1
        shr     ah, 1
        shr     ah, 1
        shr     ah, 1                                       ; AH = xxxxcccc
        or      al, ah                                      ; AL = cccccccc
        mov     ah, al                                      ; duplicate colour byte
        mov     si, ax                                      ; SI = fill word
        and     ax, dx                                      ; AL lhs AH rhs 'colour' bits
        jcxz    J0                                          ; lhs and rhs share same byte?
        dec     cx
        jcxz    J1                                          ; lhs and rhs share same word?
        // 6.1.0 general case
        not     dx                                          ; convert proto-mask to mask word
        // 6.1.1 colour lhs and rhs line
        add     di, bx                                      ; row + x0 -> ES:DI point to lhs byte
        and     es:[di], dl                                 ; mask out target bits  - 16 + EA(8)
        or      es:[di], al                                 ; colour target bits    - 16 + EA(8)
        mov     bx, cx                                      ; rhs offset = length
        inc     di                                          ; next byte
        and     es:[di + bx], dh                            ; mask out target bits  - 16 + EA(8)
        or      es:[di + bx], ah                            ; colour target bits    - 16 + EA(8)
        // 6.1.2 fill 'colour'
        mov     ax, si                                      ; AX = fill word
        // 6.1.3 handle odd or even line lengths
        shr     cx, 1                                       ; number of words to fill, lsb -> carry flag
        jnc     NC1                                         ; even so no byte to fill
        stosb                                               ; odd do one byte al 'colour'
        jcxz    END
NC1:    // 6.1.4 remaining word(s) ax 'colour'
        rep     stosw                                       ; CX is checked for !=0 before even the first step
        jmp     END
J1:     // 6.2.0 special case same word (saves 48 clock cycles on 8086 line lengths 2 - 15)
        not     dx                                          ; convert proto-mask to mask word
        // 6.2.1 colour the shared lhs:rhs word                                       Clock Cycles
        and     es:[di + bx], dx                            ; mask out target word  - 16 + EA(8)
        or      es:[di + bx], ax                            ; colour target word    - 16 + EA(8)
        jmp     END
J0:     // 6.3.0 special case same byte (saves 48 clock cycles on 8086 line lengths 0 - 7)
        and     dl, dh                                      ; combine proto-mask into dl
        not     dl                                          ; convert proto-mask to mask
        and     al, ah                                      ; combine 'colour' bits into al
        // 6.3.1 colour the combined lhs&rhs byte                                        Clock Cycles
        and     es:[di + bx], dl                            ; mask out target bits  - 16 + EA(8)
        or      es:[di + bx], al                            ; colour target bits    - 16 + EA(8)
END:
    }
}

void cga_med_vline(uint32_t p, uint16_t len, cga_med_colour_t c) {
    // __watcall DX = y, AX = x, BX = len, CL = c
    __asm {
        .8086
        cld                                                 ; lodsw / stosb count up
        // 0. len 0 draws nothing
        test    bx, bx
        jz      END
        // 1. SI = word pointer into the row table, DS = table segment, ES = VRAM
        mov     si, dx                                      ; SI = y
        shl     si, 1                                       ; word pointer
        mov     dx, CGA_VIDEO_RAM_SEGMENT
        mov     es, dx
        mov     dx, seg CGA_ROW_OFFSETS
        mov     ds, dx                                      ; DS = table segment (DS is scratch under Watcom)
        add     si, offset CGA_ROW_OFFSETS                  ; DS:SI -> CGA_ROW_OFFSETS[y]
        // 2. pixel bit, mask and colour bits
        mov     ch, cl                                      ; CH = colour
        mov     cl, al
        and     cl, 3                                       ; CL = x mod 4
        shl     cl, 1                                       ; double so shift 0, 2, 4 or 6 bits
        shr     dl, cl                                      ; DH = pixel bits shifted
        mov     dh, 0C0h                                    ; DH = mask left 2 bits 7 & 6
        shr     dh, cl                                      ; DH = mask bits shifted
        not     dh                                          ; DH = mask, keeps the other 6 pixels
        mov     cx, bx                                      ; CX = len (non-zero, so loop is safe)
        // 3. column byte
        shr     ax, 1                                       ; x / 4
        shr     ax, 1
        mov     bx, ax                                      ; BX = column byte
        // 4. one pixel per row: 1 table read, 1 VRAM read, 1 VRAM write
        lodsw                                               ; AX = row offset, SI -> next row
        add     ax, bx                                      ; + column
        xchg    ax, di                                      ; DI = target byte (1 byte opcode)
        mov     al, es:[di]                                 ; read
        and     al, dh                                      ; mask out target pixel
        or      al, dl                                      ; 'colour' it
        stosb                                               ; write
        loop    VL0
END:
    }
}
