;
; DDS MICRO-C 8086/MS-DOS: Heap memory allocation
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _malloc, _free
        EXTRN   ?heap:near
; Define a protective margin below the stack pointer
MARGIN  =       1024        ; Stack protect margin
;
; Allocate a block of memory: char *malloc(size)
;
; Blocks are allocated using a simple "first-fit" algorithm.
; If the block found is more than three bytes (the overhead
; size) larger than the requested block, it is divided into
; two blocks, leaving the remainder available for subsquent
; allocation.
;
_malloc:PUSH    BP          ; Save callers stack frame
        MOV     BP,SP       ; Address parameters
; Search for a free block of memory
        MOV     SI,OFFSET DGRP:?heap; Begin here
mallo1: CMP BYTE PTR[SI],1  ; Test for END/FREE
        JB      mallo4      ; End - allocate here
        MOV     BX,1[SI]    ; Get block size
        JNE     mallo2      ; Busy - keep looking
; Found free block, see if its large enough
        CMP     BX,4[BP]    ; Is it large enough?
        JAE     mallo3      ; Yes, its ok
; This block is not suitable, advance to next
mallo2: LEA     SI,3[BX+SI] ; Skip to next block
        JMP SHORT mallo1    ; and continue
; This block is ok to re-allocate
mallo3: SUB     BX,4[BP]    ; Calculate remaining
        SUB     BX,3        ; Convert for overhead
        JBE     mallo6      ; Too small, Don't split up
; Split this block into two blocks
        MOV     CX,BX       ; Save for later
        MOV     BX,4[BP]    ; Get requested size
        MOV BYTE PTR 3[BX+SI],1; Indicate free
        MOV     4[BX+SI],CX ; Set new size
        JMP SHORT mallo5    ; and exit
; Allocate on the end of memory
; Check that enough room is available
mallo4: MOV     BX,4[BP]    ; Get requested size
        MOV     CX,SP       ; Get current stack pointer
        SUB     CX,MARGIN   ; Adjust for protective margin
        SUB     CX,BX       ; Adjust for buffer size
        XOR     AX,AX       ; Assume it will fail
        CMP     CX,SI       ; Is there enough room?
        JBE     mallo7      ; No, return zero
; Ok to proceed and perform memory allocation
        MOV     BYTE PTR 3[BX+SI],0; Indicate end of list
mallo5: MOV     1[SI],BX    ; Set size
mallo6: MOV BYTE PTR [SI],2 ; Flag as allocated
        LEA     AX,3[SI]    ; Get address
mallo7: POP     BP          ; Restore caller
        RET
;
; Release a block of memory: free(ptr)
;
_free:  MOV     BX,SP       ; Get stack pointer
        MOV     CX,2[BX]    ; Get parameter
        SUB     CX,3        ; Backup to "real" beginning
; Search the allocation list for this block
        MOV     SI,OFFSET DGRP:?heap;
free1:  CMP BYTE PTR[SI],0  ; Test for END
        JZ      garbage     ; Yes, clean up and exit
        CMP     SI,CX       ; Is this it?
        JZ      free2       ; Yes, we have it
        MOV     BX,1[SI]    ; Get size
        LEA     SI,3[BX+SI] ; Advance to next
        JMP SHORT free1     ; And try again
; Mark the block as un-allocated
free2:  MOV BYTE PTR [SI],1 ; Release memory
;
; Garbage collection, scan the memory allocation blocks, and
; convert any contiguous un-allocated blocks to single areas.
; Also, truncate the list at the last allocated block.
;
garbage:MOV     SI,OFFSET DGRP:?heap; Point to memory area
; Check this block for allocation state
garb1:  CMP BYTE PTR[SI],1  ; Test for BUSY/END
        JB      garb6       ; End - quit
        MOV     BX,1[SI]    ; Get size of block
        JNZ     garb4       ; Busy - keep looking
; This block is free, check following
garb2:  CMP BYTE PTR 3[BX+SI],1; Test next block
        JB      garb5       ; End - truncate
        JNZ     garb3       ; Busy - stop looking
; Next block is also free
        ADD     BX,4[BX+SI] ; Add next block size
        ADD     BX,3        ; Add next block overhead
        JMP SHORT garb2     ; And keep looking
; Resave this block size
garb3:  MOV     1[SI],BX    ; Save new size
; Advance to next block
garb4:  LEA     SI,3[BX+SI] ; Advance to next
        JMP SHORT garb1     ; And continue
; Mark this block as the end of the list
garb5:  MOV BYTE PTR [SI],0 ; Indicate end of list
garb6:  RET
;
CSEG    ENDS
        END
