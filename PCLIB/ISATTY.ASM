;
; DDS MICRO-C 8086/MS-DOS: Test for TTY device
;
; ?COPY.TXT 1995-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _isatty
;
; Test for TTY device
;
_isatty:MOV     BX,SP           ; Address parms
        MOV     BX,2[BX]        ; Get file handle
        MOV     AX,4400h        ; IOCTL / Function 00
        INT     21h             ; Ask DOS
        MOV     AX,0            ; Assume false
        JC      notty           ; Error - return FALSE
        TEST    DL,0Ch          ; NUL or CLOCK?
        JNZ     notty           ; Not a TTY device
        SHL     DL,1            ; Set CY flag if device
        RCL     AL,1            ; Acc=1 if device
notty:  RET
;
CSEG    ENDS
        END
