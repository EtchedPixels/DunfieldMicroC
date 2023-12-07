;
; DDS MICRO-C 8086/MS-DOS: Get environment function
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _getenv
        EXTRN   _ENV:word
;
; Get string from environment: int getenv(char *name, char *dest)
;
_getenv: PUSH   BP          ; Save callers stack frame
        MOV     BP,SP       ; Address parameters
        MOV     ES,DGRP:_ENV; Get environment segment
        XOR     DX,DX       ; Zero return value
        MOV     DI,DX       ; Zero search offset
        MOV     BX,4[BP]    ; Get destination buffer
        MOV     [BX],DL     ; Default to null
; Test this entry
geten1: MOV     SI,6[BP]    ; Get source name
geten2: MOV     AL,ES:[DI]  ; Get char from name
        AND     AL,AL       ; End of environment?
        JZ      geten6      ; Yes, return with no match
        INC     DI          ; Advance to next
        CMP     AL,'='      ; End of name?
        JZ      geten4      ; Yes, handle it
        CMP     AL,[SI]     ; Save as name?
        JNZ     geten3      ; No, skip to next
        INC     SI          ; Advance to next
        JMP SHORT geten2    ; And proceed
; String not found, advance to next name
geten3: XOR     AL,AL       ; Scan for zero
        MOV     CX,-1       ; Maximum size loop
        CLD                 ; Scan forward
  REPNZ SCASB               ; Search for zero
        JMP SHORT geten1    ; And go again
; Found string, process it
geten4: MOV     AL,[SI]     ; Get value
        AND     AL,AL       ; End of name?
        JNZ     geten3      ; No, keep looking
        INC     DX          ; Return 1
; Copy over to destination buffer
geten5: MOV     AL,ES:[DI]  ; Get character
        MOV     [BX],AL     ; Write to dest
        INC     DI          ; Advance to next
        INC     BX          ; Advance to next
        AND     AL,AL       ; End of string?
        JNZ     geten5      ; And proceed
; All done, return to caller
geten6: MOV     AX,DX       ; Get return value
        POP     BP
        RET
;
CSEG    ENDS
        END
