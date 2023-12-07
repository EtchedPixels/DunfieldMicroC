;
; DDS MICRO-C 8086/MS-DOS: Low Level OPEN and CLOSE I/O functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _open, _close
;
; Open a file: fopen(char *filename, char options)
;
_open:  MOV     BX,SP       ; Address parms
        MOV     DX,4[BX]    ; Get filename
        MOV     BH,2[BX]    ; Get options
        AND     BH,0Fh      ; Use only SYSTEM bits
; We have option bits, check for valid open modes
        MOV     AX,3D00H    ; READ access
        CMP     BH,00000001b; 'R'ead?
        JZ      open3       ; Attempt 'R'ead access
        CMP     BH,00000010b; 'W'rite?
        JZ      open2       ; Attempt write access
        INC     AL          ; WRITE access
        CMP     BH,00000110b; 'A'ppend+'W'rite
        JZ      open4       ; Yes, handle it
        INC     AL          ; READ/WRITE access
        CMP     BH,00000011b; 'R'ead+'W'rite?
        JZ      open3       ; Attempt 'R'ead/'W'rite
; Error, return with zero
open1:  XOR     AX,AX       ; Zero return code
        RET
; Open for write, create or reset file
open2:  MOV     AH,3Ch      ; Create or reset file
        XOR     CX,CX       ; No special attributes
; Standard open mode
open3:  INT     21h         ; Issue open request
        JC      open1       ; Error, return zero
        RET
; Open for write + seek to end
open4:  INT     21H         ; Attempt file open
        JC      open2       ; Error, assume file not found
        MOV     BX,AX       ; Set up handle
        MOV     AX,4202H    ; Set file pointer, from EOF
        XOR     CX,CX       ; Zero offset
        XOR     DX,DX       ; Ditto
        INT     21h         ; Re-position
        JC      open1       ; Error, report it
        MOV     AX,BX       ; Restore handle
        RET
;
; Close an open file: fclose(int handle)
;
_close: MOV     BX,SP       ; Set up stack addressability
        MOV     BX,2[BX]    ; Get file handle
        MOV     AH,3EH      ; DOS CLOSE Function
        INT     21h         ; Close file
        JC      close1      ; Report error
        XOR     AX,AX       ; Zero result
close1: RET
;
CSEG    ENDS
;
        END
