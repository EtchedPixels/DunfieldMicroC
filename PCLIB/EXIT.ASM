;
; DDS MICRO-C 8086/MS-DOS: Terminate function
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _exit
;
; Terminate Execution with return code: exit(int rc)
;
_exit:  POP     AX          ; Trash return address
        POP     AX          ; Get return code
        MOV     AH,4ch      ; DOS EXIT Function
        INT     21h         ; Terminate
;
CSEG    ENDS
        END
