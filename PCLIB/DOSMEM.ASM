;
; DDS MICRO-C 8086/MS-DOS: Dos memory allocation routines
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _alloc_seg, _free_seg, _resize_seg
;
; Allocate a segment: alloc_seg(int size)
;
_alloc_seg: MOV BX,SP       ; Address parameters
        MOV     BX,2[BX]    ; Get # pages needed
        MOV     AH,48h      ; Allocate memory function
        INT     21h         ; Ask DOS
        JNC     RETNZ       ; Success, return
RETZ:   XOR     AX,AX       ; zero = failure
RETNZ:  RET
;
; Release allocated segment: free_seg(int segment)
;
_free_seg:  MOV BX,SP       ; Address parameters
        MOV     ES,2[BX]    ; Get segment address
        MOV     AH,49h      ; Release memory function
        INT     21h         ; Ask DOS
        JNC     RETZ        ; Success, return zero
        RET
;
; Change the size of a memory block: resize_seg(int segment, int size)
;
_resize_seg: MOV BX,SP      ; Address parameters
        MOV     ES,4[BX]    ; Get segment address
        MOV     BX,2[BX]    ; Get new size requested
        MOV     AH,4Ah      ; Resize segment function
        INT     21h         ; Ask DOS
        JNC     RETZ        ; Success, return zero
        RET
;
CSEG    ENDS
        END
