;
; Routine to implement a Terminate and Stay Resident (TSR)
; POP-UP program, for use with the DDS MICRO-C compiler.
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP        GROUP   DSEG
DSEG        SEGMENT BYTE PUBLIC 'IDATA'
DSEG        ENDS
CSEG        SEGMENT BYTE PUBLIC 'CODE'
            ASSUME  CS:CSEG,DS:DGRP,SS:DGRP
            PUBLIC  _tsr, TSRPOP, DOS_AV_P
            EXTRN   _PSP:word, ?heap:near
; Saved MICRO-C program runtime environment
MC_SS       DW      ?           ; MICRO-C stack & data segment
MC_SP       DW      ?           ; MICRO-C stack pointer
MC_ES       DW      ?           ; MICRO-C extra segment
MC_DTA_O    DW      ?           ; MICRO-C Disk Transfer Area offset
MC_DTA_S    DW      ?           ; MICRO-C Disk Transfer Area segment
; Saved DOS program runtime environment
DOS_SS      DW      ?           ; Saved DOS Stack Segment
DOS_SP      DW      ?           ; Saved DOS Stack Pointer
DOS_DTA_O   DW      ?           ; DOS Disk Transfer Area offset
DOS_DTA_S   DW      ?           ; DOS Disk Transfer Area segment
; DOS pointers and interrupt vectors
DOS_AV_P    LABEL   DWORD       ; Pointer to DOS Availability flag
DOS_AV_O    DW      ?           ; Offset
DOS_AV_S    DW      ?           ; Segment
INT28_V     LABEL   DWORD       ; Saved INT 28 vector
INT28_O     DW      ?           ; Offset
INT28_S     DW      ?           ; Segment
INT13_V     LABEL   DWORD       ; Saved INT 13 vector
INT13_O     DW      ?           ; Offset
INT13_S     DW      ?           ; Segment
INT09_V     LABEL   DWORD       ; Saved INT 09 vector
INT09_O     DW      ?           ; Offset
INT09_S     DW      ?           ; Segment
; Internal variables used by TSR function
EXEC_FN     DW      ?           ; Function to execute
HOT_KEY     DW      ?           ; Hotkey shift-status mask
TSR_BUSY    DB      0           ; TSR busy flag
BIOS_BUSY   DB      0           ; BIOS busy flag
;
; This function terminates the 'C' program, leaving it resident
; in memory, with HOT-KEY activation of a context switch to the
; specified address. Does not return unless a error occurs, in
; which case it will return with the MS-DOS error code.
;
;   int tsr(&func, int hotkey, int allocation)
;
_tsr:   PUSH    BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parameters
        PUSH    ES              ; Save EXTRA segment
; Save segments of calling 'C' program
        MOV     CS:MC_SS,SS     ; Save MICRO-C stack & data segment
        MOV     CS:MC_SP,SP     ; Save MICRO-C stack pointer
        MOV     CS:MC_ES,ES     ; Save MICRO-C extra segment
; Get function address & hotkey arguments
        MOV     AX,8[BP]        ; Get function pointer
        MOV     CS:EXEC_FN,AX   ; Save it
        MOV     AX,6[BP]        ; Get HOT-KEY mask
        MOV     CS:HOT_KEY,AX   ; Save it
; Get current programs disk transfer area
        MOV     AH,2Fh          ; Get Disk Transfer Area function
        INT     21h             ; Ask DOS
        MOV     CS:MC_DTA_O,BX  ; Save offset
        MOV     CS:MC_DTA_S,ES  ; Save segment
; Get DOS availibility flag address
        MOV     AH,34h          ; Get DOS availability flag
        INT     21h             ; Ask DOS
        MOV     CS:DOS_AV_O,BX  ; Save offset
        MOV     CS:DOS_AV_S,ES  ; Save segment
; Get current INT 28 vector
        MOV     AX,3528h        ; Get INT 28 vector
        INT     21h             ; Ask DOS
        MOV     CS:INT28_O,BX   ; Save offset
        MOV     CS:INT28_S,ES   ; Save segment
; Get current INT 13 vector
        MOV     AX,3513h        ; Get INT 13 vector
        INT     21h             ; Ask DOS
        MOV     CS:INT13_O,BX   ; Save offset
        MOV     CS:INT13_S,ES   ; Save segment
; Get current INT 09 vector
        MOV     AX,3509h        ; Get INT 09 vector
        INT     21h             ; Ask DOS
        MOV     CS:INT09_O,BX   ; Save offset
        MOV     CS:INT09_S,ES   ; Save segment
; Set up new interrupt vectors
        PUSH    DS
        MOV     AX,CS           ; Get code segment
        MOV     DS,AX           ; Set data = code
; Set the INT 28 vector
        MOV     AX,2528h        ; Set INT 28 vector
        MOV     DX,OFFSET INT28 ; Get offset to handler
        INT     21h             ; Ask DOS
; Set the INT 13 vector
        MOV     AX,2513h        ; Set INT 13 vector
        MOV     DX,OFFSET INT13 ; Get offset to handler
        INT     21h             ; Ask DOS
; Set the INT 09 vector
        MOV     AX,2509h        ; Set INT 09 vector
        MOV     DX,OFFSET INT09 ; Get offset to handler
        INT     21h             ; Ask DOS
;
        POP     DS              ; Restore data segment
; Calculate amount of memory to reserve
; First, locate the top of the heap (reserved memory)
        MOV     SI,OFFSET DGRP:?heap; Get start of heap
LOOKH1: MOV     AL,[SI]         ; Get flag byte
        AND     AL,AL           ; End of heap?
        JZ      LOOKH2          ; Yes, exit
        MOV     BX,1[SI]        ; Get block length
        LEA     SI,3[BX+SI]     ; Advance to next
        JMP     SHORT LOOKH1    ; And keep looking
; Calculate total number of 16 byte paragraphs for data
LOOKH2: MOV     AX,SI           ; Get value
        ADD     AX,4[BP]        ; Include allocation request
        MOV     CL,4            ; Shift 4 bits
        SHR     AX,CL           ; /16 = # Paragraphs
        ADD     AX,10           ; + 160 bytes TSR overhead
; Include CODE SEGMENT (small model) and PSP in allocation
        MOV     DX,SS           ; Get stack/data segment
        SUB     DX,_PSP         ; Calculate segments allocation
        ADD     DX,AX           ; Include 64K data seg
; Set up the new MICRO-C stack pointer
        SHL     AX,CL           ; Get value back
        MOV     CS:MC_SP,AX     ; And set it
; Terminate & stay resident
        MOV     AX,3100h        ; TSR, RC=0
        INT     21h             ; Ask DOS
; If we get here, an error occured.
        POP     ES              ; Restore ES
        POP     BP              ; Restore Caller
        RET
;
; *** DOS Interrupt Handlers ***
;
; INT13 - BIOS Disk Activity
INT13   PROC    FAR
        MOV     CS:BIOS_BUSY,1  ; Set disk activity flag
        PUSHF                   ; Save flags
        CALL    CS:INT13_V      ; Invoke original interrupt
        MOV     CS:BIOS_BUSY,0  ; Clear disk activity flag
        RET     2               ; Return from interrupt, saving flags
INT13   ENDP
; INT09 - Keyboard interrupt
INT09   PROC    FAR
        PUSHF                   ; Save flags
        CALL    CS:INT09_V      ; Chain to old vector
        CLI                     ; Insure no interrupts
        PUSH    DS              ; Save data seg
        PUSH    BX              ; Save BX
        LDS     BX,CS:DOS_AV_P  ; Get pointer to DOS available flag
        CMP     BYTE PTR DS:[BX],0; Test for DOS active
        POP     BX              ; Restore BX
        POP     DS              ; Restore DS
        JZ      CHKACT          ; Its ok, continue activation
ABORT:  IRET
INT09   ENDP
; INT28 - Dos command input polling
INT28   PROC    FAR
        PUSHF                   ; Save flags
        CALL    CS:INT28_V      ; Chain to previous ints
        CLI                     ; Insure no interrupts
; Test for HOT-KEY combinations
CHKACT: PUSH    AX              ; Save AX
        PUSH    DS              ; Save DS
        XOR     AX,AX           ; Get a zero
        MOV     DS,AX           ; Set up DS to address SEG 0
        MOV     AX,WORD PTR DS:[0417h]; Keybord flag = 0:0417-0418h
        AND     AX,CS:HOT_KEY   ; Test shift bits
        CMP     AX,CS:HOT_KEY   ; Does it match?
        POP     DS              ; Restore DS
        POP     AX              ; Restore AX
        JNZ     ABORT           ; No match, ignore
; Test various flags before we activate TSR
TSRPOP: CMP     CS:TSR_BUSY,0   ; TSR already active?
        JNZ     ABORT           ; Yes, abort
        CMP     CS:BIOS_BUSY,0  ; BIOS active?
        JNZ     ABORT           ; Yes, abort
; All conditions met, allow TSR to start.
; Save the machine state and context switch to the
; MICRO-C function.
        MOV     CS:TSR_BUSY,1   ; Indicate the TSR is active
        MOV     CS:DOS_SS,SS    ; Save the DOS stack segment
        MOV     CS:DOS_SP,SP    ; Save the DOS stack pointer
        MOV     SS,CS:MC_SS     ; Get MICRO-C stack segment
        MOV     SP,CS:MC_SP     ; Get MICRO-C stack pointer
; Save the machine registers
        PUSH    AX              ; Save AX
        PUSH    BX              ; Save BX
        PUSH    CX              ; Save CX
        PUSH    DX              ; Save DX
        PUSH    BP              ; Save BP
        PUSH    SI              ; Save SI
        PUSH    DI              ; Save DI
        PUSH    DS              ; Save DS
        PUSH    ES              ; Save ES
; Save the DOS Stack
        MOV     ES,CS:DOS_SS    ; Get DOS stack segment
        MOV     BX,CS:DOS_SP    ; Get DOS stack pointer
        AND     BL,0FEh         ; Insure word alignment
        MOV     CX,64           ; Size of DOS stack to save
D01:    PUSH    WORD PTR ES:[BX]; Save one word
        INC     BX              ; Advance...
        INC     BX              ; A complete word
        LOOP    D01             ; Save 64 words
; Save DTA of interrupted program
        MOV     AH,2Fh          ; Get DTA function
        INT     21h             ; Ask DOS
        MOV     CS:DOS_DTA_O,BX ; Save offset
        MOV     CS:DOS_DTA_S,ES ; Save segment
; Restore MICRO-C's DTA
        MOV     DX,CS:MC_DTA_O  ; Get offset
        MOV     DS,CS:MC_DTA_S  ; Get segment
        MOV     AH,1Ah          ; Set DTA function
        INT     21h             ; Ask DOS
; Restore MICRO-C's segment registers
        MOV     DS,CS:MC_SS     ; Restore DATA segment
        MOV     ES,CS:MC_ES     ; Restore EXTRA segment
; Call the MICRO-C function
        STI                     ; Enable interrupts
        CALL    CS:EXEC_FN      ; Call the function
        CLI                     ; Disable interrupts
; Restore the DTA of interrupted program
        MOV     DX,CS:DOS_DTA_O ; Get offset
        MOV     DS,CS:DOS_DTA_S ; Get segment
        MOV     AH,1Ah          ; Set DTA function
        INT     21h             ; Ask DOS
; Restore the DOS stack
        MOV     ES,CS:DOS_SS    ; Get DOS stack segment
        MOV     BX,CS:DOS_SP    ; Get DOS stack pointer
        AND     BL,0FEh         ; Insure word alignment
        ADD     BX,128          ; Offset to top
        MOV     CX,64           ; Restore 64 words
D02:    DEC     BX              ; Backup...
        DEC     BX              ; A complete word
        POP     WORD PTR ES:[BX]; Restore one word
        LOOP    D02             ; Restore 64 words
; Restore the machine registers
        POP     ES              ; Restore ES
        POP     DS              ; Restore DS
        POP     DI              ; Restore DI
        POP     SI              ; Restore SI
        POP     BP              ; Restore BP
        POP     DX              ; Restore DX
        POP     CX              ; Restore CX
        POP     BX              ; Restore BX
        POP     AX              ; Restore AX
        MOV     SS,CS:DOS_SS    ; Restore SS
        MOV     SP,CS:DOS_SP    ; Restore SP
; Finish cleanup and return to interrupted program
        MOV     CS:TSR_BUSY,0   ; Clear TSR busy flag
        IRET                    ; And go home
INT28   ENDP
;
CSEG    ENDS
        END
