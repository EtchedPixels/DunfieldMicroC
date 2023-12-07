;
; DDS MICRO-C 8086/MS-DOS: File lookup routines
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _find_first, _find_next
;
; Find first file: find_first(path, attrs, &name, &sizeh, &sizel, &attr, &time, &date)
;
_find_first: PUSH BP        ; Save callers stack frame
        MOV     BP,SP       ; Address parameters
        MOV     CX,16[BP]   ; Get search attributes
        MOV     DX,18[BP]   ; Get filename
        MOV     AH,4Eh      ; Find First function
        JMP SHORT DOFIND    ; And continue
;
; Find next file: find_next(&name, &sizeh, &sizel, &attr, &time, &date)
;
_find_next: PUSH BP         ; Save callers stack frame
        MOV     BP,SP       ; Address parameters
        MOV     AH,4Fh      ; Find Next function
DOFIND: INT     21h         ; Ask DOS
        JC      ERROR       ; Error occured
; Success, copy file information
        PUSH    ES          ; Save EXTRA segment
        MOV     AH,2Fh      ; Get DTA function
        INT     21h         ; Ask DOS
; Get integer information from DTA
        MOV     DI,8[BP]    ; Get address of "attrs"
        MOV     AL,ES:21[BX]; Get attributes
        XOR     AH,AH       ; Zero high byte
        MOV     [DI],AX     ; Return attributes
        MOV     DI,6[BP]    ; Get address of "time"
        MOV     AX,ES:22[BX]; Get time
        MOV     [DI],AX     ; Return time
        MOV     DI,4[BP]    ; Get address of "date"
        MOV     AX,ES:24[BX]; Get date
        MOV     [DI],AX     ; Return date
        MOV     DI,10[BP]   ; Get address of "sizel"
        MOV     AX,ES:26[BX]; Get sizel
        MOV     [DI],AX     ; Return sizel
        MOV     DI,12[BP]   ; Get address of "sizeh"
        MOV     AX,ES:28[BX]; Get sizeh
        MOV     [DI],AX     ; Return sizeh
; Copy filename into receiving buffer from DTA
        ADD     BX,30       ; Offset to name
        MOV     DI,14[BP]   ; Get address of "name"
COPY:   MOV     AL,ES:[BX]  ; Get char from name
        MOV     [DI],AL     ; Save in dest
        INC     BX          ; Next in source
        INC     DI          ; Next in dest
        AND     AL,AL       ; Test for terminator
        JNZ     COPY        ; copy all chars
        POP     ES          ; Restore EXTRA segment
; Return ZERO to indicate success
        XOR     AX,AX       ; Zero return code
ERROR:  POP     BP          ; Restore caller
        RET
;
CSEG    ENDS
        END
