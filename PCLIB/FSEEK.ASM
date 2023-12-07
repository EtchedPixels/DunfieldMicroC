;
; DDS MICRO-C 8086/MS-DOS: File positioning functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
READ    =       00000001b   ; File is opened for READ
WRITE   =       00000010b   ; File is opened for WRITE
APPEND  =       00000100b   ; File is opened for APPEND
BINARY  =       00010000b   ; File is opened for BINARY
FERROR  =       00100000b   ; An ERROR has occured
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _rewind, _fseek, _ftell
        EXTRN   _fflush:NEAR
;
; Rewind a file to beginning: rewind(fp)
;
_rewind: MOV    BX,SP           ; Address parms
        MOV     SI,2[BX]        ; Get file structure
        AND     BYTE PTR [SI],not FERROR; reset error flag
        XOR     CX,CX           ; Zero high word of offset
        XOR     DX,DX           ; Zero low word of offset
        XOR     AL,AL           ; Indicate from start of file
        JMP SHORT fseek1        ; and proceed
;
; Seek position within a file: fseek(fp, high_offset, low_offset, mode)
;
_fseek: MOV     BX,SP           ; Address parms
        MOV     SI,8[BX]        ; Get file structure
        MOV     CX,6[BX]        ; Get high word of offset
        MOV     DX,4[BX]        ; Get low word of offset
        MOV     AL,2[BX]        ; Get seek mode
        CMP     AL,1            ; Relative to current position
        JNE     fseek1          ; No, no adjustments necessary
        TEST BYTE PTR[SI],READ  ; Reading from file?
        JZ      fseek1          ; No, leave it alone
; Reading file, adjust by # chars left in buffer
        MOV     BX,5[SI]        ; Get size of buffer
        SUB     BX,7[SI]        ; Calculate remainder
        SUB     DX,BX           ; Adjust low offset
        SBB     CX,0            ; Adjust high if borrow
fseek1: PUSH    AX              ; Save mode
        PUSH    CX              ; Save high offset
        PUSH    DX              ; Save low offset
        PUSH    SI              ; Parameter is file structure
        CALL    _fflush         ; Flush it
        POP     SI              ; Restore file pointer
        POP     DX              ; Restore low offset
        POP     CX              ; Restore high offset
        POP     AX              ; Restore mode
        MOV     BX,1[SI]        ; Get file handle
        MOV     AH,42h          ; Set file pointer function
        INT     21h             ; Ask DOS
        JC      fseek2          ; Error, return with code
        XOR     AX,AX           ; Success, return zero
fseek2: RET
;
; Determine position in file: ftell(fp, &high_offset, &low_offset)
;
_ftell: PUSH    BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parms
        MOV     SI,8[BP]        ; Get file structure
        MOV     BX,1[SI]        ; Get file handle
        XOR     CX,CX           ; Zero high offset
        XOR     DX,DX           ; Zero low offset
        MOV     AX,4201h        ; Seek relative
        INT     21h             ; Ask DOS
        JC      ftell3          ; Failed, report error
        TEST BYTE PTR[SI],WRITE ; Open for write?
        JNZ     ftell1          ; Yes, special case
; Reading file, subtract any chars left in buffer from position
        MOV     CX,5[SI]        ; Get top of buffer
        SUB     CX,7[SI]        ; Calculate # chars remaining
        SUB     AX,CX           ; Decrese by that amount
        SBB     DX,0            ; Include borrow
        JMP SHORT ftell2        ; And proceed
; Writing file, add characters in buffer to position
ftell1: ADD     AX,7[SI]        ; Offset to pointer
        ADC     DX,0            ; Include overflow
ftell2: MOV     BX,6[BP]        ; Get address of high_offset
        MOV     [BX],DX         ; Write high offset
        MOV     BX,4[BP]        ; Get address of low_offset
        MOV     [BX],AX         ; Write low offset
        XOR     AX,AX           ; Zero AX, success
ftell3: POP     BP              ; Restore caller
        RET
;
CSEG    ENDS
        END
