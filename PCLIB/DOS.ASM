;
; DDS MICRO-C 8086/MS-DOS: Misc DOS functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _version, _cbreak
;
; Get DOS version: version()
;
_version: MOV   AX,3000h        ; Get version function code
        INT     21h             ; Ask DOS
        XCHG    AH,AL           ; Swap highest into AH
        RET
;
; Set/Clear breakability: cbreak(allow_break)
;
_cbreak:MOV     BX,SP           ; Address parms
        MOV     AX,3300h        ; Get break status function
        MOV     DX,2[BX]        ; Get breakable flag
        AND     DH,DH           ; Test upper byte
        JNZ     brk1            ; Get break
        INC     AL              ; Set break
brk1:   INT     21h             ; Ask DOS
        XOR     AH,AH           ; Zero high
        MOV     AL,DL           ; Get result
        RET
;
CSEG    ENDS
        END
