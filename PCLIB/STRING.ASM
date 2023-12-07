;
; DDS MICRO-C 8086/MS-DOS: String copy/concatinate
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _strcat, _strcpy
;
; Concatinate one string to another: strcat(dest, source)
;
_strcat:MOV     BX,SP           ; Address parameters
        MOV     DI,4[BX]        ; Get dest
scat1:  MOV     AL,[DI]         ; Get char from dest
        AND     AL,AL           ; End of the line?
        JZ      scpy0           ; Copy in source
        INC     DI              ; Advance
        JMP SHORT scat1         ; And keep looking
;
; Copy one string to another: strcpy(dest, source)
;
_strcpy:MOV     BX,SP           ; Address parameters
        MOV     DI,4[BX]        ; Get dest
scpy0:  MOV     SI,2[BX]        ; Get source
scpy1:  MOV     AL,[SI]         ; Get from source
        INC     SI              ; Advance
        MOV     [DI],AL         ; Write to dest
        INC     DI              ; Advance
        AND     AL,AL           ; End of string?
        JNZ     scpy1           ; No, keep going
        MOV     AX,4[BX]        ; Get dest back
        RET
;
CSEG    ENDS
        END
