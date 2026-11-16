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
        mov     dl, cl                                      ; DL = colour
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

// execution path bits, built in setup and dispatched below
#define BL_YMAJOR   2           // y steps every pixel, x on the error term
#define BL_MINUS    4           // minor axis decreases: y up (x-major) or x left (y-major)

/**
 * Bresenham line - 8086 register optimised, single inline asm block, no calls
 * @url https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
 *
 * Converted from cga_hi_bresenham_line to CGA 320x200 4 colour, 2 bits per pixel:
 * + Horizontal lines and single points take the inlined hline path, vertical lines the inlined vline path.
 * + Canonical order: x-major lines are drawn left to right, y-major top to bottom, so the
 *   major axis always increases and line(p1, p2) draws exactly the same pixels as line(p2, p1).
 * + No per-pixel address calculation: ES:DI, the pixel mask and the colour bits are stepped incrementally.
 *   - x step: rotate mask and colour bits by 2; the carry out of the mask rotate steps DI on byte wrap (no branch).
 *   - y step: flip the bank bit; add or subtract one row only when crossing back into the even bank.
 * + No table lookup in the diagonal loops; the start address is calculated once with shifts.
 * + One VRAM read and one VRAM write per pixel: read, AND ~mask, OR colour bits, write - any of the 4 colours.
 * + 4 diagonal paths = {x-major, y-major} x {minor +, minor -}, no colour specific paths.
 *
 * Register use in the diagonal loops:
 *   ES:DI = VRAM byte    DL = colour bits at the pixel    DH = ~pixel mask (keeps the other 3 pixels)
 *   SI = error term D    BX = 2 * minor delta    BP = 2 * major delta    CX = pixels left
 *
 * @note    ABSOLUTELY NO BOUNDS CHECKING! Both points must be on screen.
 * @note    Preserves ES. Leaves DS = segment of CGA_ROW_OFFSETS on the hline / vline paths (DS is scratch under Watcom).
 */
void cga_med_bresenham_line(uint32_t p1, uint32_t p2, cga_med_colour_t c) {
    // __watcall DX:AX = p1 (y0:x0), CX:BX = p2 (y1:x1), c on the stack
    __asm {
        .8086
        push    es                                          ; preserved for the caller
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
NOTV:   push    bp                                          ; diagonal paths: BP saved under the loop values
        cmp     si, di
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
        // 4. start byte, pixel mask and colour bits from AX = x0, DX = y0 - once per line
SETUP:  mov     cl, al
        and     cl, 3                                       ; CL = x0 mod 4
        shl     cl, 1                                       ; * 2 bits per pixel
        mov     ch, 0C0h
        shr     ch, cl                                      ; CH = pixel mask (leftmost pixel is bits 7-6)
        shr     ax, 1                                       ; x0 / 4
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
        add     di, dx                                      ; DI = bank + (y0 / 2) * 80 + x0 / 4
        mov     al, c                                       ; AL = ccxxxxxx (last use of BP as the frame)
        mov     ah, al
        shr     ah, 1
        shr     ah, 1                                       ; AH = xxccxxxx
        or      al, ah                                      ; AL = ccccxxxx
        mov     ah, al
        shr     ah, 1
        shr     ah, 1
        shr     ah, 1
        shr     ah, 1                                       ; AH = xxxxcccc
        or      al, ah                                      ; AL = cccccccc
        and     al, ch
        mov     dl, al                                      ; DL = colour bits at the first pixel
        not     ch
        mov     dh, ch                                      ; DH = ~pixel mask
        pop     ax                                          ; AL = path
        pop     cx                                          ; CX = n, always >= 2
        pop     si                                          ; SI = D
        pop     bx                                          ; BX = 2 * minor delta
        pop     bp                                          ; BP = 2 * major delta (caller's BP is under it)
        // 5. dispatch: conditional jumps are short, so hop through near jmps
        test    al, BL_YMAJOR
        jnz     DY
        test    al, BL_MINUS
        jnz     T_XU
        jmp     XD
T_XU:   jmp     XU
DY:     test    al, BL_MINUS
        jnz     T_YL
        jmp     YR
T_YL:   jmp     YL

        // 6.1 x-major, x++, y++ (down)
XD:     mov     al, es:[di]                                 ; read
        and     al, dh                                      ; mask out target pixel
        or      al, dl                                      ; colour it
        mov     es:[di], al                                 ; write
        ror     dl, 1                                       ; x++: colour bits
        ror     dl, 1
        ror     dh, 1                                       ; x++: mask, rotated last for the carry
        ror     dh, 1
        sbb     di, -1                                      ; carry clear on wrap 3 -> 0, so +1 - CF
        test    si, si                                      ; ? D > 0
        jle     XD1
        sub     si, bp                                      ; D -= 2 * dx
        xor     di, CGA_BANK_BIT                            ; y++: flip bank
        test    di, CGA_BANK_BIT
        jnz     XD1                                         ; even -> odd: same row index
        add     di, CGA_BYTES_PER_ROW                       ; odd -> even: next row index
XD1:    add     si, bx                                      ; D += 2 * dy
        loop    XD
        jmp     DONE

        // 6.2 x-major, x++, y-- (up)
XU:     mov     al, es:[di]
        and     al, dh
        or      al, dl
        mov     es:[di], al
        ror     dl, 1
        ror     dl, 1
        ror     dh, 1
        ror     dh, 1
        sbb     di, -1
        test    si, si
        jle     XU1
        sub     si, bp
        xor     di, CGA_BANK_BIT                            ; y--: flip bank
        test    di, CGA_BANK_BIT
        jz      XU1                                         ; odd -> even: same row index
        sub     di, CGA_BYTES_PER_ROW                       ; even -> odd: previous row index
XU1:    add     si, bx
        loop    XU
        jmp     DONE

        // 6.3 y-major, y++, x++ (right)
YR:     mov     al, es:[di]                                 ; read
        and     al, dh                                      ; mask out target pixel
        or      al, dl                                      ; colour it
        mov     es:[di], al                                 ; write
        xor     di, CGA_BANK_BIT                            ; y++ every pixel
        test    di, CGA_BANK_BIT
        jnz     YR1
        add     di, CGA_BYTES_PER_ROW
YR1:    test    si, si                                      ; ? D > 0
        jle     YR2
        sub     si, bp                                      ; D -= 2 * dy
        ror     dl, 1                                       ; x++
        ror     dl, 1
        ror     dh, 1
        ror     dh, 1
        sbb     di, -1
YR2:    add     si, bx                                      ; D += 2 * dx
        loop    YR
        jmp     DONE

        // 6.4 y-major, y++, x-- (left)
YL:     mov     al, es:[di]
        and     al, dh
        or      al, dl
        mov     es:[di], al
        xor     di, CGA_BANK_BIT
        test    di, CGA_BANK_BIT
        jnz     YL1
        add     di, CGA_BYTES_PER_ROW
YL1:    test    si, si
        jle     YL2
        sub     si, bp
        rol     dl, 1                                       ; x--
        rol     dl, 1
        rol     dh, 1
        rol     dh, 1
        adc     di, -1                                      ; carry clear on wrap 0 -> 3, so -1 + CF
YL2:    add     si, bx
        loop    YL
        jmp     DONE

        // 7. vertical line: x = AX, y = min(y0, y1), len = dy + 1
VLINE:  cmp     dx, cx
        jbe     VL0
        mov     dx, cx                                      ; DX = top y
VL0:    mov     bx, di
        inc     bx                                          ; BX = len
        mov     si, dx                                      ; SI = y
        shl     si, 1                                       ; word pointer
        mov     dx, seg CGA_ROW_OFFSETS
        mov     ds, dx                                      ; DS = table segment
        add     si, offset CGA_ROW_OFFSETS                  ; DS:SI -> CGA_ROW_OFFSETS[y]
        mov     cl, al
        and     cl, 3                                       ; CL = x mod 4
        shl     cl, 1                                       ; * 2 bits per pixel
        mov     dh, 0C0h
        shr     dh, cl                                      ; DH = pixel mask
        mov     dl, c                                       ; DL = ccxxxxxx
        mov     ch, dl
        shr     ch, 1
        shr     ch, 1
        or      dl, ch                                      ; DL = ccccxxxx
        mov     ch, dl
        shr     ch, 1
        shr     ch, 1
        shr     ch, 1
        shr     ch, 1
        or      dl, ch                                      ; DL = cccccccc
        and     dl, dh                                      ; DL = colour bits at the pixel
        not     dh                                          ; DH = mask, keeps the other 3 pixels
        mov     cx, bx                                      ; CX = len, never 0
        shr     ax, 1                                       ; x / 4
        shr     ax, 1
        mov     bx, ax                                      ; BX = column byte
VLP:    lodsw                                               ; AX = row offset, SI -> next row
        add     ax, bx
        xchg    ax, di                                      ; DI = target byte
        mov     al, es:[di]                                 ; read
        and     al, dh                                      ; mask out target pixel
        or      al, dl                                      ; colour it
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
        and     cl, 3                                       ; CL = x0 mod 4
        shl     cl, 1                                       ; * 2 bits per pixel
        shr     dl, cl                                      ; shift lhs proto mask to starting pixel
        mov     cl, al
        and     cl, 3                                       ; CL = x1 mod 4
        xor     cl, 3                                       ; pixels to the right of x1
        shl     cl, 1                                       ; * 2 bits per pixel
        shl     dh, cl                                      ; shift rhs proto mask to ending pixel
        shr     bx, 1                                       ; column byte x0 / 4
        shr     bx, 1
        shr     ax, 1                                       ; column byte x1 / 4
        shr     ax, 1
        mov     cx, ax
        sub     cx, bx                                      ; CX = rhs byte - lhs byte
        mov     ax, si                                      ; AL = colour ccxxxxxx
        mov     ah, al
        shr     ah, 1
        shr     ah, 1                                       ; AH = xxccxxxx
        or      al, ah                                      ; AL = ccccxxxx
        mov     ah, al
        shr     ah, 1
        shr     ah, 1
        shr     ah, 1
        shr     ah, 1                                       ; AH = xxxxcccc
        or      al, ah                                      ; AL = cccccccc
        mov     ah, al                                      ; duplicate colour byte
        mov     si, ax                                      ; SI = fill word
        and     ax, dx                                      ; AL lhs AH rhs colour bits
        jcxz    HJ0                                         ; lhs and rhs share same byte?
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
        mov     ax, si                                      ; AX = fill word
        shr     cx, 1                                       ; number of words to fill, lsb -> carry flag
        jnc     HNC1                                        ; even so no byte to fill
        stosb                                               ; odd do one byte
        jcxz    HVDONE
HNC1:   rep     stosw
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
HVDONE: pop     es
    }
}
