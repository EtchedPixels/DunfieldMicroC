;
; DDS MICRO-C 8086/MS-DOS: Handle duplication functions
;
; ?COPY.TXT 1995-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _dup, _dup2
;
; Duplicate a handle: dup(old)
;
_dup:   MOV     BX,SP       ; Address parms
        MOV     BX,2[BX]    ; Get handle to redirect
        MOV     AH,45h      ; Duplicate handle function
        INT     21h         ; Ask DOS
        JNC     dup1        ; OK
        XOR     AX,AX       ; Indicate failure
dup1:   RET
;
; Redirect a handle: dup2(old, new)
;
_dup2:  MOV     BX,SP       ; Address parms
        MOV     CX,2[BX]    ; Get handle to redirect
        MOV     BX,4[BX]    ; Get file handle
        MOV     AH,46h      ; Redirect handle function
        INT     21h         ; Ask DOS
        JC      dup1        ; No error
        XOR     AX,AX       ; Return OK
        RET
;
CSEG    ENDS
        END
