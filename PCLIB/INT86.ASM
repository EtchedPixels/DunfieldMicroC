;
; Low level 8086 interrupt function for use with the DDS MICRO-C compiler.
;
; ?COPY.TXT 1992-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
        PUBLIC  __AX_, __BX_, __CX_, __DX_, __SI_, __DI_
;
; Initialized register variables
;
__AX_   DW      0               ; Storage for AX
__BX_   DW      0               ; Storage for BX
__CX_   DW      0               ; Storage for CX
__DX_   DW      0               ; Storage for DX
__SI_   DW      0               ; Storage for SI
__DI_   DW      0               ; Storage for DI
;
DSEG    ENDS
;
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _int86
;
; Generate an 8086 interrupt: int86(number)
;
_int86: MOV     BX,SP           ; Address stack
        MOV     AX,2[BX]        ; Get parameter
        MOV     CS:BYTE PTR INT+1,AL; Patch interrupt number
        MOV     AX,DS           ; Get DS
        MOV     ES,AX           ; Set ES
        MOV     AX,DGRP:__AX_   ; Set up AX
        MOV     BX,DGRP:__BX_   ; Set up BX
        MOV     CX,DGRP:__CX_   ; Set up CX
        MOV     DX,DGRP:__DX_   ; Set up DX
        MOV     SI,DGRP:__SI_   ; Set up SI
        MOV     DI,DGRP:__DI_   ; Set up DI
        JMP SHORT INT           ; Flush cache
INT:    INT     0               ; Execute interrupt
        PUSHF                   ; Save FLAGS
        MOV     DGRP:__AX_,AX   ; Save new AX
        MOV     DGRP:__BX_,BX   ; Save new BX
        MOV     DGRP:__CX_,CX   ; Save new CX
        MOV     DGRP:__DX_,DX   ; Save new DX
        MOV     DGRP:__SI_,SI   ; Save new SI
        MOV     DGRP:__DI_,DI   ; Save new DI
        POP     AX              ; Return FLAGS
        RET
;
CSEG    ENDS
        END
