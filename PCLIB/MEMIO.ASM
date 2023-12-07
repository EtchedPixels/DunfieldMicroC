;
; DDS MICRO-C 8086/MS-DOS: Memory & I/O access functions
;
; ?COPY.TXT 1988-2005 Dave Dunfield
; **See COPY.TXT**.
;
DGRP    GROUP   DSEG
DSEG    SEGMENT BYTE PUBLIC 'IDATA'
DSEG    ENDS
CSEG    SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:CSEG, DS:DGRP, SS:DGRP
        PUBLIC  _peek, _peekw, _poke, _pokew, _in, _inw, _out, _outw
;
; Get a byte of memory: peek(segment, address)
;
_peek:  MOV     BX,SP           ; Address parms
        MOV     ES,4[BX]        ; Get segment
        MOV     BX,2[BX]        ; Get address
        MOV     AL,ES:[BX]      ; Read byte
        XOR     AH,AH           ; Zero high
        RET
;
; Get a word of memory: peekw(segment, address)
;
_peekw: MOV     BX,SP           ; Address parms
        MOV     ES,4[BX]        ; Get segment
        MOV     BX,2[BX]        ; Get address
        MOV     AX,ES:[BX]      ; Read word
        RET
;
; Set a byte of memory: poke(segment, address, value)
;
_poke:  MOV     BX,SP           ; Address parms
        MOV     ES,6[BX]        ; Get Segment
        MOV     AL,2[BX]        ; Get value
        MOV     BX,4[BX]        ; Get address
        MOV     ES:[BX],AL      ; Set byte
        RET
;
; Set a word of memory: pokew(segment, address, value)
;
_pokew: MOV     BX,SP           ; Address parms
        MOV     ES,6[BX]        ; Get Segment
        MOV     AX,2[BX]        ; Get value
        MOV     BX,4[BX]        ; Get address
        MOV     ES:[BX],AX      ; Set word
        RET
;
; Read a byte from an I/O port: in(port)
;
_in:    MOV     BX,SP           ; Address parms
        MOV     DX,2[BX]        ; Get port id
        IN      AL,DX           ; Read the byte
        XOR     AH,AH           ; Zero high
        RET
;
; Read a word from an I/O port: inw(port)
;
_inw:   MOV     BX,SP           ; Address parms
        MOV     DX,2[BX]        ; Get port id
        IN      AX,DX           ; Read the byte
        RET
;
; Write a byte to an I/O port: out(port, value)
;
_out:   MOV     BX,SP           ; Address parms
        MOV     DX,4[BX]        ; Get port id
        MOV     AL,2[BX]        ; Get value to write
        OUT     DX,AL           ; Write to port
        RET
;
; Write a word to an I/O port: out(port, value)
;
_outw:  MOV     BX,SP           ; Address parms
        MOV     DX,4[BX]        ; Get port id
        MOV     AX,2[BX]        ; Get value to write
        OUT     DX,AX           ; Write to port
        RET
;
CSEG    ENDS
        END
