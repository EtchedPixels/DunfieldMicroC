;
; DDS MICRO-C 8086/MS-DOS: Direct keyboard functions
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _kbget, _kbhit, _kbtst
;
; Test for a key from the keyboard and retreive it
;
_kbtst: MOV     AH,01h      ; Key status
        INT     16h         ; Call BIOS
        JZ      ret0        ; None available
;
; Get a key directly from the keyboard
;
_kbget: XOR     AH,AH       ; Get key
        INT     16h         ; Call BIOS
doret:  AND     AL,AL       ; ASCII code available?
        JZ      doret1      ; No, return entire scancode
        XOR     AH,AH       ; Zero high (return ASCII only)
doret1: AND     AX,AX       ; Ctrl-Break (Scancode = 0)?
        JNZ     doret2      ; No, its OK
        DEC     AX          ; Translate to -1
doret2: RET
;
; Test for a character from the keyboard
;
_kbhit: MOV     AH,01h      ; Key status
        INT     16h         ; Ask BIOS
        JNZ     doret1      ; We have a key
ret0:   XOR     AX,AX       ; Get zero
        RET
;
CSEG    ENDS
        END
