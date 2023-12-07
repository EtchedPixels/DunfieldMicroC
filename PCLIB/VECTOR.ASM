;
; DDS MICRO-C 8086/MS-DOS: Dos Get/Set interrupt vector
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _get_vector, _set_vector
;
; Get interrupt vector: get_vector(interrupt, &segment, &offset)
;
_get_vector:
        MOV     BX,SP           ; Address parameters
        MOV     AL,6[BX]        ; Get interrupt number
        MOV     SI,4[BX]        ; Get segment pointer
        MOV     DI,2[BX]        ; Get offset pointer
        MOV     AH,35h          ; Get vector function
        INT     21h             ; Ask DOS
        MOV     [SI],ES         ; Save segment
        MOV     [DI],BX         ; Save offset
        RET
;
; Set interrupt vector: set_vector(interrupt, segment, offset)
;
_set_vector:
        MOV     BX,SP           ; Address parameters
        MOV     AL,6[BX]        ; Get interrupt number
        MOV     DX,2[BX]        ; Get offset
        MOV     AH,25h          ; Set vector function
        PUSH    DS              ; Save our segment
        MOV     DS,4[BX]        ; Get segment
        INT     21h             ; Ask DOS
        POP     DS              ; Restore DS
        RET
;
CSEG    ENDS
        END
