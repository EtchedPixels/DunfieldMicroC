;
; Low Resolution Graphics (LRG) library
;
; Low level functions
;
; ?COPY.TXT 1998-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG,BSEG
VBUFFER EQU 0A000h      ; VGA buffer address
VWIDTH  EQU 320     ; Width of a single line
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
    PUBLIC  _lrg_open, _lrg_close, _lrg_getpal, _lrg_setpal
    PUBLIC  _lrg_fill, _lrg_plot, _lrg_hline, _lrg_vline
    PUBLIC  _lrg_blit, _lrg_draw, _lrg_erase, _lrg_scale
    PUBLIC  _lrg_retrace, _lrg_delay
;
; Initialize video system & place in mode 19 (if applicable)
;
_lrg_open:
; First, determine current mode for later
    MOV AH,0Fh      ; Get video mode function
    INT 10h     ; Call BIOS
    MOV DGRP:vmode,AL   ; Save video mode
; Check for VGA present
    MOV AX,1A00h    ; Get display code
    INT 10h     ; Call BIOS
    CMP AL,1Ah      ; VGA supported?
    JNZ initv2      ; No, report not present
    MOV AL,13h      ; Mode 13 (320 x 200)
initv1: XOR AH,AH       ; Set mode request
    INT 10h     ; Call BIOS
    XOR AX,AX       ; Indicate success
initv2: RET
;
; Return to previous video mode
;
_lrg_close:
    MOV AL,DGRP:vmode   ; Get old video mode
    JMP SHORT initv1    ; And set mode
;
; Get a palette entry: lrg_getpal(color, setting[3])
;
_lrg_getpal:
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
; Set a palette entry: lrg_setpal(color, setting[3])
;
_lrg_setpal:
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
; Fill video buffer with a single color: lrg_fill(color)
;
_lrg_fill:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to segment
    MOV ES,AX       ; Set buffer
    MOV AL,2[BX]    ; Get color
    MOV AH,AL       ; Double for STOW
    MOV CX,32000    ; Get size of buffer (words)
    XOR DI,DI       ; Start at beginning
    REP STOSW       ; Fill the buffer
    RET
;
; Plot a single pixel: lrg_plot(x, y, color)
;
_lrg_plot:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to video buffer
    MOV ES,AX       ; Set address
    MOV AH,4[BX]    ; AX = Y * 256
    MOV DI,AX       ; Save copy for later
    SHR AX,1        ; AX = Y * 128
    SHR AX,1        ; AX = Y * 64
    ADD AX,6[BX]    ; Add in X position
    ADD DI,AX       ; DI = (Y * 320) + X
    MOV AL,2[BX]    ; Get color
    MOV ES:[DI],AL  ; Point to video buffer
    RET
;
; Draw a horizontal line: lrg_hline(x, y, length, color)
;
_lrg_hline:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to video buffer
    MOV ES,AX       ; Set address
    MOV AH,6[BX]    ; AX = Y * 256
    MOV DI,AX       ; Save for later
    SHR AX,1        ; AX = Y * 128
    SHR AX,1        ; AX = Y * 64
    ADD AX,8[BX]    ; Add in X
    ADD DI,AX       ; DI = (Y * 320) + X
    MOV CX,4[BX]    ; Get length
    MOV AL,2[BX]    ; Get color
    REP STOSB       ; Write it
    RET
;
; Draw a vertical line: lrg_vline(x, y, length, color)
;
_lrg_vline:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to video buffer
    MOV ES,AX       ; Set address
    MOV AH,6[BX]    ; AX = Y * 256
    MOV DI,AX       ; Save for later
    SHR AX,1        ; AX = Y * 128
    SHR AX,1        ; AX = Y * 64
    ADD AX,8[BX]    ; Add in X
    ADD DI,AX       ; DI = (Y * 320) + X
    MOV CX,4[BX]    ; Get length
    MOV AL,2[BX]    ; Get color
vline1: MOV ES:[DI],AL  ; Write color
    ADD DI,VWIDTH   ; Offset to next byte
    LOOP    vline1      ; Write all bytes
    RET
;
; Block image transfer: lrg_blit(x, y, xs, ys, image)
;
_lrg_blit:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to video buffer
    MOV ES,AX       ; Set address
    MOV AH,8[BX]    ; AX = Y * 256
    MOV DI,AX       ; Save for later
    SHR AX,1        ; AX = Y * 128
    SHR AX,1        ; AX = Y * 64
    ADD AX,10[BX]   ; Add in X offset
    ADD DI,AX       ; DI = (Y * 320) + AX
    MOV SI,2[BX]    ; Get image address
    MOV DX,4[BX]    ; Get Y count
    MOV AX,320      ; Get screen width
    SUB AX,6[BX]    ; Compute adjustment
blit1:  MOV CX,6[BX]    ; Get X count
    REP MOVSB           ; Move a line
    ADD DI,AX       ; Next line
    DEC DL      ; More lines?
    JNZ blit1       ; Copy them all
    RET
;
; Draw a sprite: lrg_draw(x, y, sprite, save)
;
_lrg_draw:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to video buffer
    MOV ES,AX       ; Set address
    MOV AH,6[BX]    ; AX = Y * 256
    MOV DI,AX       ; Save for later
    SHR AX,1        ; AX = Y * 128
    SHR AX,1        ; AX = Y * 64
    ADD AX,8[BX]    ; Add in X offset
    ADD DI,AX       ; DI = (Y * 320) + AX
    MOV SI,2[BX]    ; Get save area
    MOV BX,4[BX]    ; Get sprite
    MOV CX,[BX]     ; Get X/Y size (CL=X, CY=Y)
    ADD BX,2        ; Skip to color data
    MOV DL,CL       ; Save X count
dsp1:   PUSH    DI
dsp2:   MOV AL,[BX]     ; Get new data from sprite
    AND AL,AL       ; Transparent?
    JZ  dsp3        ; Yes, do nothing
    MOV AH,ES:[DI]  ; Get old data from screen
    MOV ES:[DI],AL  ; Write new data to screen
    MOV [SI],AH     ; Write to save area
    INC SI      ; Skip to next in save area
dsp3:   INC DI      ; Skip to next in screen
    INC BX      ; Skip to next in sprite
    DEC CL      ; Reduce X count
    JNZ dsp2        ; Do them all
    MOV CL,DL       ; Restore count
    POP DI      ; Restore pointer
    ADD DI,320      ; Set it up
    DEC CH      ; Reduce Y count
    JNZ dsp1        ; Do them all
    RET
;
; Erase a sprite: lrg_erase(x, y, sprite, save)
;
_lrg_erase:
    MOV BX,SP       ; Address parms
    MOV AX,VBUFFER  ; Point to video buffer
    MOV ES,AX       ; Set address
    MOV AH,6[BX]    ; AX = Y * 256
    MOV DI,AX       ; Save for later
    SHR AX,1        ; AX = Y * 128
    SHR AX,1        ; AX = Y * 64
    ADD AX,8[BX]    ; Add in X offset
    ADD DI,AX       ; DI = (Y * 320) + AX
    MOV SI,2[BX]    ; Get save area
    MOV BX,4[BX]    ; Get sprite
    MOV CX,[BX]     ; Get X/Y size (CL=X, CY=Y)
    ADD BX,2        ; Skip to color data
    MOV AH,CL       ; Save X count
    MOV DX,DI       ; Save screen pointer
esp1:   MOV AL,[BX]     ; Get data from sprite
    AND AL,AL       ; Transparent?
    JZ  esp2        ; Yes, do not restore
    MOV AL,[SI]     ; Get old data from save area
    MOV ES:[DI],AL  ; Write back to screen
    INC SI      ; Skip to next in save area
esp2:   INC DI      ; Skip to next in screen
    INC BX      ; Skip to next in sprite
    DEC CL      ; Reduce X count
    JNZ esp1        ; Do them all
    MOV CL,AH       ; Restore count
    ADD DX,320      ; Offset to next line
    MOV DI,DX       ; Set new pointer
    DEC CH      ; Reduce Y count
    JNZ esp1        ; Do them all
    RET
;
; Scaling function for aspect correction: lrg_scale(value, mul, div)
;
_lrg_scale:
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
; Wait until the beginning of the next retrace cycle
;
_lrg_retrace:
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
_lrg_delay:
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
CSEG    ENDS
;
    END
