;
; DDS MICRO-C 8086/MS-DOS: Low Level REWIND and SEEK I/O functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _lrewind, _lseek, _ltell
;
; Rewind a file to beginning: lrewind(handle)
;
_lrewind: MOV   BX,SP           ; Address parms
        MOV     BX,2[BX]        ; Get handle
        XOR     CX,CX           ; Zero high word of offset
        XOR     DX,DX           ; Zero low word of offset
        XOR     AL,AL           ; Indicate from start of file
        JMP SHORT lseek1        ; and proceed
;
; Seek position within a file: lseek(handle, high_offset, low_offset, mode)
;
_lseek: MOV     BX,SP           ; Address parms
        MOV     CX,6[BX]        ; Get high word of offset
        MOV     DX,4[BX]        ; Get low word of offset
        MOV     AL,2[BX]        ; Get seek mode
        MOV     BX,8[BX]        ; Get file handle
lseek1: MOV     AH,42h          ; Set file pointer function
        INT     21h             ; Ask DOS
        JC      lseek2          ; Error, return with code
        XOR     AX,AX           ; Success, return zero
lseek2: RET
;
; Determine position in file: ltell(handle, &high_offset, &low_offset)
;
_ltell: PUSH    BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parms
        MOV     BX,8[BP]        ; Get file handle
        XOR     CX,CX           ; Zero high offset
        XOR     DX,DX           ; Zero low offset
        MOV     AX,4201h        ; Seek relative
        INT     21h             ; Ask DOS
        JC      ltell1          ; Failed, report error
        MOV     BX,6[BP]        ; Get address of high_offset
        MOV     [BX],DX         ; Write high offset
        MOV     BX,4[BP]        ; Get address of low_offset
        MOV     [BX],AX         ; Write low offset
        XOR     AX,AX           ; Zero AX, success
ltell1: POP     BP              ; Restore caller
        RET
;
CSEG    ENDS
        END
