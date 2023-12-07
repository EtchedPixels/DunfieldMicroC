;
; DDS MICRO-C 8086/MS-DOS: Low Level character READ routines
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC _lgetc, _lgets
;
; Get a character from a file: lgetc(handle)
;
_lgetc: PUSH    BP          ; Save Base
        MOV     BP,SP       ; Set up stack addressability
        MOV     BX,4[BP]    ; Get file handle
lgetc1: LEA     DX,4[BP]    ; Temporary buffer
        MOV     CX,1        ; Read one character
        MOV     AH,3Fh      ; DOS READ Function
        INT     21h         ; Read file
        JC      lgetc2      ; Error
        AND     AX,AX       ; End of file?
        JZ      lgetc2      ; Yes, indicate it
        MOV     AL,4[BP]    ; Get character
        CMP     AL,0Dh      ; Carriage return?
        JZ      lgetc1      ; Yes, ignore
        CMP     AL,1Ah      ; EOF marker
        JZ      lgetc2      ; Yes, indicate it
        SUB     AH,AH       ; Get zero high
        POP     BP          ; Restore Base
        RET
lgetc2: MOV     AX,-1       ; Indicate Error
        POP     BP          ; Restore Base
        RET
;
; Get a string from a file: lgets(char *string, int size, handle)
;
_lgets: PUSH    BP          ; Save base
        MOV     BP,SP       ; Set up stack addressability
        MOV     DI,8[BP]    ; Get destination pointer
        MOV     SI,6[BP]    ; Get input count
        MOV     BX,4[BP]    ; Get file handle
        MOV     CX,1        ; Read only 1 byte
; Read input chars till count expired or end of line
lgets1: DEC     SI          ; Reduce count
        JZ      lgets3      ; Expired
        MOV     DX,DI       ; DX = destination pointer
        MOV     AH,3Fh      ; Read file function
        INT     21h         ; Ask DOS
        JC      lgets2      ; ERROR, check for partial line
        AND     AX,AX       ; Did we read any characters?
        JZ      lgets2      ; EOF, check for partial line
; Char received, exit if end of line
        MOV     AL,[DI]     ; Get char
        CMP     AL,0Dh      ; Yes, ignore
        JZ      lgets1      ; Ignore carriage return
        CMP     AL,0Ah      ; Newline?
        JZ      lgets3      ; Yes, exit
        INC     DI          ; Advance to next
        JMP SHORT lgets1    ; Get next char
; End of input, return EOF if no partial line
lgets2: CMP     DI,8[BP]    ; At beginning of string?
        JNZ     lgets3      ; No, exit with line
        MOV     WORD PTR 8[BP],0; Set dest to zero (EOF flag)
; Return to caller with address (0 for EOF)
lgets3: MOV     BYTE PTR [DI],0; Zero terminate input line
        MOV     AX,8[BP]    ; Get dest pointer
        MOV     SP,BP       ; Clean up stack
        POP     BP          ; Restore base
        RET
;
CSEG    ENDS
        END
