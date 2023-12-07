;
; DDS MICRO-C 8086/MS-DOS: Joystick interface function
;
; ?COPY.TXT 1989-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _joystick
_joystick:
        MOV     BX,SP           ;Address parms
        MOV     AH,2[BX]        ;Get mask flags
        MOV     BX,4[BX]        ;Get array address
        MOV     DX,0201h        ;Address game port
        XOR     CX,CX           ;Zero initial count
        AND     AX,0F00h        ;Mask AH, AL=0
        OUT     DX,AL           ;Write to I/O port
j1:     IN      AL,DX           ;Read I/O port
        OR      AL,AH           ;Mask already received ports
        TEST    AL,01h          ;AX bit set
        JNZ     j2              ;No, its not
        MOV     [BX],CX         ;Save position
        OR      AH,01h          ;Disable further AX
j2:     TEST    AL,02h          ;AY bit set
        JNZ     j3              ;No, try next
        MOV     2[BX],CX        ;Save position
        OR      AH,02h          ;Disable further AY
j3:     TEST    AL,04h          ;BX bit set?
        JNZ     j4              ;No, try next
        MOV     4[BX],CX        ;Save position
        OR      AH,04h          ;Disable further BX
j4:     TEST    AL,08h          ;BY bit set?
        JNZ     j5              ;No, try next
        MOV     6[BX],CX        ;Save position
        OR      AH,08h          ;Disable further BY
j5:     INC     CX              ;Advance count
        JZ      j6              ;Overflow...
        CMP     AH,0Fh          ;All bits set?
        JNZ     j1              ;No, keep looking
j6:     NOT     AH              ;Invert bits
        AND     AL,AH           ;Set flags
        XOR     AH,AH           ;Zero high
        RET
;
CSEG    ENDS
        END
