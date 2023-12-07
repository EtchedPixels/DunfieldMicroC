;
; DDS MICRO-C 8086/MS-DOS: Buffered I/O functions
;
; ?COPY.TXT 1990-2005 Dave Dunfield
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
        PUBLIC  _getc, _fgetc, ?getc, ?putc, _putc, _fputc, _fflush
;
; Read a character from a buffered file: getc(fp)
;
_getc:
_fgetc: MOV     BX,SP       ; Address parms
        MOV     SI,2[BX]    ; Get address of buffer
;
; Read a byte of data from file(SI) into AX
;
?getc:  MOV     BX,7[SI]    ; Get pointer
getc1:  CMP     BX,5[SI]    ; Any data in buffer?
        JB      getc3       ; Yes, its ok
; Read a block of data from the file
        MOV     BX,1[SI]    ; Get file handle
        MOV     CX,3[SI]    ; Get # bytes/read
        LEA     DX,9[SI]    ; Point to I/O buffer
        MOV     AH,3Fh      ; Read file command
        INT     21h         ; Ask DOS
        JC      xerror      ; Error, return EOF and set error flag
        AND     AX,AX       ; Zero bytes read?
        JZ      xeof        ; we have no data
getc2:  MOV     5[SI],AX    ; Record number of bytes
        XOR     BX,BX       ; Zero count
; Read the character from the buffer
getc3:  MOV     AL,9[BX+SI] ; Read the data value
        INC     BX          ; Advance to next
        TEST BYTE PTR[SI],BINARY; BINARY read?
        JNZ     getc4       ; Yes, no processing
        CMP     AL,0Dh      ; Carriage return?
        JZ      getc1       ; Yes, ignore it
        CMP     AL,1Ah      ; End of file?
        JZ      xeof        ; Yes, report eof
getc4:  MOV     7[SI],BX    ; Resave input
        XOR     AH,AH       ; Zero high
        RET
;
; Flush the buffer: fflush(fp)
;
_fflush:MOV     BX,SP
        MOV     SI,2[BX]    ; Get I/O buffer
        XOR     AX,AX       ; Success error code
flush1: MOV     CX,7[SI]    ; Get buffer size
        XOR     BX,BX       ; Get zero
        MOV     5[SI],BX    ; Clear buffer size
        MOV     7[SI],BX    ; Clear I/O pointer
        TEST BYTE PTR[SI],WRITE; Open for WRITE?
        JZ      xret        ; No, just exit
; Buffer is full, write it out
        MOV     BX,1[SI]    ; Get Handle
        LEA     DX,9[SI]    ; Point to I/O buffer
        PUSH    AX          ; Save return code
        MOV     AH,40h      ; Write file command
        INT     21h         ; Ask DOS
        POP     BX          ; Restore return code
        JC      xerror      ; Report error
        CMP     AX,CX       ; In range?
        MOV     AX,BX       ; Set return code
        JZ      xret        ; OK, return
; Error, return EOF to caller
xerror: OR      BYTE PTR [SI],FERROR; I/O error
; End of file - return EOF
xeof:   MOV     AX,-1       ; Report eof
xret:   RET
;
; Write a character to a buffered file: (f)putc(c, fp)
;
_putc:
_fputc: MOV     BX,SP       ; Address parms
        MOV     SI,2[BX]    ; Get address of buffer
        MOV     AX,4[BX]    ; Get char to write
;
; Write character(AX) into file(SI)
;
?putc:  CMP     AL,0Ah      ; Is it a NEWLINE?
        JNZ     putc1       ; Its ok
        TEST BYTE PTR[SI],BINARY; BINARY mode?
        JNZ     putc1       ; Yes, its ok
        MOV     AL,0Dh      ; Carriage return
        CALL    putc1       ; Write the character
        MOV     AL,0Ah      ; Now output the LINE-FEED
; Write the character into the I/O buffer
putc1:  MOV     BX,7[SI]    ; Get I/O buffer position
        MOV     9[BX+SI],AL ; Write to buffer
        INC     BX          ; Advance
        MOV     7[SI],BX    ; Resave
        CMP     BX,3[SI]    ; Is it full?
        JAE     flush1      ; Yes, handle it
        RET
;
CSEG    ENDS
        END
