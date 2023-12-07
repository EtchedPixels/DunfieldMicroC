;
; Low level IBM/PC console keyboard/window interface routines
; for use with the DDS MICRO-C compiler.
;
; ?COPY.TXT 1989-2005 Dave Dunfield
; **See COPY.TXT**.
;
CBASE   EQU     0B800h          ; Color video screen segment
MBASE   EQU     0B000h          ; Monochrome video screen segment
SPKCTL  EQU     0061h           ; Speaker control port
SPKFRE  EQU     0042h           ; Speaker frequency port
SPKMOD  EQU     0043h           ; Speaker mode port
;
DGRP    GROUP   DSEG,BSEG
;
; Initialized variables & tables
;
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
; Special key table - General keys
_W_KEYS DW      4800h           ; KUA   (Up arrow)
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
        DW      7300h           ; CLA   (CTRL-Left Arrow)
        DW      7400h           ; CRA   (CTRL-Right Arrow)
; ALT letter keys
        DW      1E00h           ; ALT-A
        DW      3000h           ; ALT-B
        DW      2E00h           ; ALT-C
        DW      2000h           ; ALT-D
        DW      1200h           ; ALT-E
        DW      2100h           ; ALT-F
        DW      2200h           ; ALT-G
        DW      2300h           ; ALT-H
        DW      1700h           ; ALT-I
        DW      2400h           ; ALT-J
        DW      2500h           ; ALT-K
        DW      2600h           ; ALT-L
        DW      3200h           ; ALT-M
        DW      3100h           ; ALT-N
        DW      1800h           ; ALT-O
        DW      1900h           ; ALT-P
        DW      1000h           ; ALT-Q
        DW      1300h           ; ALT-R
        DW      1F00h           ; ALT-S
        DW      1400h           ; ALT-T
        DW      1600h           ; ALT-U
        DW      2F00h           ; ALT-V
        DW      1100h           ; ALT-W
        DW      2D00h           ; ALT-X
        DW      1500h           ; ALT-Y
        DW      2C00h           ; ALT-Z
        DW      0
; BOX character table
BOXTAB  DB      0C4h, 0B3h, 0DAh, 0BFh, 0C0h, 0D9h
        DB      0CDh, 0BAh, 0C9h, 0BBh, 0C8h, 0BCh
        DB      0CDh, 0B3h, 0D5h, 0B8h, 0D4h, 0BEh
_W_COLS DB      80              ; Number of columns on screen
_W_ROWS DB      25              ; Number of rows on screen
DSEG    ENDS
;
; Uninitialized variables and tables
;
BSEG SEGMENT BYTE PUBLIC 'UDATA'
_W_OPEN DW      1 DUP(?)        ; Last opened window
_W_BASE DW      1 DUP(?)        ; Mono=B000h, Cga=B800h
_W_PAGE DB      1 DUP(?)        ; Current video page
BSEG ENDS
;
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _wopen, _wclose, _w_close, _wputc, _w_putc
        PUBLIC  _wclwin, _w_clwin, _wcleow, _w_cleow, _wcleol, _w_cleol
        PUBLIC  _wgotoxy, _w_gotoxy, _wupdatexy, _w_updatexy, _wgetc
        PUBLIC  _w_getc, _wtstc, _w_tstc, _wcursor_off, _wcursor_block
        PUBLIC  _wcursor_line, _W_OPEN, _W_BASE, _W_PAGE, _W_COLS, _W_ROWS
        EXTRN   _malloc:NEAR, _free:NEAR
;
; Close current window: wclose()
;
_wclose:CALL    XSETUP          ; Get last window
        JMP SHORT WCLOS0        ; And proceed
;
; Close a window: w_close(window)
;
_w_close:CALL   WSETUP          ; Get parameters
WCLOS0: XOR     DX,DX           ; Zero position
        MOV     BH,1[SI]        ; Get open flags
; If window was BOXed, adjust sizes first
        TEST    BH,60h          ; Was it BOXed
        JZ      WCLOS1          ; No, don't adjust
        MOV     AX,2[SI]        ; Get position
        DEC     AL              ; Expand 'X'
        DEC     AH              ; Expand 'Y'
        MOV     2[SI],AX        ; Resave
        MOV     AX,4[SI]        ; Get size
        ADD     AH,2            ; Adjust for bars
        ADD     AL,2            ; Adjust for bars
        MOV     4[SI],AX        ; Resave
; Clear window if requested
WCLOS1: TEST    BH,08h          ; Clear window?
        JZ      WCLOS2          ; No, try next
        CALL    CLEOS           ; Clear the window
        MOV     BH,1[SI]        ; Get flags back
; Restore previous contents if saved
WCLOS2: TEST    BH,80h          ; Do we restore it
        JZ      WCLOS4          ; No, just clear
; ... Set up pointers to screen & save area
        CALL    XYADDR          ; Calculate base address
        MOV     DX,4[SI]        ; Get 'X' and 'Y' sizes
        PUSH    SI              ; Save SI
        LEA     SI,14[SI]       ; Offset to data area
; ... Restore one line at a time
        MOV     AL,DGRP:_W_COLS ; Get # columns
        XOR     AH,AH           ; Zero high
        SHL     AX,1            ; Adjust for word entries
WCLOS3: MOV     DI,BX           ; Get destination address
        MOV     CL,DL           ; Get 'X' size
        XOR     CH,CH           ; Zero high size
    REP MOVSW                   ; Move one line
        ADD     BX,AX           ; Offset to next line
        DEC     DH              ; Reduce count
        JNZ     WCLOS3          ; And proceed
        POP     SI              ; Restore SI
        MOV     BH,1[SI]        ; Get flags back
; Restore the cursor
WCLOS4: MOV     CX,10[SI]       ; Get cursor shape
        MOV     AH,01h          ; Set shape
        INT     10h             ; Call BIOS
        MOV     DX,12[SI]       ; Get position
        MOV     BH,DGRP:_W_PAGE ; Get display page
        MOV     AH,02h          ; Set position
        INT     10h             ; Call BIOS
; If this is the active window, switch to previously open
        CMP     SI,DGRP:_W_OPEN ; Is this it?
        JNZ     WCLOS5          ; No, don't reset
        MOV     AX,8[SI]        ; Get last free
        MOV     DGRP:_W_OPEN,AX ; Save it
; Release the buffer contents
WCLOS5: PUSH    SI              ; Save address of buffer
        CALL    _free           ; Release it
        POP     SI              ; Fixup stack
WCLOS6: POP     ES              ; Restore ES
        POP     BP              ; Restore BP
        RET
;
; Open a window: wopen(px, py, sx, sy, flags)
;
_wopen: PUSH    BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parameters
        PUSH    ES              ; Save ES
; Set up video mode, active page & Number of columns
        MOV     AH,0Fh          ; Get video mode
        INT     10h             ; Call BIOS
        MOV     DGRP:_W_COLS,AH ; Save number of columns
        MOV     DGRP:_W_PAGE,BH ; Save video page
        MOV     BX,CBASE        ; Assume COLOR address
        CMP     AL,07h          ; Monochrome?
        JNZ     OINIT1          ; No, assumption correct
        MOV     BX,MBASE        ; Get MONOCHROME address
OINIT1: MOV     DGRP:_W_BASE,BX ; Set video base address
; Get number of rows (if supported)
        XOR     AX,AX           ; Get zero
        MOV     ES,AX           ; Address BIOS data area
        OR      AL,ES:[0484h]   ; Get # rows
        JZ      OINIT2          ; Unsupported
        INC     AL              ; Adjust to normal offset
        MOV     DGRP:_W_ROWS,AL ; Set # rows
OINIT2: MOV     ES,BX           ; Set up segment
; Allocate a window buffer
        MOV     AL,5[BP]        ; Get video flags
        AND     AL,80h          ; Do we save screen
        JZ      OINIT3          ; No, do not
        MOV     AL,6[BP]        ; Get 'Y' size
        MOV     BL,8[BP]        ; Get 'X' size
        MUL     BL              ; Calculate total size
        SHL     AX,1            ; X2 for attributes
OINIT3: ADD     AX,14           ; Include overhead
        PUSH    AX              ; Pass as parameter
        CALL    _malloc         ; Allocate the buffer
        MOV     SI,AX           ; Set up pointer
        POP     DX              ; Clean up stack
        AND     AX,AX           ; Did we get it?
        JZ      WCLOS6          ; Couldn't allocate - error
; Fill in window parameter block
        MOV     DL,12[BP]       ; Get 'X' position
        MOV     DH,10[BP]       ; Get 'Y' position
        MOV     2[SI],DX        ; Save it
        MOV     DL,8[BP]        ; Get 'X' size
        MOV     DH,6[BP]        ; Get 'Y' size
        MOV     4[SI],DX        ; Save size
        MOV     BX,4[BP]        ; Get window flags
        MOV     [SI],BX         ; Save attributes & flags
        MOV     DX,DGRP:_W_OPEN ; Get last opened window
        MOV     8[SI],DX        ; Save in buffer
        XOR     DX,DX           ; Reset cursor position
        MOV     6[SI],DX        ; Save initial cursor position
; Save window contents if required
        TEST    BH,80h          ; Do we save it?
        JZ      OINIT5          ; No we don't
; ... Set up pointers to screen & save area
        CALL    XYADDR          ; Get base address
        MOV     DX,4[SI]        ; Get 'X' and 'Y' sizes
        LEA     DI,14[SI]       ; Offset to data area
        PUSH    SI              ; Save SI
; ... Save one line at a time
        MOV     AL,DGRP:_W_COLS ; Get # columns
        XOR     AH,AH           ; Zero high
        SHL     AX,1            ; Adjust for word entries
        PUSH    ES              ; Stack ES
        PUSH    DS              ; And DS
        POP     ES              ; For swap
        POP     DS              ; ES <> DS
OINIT4: MOV     SI,BX           ; Get source address
        MOV     CL,DL           ; Get 'X' size
        XOR     CH,CH           ; Zero high size
    REP MOVSW                   ; Move one line
        ADD     BX,AX           ; Offset to next line
        DEC     DH              ; Reduce count
        JNZ     OINIT4          ; And proceed
; ... Restore registers and continue
        PUSH    ES              ; Stack ES
        PUSH    DS              ; And DS
        POP     ES              ; For swap
        POP     DS              ; ES <> DS
        POP     SI              ; Restore SI
        XOR     DX,DX           ; Reset cursor position
        MOV     BH,5[BP]        ; Get flags back
; Draw box if required
OINIT5: MOV     BP,OFFSET DGRP:BOXTAB; Point to BOX table
        MOV     CX,4[SI]        ; Get size of box
        SUB     CH,2            ; Adjust for bar
        DEC     CL              ; Adjust for bar
        AND     BH,60h          ; Draw BOX?
        JZ      OINIT8          ; No we don't
; ... Select box type
        CMP     BH,40h          ; Box1?
        JZ      OINIT6          ; Use this one
        ADD     BP,6            ; Offset to next
        CMP     BH,20h          ; Box2?
        JZ      OINIT6          ; Use this one
        ADD     BP,6            ; Offset to next
OINIT6: PUSH    CX              ; Save size
        CALL    XYADDR          ; Get address
; ... Draw the top (With corners)
        MOV     AH,[SI]         ; Get attribute
        MOV     AL,2[BP]        ; Top left corner
        MOV     ES:[BX],AX      ; Write top corner
        LEA     DI,2[BX]        ; Get value
        XOR     CH,CH           ; Zero high
        DEC     CL              ; Adjust for bar
        MOV     AL,0[BP]        ; Horizontal line
    REP STOSW                   ; Draw the line
        MOV     AL,3[BP]        ; Top right hand corner
        MOV     ES:[DI],AX      ; Write it
        POP     CX              ; Restore position
; ... Draw the box sides
OINIT7: INC     DH              ; Advance
        MOV     AL,1[BP]        ; Box side
        CALL    XYADDR          ; Get position
        MOV     ES:[BX],AX      ; Write it
        ADD     DL,CL           ; Offset
        CALL    XYADDR          ; Get position
        MOV     ES:[BX],AX      ; Write it
        SUB     DL,CL           ; Backup
        DEC     CH              ; Reduce count
        JNZ     OINIT7          ; Do them all
; ... Draw the bottom (With corners)
        INC     DH              ; Advance to bottom
        CALL    XYADDR          ; get address
        MOV     AL,4[BP]        ; Lower left corner
        MOV     ES:[BX],AX      ; Write it
        LEA     DI,2[BX]        ; Position to line
        DEC     CL              ; Adjust for bar
        MOV     AL,0[BP]        ; Horizontal line
    REP STOSW                   ; Write it out
        MOV     AL,5[BP]        ; Lower right corner
        MOV     ES:[DI],AX      ; Write it out
; ... Reduce size of active region
        MOV     AX,2[SI]        ; Get position
        INC     AL              ; Advance 'X'
        INC     AH              ; Advance 'Y'
        MOV     2[SI],AX        ; Resave
        MOV     AX,4[SI]        ; Get size
        SUB     AH,2            ; Chop out bars
        SUB     AL,2            ; Chop out bars
        MOV     4[SI],AX        ; And resave
        XOR     DX,DX           ; Reset cursor position
; Clear screen if required
OINIT8: MOV     BH,1[SI]        ; Get flags back
        TEST    BH,10h          ; Clear screen?
        JZ      OINIT9          ; No, its ok
        CALL    CLEOS           ; Clear one line
; Save cursor information
OINIT9: MOV     BH,DGRP:_W_PAGE ; Get video page
        MOV     AH,03h          ; Get cursor info
        INT     10h             ; Get cursor
        MOV     10[SI],CX       ; Save shape
        MOV     12[SI],DX       ; Save position
        MOV     AX,SI           ; Get buffer address
        MOV     DGRP:_W_OPEN,AX ; Save this window pointer
        JMP SHORT OEXIT1        ; Retore caller & return
;
; Write character into current window: wputc(int c)
;
_wputc: CALL    XSETUP          ; Get last open window
        MOV     AX,4[BP]        ; Get character
        JMP SHORT TSTCHR        ; And proceed
;
; Write a character to the video display: w_putc(int c, window)
;
_w_putc:CALL    WSETUP          ; Set up video addresses
        MOV     AX,6[BP]        ; Get character to display
TSTCHR: CMP     AX,'G'-40h      ; Bell character
        JZ      DOBELL          ; Yes, handle it
        CMP     AX,'M'-40h      ; Is it carriage return
        JZ      DOCR            ; Yes, handle it
        CMP     AX,'H'-40h      ; Is it backspace
        JZ      DOBS            ; Yes, handle it
        CMP     AX,'J'-40h      ; Is it newline?
        JZ      DONL            ; Yes, handle it
; Normal Character, output it
        CALL    XYADDR          ; Calculate address
        MOV     AH,[SI]         ; Get video attributes
        MOV     ES:[BX],AX      ; Write to video display
; Advance 'X' position
        INC     DL              ; Advance 'X'
        CMP     DL,4[SI]        ; Are we over?
        JB      VEXIT           ; No, its ok
        DEC     DL              ; Restore it
        MOV     AL,1[SI]        ; Get flags
        TEST    AL,01h          ; Line wrap enabled?
        JZ      VEXIT           ; No, skip it
RESETX: XOR     DL,DL           ; Reset 'X' position
; Advance 'Y' position
ADVY:   INC     DH              ; Advance 'Y' position
        CMP     DH,5[SI]        ; are we over?
        JB      VEXIT           ; No, Its OK
        DEC     DH              ; Reset it
        TEST    AL,02h          ; Screen wrap enabled?
        JZ      VEXIT           ; No, skip it
        CALL    SCROLL          ; Scroll window
; Restore callers environment & exit
VEXIT:  MOV     6[SI],DX        ; Resave 'X' and 'Y'
OEXIT1: POP     ES              ; Restore callers ES
        POP     BP              ; Restore callers stack frame
        RET
; Handle CARRIAGE RETURN
DOCR:   XOR     DL,DL           ; Reset 'X' position
        JMP     SHORT VEXIT     ; and proceed
; Handle BACKSPACE character
DOBS:   AND     DL,DL           ; Already at first col?
        JZ      VEXIT           ; Yes, don't backup
        DEC     DL              ; Reduce 'X' position
        JMP     SHORT VEXIT     ; And exit
; Handle NEWLINE character
DONL:   MOV     AL,1[SI]        ; Get flags
        TEST    AL,04h          ; LF only set?
        JZ      RESETX          ; No, reset 'X'
        JMP     SHORT ADVY      ; New line only
; Handle BELL character
DOBELL: IN      AL,SPKCTL       ; Get speaker control port
        OR      AL,03h          ; Enable speaker output
        OUT     SPKCTL,AL       ; Write to port
        MOV     AL,0B6h         ; Get mode setting
        OUT     SPKMOD,AL       ; Write to mode port
        MOV     CX,3            ; Number of ticks to wait
        OUT     SPKFRE,AL       ; Write to frequency port
        MOV     AX,04h          ; Get HIGH byte
        OUT     SPKFRE,AL       ; Write to port
        MOV     ES,AX           ; Address bios data
dobel1: MOV     BX,ES:[042Ch]   ; Get tick counter
        CMP     AX,BX           ; Changed?
        JZ      dobel1          ; No, wait for tick
        MOV     AX,BX           ; Get new value
        LOOP    dobel1          ; Wait for it
        IN      AL,SPKCTL       ; Read bits
        AND     AL,0FCh         ; Clear speaker
        OUT     SPKCTL,AL       ; Write back
        JMP     SHORT VEXIT     ; and exit
;
; CLEAR current window: wclwin()
;
_wclwin:CALL    XSETUP          ; Get current window
        JMP SHORT CLSCR1        ; And proceed
;
; CLEAR entire window: w_clwin(window)
;
_w_clwin: CALL WSETUP           ; Setup video
CLSCR1: XOR     DX,DX           ; Reset to top of screen
        MOV     6[SI],DX        ; Reset cursor
        CALL    UPDT1           ; Position cursor
        XOR     DX,DX           ; Reset again
        JMP SHORT CLSCR2        ; And proceed
;
; CLEAR to end of current window: wcleow()
;
_wcleow:CALL    XSETUP          ; Get current window
        JMP SHORT CLSCR2        ; And proceed
;
; CLEAR to END OF WINDOW: w_cleow(window)
;
_w_cleow:CALL   WSETUP          ; Setup video
CLSCR2: CALL    CLEOS           ; Clear to end
        JMP SHORT OEXIT1        ; Restore caller & return
;
; CLEAR to end of line in current: wcleol()
;
_wcleol:CALL    XSETUP          ; Get current window
        JMP SHORT CLEOL1        ; And proceed
;
; CLEAR to END OF LINE: w_cleol(window)
;
_w_cleol:CALL WSETUP            ; Setup video
CLEOL1: CALL    CLEOL           ; Clear the line
        JMP SHORT OEXIT1        ; Restore caller & return
;
; Position the cursor in current window: wgotoxy(int x, int y)
;
_wgotoxy:MOV    BX,SP           ; Address parameters
        MOV     AL,4[BX]        ; Get 'X' value
        MOV     AH,2[BX]        ; Get 'Y' value
        MOV     SI,DGRP:_W_OPEN ; Get open window
        JMP SHORT GOTO1         ; And proceed
;
; Position the cursor in window: w_gotoxy(int x, int y, window)
;
_w_gotoxy:MOV   BX,SP           ; Address parameters
        MOV     AL,6[BX]        ; Get 'X' value
        MOV     AH,4[BX]        ; Get 'Y' value
        MOV     SI,2[BX]        ; Get window buffer
GOTO1:  CMP     AL,4[SI]        ; In range?
        JAE     UPDT2           ; No, error
        CMP     AH,5[SI]        ; In range
        JAE     UPDT2           ; No, error
        MOV     6[SI],AX        ; Save values
        JMP SHORT UPDT1         ; And proceed
;
; Update the cursor in current window: wgotoxy()
;
_wupdatexy:MOV SI,DGRP:_W_OPEN; Get open window
        JMP SHORT UPDT1         ; And proceed
;
; Update the cursor position: w_updatexy(window)
;
_w_updatexy:MOV BX,SP           ; Address parameters
        MOV     SI,2[BX]        ; Get caller
UPDT1:  MOV     DX,2[SI]        ; Get starting address
        ADD     DX,6[SI]        ; Offset into window
        MOV     BH,DGRP:_W_PAGE ; Get video page
        MOV     AH,02h          ; Set cursor function
        INT     10h             ; Call DOS
UPDT2:  RET
;
; Check for a key from the keyboard: c = w_tstc(window)
;
_w_tstc:MOV     AH,01h          ; Check for key
        INT     16h             ; Do we have key?
        JNZ     _w_getc         ; Yes, read it
WTST1:  XOR     AX,AX           ; No key today
        RET
;
; Check for key with cursor in current window: c = wtstc()
;
_wtstc: MOV     AH,01h          ; Check for key
        INT     16h             ; Do we have key?
        JZ      WTST1           ; No, return zero
;
; Get in cursor in current window: c = wgetc()
;
_wgetc: MOV     SI,DGRP:_W_OPEN ; Get active window
        JMP SHORT GETC1         ; And proceed
;
; Get a key from the keyboard with translations: c = w_getc(window)
;
_w_getc:MOV     BX,SP           ; Address parameters
        MOV     SI,2[BX]        ; Get window
GETC1:  CALL    UPDT1           ; Update the cursor position
; Call BIOS to read key
        XOR     AH,AH           ; Function code 0 - read key
        INT     16h             ; Call bios
; Lookup key for special entries
        MOV     CL,80h          ; Beginning function code
        MOV     BX,OFFSET DGRP:_W_KEYS; Address of keys array
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
; Set the CURSOR OFF: wcursor_off()
;
_wcursor_off:
        MOV     CX,2020h        ; Value for cursor off
        JMP SHORT SETC2         ; And set it
;
; Set the CURSOR to a block: wcursor_block()
;
_wcursor_block:
        MOV     CX,0006h        ; Color block cursor
        MOV     DX,000Bh        ; Monochrome block cursor
        JMP SHORT SETC1         ; and proceed
;
; Set the CURSOR to a LINE: wcursor_line()
;
_wcursor_line:
        MOV     CX,0607h        ; Color line cursor
        MOV     DX,0B0Ch        ; Monochrome line cursor
SETC1:  MOV     AX,DGRP:_W_BASE ; Get video base
        CMP     AX,MBASE        ; Monochrome?
        JNZ     SETC2           ; No, do it
        MOV     CX,DX           ; Copy for later
; Set cursor to value in CX
SETC2:  MOV     AH,1            ; Set cursor type
        INT     10h             ; Call BIOS
        RET
;
; Set up addressability to video display & stack registers
; Exit: ES = video base address
;       SI = window buffer address
;       DX = cursor address (X/Y)
;
WSETUP: POP     AX              ; Get return address
        PUSH    BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parameters
        MOV     SI,4[BP]        ; Get window buffer
WSET1:  PUSH    ES              ; Save callers Extra segment
        MOV     ES,DGRP:_W_BASE ; Point to video base
        MOV     DX,6[SI]        ; Get 'X' and 'Y' position
        JMP     AX              ; Return to caller
;
; Set up addressability to currently open window
;
XSETUP: POP     AX              ; Get return address
        PUSH    BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parameters
        MOV     SI,DGRP:_W_OPEN ; Get open window
        JMP SHORT WSET1         ; And proceed
;
; Scroll window forward one line
;
SCROLL: PUSH    DS              ; Save data segment
        PUSH    DX              ; Save cursor
        PUSH    SI              ; Save SI
; First, calculate base address of window
        MOV     AL,3[SI]        ; Get 'Y' position
        MUL     DGRP:_W_COLS    ; Calculate 'Y' offset
        MOV     BL,2[SI]        ; Get 'X' position
        XOR     BH,BH           ; Zero high
        ADD     BX,AX           ; BX = character position
        SHL     BX,1            ; Adjust for word entries
        MOV     DI,BX           ; Get base window address
; Scroll one line at a time
        MOV     DX,4[SI]        ; Get 'X' and 'Y' sizes
        DEC     DH              ; Don't copy to last
        MOV     AL,DGRP:_W_COLS ; Get # columns
        XOR     AH,AH           ; Zero high
        SHL     AX,1            ; Adjust for word entries
        PUSH    ES              ; Stack ES
        POP     DS              ; To place in DS
SCROL1: PUSH    DI              ; Save DI
        MOV     SI,DI           ; Begin with same address
        ADD     SI,AX           ; Offset to next line
        MOV     CL,DL           ; Get 'X' size
        XOR     CH,CH           ; Zero high size
    REP MOVSW                   ; Move one line
        POP     DI              ; Restore it
        ADD     DI,AX           ; Offset to next line
        DEC     DH              ; Reduce count
        JNZ     SCROL1          ; And proceed
; Clear bottom line of screen
        MOV     CL,DL           ; Get 'X' size
        POP     SI              ; Restore SI
        POP     DX              ; Restore cursor
        POP     DS              ; Restore data segment
        MOV     AH,[SI]         ; Get attribute
        MOV     AL,' '          ; And space
    REP STOSW                   ; Clear it
        RET
;
; Clear from cursor(DX) to end of window
;
CLEOS:  PUSH    DX              ; Save cursor
CLEOS1: CALL    CLEOL           ; Get address
        XOR     DL,DL           ; Zero 'X'
        INC     DH              ; Advance 'Y'
        CMP     DH,5[SI]        ; Are we finished
        JB      CLEOS1          ; Keep going
        POP     DX              ; Restore cursor
        RET
;
; Clear from cursor(DX) to end of line
;
CLEOL:  CALL    XYADDR          ; Get address
        MOV     DI,BX           ; Set up address
        MOV     CL,4[SI]        ; Get size of li ne
        SUB     CL,DL           ; Calculate remaining
        XOR     CH,CH           ; Zero high
        MOV     AH,[SI]         ; Get attribute
        MOV     AL,' '          ; Clear to space
    REP STOSW                   ; Clear the li ne
        RET
;
; Calculate screen address from X/Y position (in DX)
; On exit: BX = address
;
XYADDR: PUSH    AX              ; Save AX
        MOV     AL,DH           ; Get 'Y' position
        ADD     AL,3[SI]        ; Offset from start of screen
        MUL     DGRP:_W_COLS    ; Calculate 'Y' offset
        MOV     BL,DL           ; Get 'X' position
        ADD     BL,2[SI]        ; Offset from start of screen
        XOR     BH,BH           ; Zero high byte
        ADD     BX,AX           ; BX = character position
        SHL     BX,1            ; Adjust for word entries
        POP     AX              ; Restore it
        RET
;
CSEG    ENDS
        END
