;
; DDS MICRO-C 8086/MS-DOS: Generate tones on the PC speaker
;
; ?COPY.TXT 1994-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _sound, _sound_off
; Hardware addresses
SPKCTL  =       0061h       ; Speaker control port
SPKFRE  =       0042h       ; Speaker frequency port
SPKMOD  =       0043h       ; Speaker mode port
;
; Set the tone to be output by the PC speaker
;
_sound: MOV     BX,SP       ; Address stack
        MOV     BX,2[BX]    ; Get frequency to output
; Compute timer value for specified frequency
        MOV     DX,0012h    ; Get HIGH adjustment
        CMP     BX,DX       ; Out of range
        JBE     _sound_off  ; Don't output
        MOV     AX,34DDh    ; Get LOW adjustment
        DIV     BX          ; Compute timer value
        MOV     BX,AX       ; Save for later
; Set up speaker to internal timer output mode
        IN      AL,SPKCTL   ; Get speaker control port
        OR      AL,03h      ; Enable speaker output
        OUT     SPKCTL,AL   ; Write to port
        MOV     AL,0B6h     ; Get mode setting
        OUT     SPKMOD,AL   ; Write to mode port
; Write timer count to set frequency
        MOV     AL,BL       ; Get LOW byte
        OUT     SPKFRE,AL   ; Write to port
        MOV     AL,BH       ; Get HIGH byte
        OUT     SPKFRE,AL   ; Write to port
        RET
;
; Turn speaker OFF
;
_sound_off:
        IN      AL,SPKCTL   ; Read bits
        AND     AL,0FCh     ; Clear speaker
        OUT     SPKCTL,AL   ; Write back
        RET
;
CSEG    ENDS
        END
