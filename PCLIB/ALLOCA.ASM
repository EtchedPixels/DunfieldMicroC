;
; DDS MICRO-C 8086/MS-DOS: Allocate automatic (stack) memory
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _alloca
        EXTRN   ?heap:near
; Define a protective margin above the heap.
MARGIN  =       100         ; Stack protect margin
;
; Allocate a block of memory on the stack
;
_alloca:POP     CX          ; Get return address
; First, locate the end of allocated heap memory
        MOV     SI,OFFSET DGRP:?heap; At beginning
alloc1: CMP BYTE PTR[SI],0  ; Test for END
        JZ      alloc2      ; End of list located
        MOV     BX,1[SI]    ; Get size of block
        LEA     SI,3[BX+SI] ; Advance to next block
        JMP SHORT alloc1    ; And keep looking
; Next, check that there's enough room on the stack
alloc2: ADD     SI,MARGIN   ; Offset for protective margin
        MOV     BX,SP       ; Get stack pointer
        SUB     BX,[BX]     ; Allocate the space
        XOR     AX,AX       ; Assume error (NULL)
        CMP     BX,SI       ; Enough space?
        JBE     alloc3      ; No, report error
; Finally, Adjust the stack pointer to reserve the memory
        MOV     SP,BX       ; Keep allocation
        LEA     AX,2[BX]    ; Copy & adjust for parm pop
alloc3: JMP     CX          ; Exit with address
;
CSEG    ENDS
        END
