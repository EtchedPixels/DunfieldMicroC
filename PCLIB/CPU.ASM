;
; DDS MICRO-C 8086/MS-DOS: Determine processor (CPU) type
;
; *** Thanks to Doug Terry & Chris Riordon for the methods ***
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _cpu
;
; Determine CPU type, returns:
;       0 = 8088/8086
;       1 = 80188/80186
;       2 = 80286
;       3 = 80386/80486
;
_cpu:   PUSHF               ; Save processor flags
; 8088/8086: Upper four flags preset to 1's
        XOR     BX,BX       ; 8088/8086 = return 0
        XOR     AX,AX       ; Start with zero
        PUSH    AX          ; Stack it, so we can
        POPF                ; Set flags to zeros
        PUSHF               ; Stack it, so we can
        POP     AX          ; Get flags contents back
        AND     AX,0F000h   ; Test upper bits
        CMP     AX,0F000h   ; All set?
        JZ      cpudet      ; 8088/8086 detected
; 80188/80186: Decrements SP BEFORE value pushed
        INC     BX          ; 80188/80186 = return 1
        PUSH    SP          ; Stack SP for test
        POP     AX          ; Get stacked value
        CMP     AX,SP       ; 286+ decrements AFTER
        JNZ     cpudet      ; 80188/80186 detected
; 808286: Upper four flags preset to 0's
        INC     BX          ; 80286 = return 2
        MOV     AX,0F000h   ; Get high bits
        PUSH    AX          ; Stack it, so we can
        POPF                ; Set flags to ones
        PUSHF               ; Stack it, so we can
        POP     AX          ; Get flags contents back
        AND     AX,0F000h   ; Test upper bits...
        JZ      cpudet      ; 80286 detected
; All other types exausted, must be 80386 or 80486
        INC     BX          ; 80386+ = return 3
; Return with found CPU type
cpudet: MOV     AX,BX       ; Get return code
        POPF                ; Restore original flags
        RET
;
CSEG    ENDS
        END
