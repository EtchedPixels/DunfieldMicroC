;
; DDS MICRO-C 8086/MS-DOS: Long jump routines
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _setjmp, _longjmp
;
; Set a long jump: setjmp(int environment[3])
;
_setjmp:POP     CX          ; CX = return address
        MOV     BX,SP       ; Address stack
        MOV     SI,[BX]     ; Ptr to save area
        MOV     [SI],SP     ; Save current stack
        MOV     2[SI],BP    ; Save callers stack frame
        MOV     4[SI],CX    ; Save return address
        XOR     AX,AX       ; Zero return code
        JMP     CX          ; Return to caller
;
; Perform a long jump: longjmp(int environment[3], int rvalue)
;
_longjmp:MOV    BX,SP       ; Address parmeters
        MOV     SI,4[BX]    ; Ptr to save area
        MOV     AX,2[BX]    ; Get return value
        MOV     SP,[SI]     ; Restore stack
        MOV     BP,2[SI]    ; Restore base ptr
        JMP     4[SI]       ; Return to caller
;
CSEG    ENDS
        END
