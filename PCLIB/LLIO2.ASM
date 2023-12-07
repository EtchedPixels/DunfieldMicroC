;
; DDS MICRO-C 8086/MS-DOS: Low Level READ and WRITE I/O functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _read, _write
;
; READ a block of data from a file: read(char *block, int size, int handle)
;
_read:  MOV     BX,SP       ; Set up stack addressability
        MOV     DX,6[BX]    ; Get buffer address
        MOV     CX,4[BX]    ; Get number of bytes
        MOV     BX,2[BX]    ; Get file handle
        MOV     AH,3Fh      ; DOS READ function
        INT     21h         ; Read file
        JNC     write1      ; A-OK, return with count
read1:  XOR     AX,AX       ; Zero indicates error
        RET
;
; WRITE a block of data to a file: write(char *block, int size, int handle)
;
_write: MOV     BX,SP       ; Set up stack addressability
        MOV     DX,6[BX]    ; Get pointer to block
        MOV     CX,4[BX]    ; Get number of bytes
        MOV     BX,2[BX]    ; Get file handle
        MOV     AH,40h      ; DOS WRITE Function
        INT     21h         ; Write to file
        JC      write1      ; Failure, preserve retcode
        CMP     AX,CX       ; Correct number of chars?
        JZ      read1       ; Yes, its OK
        MOV     AX,-1       ; Indicate failure
write1: RET
;
CSEG    ENDS
;
        END
