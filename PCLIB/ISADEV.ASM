;
; DDS MICRO-C 8086/MS-DOS: Test for ANY device
;
; ?COPY.TXT 1995-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _isadev
;
; Test for ANY device
;
_isadev:MOV     BX,SP           ; Address parms
        MOV     BX,2[BX]        ; Get file handle
        MOV     AX,4400h        ; IOCTL / Function 00
        INT     21h             ; Ask DOS
        MOV     AX,0            ; Assume false
        JC      nodev           ; Error - return FALSE
        SHL     DL,1            ; Set CY flag if device
        RCL     AL,1            ; Acc=1 if device
nodev:  RET
;
CSEG    ENDS
        END
