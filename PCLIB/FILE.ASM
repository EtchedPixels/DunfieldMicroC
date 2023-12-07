;
; DDS MICRO-C 8086/MS-DOS: File Managment routines
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _create, _delete, _unlink, _remove
        PUBLIC  _rename, _get_attr, _set_attr
;
; Create a file: create(pathname, attrs)
;
_create: MOV    BX,SP           ; Address parms
        MOV     DX,4[BX]        ; Get ptr to pathname
        MOV     CX,2[BX]        ; Get attributes
        MOV     AH,3Ch          ; Create file function
        INT     21h             ; Ask DOS
        JNC     RETZ            ; Success, return zero
        RET
;
; Delete a file: delete(pathname)
;
_delete:
_remove:
_unlink: MOV    BX,SP           ; Address parms
        MOV     DX,2[BX]        ; Get ptr to pathname
        MOV     AH,41h          ; Delete file function
        INT     21h             ; Ask DOS
        JNC     RETZ            ; Success, return zero
        RET
;
; Rename a file: rename(oldname, newname)
;
_rename: MOV    BX,SP           ; Address parms
        MOV     AX,DS           ; Get data segment
        MOV     ES,AX           ; Copy to extra for newname
        MOV     DX,4[BX]        ; Get old name
        MOV     DI,2[BX]        ; Get new name
        MOV     AH,56h          ; Rename function
        INT     21h             ; Ask dos
        JNC     RETZ            ; Success, return zero
        RET
;
; Get attributes of a file: get_attr(pathname, &attrs)
;
_get_attr: MOV  BX,SP           ; Address parms
        MOV     DX,4[BX]        ; Get ptr to pathname
        MOV     AX,4300h        ; Get attrs function
        INT     21h             ; Ask DOS
        JC      RETNZ           ; Error, return code
        MOV     BX,2[BX]        ; Get ptr to attrs
        MOV     [BX],CX         ; Set attr variable
RETZ:   XOR     AX,AX           ; Zero return code
RETNZ:  RET
;
; Set attributes of a file: set_attr(pathname, attrs)
;
_set_attr: MOV  BX,SP           ; Address parms
        MOV     DX,4[BX]        ; Get ptr to pathname
        MOV     CX,2[BX]        ; Get new attributes
        MOV     AX,4301h        ; Set attrs function
        INT     21h             ; Ask DOS
        JNC     RETZ            ; Success, return zero
        RET
;
CSEG    ENDS
        END
