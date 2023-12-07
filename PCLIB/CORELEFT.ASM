;
; DDS MICRO-C 8086/MS-DOS: Determine the amount of free memory remaining
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _coreleft
        EXTRN   ?heap:near
; Define a protective margin above the heap.
MARGIN  =       100         ; Stack protect margin
;
; Determine amount of free memory remaining in the near heap
;
_coreleft:
; First, locate the end of allocated heap memory
        MOV     SI,OFFSET DGRP:?heap; At beginning
corel1: MOV     AL,[SI]     ; Get flag byte
        AND     AL,AL       ; End of list?
        JZ      corel2      ; Yes, we found it
        MOV     BX,1[SI]    ; Get size of block
        LEA     SI,3[BX+SI] ; Advance to next block
        JMP SHORT corel1    ; And keep looking
corel2: ADD     SI,MARGIN   ; Offset for protective margin
        MOV     AX,SP       ; Lowest address on stack
        SUB     AX,SI       ; Highest address on heap
        RET
;
CSEG    ENDS
        END
