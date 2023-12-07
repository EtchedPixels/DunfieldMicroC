;
; DDS MICRO-C 8086/MS-DOS: File lookup routines
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _findfirst, _findnext
;
; Find first file: findfirst(path, struct, attrs)
;
_findfirst:MOV  BX,SP       ; Address parameters
        MOV     DX,4[BX]    ; Get pointer
        MOV     AH,1Ah      ; Set DTA
        INT     21h         ; Call DOS
        MOV     CX,2[BX]    ; Get search attributes
        MOV     DX,6[BX]    ; Get filename
        MOV     AH,4Eh      ; Find First function
        JMP SHORT DOFIND    ; And continue
;
; Find next file: findnext(struct)
;
_findnext:MOV   BX,SP       ; Address parameters
        MOV     DX,2[BX]    ; Get pointer
        MOV     AH,1Ah      ; Set DTA
        INT     21h         ; Call DOS
        MOV     AH,4Fh      ; Find Next function
DOFIND: INT     21h         ; Ask DOS
        JC      ERROR       ; Error occured
; Return ZERO to indicate success
        XOR     AX,AX       ; Zero return code
ERROR:  RET
;
CSEG    ENDS
        END
