;
; DDS MICRO-C 8086/MS-DOS: Segment register manipulation
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _get_cs, _get_ds, _get_es, _set_es, _copy_seg
;
; Get the 80X86 segment registers: get_xx()
;
_get_cs: MOV    AX,CS       ; Get the CODE segment
        RET
_get_ds: MOV    AX,DS       ; Get the DATA segment
        RET
_get_es: MOV    AX,ES       ; Get the EXTRA segment
        RET
;
; Set the 80X86 EXTRA segment register: set_es(value)
;
_set_es: MOV    BX,SP       ; Address parameters
        MOV     ES,2[BX]    ; Set extra segment
        RET
;
; Inter segment copy: copy_seg(toseg, tooffset, fromseg, fromoffset, length)
;
_copy_seg: MOV  BX,SP       ; Address parameters
        PUSH    DS          ; Save data segment
        MOV     ES,10[BX]   ; Get destination segment
        MOV     DI,8[BX]    ; Get destination offset
        MOV     SI,4[BX]    ; Get source offset
        MOV     CX,2[BX]    ; Get count value
        MOV     DS,6[BX]    ; Get source segment
    REP MOVSB               ; Perform the move
        POP     DS          ; Restore data segment
        RET
;
CSEG    ENDS
        END
