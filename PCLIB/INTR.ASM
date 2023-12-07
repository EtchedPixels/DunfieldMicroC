;
; Low level 8086 interrupt function for use with the DDS MICRO-C compiler.
;
; ?COPY.TXT 1995-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
;
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _intr
;
; Generate an 8086 interrupt: int86(number)
;
_intr:  PUSH    BP              ; Save callers stack frame
        MOV     BP,SP           ; Address function args
        MOV     AX,6[BP]        ; Get interrupt number
        MOV     CSEG:BYTE PTR INT+1,AL; Patch interrupt number
        MOV     BX,4[BP]        ; Get register structure
        MOV     CX,4[BX]        ; Pass CS
        MOV     DX,6[BX]        ; Pass DS
        MOV     BP,8[BX]        ; Pass BP
        MOV     SI,10[BX]       ; Pass SI
        MOV     DI,12[BX]       ; Pass DI
        MOV     ES,16[BX]       ; Pass ES
        TEST    AH,01h          ; Set flags?
        JZ      intr1           ; No, don't set
        PUSH    WORD PTR 18[BX] ; Save flags
        POPF                    ; Pass flags
intr1:  MOV     AX,[BX]         ; Pass AX
        PUSH    WORD PTR 14[BX] ; Save DS
        MOV     BX,2[BX]        ; Pass BX
        POP     DS              ; Pass DS
        MOV     CSEG:savSP,SP   ; Save SP
        MOV     CSEG:savSS,SS   ; Save SS
        JMP SHORT INT           ; Flush cache
; Code segment variables
savSP   DW      0               ; Saved SP
savSS   DW      0               ; Saved SS
INT:    INT     0               ; Generate interupt
; Pick up pieces after interrupt
        CLI                     ; Inhibit interrupt
        MOV     SP,CSEG:savSP   ; Restore SP
        MOV     SS,CSEG:savSS   ; Restore SS
        STI                     ; Allow interrupts
        PUSHF                   ; Save flags
        PUSH    DS              ; Save data seg
        PUSH    BX              ; Save pointer
        MOV     DS,CSEG:savSS   ; Dataseg and stack are always same
        MOV     BX,SP           ; Address stack
        MOV     BX,10[BX]       ; Get register struct
        MOV     [BX],AX         ; Save AX
        POP     WORD PTR 2[BX]  ; Save BX
        MOV     4[BX],CX        ; Save CX
        MOV     6[BX],DX        ; Save DX
        MOV     8[BX],BP        ; Save BP
        MOV     10[BX],SI       ; Save SI
        MOV     12[BX],DI       ; Save DI
        POP     WORD PTR 14[BX] ; Save DS
        MOV     16[BX],ES       ; Save ES
        POP     WORD PTR 18[BX] ; Save flags
        POP     BP              ; Restore caller
        RET
;
CSEG    ENDS
        END
