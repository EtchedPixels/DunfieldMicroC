;
; Low level IBM/PC console interface routines, supporting a single
; full-screen video window, for use with the DDS MICRO-C compiler.
;
; ?COPY.TXT 1989-2005 Dave Dunfield
; **See COPY.TXT**.
;
; Define video screen attributes
NORMAL  EQU     07h             ; Normal mode video attributes
REVERSE EQU     70h             ; Special effect video attribute
CBASE   EQU     0B800h          ; Color video screen segment
MBASE   EQU     0B000h          ; Monochrome video screen segment
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
;
; Initialized variables & tables
;
_V_BASE DW      CBASE           ; Mono=B000h, Cga=B800h
_V_PAGE DB      0               ; Current video page
_V_ATTR DB      0               ; Current video attribute
_V_XY   DW      0               ; X & Y Cursor position
; Special key table - General keys
_V_KEYS DW      4800h           ; KUA   (Up arrow)
        DW      5000h           ; KDA   (Down arrow)
        DW      4B00h           ; KLA   (Left arrow)
        DW      4D00h           ; KRA   (Right arrow)
        DW      4900h           ; KPU   (PgUp)
        DW      5100h           ; KPD   (PgDn)
        DW      4700h           ; KHO   (Home)
        DW      4F00h           ; KEN   (End)
        DW      4E2Bh           ; KKP   (Keypad '+')
        DW      4A2Dh           ; KKM   (Keypad '-')
        DW      5200h           ; KIN   (Ins)
        DW      5300h           ; KDL   (Del)
        DW      0E08h           ; KBS   (Backspace)
; Function keys
        DW      3B00h           ; K1    (F1)
        DW      3C00h           ; K2    (F2)
        DW      3D00h           ; K3    (F3)
        DW      3E00h           ; K4    (F4)
        DW      3F00h           ; K5    (F5)
        DW      4000h           ; K6    (F6)
        DW      4100h           ; K7    (F7)
        DW      4200h           ; K8    (F8)
        DW      4300h           ; K9    (F9)
        DW      4400h           ; K10   (F10)
; Special control keys
        DW      8400h           ; CPU   (CTRL-PgUp)
        DW      7600h           ; CPD   (CTRL-PgDn)
        DW      7700h           ; CHO   (CTRL-Home)
        DW      7500h           ; CEN   (CTRL-End)
        DW      0
DSEG    ENDS
;
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _vopen, _vtstc, _vgetc, _vputc, _vclscr, _vcleos, _vcleol
        PUBLIC  _vcursor_off, _vcursor_line, _vcursor_block,
        PUBLIC  _vgotoxy, _vupdatexy, _V_ATTR, _V_BASE, _V_XY
;
; Check for a key from the keyboard: c = vtstc()
;
_vtstc: MOV     AH,01h          ; Check for key
        INT     16h             ; Do we have key?
        JNZ     _vgetc          ; Yes, read it
        XOR     AX,AX           ; No key today
        RET
;
; Get a key from the keyboard with translations: c = vgetc()
;
_vgetc: CALL    _vupdatexy      ; Update the cursor position
; Call BIOS to read key
        XOR     AH,AH           ; Function code 0 - read key
        INT     16h             ; Call bios
; Lookup key for special entries
        MOV     CL,80h          ; Beginning function code
        MOV     BX,OFFSET DGRP:_V_KEYS; Address of keys array
LOKKEY: MOV     DX,[BX]         ; Get key entry
        CMP     AX,DX           ; Does it match?
        JZ      FNDKEY          ; We found it
        ADD     BX,2            ; Skip ahead
        INC     CL              ; Advance key code
        OR      DH,DL           ; End of table?
        JNZ     LOKKEY          ; Keep looking
; Translate ENTER key to C newline
        CMP     AX,1C0Dh        ; ENTER key?
        JNZ     NORKEY          ; Yes, we have it
        MOV     CL,'J'-40h      ; Newline is ^J (LF)
; Translate special keys
FNDKEY: MOV     AL,CL           ; Set return value
NORKEY: CBW                     ; Set high bits
        RET
;
; Write a character to the video display: vputc(int c)
;
_vputc: CALL    VSETUP          ; Set up video addresses
        MOV     AX,4[BP]        ; Get character to display
; Handle BELL
TSTBEL: CMP     AX,'G'-40h      ; BELL code?
        JNZ     TSTCR           ; No, try next
        MOV     AX,0E07h        ; Write BELL code
        XOR     BX,BX           ; Write to page 0
        INT     10h             ; Call BIOS
        JMP     SHORT VEXIT     ; and exit
; Handle CARRIAGE RETURN
TSTCR:  CMP     AX,'M'-40h      ; Is it carriage return
        JNZ     TSTBS           ; No, try next
        XOR     DL,DL           ; Reset 'X' position
        JMP     SHORT VEXIT     ; and proceed
; Handle BACKSPACE
TSTBS:  CMP     AX,'H'-40h      ; Is it backspace
        JNZ     TSTNL           ; No, try line-feed
        AND     DL,DL           ; Already at first col?
        JZ      VEXIT           ; Yes, don't backup
        DEC     DL              ; Reduce 'X' position
        JMP     SHORT VEXIT     ; And exit
; Handle NEWLINE
TSTNL:  CMP     AX,'J'-40h      ; Is it newline?
        JNZ     NORCHR          ; Yes, advance line
        XOR     DL,DL           ; Reset cursor
        JMP     SHORT ADVY      ; And goto a new line
; Normal Character, output it
NORCHR: MOV     AH,DGRP:_V_ATTR ; Get video attributes
        MOV     ES:[BX],AX      ; Write to video display
; Advance 'X' position
ADVX:   INC     DL              ; Advance 'X'
        CMP     DL,80           ; Are we over?
        JB      VEXIT           ; No, its ok
        XOR     DL,DL           ; Reset 'X' posotion
; Advance 'Y' position
ADVY:   MOV     SI,160          ; Copy second line...
        MOV     DI,0            ; into first line
        INC     DH              ; Advance # lines
        CMP     DH,25           ; are we over?
        JB      VEXIT           ; No, Its OK
        DEC     DH              ; Reset it
        MOV     CX,80*24        ; # characters to move
        MOV     BX,ES           ; Get video address
        MOV     DS,BX           ; Set source display
    REP MOVSW                   ; Scroll screen
        MOV     CX,80           ; Size of one line
DOCLR:  MOV     AX,0720h        ; clear with blank
    REP STOSW                   ; Clear line
; Restore callers environment & exit
VEXIT:  POP     DS              ; Restore callers Data seg
        POP     ES              ; Restore callers ES
        MOV     DGRP:_V_XY,DX   ; Resave 'X' and 'Y'
        POP     BP              ; Restore callers stack frame
        RET
;
; Initialize the video display: vopen()
;
_vopen:
        MOV     AH,0Fh          ; Get video mode
        INT     10h             ; CAll BIOS
        MOV     DGRP:_V_PAGE,BH ; Save video page
        MOV     BX,CBASE        ; Assume COLOR address
        CMP     AL,07h          ; Monochrome?
        JNZ     VINIT1          ; No, assumption correct
        MOV     BX,MBASE        ; Get MONOCHROME address
VINIT1: MOV     DGRP:_V_BASE,BX ; Set video base address
        MOV     DGRP:_V_ATTR,NORMAL; Set for normal video
;
; CLEAR entire SCREEN: vclscr()
;
_vclscr: CALL   VSETUP          ; Setup video
        XOR     DX,DX           ; Reset cursor position
        MOV     AH,02h          ; Set cursor position
        MOV     BH,DGRP:_V_PAGE ; Get current page
        INT     10h             ; Call BIOS
        MOV     CX,80*25        ; # entries to clear
        MOV     DI,DX           ; Start of display
        JMP     SHORT DOCLR     ; Perform the clear
;
; CLEAR to END OF SCREEN: vcleos()
;
_vcleos: CALL   VSETUP          ; Setup video
        MOV     CX,80*25        ; Get end of screen
        MOV     DI,BX           ; Start from here
        SHR     BX,1            ; Calculate cursor character position
        SUB     CX,BX           ; Calculate # remaining
        JMP     SHORT DOCLR     ; Perform the clear
;
; CLEAR to END OF LINE: vcleol()
;
_vcleol: CALL   VSETUP          ; Setup video
        MOV     CX,80           ; Get length
        SUB     CL,DL           ; Calculate # remaining
        MOV     DI,BX           ; Start here
        JMP     SHORT DOCLR     ; perform the clear
;
; Position the cursor on the video display: vgotoxy(int x, int y)
;
_vgotoxy: PUSH  BP              ; Save callers stack frame
        MOV     BP,SP           ; Set up addressability to args
        MOV     AL,6[BP]        ; Get 'X' value
        MOV     AH,4[BP]        ; Get 'Y' value
        MOV     DGRP:_V_XY,AX   ; Set X/Y values
        POP     BP              ; Restore caller
;
; Update the cursor position: vupdatexy()
;
_vupdatexy: MOV DX,DGRP:_V_XY   ; Get X & Y pointers
        MOV     BH,DGRP:_V_PAGE ; Get video page
        MOV     AH,02h          ; Set cursor function
        INT     10h             ; Call DOS
        RET
;
; Set up addressability to video display & stack registers
; Exit: (ES = video base address, DX=cursor position, BX=cursor address
;
VSETUP: POP     AX              ; Get return address
        PUSH    BP              ; Save callers stack frame
        MOV     BP,SP           ; Set up addressability to args
        PUSH    ES              ; Save callers Extra segment
        PUSH    DS              ; Save callers Data segment
        PUSH    AX              ; Replace return address
; Set up addresability to the video display, and
; Establish the current cursor position.
        MOV     ES,DGRP:_V_BASE ; Point to video base
        MOV     DX,DGRP:_V_XY   ; Get 'X' and 'Y' position
; Calculate cursor address from 'X' and 'Y' position
        MOV     AL,DH           ; Copy 'Y'
    ifdef _FAST_
; Multiply AL by 160 for 80*2 bytes/line (Fast long version)
        XOR     AH,AH           ; AX = 'Y'
        SHL     AX,1            ; * 2
        SHL     AX,1            ; * 4
        SHL     AX,1            ; * 8
        SHL     AX,1            ; * 16
        SHL     AX,1            ; * 32
        MOV     BX,AX           ; Copy at *32
        SHL     AX,1            ; * 64
        SHL     AX,1            ; * 128
        ADD     AX,BX           ; * 160
    else
; Multiply AL by 160 for 80*2 bytes/line (Slower compact version)
        MOV     BL,160          ; Size of line (in bytes)
        MUL     BL              ; Calculate 'Y' offset
    endif
        MOV     BL,DL           ; Copy 'X'
        XOR     BH,BH           ; Zero high byte
        SHL     BX,1            ; * 2 for char & attribute bytes
        ADD     BX,AX           ; BX = character position
        RET
;
; Set the CURSOR OFF: wcursor_off()
;
_vcursor_off:
        MOV     CX,2020h        ; Value for cursor off
        JMP SHORT SETC2         ; And set it
;
; Set the CURSOR to a block: wcursor_block()
;
_vcursor_block:
        MOV     CX,0006h        ; Color block cursor
        MOV     DX,000Bh        ; Monochrome block cursor
        JMP SHORT SETC1         ; and proceed
;
; Set the CURSOR to a LINE: wcursor_line()
;
_vcursor_line:
        MOV     CX,0607h        ; Color line cursor
        MOV     DX,0B0Ch        ; Monochrome line cursor
SETC1:  MOV     AX,DGRP:_V_BASE ; Get video base
        CMP     AX,0B000h       ; Monochrome?
        JNZ     SETC2           ; No, do it
        MOV     CX,DX           ; Copy for later
; Set cursor to value in CX
SETC2:  MOV     AH,1            ; Set cursor type
        INT     10h             ; Call BIOS
        RET
;
CSEG    ENDS
        END
