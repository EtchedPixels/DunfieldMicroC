;
; DDS MICRO-C Runtime library startup code for: PC 8086 using MASM
;
;   This file must be assembled using 'MASM/ML'. If assembled
; without changes, it will produce the SMALL runtime library.
; To create the TINY runtime library, define '_TINY_':
;
; masm/ml/d_TINY_ pc86.rl,pc86rl_t.obj;
; masm/ml         pc86.rl,pc86rl_s.obj;
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
        PUBLIC  ?eq,?ne,?lt,?le,?gt,?ge,?ult,?ule,?ugt,?uge
        PUBLIC  ?not,?switch,?temp,?heap,_PSP,_ENV,_ARGC,_ARGV
        PUBLIC  _stdin, _stdout, _stderr
        EXTRN   _main:NEAR
;
    ifdef   _TINY_
;
; For TINY model, group ALL segments, DS & SS are already set up
;
DGRP    GROUP   CSEG,DSEG,BSEG,MSEG
CSEG    SEGMENT PARA PUBLIC 'CODE'
        ASSUME  CS:CSEG,DS:DGRP,SS:DGRP
        ORG     100h        ; COM programs start here
ENTRY:  MOV     BX,4096     ; Ask for 64K segment
        MOV     AH,4Ah      ; Resize memory block
        INT     21h         ; Ask DOS
    else
;
; For SMALL model, group only DATA segments, set up DS & SS
;
DGRP    GROUP   DSEG,BSEG,MSEG,SSEG
CSEG    SEGMENT PARA PUBLIC 'CODE'
        ASSUME  CS:CSEG,DS:DGRP,SS:DGRP
;
ENTRY:  MOV     BX,DGRP     ; Get data segment
        MOV     DS,BX       ; Set up DS
; Calculate size of program (in paragraphs)
        MOV     AX,ES       ; Get PSP segment
        SUB     BX,AX       ; Calculate # pages code
        ADD     BX,4096     ; Add 64K data segment
        MOV     AH,4Ah      ; Resize memory block
        INT     21h         ; Ask DOS
        MOV     AX,DS       ; Get DS
        MOV     SS,AX       ; Set up SS
    endif
        XOR     AX,AX       ; Get zero
        MOV     SP,AX       ; Set up stack
; Zero unitialized storage
        MOV     DI,OFFSET DGRP:?temp; Point to first locaton
        MOV     CX,(OFFSET DGRP:?heap)+1; Point to last location
        SUB     CX,DI       ; Compute size
    ifdef _TINY_
    REP STOSB               ; Zero the memory
    else
        PUSH    ES          ; Save PSP segment
        PUSH    DS          ; Get Data Segment
        POP     ES          ; Target for STOS
    REP STOSB               ; Zero the memory
        POP     ES          ; Restore PSP segment
    endif
; Copy argument strings into reserved area and zero
; terminate them. Set up ARGV to point to the new copies.
        MOV     SI,0081H    ; Point to command tail in PSP
        MOV     DI,OFFSET DGRP:ARGS; Local buffer to hold
        MOV     BX,OFFSET DGRP:_ARGV+2; Base address of ARGV
        MOV     CX,1        ; Initially, ARGC = 1
    ifdef _TINY_
entry1: MOV     AL,[SI]     ; Get char from command tail
    else
entry1: MOV     AL,ES:[SI]  ; Get char from command tail
    endif
        CMP     AL,0DH      ; End of input line?
        JZ      entry4      ; Yes, exit
        INC     SI          ; Skip to next
        CMP     AL,' '      ; Leading space?
        JZ      entry1      ; No, process operand
; We are at the beginning of an operand, save its address,
; increment the count, and skip to the end of it.
        MOV     [BX],DI     ; Save pointer in ARGV
        ADD     BX,2        ; Adjust to next
        INC     CX          ; Advance argument count
entry2: MOV     [DI],AL     ; Write it
        INC     DI          ; Advance to next in dest
    ifdef   _TINY_
        MOV     AL,[SI]     ; Get char from argument
    else
        MOV     AL,ES:[SI]  ; Get char from argument
    endif
        CMP     AL,0Dh      ; End of input line?
        JZ      entry3      ; yes, exit
        INC     SI          ; Advance to next in source
        CMP     AL,' '      ; Terminating space?
        JNZ     entry2      ; No, keep looking
        INC     DI          ; Skip zero
        JMP     SHORT entry1; And get next parm
entry3: INC     DI          ; Skip zero
entry4: MOV     DGRP:_ARGC,CX; Save argument count
        MOV     DGRP:_PSP,ES; Save PSP segment
; Get the program name from the environment
    ifdef _TINY_
        MOV     AX,DS:WORD PTR 002Ch; Get segment of ENV
    else
        MOV     AX,ES:WORD PTR 002Ch; Get segment of ENV
    endif
        MOV     DGRP:_ENV,AX; Save environment for later
        MOV     ES,AX       ; Set up extra segment
        MOV     SI,0        ; Begin at entry
entry5: MOV     AL,ES:[SI]  ; Get char from environment
        INC     SI          ; Advance to next
        AND     AL,AL       ; Zero terminator
        JNZ     entry5      ; Look for end
        OR      AL,ES:[SI]  ; End of list?
        JNZ     entry5      ; No, keep looking
        ADD     SI,3        ; Skip next zero
        MOV     DGRP:_ARGV,DI; Save pointer
; Copy over program name
entry6: MOV     AL,ES:[SI]  ; Get char from program name
        MOV     [DI],AL     ; Save in variable
        INC     SI          ; Advance to next in source
        INC     DI          ; Advance to next in dest
        AND     AL,AL       ; End of name?
        JNZ     entry6      ; No, keep looking
        MOV     AX,OFFSET DGRP:_ARGV; Get address of ARGV
        PUSH    CX          ; Pass ARGC argument
        PUSH    AX          ; Pass ARGV argument
        CALL    _main       ; Execute user program
; If 'main' does not call 'exit', it will return to here. Do an
; explicit terminate, since we have changed the stack.
        MOV     AX,4C00h    ; Exit function, RC=0
        INT     21h         ; Ask DOS
;
; Compare library routines (used when 1/0 result is required)
;
?eq:    CMP     AX,CX       ; CX == AX
        JZ      TRUE
        XOR     AX,AX
        RET
?ne:    CMP     AX,CX       ; CX != AX
        JNZ     TRUE
        XOR     AX,AX
        RET
?lt:    CMP     AX,CX       ; CX < AX
        JG      TRUE
        XOR     AX,AX
        RET
?le:    CMP     AX,CX       ; CX <= AX
        JGE     TRUE
        XOR     AX,AX
        RET
?gt:    CMP     AX,CX       ; CX > AX
        JL      TRUE
        XOR     AX,AX
        RET
?ge:    CMP     AX,CX       ; CX >= AX
        JLE     TRUE
        XOR     AX,AX
        RET
?ult:   CMP     AX,CX       ; CX < AX (unsigned)
        JA      TRUE
        XOR     AX,AX
        RET
?ule:   CMP     AX,CX       ; CX <= AX (unsigned)
        JAE     TRUE
        XOR     AX,AX
        RET
?ugt:   CMP     AX,CX       ; CX > AX (unsigned)
        JB      TRUE
        XOR     AX,AX
        RET
?uge:   CMP     AX,CX       ; CX >= AX (unsigned)
        JBE     TRUE
FALSE:  XOR     AX,AX
        RET
;
; Logically negate (ax)
;
?not:   AND     AX,AX       ; Test for non-zero
        JNZ     FALSE       ; Not zero, invert
TRUE:   MOV     AX,1        ; Create a true value
        AND     AX,AX       ; Set condition code
        RET
;
; Handle switch statement
;
?switch:
    ifdef   _TINY_
        MOV     CX,[BX]     ; Get label value
        JCXZ    SWEND       ; End of table - Special case
        ADD     BX,4        ; Skip to next
        CMP     AX,-2[BX]   ; Does it match?
        JNZ     ?switch     ; No, keep looking
        JMP     CX          ; Execute case code
SWEND:  JMP     2[BX]       ; Execute default code
    else
        MOV     CX,CS:[BX]  ; Get label value
        JCXZ    SWEND       ; End of table - Special case
        ADD     BX,4        ; Skip to next
        CMP     AX,CS:-2[BX]; Does it match?
        JNZ     ?switch     ; No, keep looking
        JMP     CX          ; Execute case code
SWEND:  JMP     CS:2[BX]    ; Execute default code
    endif
;
CSEG    ENDS
; Define the initialized data segment
DSEG    SEGMENT PARA PUBLIC 'IDATA'
stdin   DB      01h         ; READ mode
        DW      0           ; STDIN file handle
        DW      25,0,0      ; Buffer info
        DB      'MICRO-C by Dave Dunfield',0
stdout  DB      02h         ; WRITE mode
        DW      1           ; STDOUT file handle
        DW      1,0,0       ; Buffer info
        DB      0           ; 1 character I/O buffer
_stdin  DW      OFFSET DGRP:stdin
_stdout DW      OFFSET DGRP:stdout
_stderr DB      02h         ; WRITE mode
        DW      2           ; STDERR file handle
        DW      1,0,0       ; Buffer info
        DB      0           ; 1 character I/O buffer
DSEG    ENDS
; Define the uninitialized (BULK) segment
BSEG    SEGMENT BYTE PUBLIC 'UDATA'
?temp   DW      1   dup (?)     ; Temporary register
_PSP    DW      1   dup (?)     ; Program Segment Prefix
_ENV    DW      1   dup (?)     ; Environment segment
_ARGC   DW      1   dup (?)     ; Global argument count
_ARGV   DW      20  dup (?)     ; Global argument strings
ARGS    DB      128 dup (?)     ; Storage for parsed arguments
BSEG    ENDS
; Define the memory allocation segment
MSEG    SEGMENT BYTE PUBLIC 'UDATA'
?heap   DB      1   dup (?)     ; Free memory list
MSEG    ENDS
; Define the stack segment (Small model only)
    ifndef  _TINY_
SSEG    SEGMENT BYTE STACK 'STACK'
        DW      32  dup(?)      ; Minimal stack to start
SSEG    ENDS
    endif
;
        END     ENTRY
