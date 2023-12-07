;
; DDS MICRO-C 8086/MS-DOS: Memory & I/O access functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _memcpy, _memmove, _memset
;
; Copy memory: memcpy(dest, source, size)
;
_memcpy:MOV     BX,SP           ; Address parms
        MOV     CX,2[BX]        ; Get length
        AND     CX,CX           ; Special case
        JZ      memc2           ; Nothing to do
        MOV     DI,6[BX]        ; Get dest
        MOV     SI,4[BX]        ; Get source
memc1:  PUSH    DS              ; Save data seg
        POP     ES              ; Set extra seg
    REP MOVSB                   ; Move it
        CLD                     ; Insure we leave forward
memc2:  MOV     AX,6[BX]        ; Get dest
        RET
;
; Move memory: memmove(dest, source, size)
;
_memmove:MOV    BX,SP           ; Address parms
        MOV     CX,2[BX]        ; Get length
        AND     CX,CX           ; Special case
        JZ      memc2           ; Nothing to do
        MOV     DI,6[BX]        ; Get dest
        MOV     SI,4[BX]        ; Get source
        CMP     DI,SI           ; Is dest > source
        JBE     memc1           ; No special case
        ADD     DI,CX           ; Offset to end
        ADD     SI,CX           ; Offset to end
        DEC     DI              ; Adjust to last element
        DEC     SI              ; Adjust to last element
        STD                     ; String backwards
        JMP SHORT memc1         ; And do it
;
; Set memory: memset(dest, value, size)
;
_memset:MOV     BX,SP           ; Address parms
        MOV     CX,2[BX]        ; Get length
        AND     CX,CX           ; Special case
        JZ      memc2           ; Nothing to do
        MOV     DI,6[BX]        ; Get dest
        MOV     AX,4[BX]        ; Get value
        PUSH    DS              ; Save DS
        POP     ES              ; And set ES
    REP STOSB                   ; Write the value
        JMP SHORT memc2         ; Return dest
;
CSEG    ENDS
        END
