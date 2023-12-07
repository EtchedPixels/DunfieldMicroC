;
; DDS MICRO-C 8086/MS-DOS: Get/Set current drive
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _get_drive, _set_drive
;
; Get current disk drive: get_drive()
;
_get_drive: MOV AH,19h          ; Get disk function
        INT     21h             ; Ask DOS
        XOR     AH,AH           ; Zero high
        RET
;
; Set current disk drive: set_drive(drive)
;
_set_drive: MOV BX,SP           ; Address parameters
        MOV     DL,2[BX]        ; Get drive ID
        MOV     AH,0Eh          ; Select disk function
        INT     21h             ; Ask DOS
        XOR     AH,AH           ; Zero high
        RET
;
CSEG    ENDS
        END
