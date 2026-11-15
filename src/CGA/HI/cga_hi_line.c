#include "cga_hi_line.h"

#include "../cga_constants.h"
#include "../cga_row_offsets.h" // IWYU pragma: keep

// execution path bits, built in setup and dispatched below
#define BL_BLACK    1           // AND ~bit instead of OR bit
#define BL_YMAJOR   2           // y steps every pixel, x on the error term
#define BL_MINUS    4           // minor axis decreases: y up (x-major) or x left (y-major)

void cga_hi_hline(uint32_t p, uint16_t len, cga_hi_colour_t c) {
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
        and     cl, 7                                       ; CL is x0 mod 8
        shr     dl, cl                                      ; shift lhs proto mask to starting pixel
        mov     cl, al                                      ; CL = x1
        and     cl, 7                                       ; CL is x1 mod 8
        xor     cl, 7                                       ; convert to bits to shift left
        shl     dh, cl                                      ; shift rhs proto mask to ending pixel
        // 4. reduce x0 and x1 to column bytes
        shr     bx, 1                                       ; calculate column byte x0 / 8
        shr     bx, 1                                       ; poor old 8086 only has opcodes shifts by an implicit 1 or CL
        shr     bx, 1
        shr     ax, 1                                       ; calculate column byte x1 / 8
        shr     ax, 1
        shr     ax, 1
        // 5. CX = rhs byte - lhs byte: 0 same byte, 1 same word, else general case
        mov     cx, ax                                      ; CX = x1
        sub     cx, bx                                      ; CX = x1 - x0
        // 6.0.0 work out 'colour' bits into AL and AH
        mov     ax, si                                      ; AX = colour
        mov     ah, al                                      ; AH = AL = colour
        test    al, al
        jz      BLK                                         ; branching to hard code 'colour' saves a few cycles
        mov     ax, dx                                      ; proto-mask is white bits to 'colour'
WHT:    jcxz    J0                                          ; lhs and rhs share same byte?
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
        // fill 'colour'
        mov     ax, 0FFFFh                                  ; AX white
        // 6.1.3 handle odd or even line lengths
        shr     cx, 1                                       ; number of words to fill, lsb -> carry flag
        jnc     NC1                                         ; even so no byte to fill
        stosb                                               ; odd do one byte al 'colour'
        jcxz    END
NC1:    // 6.1.4 remaining word(s) ax 'colour'
        rep     stosw                                       ; CX is checked for !=0 before even the first step
        jmp     END
BLK:    jcxz    J0                                          ; lhs and rhs share same byte?
        dec     cx
        jcxz    J1                                          ; lhs and rhs share same word?
        not     dx                                          ; convert proto-mask to mask word
        add     di, bx                                      ; have ES:DI point to lhs
        and     es:[di], dl                                 ; mask out black bits
        mov     bx, cx                                      ; rhs offset = length
        inc     di                                          ; next byte
        and     es:[di + bx], dh                            ; mask out black bits
        mov     ax, 0                                       ; AX black
        shr     cx, 1                                       ; number of words to fill, lsb -> carry flag
        jnc     NC0                                         ; even so no byte to fill
        stosb                                               ; odd do one byte al 'colour'
        jcxz    END
NC0:    rep     stosw                                       ; CX is checked for !=0 before even the first step
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

void cga_hi_vline(uint32_t p, uint16_t len, cga_hi_colour_t c) {
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
        and     cl, 7                                       ; CL = x mod 8
        mov     dh, 80h
        shr     dh, cl                                      ; DH = pixel bit (leftmost pixel is bit 7)
        xor     dl, dl                                      ; DL = black bits
        test    ch, ch
        jz      VBLK
        mov     dl, dh                                      ; DL = 'colour' bits
VBLK:   not     dh                                          ; DH = mask, keeps the other 7 pixels
        mov     cx, bx                                      ; CX = len (non-zero, so loop is safe)
        // 3. column byte
        shr     ax, 1                                       ; x / 8
        shr     ax, 1
        shr     ax, 1
        mov     bx, ax                                      ; BX = column byte
        // 4. one pixel per row: 1 table read, 1 VRAM read, 1 VRAM write
VL0:    lodsw                                               ; AX = row offset, SI -> next row
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

/**
 * Bresenham line - 8086 register optimised, single inline asm block, no calls
 * @url https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
 *
 * Converted from my hga_bresenham_line (2024) to CGA 640x200, packed points and incremental addressing:
 * + Horizontal lines and single points take the inlined hline path, vertical lines the inlined vline path.
 * + Canonical order: x-major lines are drawn left to right, y-major top to bottom, so the
 *   major axis always increases and line(p1, p2) draws exactly the same pixels as line(p2, p1).
 * + No per-pixel address calculation: ES:DI and the pixel bit are stepped incrementally.
 *   - x step: rotate the pixel bit; the carry out of the rotate steps DI on byte wrap (adc / sbb, no branch).
 *   - y step: flip the bank bit; add or subtract one row only when crossing back into the even bank.
 * + No table lookup in the diagonal loops; the start address is calculated once with shifts.
 * + One read-modify-write per pixel: OR bit for colour, AND ~bit for black.
 * + 8 diagonal paths = {x-major, y-major} x {minor +, minor -} x {colour, black}.
 *
 * Register use in the diagonal loops:
 *   ES:DI = VRAM byte    DL = pixel bit (colour) or ~bit (black)
 *   SI = error term D    BX = 2 * minor delta    BP = 2 * major delta    CX = pixels left
 *
 * @note    Leaves DS = segment of CGA_ROW_OFFSETS on the hline / vline paths (DS is scratch under Watcom).
 */
void cga_hi_bresenham_line(uint32_t p1, uint32_t p2, cga_hi_colour_t c) {
    // __watcall DX:AX = p1 (y0:x0), CX:BX = p2 (y1:x1), c on the stack
    __asm {
        .8086
        cld                                                 ; stosb / stosw / lodsw count up
        mov     si, CGA_VIDEO_RAM_SEGMENT
        mov     es, si                                      ; ES = VRAM for every path
        // 1. absolute deltas
        mov     si, bx
        sub     si, ax                                      ; SI = x1 - x0
        jns     DX0
        neg     si                                          ; SI = dx = abs(x1 - x0)
DX0:    mov     di, cx
        sub     di, dx                                      ; DI = y1 - y0
        jns     DY0
        neg     di                                          ; DI = dy = abs(y1 - y0)
        // 2. select horizontal (and single point), vertical or diagonal
DY0:    test    di, di
        jnz     NOTH
        jmp     HLINE                                       ; dy == 0
NOTH:   test    si, si
        jnz     NOTV
        jmp     VLINE                                       ; dx == 0
NOTV:   cmp     si, di
        jae     XMAJ                                        ; dx >= dy: x-major, includes 45 degrees
        // 3.1 y-major: canonical order top to bottom
        cmp     dx, cx
        jbe     YM0
        xchg    ax, bx                                      ; swap p1 and p2
        xchg    dx, cx
YM0:    mov     cx, BL_YMAJOR                               ; CX = path
        cmp     bx, ax
        jae     YM1
        or      cx, BL_MINUS                                ; x1 < x0: x--
YM1:    mov     bx, di                                      ; BX = dy
        shl     di, 1
        push    di                                          ; [two_major] = 2 * dy
        shl     si, 1
        push    si                                          ; [two_minor] = 2 * dx
        sub     si, bx
        push    si                                          ; [err] = 2 * dx - dy
        inc     bx
        push    bx                                          ; [n] = dy + 1
        push    cx                                          ; [path]
        jmp     SETUP
        // 3.2 x-major: canonical order left to right
XMAJ:   cmp     ax, bx
        jbe     XM0
        xchg    ax, bx                                      ; swap p1 and p2
        xchg    dx, cx
XM0:    xor     bx, bx                                      ; BX = path
        cmp     cx, dx
        jae     XM1
        mov     bx, BL_MINUS                                ; y1 < y0: y--
XM1:    mov     cx, si                                      ; CX = dx
        shl     si, 1
        push    si                                          ; [two_major] = 2 * dx
        shl     di, 1
        push    di                                          ; [two_minor] = 2 * dy
        sub     di, cx
        push    di                                          ; [err] = 2 * dy - dx
        inc     cx
        push    cx                                          ; [n] = dx + 1
        push    bx                                          ; [path]
        // 4. start byte and pixel bit from AX = x0, DX = y0 - once per line
SETUP:  mov     cl, al
        and     cl, 7                                       ; CL = x0 mod 8
        mov     ch, 80h
        shr     ch, cl                                      ; CH = pixel bit
        shr     ax, 1                                       ; x0 / 8
        shr     ax, 1
        shr     ax, 1
        mov     di, ax                                      ; DI = column byte
        shr     dx, 1                                       ; DX = y0 / 2, CF = odd row
        sbb     ax, ax                                      ; AX = 0000h or FFFFh
        and     ax, CGA_BANK_BIT
        add     di, ax                                      ; + bank
        xchg    dh, dl                                      ; DX = (y0 / 2) * 256 (y0 / 2 < 100 so DH was 0)
        shr     dx, 1
        shr     dx, 1                                       ; * 64
        add     di, dx
        shr     dx, 1
        shr     dx, 1                                       ; * 16
        add     di, dx                                      ; DI = bank + (y0 / 2) * 80 + x0 / 8
        pop     ax                                          ; AL = path
        cmp     c, 0
        jne     SCOL
        or      al, BL_BLACK
        not     ch                                          ; black uses ~bit
SCOL:   mov     dl, ch                                      ; DL = pixel bit or ~bit
        mov     dh, al                                      ; DH = path
        pop     cx                                          ; CX = n, always >= 2
        pop     si                                          ; SI = D
        pop     bx                                          ; BX = 2 * minor delta
        pop     ax
        push    bp                                          ; no parameters referenced after this point
        mov     bp, ax                                      ; BP = 2 * major delta
        // 5. dispatch: conditional jumps are short, so hop through near jmps
        test    dh, BL_YMAJOR
        jnz     DY
        test    dh, BL_MINUS
        jnz     DXU
        test    dh, BL_BLACK
        jnz     T_XDB
        jmp     XDW
T_XDB:  jmp     XDB
DXU:    test    dh, BL_BLACK
        jnz     T_XUB
        jmp     XUW
T_XUB:  jmp     XUB
DY:     test    dh, BL_MINUS
        jnz     DYL
        test    dh, BL_BLACK
        jnz     T_YRB
        jmp     YRW
T_YRB:  jmp     YRB
DYL:    test    dh, BL_BLACK
        jnz     T_YLB
        jmp     YLW
T_YLB:  jmp     YLB

        // 6.1 x-major, x++, y++ (down), colour
XDW:    or      es:[di], dl                                 ; plot
        ror     dl, 1                                       ; x++
        adc     di, 0                                       ; bit wrapped 01h -> 80h: next byte
        test    si, si                                      ; ? D > 0
        jle     XDW1
        sub     si, bp                                      ; D -= 2 * dx
        xor     di, CGA_BANK_BIT                            ; y++: flip bank
        test    di, CGA_BANK_BIT
        jnz     XDW1                                        ; even -> odd: same row index
        add     di, CGA_BYTES_PER_ROW                       ; odd -> even: next row index
XDW1:   add     si, bx                                      ; D += 2 * dy
        loop    XDW
        jmp     DONE

        // 6.2 x-major, x++, y++ (down), black
XDB:    and     es:[di], dl                                 ; plot
        ror     dl, 1                                       ; x++
        sbb     di, -1                                      ; ~bit: carry clear on wrap, so +1 - CF
        test    si, si
        jle     XDB1
        sub     si, bp
        xor     di, CGA_BANK_BIT
        test    di, CGA_BANK_BIT
        jnz     XDB1
        add     di, CGA_BYTES_PER_ROW
XDB1:   add     si, bx
        loop    XDB
        jmp     DONE

        // 6.3 x-major, x++, y-- (up), colour
XUW:    or      es:[di], dl
        ror     dl, 1
        adc     di, 0
        test    si, si
        jle     XUW1
        sub     si, bp
        xor     di, CGA_BANK_BIT                            ; y--: flip bank
        test    di, CGA_BANK_BIT
        jz      XUW1                                        ; odd -> even: same row index
        sub     di, CGA_BYTES_PER_ROW                       ; even -> odd: previous row index
XUW1:   add     si, bx
        loop    XUW
        jmp     DONE

        // 6.4 x-major, x++, y-- (up), black
XUB:    and     es:[di], dl
        ror     dl, 1
        sbb     di, -1
        test    si, si
        jle     XUB1
        sub     si, bp
        xor     di, CGA_BANK_BIT
        test    di, CGA_BANK_BIT
        jz      XUB1
        sub     di, CGA_BYTES_PER_ROW
XUB1:   add     si, bx
        loop    XUB
        jmp     DONE

        // 6.5 y-major, y++, x++ (right), colour
YRW:    or      es:[di], dl                                 ; plot
        xor     di, CGA_BANK_BIT                            ; y++ every pixel
        test    di, CGA_BANK_BIT
        jnz     YRW1
        add     di, CGA_BYTES_PER_ROW
YRW1:   test    si, si                                      ; ? D > 0
        jle     YRW2
        sub     si, bp                                      ; D -= 2 * dy
        ror     dl, 1                                       ; x++
        adc     di, 0
YRW2:   add     si, bx                                      ; D += 2 * dx
        loop    YRW
        jmp     DONE

        // 6.6 y-major, y++, x++ (right), black
YRB:    and     es:[di], dl
        xor     di, CGA_BANK_BIT
        test    di, CGA_BANK_BIT
        jnz     YRB1
        add     di, CGA_BYTES_PER_ROW
YRB1:   test    si, si
        jle     YRB2
        sub     si, bp
        ror     dl, 1
        sbb     di, -1
YRB2:   add     si, bx
        loop    YRB
        jmp     DONE

        // 6.7 y-major, y++, x-- (left), colour
YLW:    or      es:[di], dl
        xor     di, CGA_BANK_BIT
        test    di, CGA_BANK_BIT
        jnz     YLW1
        add     di, CGA_BYTES_PER_ROW
YLW1:   test    si, si
        jle     YLW2
        sub     si, bp
        rol     dl, 1                                       ; x--
        sbb     di, 0                                       ; bit wrapped 80h -> 01h: previous byte
YLW2:   add     si, bx
        loop    YLW
        jmp     DONE

        // 6.8 y-major, y++, x-- (left), black
YLB:    and     es:[di], dl
        xor     di, CGA_BANK_BIT
        test    di, CGA_BANK_BIT
        jnz     YLB1
        add     di, CGA_BYTES_PER_ROW
YLB1:   test    si, si
        jle     YLB2
        sub     si, bp
        rol     dl, 1
        adc     di, -1                                      ; ~bit: carry clear on wrap, so -1 + CF
YLB2:   add     si, bx
        loop    YLB
        jmp     DONE

        // 7. vertical line: x = AX, y = min(y0, y1), len = dy + 1
VLINE:  cmp     dx, cx
        jbe     VL0
        mov     dx, cx                                      ; DX = top y
VL0:    mov     bx, di
        inc     bx                                          ; BX = len
        mov     cl, c                                       ; CL = colour
        mov     si, dx                                      ; SI = y
        shl     si, 1                                       ; word pointer
        mov     dx, seg CGA_ROW_OFFSETS
        mov     ds, dx                                      ; DS = table segment
        add     si, offset CGA_ROW_OFFSETS                  ; DS:SI -> CGA_ROW_OFFSETS[y]
        mov     ch, cl                                      ; CH = colour
        mov     cl, al
        and     cl, 7                                       ; CL = x mod 8
        mov     dh, 80h
        shr     dh, cl                                      ; DH = pixel bit
        xor     dl, dl                                      ; DL = black bits
        test    ch, ch
        jz      VBLK
        mov     dl, dh                                      ; DL = 'colour' bits
VBLK:   not     dh                                          ; DH = mask, keeps the other 7 pixels
        mov     cx, bx                                      ; CX = len, never 0
        shr     ax, 1                                       ; x / 8
        shr     ax, 1
        shr     ax, 1
        mov     bx, ax                                      ; BX = column byte
VLP:    lodsw                                               ; AX = row offset, SI -> next row
        add     ax, bx
        xchg    ax, di                                      ; DI = target byte
        mov     al, es:[di]                                 ; read
        and     al, dh                                      ; mask out target pixel
        or      al, dl                                      ; 'colour' it
        stosb                                               ; write
        loop    VLP
        jmp     HVDONE

        // 8. horizontal line and single point: x = min(x0, x1), y = DX, len = dx + 1
HLINE:  cmp     ax, bx
        jbe     HL0
        mov     ax, bx                                      ; AX = left x
HL0:    mov     bx, si
        inc     bx                                          ; BX = len
        mov     cl, c                                       ; CL = colour
        mov     di, seg CGA_ROW_OFFSETS
        mov     ds, di                                      ; DS = table segment
        xchg    bx, dx                                      ; BX = y DX = len
        shl     bx, 1                                       ; convert BX word pointer
        mov     di, CGA_ROW_OFFSETS[bx]                     ; lookup y offset
        mov     si, cx                                      ; SI = colour, CX is free
        mov     bx, ax                                      ; BX = x0
        add     ax, dx
        dec     ax                                          ; AX = x1 = x0 + len - 1
        mov     dx, 0FFFFh                                  ; DL lhs DH rhs proto-masks (little endian)
        mov     cl, bl
        and     cl, 7                                       ; CL is x0 mod 8
        shr     dl, cl                                      ; shift lhs proto mask to starting pixel
        mov     cl, al
        and     cl, 7                                       ; CL is x1 mod 8
        xor     cl, 7                                       ; convert to bits to shift left
        shl     dh, cl                                      ; shift rhs proto mask to ending pixel
        shr     bx, 1                                       ; column byte x0 / 8
        shr     bx, 1
        shr     bx, 1
        shr     ax, 1                                       ; column byte x1 / 8
        shr     ax, 1
        shr     ax, 1
        mov     cx, ax
        sub     cx, bx                                      ; CX = rhs byte - lhs byte
        mov     ax, si                                      ; AX = colour
        mov     ah, al                                      ; AH = AL = colour
        test    al, al
        jz      HBLK                                        ; branching to hard code 'colour' saves a few cycles
        mov     ax, dx                                      ; proto-mask is white bits to 'colour'
HWHT:   jcxz    HJ0                                         ; lhs and rhs share same byte?
        dec     cx
        jcxz    HJ1                                         ; lhs and rhs share same word?
        not     dx                                          ; convert proto-mask to mask word
        add     di, bx                                      ; row + x0 -> ES:DI point to lhs byte
        and     es:[di], dl                                 ; mask out target bits
        or      es:[di], al                                 ; colour target bits
        mov     bx, cx                                      ; rhs offset = length
        inc     di                                          ; next byte
        and     es:[di + bx], dh                            ; mask out target bits
        or      es:[di + bx], ah                            ; colour target bits
        mov     ax, 0FFFFh                                  ; AX white
        shr     cx, 1                                       ; number of words to fill, lsb -> carry flag
        jnc     HNC1                                        ; even so no byte to fill
        stosb                                               ; odd do one byte
        jcxz    HVDONE
HNC1:   rep     stosw
        jmp     HVDONE
HBLK:   jcxz    HJ0                                         ; lhs and rhs share same byte?
        dec     cx
        jcxz    HJ1                                         ; lhs and rhs share same word?
        not     dx                                          ; convert proto-mask to mask word
        add     di, bx                                      ; ES:DI point to lhs
        and     es:[di], dl                                 ; mask out black bits
        mov     bx, cx                                      ; rhs offset = length
        inc     di                                          ; next byte
        and     es:[di + bx], dh                            ; mask out black bits
        mov     ax, 0                                       ; AX black
        shr     cx, 1                                       ; number of words to fill, lsb -> carry flag
        jnc     HNC0                                        ; even so no byte to fill
        stosb                                               ; odd do one byte
        jcxz    HVDONE
HNC0:   rep     stosw
        jmp     HVDONE
HJ1:    not     dx                                          ; special case same word
        and     es:[di + bx], dx                            ; mask out target word
        or      es:[di + bx], ax                            ; colour target word
        jmp     HVDONE
HJ0:    and     dl, dh                                      ; special case same byte
        not     dl
        and     al, ah
        and     es:[di + bx], dl                            ; mask out target bits
        or      es:[di + bx], al                            ; colour target bits
        jmp     HVDONE

DONE:   pop     bp                                          ; diagonal paths pushed BP
HVDONE:
    }
}
