;
; DDS MICRO-C 8086/MS-DOS: File write routines
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _lputc, _lputs
;
; PUT a character to a file: lputc(char c, int handle)
;
_lputc: MOV     BX,SP       ; Set up stack addressability
        LEA     DX,4[BX]    ; Point to character
        MOV     CX,1        ; Write one character
        CMP     WORD PTR 4[BX],000Ah    ; Test for newline special case
        JNZ     lputc1      ; No, handle normally
        MOV     WORD PTR 4[BX],0A0Dh    ; Convert to LF-CR
        INC     CX          ; Two characters now
lputc1: MOV     BX,2[BX]    ; Get file handle
        MOV     AH,40h      ; DOS WRITE Function
        INT     21h         ; Write to file
        JC      lputc2      ; Failure, preserve retcode
        CMP     AX,CX       ; Right # characters?
        MOV     AX,0        ; Assume success
        JZ      lputc2      ; Its OK - success
        DEC     AX          ; Get -1
lputc2: RET
;
; PUT a string to a file: lputs(char *string, int handle);
;
_lputs: MOV     BX,SP       ; Set up addressability
        MOV     SI,4[BX]    ; Get pointer to string
        MOV     DI,2[BX]    ; Get file pointer
lputs1: MOV     AL,[SI]     ; Get char
        XOR     AH,AH       ; Zero high byte
        AND     AL,AL       ; Terminator?
        JZ      lputc2      ; Yes, exit
        INC     SI          ; Advance to next
        PUSH    AX          ; Character to write
        PUSH    DI          ; FILE pointer
        CALL    _lputc      ; Output character
        ADD     SP,4        ; Clean up stack
        AND     AX,AX       ; Did it pass?
        JZ      lputs1      ; Yes, go again
        RET
;
CSEG    ENDS
        END
