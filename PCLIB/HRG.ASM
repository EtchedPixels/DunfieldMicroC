;
; High Resolution Graphics (HRG) library
;
; Low level functions
;
; ?COPY.TXT 1998-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG,BSEG
VBUFFER EQU 0A000h      ; VGA buffer address
VWIDTH  EQU 640     ; Width of a single line
PMASK   EQU 03C6h       ; Palette mask register
PREAD   EQU 03C7h       ; Palette read address register
PWRITE  EQU 03C8h       ; Palette write address register
PDATA   EQU 03C9h       ; Palette data register
VSTATUS EQU 03DAh       ; VGA input status word
;
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
; Bulk storage (uninitialized) data segment
BSEG    SEGMENT BYTE PUBLIC 'UDATA'
vmode   DB  1 DUP(?)
BSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
    ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
    PUBLIC  _hrg_open, _hrg_close, _hrg_getpal, _hrg_setpal
    PUBLIC  _hrg_plot, _hrg_scale, _hrg_hline, _hrg_vline
    PUBLIC  _hrg_fill, _hrg_retrace, _hrg_delay
;
; Initialize video system & place in mode 12 (if applicable)
;
_hrg_open:
; First, determine current mode for later
    MOV AH,0Fh      ; Get video mode function
    INT 10h     ; Call BIOS
    MOV DGRP:vmode,AL   ; Save video mode
; Check for VGA present
    MOV AX,1A00h    ; Get display code
    INT 10h     ; Call BIOS
    CMP AL,1Ah      ; VGA supported?
    JNZ initv2      ; No, report not present
    MOV AL,12h      ; Mode 12 (640 x 480)
    XOR AH,AH       ; Set mode request
    INT 10h     ; Call BIOS
; Setup all palette registers to 0-15
    MOV AX,1002h    ; Set all palette registers
    PUSH    CS      ; Get CS
    POP ES      ; Set ES
    MOV DX,OFFSET PTABLE; Point to table
    INT 10h     ; Ask BIOS
; 3CE=0205, set initial mode
    MOV DX,03CEh    ; Graphic controller address
    MOV AX,0205h    ; Mode=02
    OUT DX,AX       ; Write it
    XOR AX,AX       ; Indicate success
initv2: RET
PTABLE: DB  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0
;
; Return to previous video mode
;
_hrg_close:
    MOV AL,DGRP:vmode   ; Get old video mode
    XOR AH,AH       ; Set mode request
    INT 10h     ; Call BIOS
    XOR AX,AX       ; Indicate success
    RET
;
; Get a palette entry: hrg_getpal(color, setting[3])
;
_hrg_getpal:
    MOV BX,SP       ; Address parms
    MOV DX,PMASK    ; Point to mask register
    MOV AL,0FFh     ; Allow all colors
    OUT DX,AL       ; Write it
    MOV AL,4[BX]    ; Get color
    INC DX      ; Advance to PREAD
    OUT DX,AL       ; Write it
    OUT DX,AL       ; Possible hware bug!
    MOV BX,2[BX]    ; Get array pointer
    INC DX      ; Advance to PWRITE
    INC DX      ; Advance to PDATA
    IN  AL,DX       ; Read RED value
    MOV [BX],AL     ; Write it
    INC BX      ; Skip to next
    IN  AL,DX       ; Read GREEN byte
    MOV [BX],AL     ; Write it
    INC BX      ; Skip to next
    IN  AL,DX       ; Read BLUE value
    MOV [BX],AL     ; Write it
    RET
;
; Set a palette entry: hrg_setpal(color, setting[3])
;
_hrg_setpal:
    MOV BX,SP       ; Address parms
    MOV DX,PMASK    ; Point to mask register
    MOV AL,0FFh     ; Allow all colors
    OUT DX,AL       ; Write it
    MOV AL,4[BX]    ; Get color
    INC DX      ; Advance to PREAD
    INC DX      ; Advance to PWRITE
    OUT DX,AL       ; Write it
;   OUT DX,AL       ; Possible hware bug!
    MOV BX,2[BX]    ; Get array pointer
    INC DX      ; Advance to PDATA
    MOV AL,[BX]     ; Get RED value
    OUT DX,AL       ; Write to card
    INC BX      ; Skip to next
    MOV AL,[BX]     ; Get GREEN value
    OUT DX,AL       ; Write to card
    INC BX      ; Skip to next
    MOV AL,[BX]     ; Get BLUE value
    OUT DX,AL       ; Write to card
    RET
;
; Wait until the beginning of the next retrace cycle
;
_hrg_retrace:
    MOV DX,VSTATUS  ; Point to status word
ret1:   IN  AL,DX       ; Get status
    AND AL,08h      ; Retrace in progress
    JNZ ret1        ; Yes, wait till finished
ret2:   IN  AL,DX       ; Get status
    AND AL,08h      ; Retrace in progress
    JZ  ret2        ; No, wait for one
    RET
;
; Generate a delay of 'n' retrace intervals (1/60th second)
;
_hrg_delay:
    MOV BX,SP       ; Address stack
    MOV CX,2[BX]    ; Get delay count
    MOV DX,VSTATUS  ; Point to status word
del1:   IN  AL,DX       ; Get status
    AND AL,08h      ; Retrace in progress
    JNZ del1        ; Yes, wait till finished
del2:   IN  AL,DX       ; Get status
    AND AL,08h      ; Retrace in progress
    JZ  del2        ; No, wait for one
    LOOP    del1        ; Wait for them all
    RET
;
; Scaling function for aspect correction: lrg_scale(value, mul, div)
;
_hrg_scale:
    MOV BX,SP       ; Address stack
    MOV AX,6[BX]    ; Get value
    MUL WORD PTR 4[BX]  ; Multiply to 32 bit product
    MOV BX,2[BX]    ; Get divisor
    DIV BX      ; Divide back to 16 bit result
    SHR BX,1        ; /2 for test
    JZ  scale1      ; Special case (/1)
    INC DX      ; .5 rounds up
    SUB BX,DX       ; Set 'C' if remainder > half
    ADC AX,0        ; Increment result to scale
scale1: RET
;
; Plot a single pixel: hrg_plot(x, y, color)
;
_hrg_plot:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to video buffer
    MOV ES,AX       ; Set address
; address = (Y*80)+(x/8)
    MOV AX,4[BX]    ; AX=y
    MOV CL,4        ; *16
    SHL AX,CL       ; AX *= 16
    MOV DI,AX       ; DI = y*16
    SHL AX,1        ; AX *= 32
    SHL AX,1        ; AX *= 64
    ADD DI,AX       ; DI = y * 80
    MOV AX,6[BX]    ; AX = x
    SHR AX,1        ; /2
    SHR AX,1        ; /4
    SHR AX,1        ; /8
    ADD DI,AX       ; DI = (Y*80)+(x/8)
; 3CE=0205
    MOV DX,03CEh    ; Graphic controller address
;   MOV AX,0205h    ; Mode=02
;   OUT DX,AX       ; Write it
; m = 0x80 >> (x & 7)
    MOV CL,6[BX]    ; Get X
    AND CL,07h      ; Modulo 8
    MOV AX,8008h    ; High bit + mask register
    SHR AH,CL       ; Shift it
; 3CE=(m << 8) | 0x08
    OUT DX,AX       ; Write it
; read display[address]
    MOV AL,ES:[DI]  ; Read location
; display[address] = c
    MOV AL,2[BX]    ; Get Color
    MOV ES:[DI],AL  ; Write it
; 3CE=FF08
;   MOV AX,0FF08h
;   OUT DX,AX
; 3CE=1005
;   MOV AX,1005h
;   OUT DX,AX
    RET
;
; Fill video screen with specified color
;
_hrg_fill:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to buffer
    MOV ES,AX       ; Address data
    MOV DX,03CEh    ; Address VGA port
    MOV AX,0FF08h   ; Mask ALL
    OUT DX,AX       ; Write it
    MOV AL,2[BX]    ; Get color
    MOV AH,AL       ; Double for STOW
    MOV CX,38400    ; Size of vbuffer (80*480)
    XOR DI,DI       ; Zero DI
    REP STOSW       ; Fill with color
    RET
;
; Draw a vertical line: hrg_vline(x, y, length, color)
;
_hrg_vline:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to video buffer
    MOV ES,AX       ; Set address
    MOV AX,6[BX]    ; AX=y
    MOV CL,4        ; *16
    SHL AX,CL       ; AX *= 16
    MOV DI,AX       ; DI = y*16
    SHL AX,1        ; AX *= 32
    SHL AX,1        ; AX *= 64
    ADD DI,AX       ; DI = y * 80
    MOV AX,8[BX]    ; AX = x
    SHR AX,1        ; /2
    SHR AX,1        ; /4
    SHR AX,1        ; /8
    ADD DI,AX       ; DI = (Y*80)+(x/8)
    MOV DX,03CEh    ; Graphic controller address
; 3CE = (0x8000 >> (x & 7)) | 0x08
    MOV CL,8[BX]    ; Get X
    AND CL,07h      ; Modulo 8
    MOV AX,8008h    ; High bit + mask register
    SHR AH,CL       ; Shift it
    OUT DX,AX       ; Write it
; Get color & address
    MOV CX,4[BX]    ; Read length
    MOV AL,2[BX]    ; Get Color
; read/write display[address]
vl1:    MOV AH,ES:[DI]  ; Read location
    MOV ES:[DI],AL  ; Write it
    ADD DI,80       ; Skip to next
    LOOP    vl1     ; Do them all
    RET
;
; Draw a horizontal line: hrg_hline(x, y, length, color)
;
_hrg_hline:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to video buffer
    MOV ES,AX       ; Set address
; First, compute starting address
    MOV AX,6[BX]    ; AX=y
    MOV CL,4        ; *16
    SHL AX,CL       ; AX *= 16
    MOV DI,AX       ; DI = y*16
    SHL AX,1        ; AX *= 32
    SHL AX,1        ; AX *= 64
    ADD DI,AX       ; DI = y * 80
    MOV AX,8[BX]    ; AX = x
    SHR AX,1        ; /2
    SHR AX,1        ; /4
    SHR AX,1        ; /8
    ADD DI,AX       ; DI = (Y*80)+(x/8)
    MOV DX,03CEh    ; Graphic controller address
; Compute bit-mask
    MOV AX,8008h    ; High bit + mask register
    MOV SI,4[BX]    ; Get length
    MOV CL,8[BX]    ; Get X
    AND CL,07h      ; Modulo 8
    JZ  vh2     ; We are at a boundary
; Write first section a bit at a time
    SHR AH,CL       ; Shift it
    MOV CL,2[BX]    ; Get color
vh1:    OUT DX,AX       ; Write it
    MOV CH,ES:[DI]  ; Read location
    MOV ES:[DI],CL  ; Write it
    DEC SI      ; Reduce count
    JZ  vh6     ; Done them all
    SHR AH,1        ; Shift it
    JNZ vh1     ; Do all bits
    INC DI      ; Advance to next
; Write x bytes in chunks of 8
vh2:    MOV AH,0FFh     ; Enable all bits
    OUT DX,AX       ; Write it
    MOV CL,2[BX]    ; Get color
vh3:    CMP SI,8        ; More than 8 left?
    JB  vh4     ; No, stop now
    MOV ES:[DI],CL  ; Write it
    INC DI      ; Advance to next
    SUB SI,8        ; Reduce count
    JNZ vh3     ; Do them all
    RET
; Write last section a bit at a time
vh4:    MOV AH,80h      ; Start at first bit
vh5:    OUT DX,AX       ; Set mask
    MOV CH,ES:[DI]  ; Read memory
    MOV ES:[DI],CL  ; Write it
    SHR AH,1        ; Next bit
    DEC SI      ; Reduce count
    JNZ vh5     ; Do them all
vh6:    RET
CSEG    ENDS
;
    END
