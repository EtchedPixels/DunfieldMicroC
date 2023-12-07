;
; Low level ASYNC serial I/O driver for
; use with DDS MICRO-C compiler on IBM/PC.
;
; ?COPY.TXT 1990-2005 Dave Dunfield
; **See COPY.TXT**.
;
; Misc constants.
BUFSIZ  EQU     256             ; Input buffer size (must be power of 2)
WINDOW  EQU     BUFSIZ-30       ; Flow control assertion window
XON     EQU     'Q'-40h         ; Output enable character
XOFF    EQU     'S'-40h         ; Output inhibit character
RXRDY   EQU     00000001b       ; Uart Receiver ready flag
TXRDY   EQU     00100000b       ; Uart Transmitter ready flag
S8259   EQU     0020h           ; 8259 interrupt controller
; Bits in driver control flags
RFLOW   EQU     10000000b       ; Received flow control
TFLOW   EQU     01000000b       ; Transmit flow control
TXOFF   EQU     00100000b       ; Transmit XOFF pending
TXON    EQU     00010000b       ; Transmit XON  pending
XPARENT EQU     00001000b       ; Transparency enabled
HFLOW   EQU     00000100b       ; Hardware flow control
;
DGRP    GROUP   DSEG,BSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
;
; Initialized variables & tables
;
_Cflags DB      0               ; Control flags
cominfo DW      0               ; Comm port information word
DSEG    ENDS
;
BSEG    SEGMENT BYTE PUBLIC 'UDATA'
comaddr DW      1 DUP(?)        ; Comm port address
oldoff  DW      1 DUP(?)        ; Saved old interrupt offset
oldseg  DW      1 DUP(?)        ; Saved old interrupt segment
rdptr   DW      1 DUP(?)        ; Buffer read pointer
wrptr   DW      1 DUP(?)        ; Buffer write pointer
buffer  DB      BUFSIZ DUP(?)   ; Receive buffer
BSEG    ENDS
;
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _Cflags, _Copen, _Cclose, _Ctestc, _Cgetc, _Cputc, _Csignals
;
; Open the com port: Copen(int port, int speed, char mode, char modem)
;
_Copen: PUSH    BP              ; Save callers stack
        MOV     BP,SP           ; Address parameters
        CALL    _Cclose         ; Insure its closed
        MOV     AX,10[BP]       ; Get com port number
        MOV     BX,1030h        ; Comm 1-3 mask & int vector
        MOV     DX,03FDh        ; Comm 1 address
        DEC     AX              ; Is it com1?
        JZ      copen1          ; Yes, it is
        MOV     DX,02FDh        ; Comm 2 address
        DEC     AX              ; Is it com2?
        JZ      copen2          ; Yes it is
        MOV     DX,03EDh        ; Comm 3 address
        DEC     AX              ; Is it com3?
        JZ      copen1          ; Yes it is
        MOV     DX,02EDh        ; Comm 4 address
        DEC     AX              ; Is it com4?
        JZ      copen2          ; Yes it is
; Report failure to open port
fail:   STI                     ; Insure interrupts enabled
        MOV     AX,-1           ; Indicate failure
        POP     BP              ; Restore caller
        RET
; Proceed with opening the comm port
copen2: MOV     BX,082Ch        ; Comm 2-4 mask & int vector
copen1: MOV     DGRP:comaddr,DX ; Save address
        CLI                     ; Inhibit interrupts
; Setup the uart
        DEC     DX              ; Backup ...
        DEC     DX              ; to line control register (FB)
        IN      AL,DX           ; Read current value
        OR      AL,80h          ; Enable baud rate register
        OUT     DX,AL           ; Write it
        MOV     AX,8[BP]        ; Get baud rate
        SUB     DX,3            ; Point to baud rate LSB (F8)
        OUT     DX,AL           ; Write it
        INC     DX              ; Advance to MSB (F9)
        MOV     AL,AH           ; Get MSB
        OUT     DX,AL           ; Write it
        DEC     DX              ; Backup to LSB (F8)
        IN      AL,DX           ; Re-read LSB
        MOV     AH,AL           ; Copy for later
        INC     DX              ; Back to MSB (F9)
        IN      AL,DX           ; Re-read MSB
        XCHG    AH,AL           ; Swap for multi
        CMP     AX,8[BP]        ; Does it match
        JNZ     fail            ; No, its dosn't
        MOV     AL,6[BP]        ; Get mode
        AND     AL,7Fh          ; Insure no baud rate
        INC     DX              ; Advance...
        INC     DX              ; to line control register (FB)
        OUT     DX,AL           ; Write it
        MOV     AL,01h          ; Receive interrupt only
        DEC     DX              ; Backup ...
        DEC     DX              ; to Interrupt enable register (F9)
        OUT     DX,AL           ; Write it
        MOV     AL,4[BP]        ; Get modem control
        ADD     DX,3            ; Point to modem control register (FC)
        OUT     DX,AL           ; Write it
; Clear out any pending characters
        SUB     DX,4            ; Point to data register (F8)
        IN      AL,DX           ; Read to clear interrupt
        IN      AL,DX           ; Read to clear interrupt
; Setup the interrupt controller
        MOV     DGRP:cominfo,BX ; Save INFO
        NOT     BH              ; Complement for enable
        IN      AL,S8259+1      ; Read interrupt mask
        AND     AL,BH           ; Enable serial port
        OUT     S8259+1,AL      ; Write interrupt controller
; Setup the interrupt vector
        XOR     BH,BH           ; Zero HIGH
        MOV     CS:DSVAL,DS     ; Save data segment for int handler
        XOR     AX,AX           ; Get a zero
        MOV     DGRP:rdptr,AX   ; Zero read pointer
        MOV     DGRP:wrptr,AX   ; Zero write pointer
        MOV     ES,AX           ; Zero EXTRA
        MOV     AX,ES:[BX]      ; Get old offset
        MOV     DGRP:oldoff,AX  ; Save old offset
        MOV     AX,ES:2[BX]     ; Get old segmemt
        MOV     DGRP:oldseg,AX  ; Save segment
        MOV     AX,OFFSET CSEG:COMINT; Point to routine
        MOV     ES:[BX],AX      ; Write new offset
        MOV     ES:2[BX],CS     ; Write new segment
        STI                     ; Re-enable interrupts
        XOR     AX,AX           ; Indicate success
        POP     BP              ; Restore caller
        RET
;
; Close the comm port: Cclose()
;
_Cclose:XOR     AX,AX           ; Get zero
        MOV     ES,AX           ; Point to interrupt vectors
        MOV     BX,DGRP:cominfo ; Get old vector
        AND     BX,BX           ; Is it set?
        JZ      cclo1           ; No, its not
        MOV     DGRP:cominfo,AX ; Indicate not set
        CLI                     ; Disable interrupts
; Disable interrupts on the uart
        MOV     DX,DGRP:comaddr ; Get uart address
        SUB     DX,4            ; Point to interrupt enable register (F9)
        OUT     DX,AL           ; Write zero (Disable all)
; Disconnect interrupt line (to allow COM3-4)
        ADD     DX,3            ; Point to Modem control register (FC)
        IN      AL,DX           ; Get current value
        AND     AL,07h          ; Clear OUT-2 (interrupt connect)
        OUT     DX,AL           ; Write new value
; Disable COMM interrupt on the 8259
        IN      AL,S8259+1      ; Read interrupt mask
        OR      AL,BH           ; Disable comm interrupts
        OUT     S8259+1,AL      ; Write interrupt mask
; Restore the old comm interrupt vector
        XOR     BH,BH           ; Zero high
        MOV     DX,DGRP:oldoff  ; Get old offset
        MOV     ES:[BX],DX      ; Restore old offset
        MOV     DX,DGRP:oldseg  ; Get old segment
        MOV     ES:2[BX],DX     ; Restore old segment
        STI                     ; Re-enable interrupts
cclo1:  RET
;
; Test for char from com port: int Ctestc()
;
_Ctestc: MOV    BX,DGRP:rdptr   ; Get read pointer
        CMP     BX,DGRP:wrptr   ; Test for data in buffer
        JNZ     cinp            ; Yes, we have some
        MOV     AX,-1           ; Report no data available
        RET
;
; Read a character from the comport: int Cgetc()
;
_Cgetc: MOV     BX,DGRP:rdptr   ; Get read pointer
        CMP     BX,DGRP:wrptr   ; Test for data in buffer
        JZ      _Cgetc          ; No characters, wait for them
; Read character from com port
cinp:   MOV     DI,OFFSET DGRP:buffer; Get I/O buffer address
        MOV     AL,[BX+DI]      ; Get character from buffer
        XOR     AH,AH           ; Zero high
        INC     BX              ; Advance read pointer
        AND     BX,(BUFSIZ-1)   ; Mask for buffer wrap
        MOV     DGRP:rdptr,BX   ; Resave read pointer
        CMP     BX,DGRP:wrptr   ; Did we just empty buffer?
        JNZ     cinp4           ; No, its ok
        PUSH    AX              ; Save for later
; Buffer is empty, send XON if necessary
        MOV     DX,DGRP:comaddr ; Point to com port
        CLI                     ; No interrupts
        MOV     AH,DGRP:_Cflags ; Get control flags
        TEST    AH,HFLOW        ; Hardware flow?
        JZ      cinp0b          ; Not supported
        DEC     DX              ; Backup to control
        IN      AL,DX           ; Get status
        TEST    AL,02h          ; Already clear?
        JNZ     cinp0a          ; Nothing to do
        OR      AL,02h          ; Set flag
        OUT     DX,AL           ; Write it
cinp0a: INC     DX              ; Advance
cinp0b: TEST    AH,TFLOW        ; Flow controlled?
        JZ      cinp3           ; No, its not
        AND     AH,NOT (TFLOW OR TXOFF OR TXON); Clear the flags
        IN      AL,DX           ; Get status
        TEST    AL,TXRDY        ; Ok to send?
        JZ      cinp1           ; No, set pending
        SUB     DX,5            ; Backup to data port
        MOV     AL,XON          ; Get XON character
        OUT     DX,AL           ; Send the XON
        JMP SHORT cinp2         ; And continue
; Cannot send now, set pending flag
cinp1:  OR      AH,TXON         ; Set XON pending flag
cinp2:  MOV     DGRP:_Cflags,AH ; Resave the flags
cinp3:  STI                     ; Re-enable interrupts
        POP     AX              ; Restore character
cinp4:  RET
;
; Write a character to the com port: Cputc(char c)
;
_Cputc: PUSH    BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parameters
cput1:  MOV     DX,DGRP:comaddr ; Get address of uart
        IN      AL,DX           ; Read uart status
        TEST    AL,TXRDY        ; Ok to transmit
        JZ      cput1           ; No, wait for it
        CLI                     ; Disallow interrupts
        MOV     AH,DGRP:_Cflags ; Get control flags
; Test for pending XOFF to send
        TEST    AH,TXOFF        ; Transmit XOFF?
        JZ      cput2           ; No, try next
        MOV     AL,XOFF         ; Get XOFF
        AND     AH,NOT TXOFF    ; Clear the bit
        JMP SHORT cput3         ; Write to comm port
; Test for pending XON to send
cput2:  TEST    AH,TXON         ; Transmit XON?
        JZ      cput4           ; No, output character
        MOV     AL,XON          ; Get XON
        AND     AH,NOT TXON     ; Clear the bit
; Resave the control flags & proceed
cput3:  MOV     DGRP:_Cflags,AH ; Re-save control flags
        STI                     ; Re-enable interrupts
        SUB     DX,5            ; Position to data address
        OUT     DX,AL           ; Write character
        JMP SHORT cput1         ; And proceed
; No pending flow control, output data
cput4:  STI                     ; Re-enable interrupts
        TEST    AH,RFLOW        ; Output inhibited?
        JNZ     cput1           ; Yes, wait for it
        TEST    AH,HFLOW        ; Hardware flow enabled?
        JZ      cput5           ; No, it's OK
        INC     DX              ; Point to line control
        IN      AL,DX           ; Read line status
        TEST    AL,10h          ; CTS enabled?
        JZ      cput1           ; No - wait for it
        DEC     DX              ; Backup
cput5:  SUB     DX,5            ; Position to data address
        MOV     AL,4[BP]        ; Get character
        OUT     DX,AL           ; Write to comm port
        POP     BP              ; Restore caller
        RET
;
; Read the com port signals: int Csignals()
;
_Csignals: MOV  DX,DGRP:comaddr ; Get the com port address
        INC     DX              ; Advance to modem status register
        IN      AL,DX           ; Read modem status
        XOR     AH,AH           ; Zero high bits
        RET
;
; Comms Interrupt handler
;
COMINT: PUSH    AX              ; Save AX
        PUSH    BX              ; Save BX
        PUSH    DX              ; Save DX
        PUSH    DI              ; Save DI
        PUSH    DS              ; Save DS
        MOV     DS,CS:DSVAL     ; Get data segment
        MOV     DX,DGRP:comaddr ; Get com port I/O address
        IN      AL,DX           ; Read uart status register
        TEST    AL,RXRDY        ; Receiver ready?
        JZ      cint4           ; No, Spurious interrupt
        SUB     DX,5            ; Backup to data port
        MOV     AH,DGRP:_Cflags ; Get comm flags
        IN      AL,DX           ; Read data character
        TEST    AH,XPARENT      ; Are we transparent?
        JNZ     cint2           ; Yes, do not interpret flow control
; Test for XOFF, inhibit output
        CMP     AL,XOFF         ; Is it XOFF?
        JNZ     cint1           ; No, try next
        OR      AH,RFLOW        ; Set flow control bit
        JMP SHORT cint3         ; and continue
; Test for XON, enable output
cint1:  CMP     AL,XON          ; Is it XON
        JNZ     cint2           ; No, its not
        AND     AH,NOT RFLOW    ; Reset flow control bit
        JMP SHORT cint3         ; and continue
; Normal character, stuff in buffer
cint2:  MOV     DI,OFFSET DGRP:buffer; Get I/O buffer address
        MOV     BX,DGRP:wrptr   ; Get write pointer
        MOV     [BX+DI],AL      ; Write into buffer
        INC     BX              ; Advance
        AND     BX,(BUFSIZ-1)   ; Mask for buffer wrap
        MOV     DGRP:wrptr,BX   ; Resave pointer
; Test for nearing end of buffer
        SUB     BX,DGRP:rdptr   ; Calculate size of buffer
        AND     BX,(BUFSIZ-1)   ; Mask for buffer wrap
        CMP     BX,WINDOW       ; Are we nearing end
        JB      cint4           ; No, its ok
        ADD     DX,4            ; Offset to control
; If hardware flow control enabled - drop RTS
        TEST    AH,HFLOW        ; Hardware flow control?
        JZ      cint2a          ; Not enabled
        IN      AL,DX           ; Read current value
        TEST    AL,02h          ; Already clear
        JZ      cint2a          ; Yes, don't reset
        AND     AL,0FDh         ; Clear
        OUT     DX,AL           ; Rewrite
cint2a: TEST    AH,XPARENT      ; Are we transparent?
        JNZ     cint4           ; Don't send flow ctrl
; Send XOFF, flow control dest
        OR      AH,TFLOW OR TXOFF; Indicate flow control asserted
        AND     AH,NOT TXON     ; Insure no XON pending
        INC     DX              ; Adjust to status
        IN      AL,DX           ; Read status
        TEST    AL,TXRDY        ; Test for transmitter ready
        JZ      cint3           ; Not ready
        SUB     DX,5            ; Backup to data port
        MOV     AL,XOFF         ; Get XOFF character
        OUT     DX,AL           ; Write to port
        AND     AH,NOT TXOFF    ; No pending XOFF needed
; Resave status flags to record changes
cint3:  MOV     DGRP:_Cflags,AH ; Resave flags
; Reset 8259, Restore registers & return from interrupt
cint4:  MOV     AL,20h          ; End of Interrupt command
        OUT     S8259,AL        ; Write to interrupt controller
        POP     DS              ; Restore DS
        POP     DI              ; Restore DI
        POP     DX              ; Restore DX
        POP     BX              ; Restore BX
        POP     AX              ; Restore AX
        IRET
; Saved data segment incase we are running in small model
DSVAL   DW      0               ; Stored code segment value
;
CSEG    ENDS
        END
