;
; Functions to perform basic arithmetic operations on arbitrarily
; large unsigned numbers. Supports: + - * / % <<1 >>1 == != < >
;
; ?COPY.TXT 1994-2005 Dave Dunfield
; **See COPY.TXT**.
;
; To work on other than 32 bits, adjust the following constant:
LSIZE   EQU     4               ; 4 bytes (32 bits)
;
DGRP    GROUP DSEG,BSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
BSEG    SEGMENT BYTE PUBLIC 'UDATA'
_Longreg DB     LSIZE DUP(?)
BSEG    ENDS
        PUBLIC  _Longreg
        PUBLIC  _longadd, _longsub, _longshl, _longshr, _longcpy
        PUBLIC  _longtst, _longmul, _longdiv, _longcmp, _longset
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME CS:CSEG, DS:DGRP, SS:DGRP
;
; Add two LONG numbers: longadd(num1, num2)
;
_longadd: MOV   BX,SP           ; Address parms
        MOV     DI,4[BX]        ; Get dest
        MOV     SI,2[BX]        ; Get source
?bgadd: MOV     CX,LSIZE        ; Get # bytes to add
        XOR     AH,AH           ; Zero high, no carry in
?1:     MOV     AL,[DI]         ; Get source
        ADC     AL,[SI]         ; Advance
        MOV     [DI],AL         ; Resave
        INC     DI              ; Advance dest
        INC     SI              ; Advanuce source
        LOOP    ?1              ; Do them all
        MOV     AL,AH           ; Zero LOW
        RCL     AL,1            ; Shift in carry
        RET
;
; Subtract two LONG numbers: longsub(num1, num2)
;
_longsub: MOV   BX,SP           ; Address parms
        MOV     DI,4[BX]        ; Get dest
        MOV     SI,2[BX]        ; Get source
?bgsub: MOV     CX,LSIZE        ; Get # bytes to subtract
        XOR     AH,AH           ; Zero high, no carry in
?2:     MOV     AL,[DI]         ; Get byte from dest
        SBB     AL,[SI]         ; Subtract byte from source
        MOV     [DI],AL         ; Resave
        INC     DI              ; Advance dest
        INC     SI              ; Advance source
        LOOP    ?2              ; Do them all
        MOV     AL,AH           ; Zero LOW
        RCL     AL,1            ; Shift in borrow
        RET
;
; Shift a LONG number left: longshr(num1)
;
_longshl: MOV   BX,SP           ; Address parms
        MOV     DI,2[BX]        ; Get dest
?bgshl: XOR     AX,AX           ; Zero result, no carry in
?bgslc: MOV     CX,LSIZE        ; Get byte count
?3:     RCL     BYTE PTR [DI],1 ; Shift a byte
        INC     DI              ; Advance dest
        LOOP    ?3              ; Do them all
        RCL     AL,1            ; Return carry out
        RET
;
; Shift a LONG number right: longshl(num1)
;
_longshr: MOV   BX,SP           ; Address parms
        MOV     DI,2[BX]        ; Get dest
?bgshr: MOV     CX,LSIZE        ; Get byte count
        ADD     DI,CX           ; Advance to end
        XOR     AX,AX           ; Zero result, no carry in
?4:     DEC     DI              ; Backup register
        RCR     BYTE PTR [DI],1 ; Shift it right
        LOOP    ?4              ; Do them all
        RCL     AL,1            ; Return carry out
        RET
;
; Copy one LONG number to another: longcpy(num1, num2)
;
_longcpy: MOV   BX,SP           ; Address parms
        MOV     DI,4[BX]        ; Get dest
        MOV     SI,2[BX]        ; Get source
?bgcpy: MOV     CX,LSIZE        ; Get byte count
?5:     MOV     AL,[SI]         ; Get byte from source
        MOV     [DI],AL         ; Write byte to dest
        INC     SI              ; Advance source
        INC     DI              ; Advance dest
        LOOP    ?5              ; Copy them all
        RET
;
; Zero the working register
;
?ztemp: MOV     DI,OFFSET DGRP:_Longreg ; Point to register
        MOV     DX,DI           ; Copy for speedy access
?zlong: MOV     CX,LSIZE        ; Get length to zero
        XOR     AL,AL           ; Get zero
?6:     MOV     [DI],AL         ; Write a zero
        INC     DI              ; Advance dest
        LOOP    ?6              ; Do them all
        RET
;
; Test a LONG number for zero/non-zero: longtst(num1)
;
_longtst: MOV   BX,SP           ; Address parms
        MOV     DI,2[BX]        ; Get parm
?bgtst: MOV     CX,LSIZE        ; Get byte count
        XOR     AX,AX           ; Begin with zero
?7:     OR      AL,[DI]         ; Get byte from string
        JNZ     ?8              ; Not zero
        INC     DI              ; Advance pointer
        LOOP    ?7              ; To them all
        AND     AL,AL           ; Set 'Z'
?8:     RET
;
; Multiply two LONG numbers: longmul(num1, num2)
;
_longmul: MOV   BX,SP           ; Address parms
        SUB     SP,LSIZE        ; Local copy
        CALL    ?ztemp          ; Zero the temp register
        MOV     DI,SP           ; Point to temp
        MOV     SI,2[BX]        ; Point to num2
        CALL    ?bgcpy          ; Make a copy
?9:     MOV     DI,4[BX]        ; Point to num1
        CALL    ?bgshr          ; Shift right
        AND     AL,AL           ; Carry out?
        JZ      ?10             ; No, don't add
        MOV     DI,DX           ; Point to result
        MOV     SI,SP           ; Point to temp
        CALL    ?bgadd          ; Add them
?10:    MOV     DI,4[BX]        ; Point to num1
        CALL    ?bgtst          ; Hit zero
        JZ      ?11             ; Yes, exit
        MOV     DI,SP           ; Point to temp
        CALL    ?bgshl          ; Shift it left
        MOV     DI,SP           ; Point to temp
        CALL    ?bgtst          ; Is it zero?
        JNZ     ?9              ; No, keep going
?11:    ADD     SP,LSIZE        ; Fix stack
        MOV     DI,4[BX]        ; Dest = num1
        MOV     SI,DX           ; Point to result
        JMP     SHORT ?bgcpy    ; Perform copy
;
; Divide two LONG numbers: longdiv(num1, num2)
;
_longdiv: PUSH  BP              ; Save callers stack frame
        MOV     BP,SP           ; Address parms
        CALL    ?ztemp          ; Zero temporary register
        MOV     BL,(LSIZE*8)+1  ; Loop count (# bits)
?12:    CLC                     ; No carry in
?13:    MOV     DI,6[BP]        ; Point to num1
        CALL    ?bgslc          ; Shift it over AL.0 = CY out
        DEC     BL              ; Reduce count
        JZ      ?14             ; Time to exit
        MOV     DI,DX           ; Point to remainder
        RCR     AL,1            ; CY = AL.0
        CALL    ?bgslc          ; Shift it in
        MOV     DI,DX           ; Point to remainder
        MOV     SI,4[BP]        ; Point to num2
        CALL    ?bgcmp          ; Compare
        AND     AH,AH           ; reg < num2
        JNZ     ?12             ; Carry in 0
        MOV     DI,DX           ; Point to remainder
        MOV     SI,4[BP]        ; Point to num2
        CALL    ?bgsub          ; Perform subtract
        STC                     ; Carry in 1
        JMP     SHORT ?13       ; And proceed
?14:    POP     BP              ; Restore caller
        RET
;
; Compare two LONG numbers: longcmp(num1, num2)
;
_longcmp: MOV   BX,SP           ; Point to parameters
        MOV     DI,4[BX]        ; Get num1
        MOV     SI,2[BX]        ; Get num2
?bgcmp: MOV     CX,LSIZE        ; Get byte count
        ADD     DI,CX           ; Offset num1
        ADD     SI,CX           ; Offset num2
?15:    DEC     DI              ; Backup num1
        DEC     SI              ; Backup num2
        MOV     AL,[DI]         ; Get source
        CMP     AL,[SI]         ; Are they the same?
        JB      ?16             ; num1 < num2
        JA      ?17             ; num1 > num2
        LOOP    ?15             ; Do them all
        XOR     AX,AX           ; Num1 == num2
        RET
?16:    MOV     AX,-1           ; Return -1
        RET
?17:    MOV     AX,1            ; Return 1
        RET
;
; Set a LONG number: longset(num1, value);
;
_longset: MOV   BX,SP           ; Address parms
        MOV     DI,4[BX]        ; Point to number
        CALL    ?zlong          ; Zero it
        MOV     DI,4[BX]        ; Get pointer back
        MOV     AX,2[BX]        ; Get value
        MOV     [DI],AX         ; Save it
        RET
;
CSEG    ENDS
        END
