;
; DDS MICRO-C 8086/MS-DOS: Time and Date functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _get_time, _set_time, _get_date, _set_date
;
; Get time of day: get_time(&hour, &minite, &second)
;
_get_time: PUSH BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parms
        MOV     AH,2Ch          ; Get time function
        INT     21h             ; Ask DOS
        XOR     AH,AH           ; Zero high
        MOV     AL,CH           ; Get hour
        MOV     BX,8[BP]        ; Get ptr to var
        MOV     [BX],AX         ; Set hour
        MOV     AL,CL           ; Get minite
        MOV     BX,6[BP]        ; Get ptr to var
        MOV     [BX],AX         ; Set minite
        MOV     AL,DH           ; Get second
        MOV     BX,4[BP]        ; Get pointer to var
        MOV     [BX],AX         ; Set second
        MOV     AL,DL           ; Get hundredths
        POP     BP              ; Restore caller
        RET
;
; Set time of day: set_time(hour, minite, second)
;
_set_time: MOV  BX,SP           ; Address parms
        MOV     CH,6[BX]        ; Get hour
        MOV     CL,4[BX]        ; Get minite
        MOV     DH,2[BX]        ; Get Second
        XOR     DL,DL           ; Clear hundred
        MOV     AH,2Dh          ; Set time function
        INT     21h             ; Ask DOS
        CBW                     ; Convert to 16 bit result
        RET
;
; Get todays date: get_date(&day, &month, &year)
;
_get_date: PUSH BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parms
        MOV     AH,2Ah          ; Get date function
        INT     21h             ; Ask DOS
        XOR     AH,AH           ; Zero high
        PUSH    AX              ; Save day of week
        MOV     AL,DL           ; Get day
        MOV     BX,8[BP]        ; Get ptr to var
        MOV     [BX],AX         ; Set day
        MOV     AL,DH           ; Get month
        MOV     BX,6[BP]        ; Get ptr to var
        MOV     [BX],AX         ; Set month
        MOV     BX,4[BP]        ; Get pointer to var
        MOV     [BX],CX         ; Set Year
        POP     AX              ; Restore day of week
        POP     BP              ; Restore caller
        RET
;
; Set todays date: set_date(day, month, year)
;
_set_date: MOV  BX,SP           ; Address parms
        MOV     DL,6[BX]        ; Get day
        MOV     DH,4[BX]        ; Get month
        MOV     CX,2[BX]        ; Get year
        MOV     AH,2Bh          ; Set date function
        INT     21h             ; Ask DOS
        CBW                     ; Convert to 16 bit result
        RET
;
CSEG    ENDS
        END
