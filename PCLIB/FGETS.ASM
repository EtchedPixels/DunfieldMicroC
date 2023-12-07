;
; DDS MICRO-C 8086/MS-DOS: String I/O functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        EXTRN   ?putc:NEAR, ?getc:NEAR
        PUBLIC  _fputs, _fgets
;
; Read a string from a file: fgets(buffer, size, fp)
;
_fgets: MOV     BX,SP           ; Address parms
        MOV     SI,2[BX]        ; Get FP
        MOV     CX,4[BX]        ; Get length
        MOV     DI,6[BX]        ; Get buffer
fgets1: DEC     CX              ; Reduce count
        JZ      fgets3          ; Expired, exit
        PUSH    CX              ; Save count
        CALL    ?getc           ; Get a character
        POP     CX              ; Restore
        MOV     BX,SP           ; Re-address parms
        AND     AH,AH           ; Test for error
        JNZ     fgets2          ; End of file
        CMP     AL,0Ah          ; Newline?
        JZ      fgets3          ; Yes, terminate
        MOV     [DI],AL         ; Write it
        INC     DI              ; Advance to next
        JMP SHORT fgets1        ; And continue
; End of file, test for partial input line
fgets2: XOR     AX,AX           ; Assume EOF signal
        CMP     DI,6[BX]        ; Any data?
        JZ      fgets4          ; No, really IS eof
fgets3: MOV     BYTE PTR [DI],0 ; Zero terminate the string
        MOV     AX,6[BX]        ; Get pointer
fgets4: RET
;
; Write a string to a file: fputs(string, fp)
;
_fputs: MOV     BX,SP           ; Address parms
        MOV     SI,2[BX]        ; Get FP
        MOV     DI,4[BX]        ; Get STRING
        XOR     AX,AX           ; Zero high
fputs1: MOV     BL,[DI]         ; Get next char
        AND     BL,BL           ; End of string?
        JZ      fputs2          ; Yes, exit
        MOV     AL,BL           ; Copy for putc
        CALL    ?putc           ; Write the character
        INC     DI              ; Advance to next
        INC     AX              ; Error in putc?
        JNZ     fputs1          ; No error, proceed
fputs2: DEC     AX              ; Correct return
        RET
;
CSEG    ENDS
;
        END
