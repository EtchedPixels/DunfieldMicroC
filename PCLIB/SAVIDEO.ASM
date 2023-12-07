;
; DDS MICRO-C 8086/IBM-PC: Video save & restore functions
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _save_video, _restore_video
; Video screen segment addresses
CBASE   EQU     0B800h          ; Color
MBASE   EQU     0B000h          ; Monochrome
;
; Save the video screen & state: save_video(char *buffer)
;
_save_video:
        PUSH    BP              ; Save caller
        MOV     BP,SP           ; Address params
        MOV     BP,4[BP]        ; Get pointer to save area
; Save video page & base address
        MOV     AH,0Fh          ; Get video mode
        INT     10h             ; Call BIOS
        MOV     0[BP],AL        ; Save video mode
        MOV     1[BP],BH        ; Save video page
; Get video base segment address
        MOV     SI,CBASE        ; Assume COLOR base
        CMP     AL,07h          ; Monochrome?
        JNZ     savev1          ; Assumption correct
        MOV     SI,MBASE        ; Reset to MONOCHROME base
; Save the cursor position & type
savev1: MOV     AH,03h          ; Get cursor position
        INT     10h             ; Call BIOS
        MOV     2[BP],DX        ; Save cursor position
        MOV     4[BP],CX        ; Save cursor type
; Save the contents of video memory
        PUSH    DS              ; Save DS
        MOV     DS,SI           ; DS = source   (screen)
        POP     ES              ; ES = dest     (buffer)
        LEA     DI,6[BP]        ; Get buffer address
        MOV     SI,0            ; Start at offset zero
        MOV     CX,25*80        ; Screen size (in words)
    REP MOVSW                   ; Move a string of words
        MOV     AX,ES           ; Get actual data area
        MOV     DS,AX           ; Restore DS
        POP     BP              ; Restore caller
        RET
;
; Restore the video screen & state: restore_video(char *buffer)
;
_restore_video:
        PUSH    BP              ; Save caller
        MOV     BP,SP           ; Address params
        MOV     BP,4[BP]        ; Get pointer to save area
; Calculate the base video address
        MOV     AL,0[BP]        ; Get previous mode
        MOV     SI,CBASE        ; Assume COLOR base
        CMP     AL,07h          ; Monochrome?
        JNZ     restv1          ; Assumption correct
        MOV     SI,MBASE        ; Switch to MONOCHROME
; Restore the cursor
restv1: MOV     BH,1[BP]        ; Get video page
        MOV     DX,2[BP]        ; Get previous position
        MOV     AH,02h          ; Set cursor position
        INT     10h             ; Call BIOS
        MOV     CX,4[BP]        ; Get previous type
        MOV     AH,01h          ; Set cursor type
        INT     10h             ; Call BIOS
; Restore the video memory
        MOV     ES,SI           ; ES = dest (Screen)
        LEA     SI,6[BP]        ; Get source offset
        MOV     DI,0            ; Get dest offset
        MOV     CX,25*80        ; Screen size (in words)
    REP MOVSW                   ; Perform the copy
        POP     BP              ; Restore caller
        RET
;
CSEG    ENDS
        END
