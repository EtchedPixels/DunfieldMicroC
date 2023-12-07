;
; DDS MICRO-C 8086/MS-DOS: Execute function
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG,BSEG
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _exec
;
; Execute a DOS command file: int exec(char *command, char *tail)
;
_exec:  PUSH    BP          ; Save callers stack frame
        MOV     BP,SP       ; Set up addressibility
        SUB     SP,128      ; Reserve some space
; Generate DOS format command tail
        MOV     SI,4[BP]    ; Get parameter address
        LEA     DI,-127[BP] ; And address of new command tail
        XOR     AL,AL       ; Zero initial length
sys1:   MOV     AH,[SI]     ; Get character from source
        AND     AH,AH       ; End of string?
        JZ      sys2        ; Yes, exit
        MOV     [DI],AH     ; Write to dest
        INC     SI          ; Advance source
        INC     DI          ; Advance dest
        INC     AL          ; Advance length
        JMP     SHORT sys1  ; And go it again
sys2:   MOV     BYTE PTR [DI],0Dh; Carriage return terminator
        LEA     DI,-128[BP] ; Address of command tail
        MOV     [DI],AL     ; Save string length
; Set up parameter block on stack
        MOV     SI,OFFSET DGRP:fcblk; Address of fcb's
        PUSH    DS          ; Segment of fcb#2
        PUSH    SI          ; Offset of fcb#2
        PUSH    DS          ; Segment of fcb#1
        PUSH    SI          ; Offset of fcb#1
        PUSH    SS          ; Segment of command tail
        PUSH    DI          ; Offset of command tail
        XOR     AX,AX       ; Get a zero
        PUSH    AX          ; No environment block specified
        MOV     BX,SP       ; Address of parm block
; Set up parms for EXEC, and save registers
        MOV     AX,SS       ; Get data seg
        MOV     DGRP:savess,AX; Save stack pointer
        MOV     ES,AX       ; Copy to extra
        MOV     DX,6[BP]    ; Address of command name
        PUSH    BP          ; Save BP
        MOV     DGRP:savesp,SP; Save stack pointer
; EXECute the program
        MOV     AX,4B00h    ; EXEC function
        INT     21h         ; ASK DOS
        JC      sys3        ; Error, report it
; Command was successful, restore needed registers
        MOV     AX,DGRP:savess; Get data group offset
        MOV     DS,AX       ; Restore DS
        CLI                 ; Inhibit interrupts
        MOV     SS,AX       ; Restore SS
        MOV     SP,DGRP:savesp; Restore stack pointer
        STI                 ; Reenable interrupts
        POP     BP          ; Restore callers stack frame
        MOV     AH,4Dh      ; Get termination status
        INT     21h         ; Ask DOS
; All done, return to caller
sys3:   MOV     SP,BP       ; Restore stack
        POP     BP          ; Resore callers stack frame
        RET
;
CSEG    ENDS
; Data storage segment
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
fcblk   DB      0
        DB      11 dup(' ')
        DB      25 dup(0)
DSEG    ENDS
; Bulk Storage Segment
BSEG    SEGMENT BYTE PUBLIC 'UDATA'
savess  DW      1 dup(?)
savesp  DW      1 dup(?)
BSEG    ENDS
;
        END
