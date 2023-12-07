;
; DDS MICRO-C 8086/MS-DOS: Block I/O functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        EXTRN   ?getc:NEAR, ?putc:NEAR
        PUBLIC  _fget, _fput
;
; Read a block from a file: fget(buffer, size, fp)
;
_fget:  PUSH    BP              ; Save caller
        MOV     BP,SP           ; Address parms
        MOV     SI,4[BP]        ; Get FP
        MOV     DI,8[BP]        ; Get BLOCK
        XOR     CX,CX           ; Zero count
fget1:  PUSH    CX              ; Save count
        CALL    ?getc           ; Get a character
        POP     CX              ; Restore
        AND     AH,AH           ; End of file?
        JNZ     fget2           ; Yes, terminate early
        MOV     [DI],AL         ; Write it
        INC     DI              ; Advance to next
        INC     CX              ; Reduce count
        CMP     CX,6[BP]        ; At end?
        JB      fget1           ; do them all
fget2:  MOV     AX,CX           ; Return count
        POP     BP              ; Restore caller
        RET
;
; Write a block to a file: fput(buffer, size, fp)
;
_fput:  PUSH    BP              ; Save caller
        MOV     BP,SP           ; Address parms
        MOV     SI,4[BP]        ; Get FP
        MOV     DI,8[BP]        ; Get BLOCK
        XOR     CX,CX           ; Zero count
        XOR     AH,AH           ; Zero high
fput1:  MOV     AL,[DI]         ; Get next char
        PUSH    CX              ; Save count
        CALL    ?putc           ; Write the character
        POP     CX              ; Restore count
        AND     AH,AH           ; Error?
        JNZ     fput2           ; Yes, quit
        INC     DI              ; Advance to next
        INC     CX              ; Advance count
        CMP     CX,6[BP]        ; At end?
        JB      fput1           ; No, keep going
fput2:  MOV     AX,CX           ; Return count
        POP     BP              ; Restore caller
        RET
;
CSEG    ENDS
;
        END
