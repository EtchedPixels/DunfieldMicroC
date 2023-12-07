;
; Wait for a delay, by watching the BIOS tick counter.
;
; ?COPY.TXT 1994-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _delay
; delay(milliseconds)
_delay: MOV     BX,SP           ; Address parms
        MOV     AX,2[BX]        ; Get count
        XOR     DX,DX           ; Zero High
        ADD     AX,27           ; Adjust for averaging
        MOV     BX,55           ; Milliseconds/tick
        DIV     BX              ; Compute ticks
        AND     AX,AX           ; Less that one tick?
        JZ      del2            ; Yes, ignore
        MOV     CX,AX           ; Save loop count
        MOV     AX,0040h        ; BIOS Data segment
        MOV     ES,AX           ; Set up pointer
        MOV     BX,006Ch        ; Tick count offset
        MOV     DX,ES:[BX]      ; Save initial count
del1:   MOV     AX,ES:[BX]      ; Get current count
        CMP     AX,DX           ; Changed?
        JE      del1            ; No, it hasn't
        MOV     DX,AX           ; Save new count
        LOOP    del1            ; Wait till changed
del2:   RET
CSEG    ENDS
        END
