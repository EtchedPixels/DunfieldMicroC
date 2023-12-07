;
; DDS MICRO-C 8086/MS-DOS: Directory Managment routines
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _mkdir, _rmdir, _chdir, _cd, _getdir
;
; Make a directory: mkdir(pathname)
;
_mkdir: MOV     BX,SP           ; Address parms
        MOV     DX,2[BX]        ; Get ptr to pathname
        MOV     AH,39h          ; Create directory function
        INT     21h             ; Ask DOS
        JNC     RETZ            ; Success, return zero
        RET
;
; Remove a directory: rmdir(pathname)
;
_rmdir: MOV     BX,SP           ; Address parms
        MOV     DX,2[BX]        ; Get ptr to pathname
        MOV     AH,3Ah          ; Delete directory function
        INT     21h             ; Ask DOS
        JNC     RETZ            ; Success, return zero
        RET
;
; Change current directory: cd(pathname)
;
_chdir:
_cd:    MOV     BX,SP           ; Address parms
        MOV     DX,2[BX]        ; Get ptr to pathname
        MOV     AH,3Bh          ; Set directory function
        INT     21h             ; Ask DOS
        JNC     RETZ            ; Success, return zero
        RET
;
; Get current directory: getdir(&pathname[64])
;
_getdir: MOV    BX,SP           ; Address parms
        XOR     DL,DL           ; Assume default drive
        MOV     SI,2[BX]        ; Get name buffer
        MOV     AH,47h          ; Get directory function
        INT     21h             ; Ask DOS
        JC      RETNZ           ; Error, set return code
RETZ:   XOR     AX,AX           ; Get success code
RETNZ:  RET
;
CSEG    ENDS
        END
