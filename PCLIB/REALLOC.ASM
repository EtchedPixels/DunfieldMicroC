;
; DDS MICRO-C 8086/MS-DOS: Reallocate heap memory
;
; ?COPY.TXT 1995-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _realloc
        EXTRN   ?heap:near, _malloc:near, _free:near
;
; Reallocate heap memory: char *realloc(block, size)
;
_realloc:MOV    DI,SP       ; Address parameters
; Release initial block - data remains intact
        PUSH    WORD PTR 4[DI]; Pass block
        CALL    _free       ; Release block
        POP     AX          ; Clean stack
        MOV     CX,2[DI]    ; Get requested size
; Search heap for a free block of memory
        MOV     SI,OFFSET DGRP:?heap; Begin here
real1:  LEA     DI,3[SI]    ; Get pointer to actual block
        CMP BYTE PTR[SI],1  ; Test for free/end
        JB      real2       ; End of list - allocate
        MOV     BX,1[SI]    ; Get block size
        LEA     SI,[BX+DI]  ; Skip to next
        JNZ     real1       ; Not free - keep looking
        CMP     BX,CX       ; Is it large enough?
        JB      real1       ; No, keep looking
; This block is large enough:
; Copy data first so that malloc() doesn't corrupt (lower block ending)
; Call malloc() to mark block as allocated.
        MOV     BX,SP       ; Address stack
        CALL    real3       ; Copy in block
        JMP     _malloc     ; Allocate the memory
; No suitable block in list:
; Call malloc() first to insure room available (won't corrupt at end)
; Copy data to newly allocated block
real2:  PUSH    CX          ; Pass new size
        CALL    _malloc     ; Allocate memory
        POP     CX          ; Clean stack
        AND     AX,AX       ; Success?
        JZ      real4       ; No, failure
        MOV     BX,SP       ; Address stack
; Copy block contents
real3:  MOV     SI,4[BX]    ; Get source block
        CMP     DI,SI       ; Same block?
        JZ      real4       ; No need to copy
        PUSH    DS          ; Save data
        POP     ES          ; Set extra
    REP MOVSB               ; Copy the memory
real4:  RET
;
CSEG    ENDS
        END
