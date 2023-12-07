;
; DDS MICRO-C 8086/MS-DOS: Interrupt Enable & Disable
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _enable, _disable
;
; Enable 8086 interrupts
;
_enable: STI                ; Enable interrupts
        RET
;
; Disable 8086 interrupts
;
_disable: CLI               ; Disable interrupts
        RET
;
CSEG    ENDS
        END
