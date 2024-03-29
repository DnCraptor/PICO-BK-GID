;блок расширенной арифметики из комплекта TURBO8

ARIFM:  MOV     PC,@#10
        ADD     (PC)+,@(PC)+
        .WORD   @START+4
        .WORD   10
        MOV     #340,@#12
        RETURN

START:  SUB     #2,SP
        MOV     SP,(SP)
        ADD     #6,(SP)
        MOV     R5,-(SP)
        MOV     R4,-(SP)
        MOV     R3,-(SP)
        MOV     R2,-(SP)
        MOV     R1,-(SP)
        MOV     R0,-(SP)
        MOV     16(SP),R0
        MOV     -(R0),R1
        BIC     #7,R1

FADD:   CMP     #75000,R1
        BNE     FSUB
        CALL    WODFL
        BR      FADSU

FSUB:   CMP     #75010,R1
        BNE     FMUL
        CALL    WODFL
        ADD     #100,REVI2

FADSU:  SUB     R4,R5
        BEQ     2
        BPL     1
        NEG     R5
0:      ASR     R2
        ROR     R3
        SOB     R5,0
        BR      2
1:      ADD     R5,R4
        ASR     R0
        ROR     R1
        SOB     R5,1
2:      BIT     #10,REVI2
        BEQ     3
        COM     R0
        COM     R1
        ADD     #1,R1
        ADC     R0
3:      BIT     #100,REVI2
        BEQ     4
        COM     R2
        COM     R3
        ADD     #1,R3
        ADC     R2
4:      BIC     #1,REVI2
        ADD     R3,R1
        ADC     R0
        ADD     R2,R0
        BPL     FMUDI
        INC     REVI2
        COM     R0
        COM     R1
        ADD     #1,R1
        ADC     R0
        BR      FMUDI

FMUL:   CMP     #75020,R1
        BNE     FDIV
        CALL    WODFL
        DEC     R5
        MOV     R4,-(SP)
        MOV     R5,-(SP)
        MOV     #31,R4
        CLR     -(SP)
        CLR     R5
6:      ASR     (SP)
        ROR     R5
        ASR     R0
        ROR     R1
        BCC     5
        ADD     R3,R5
        ADC     (SP)
        ADD     R2,(SP)
5:      SOB     R4,6
        BR      FDIMU

FDIV:   CMP     #75030,R1
        BNE     ASH
        CALL    WODFL
        TST     R2
        BNE     7
        BIS     #13,@REVI0
        BR      ERROR
7:      DEC     R5
        NEG     R5
        MOV     R4,-(SP)
        MOV     R5,-(SP)
        MOV     #31,R4
        CLR     -(SP)
        CLR     R5
8:      ASL     R5
        ROL     (SP)
        SUB     R3,R1
        SBC     R0
        SUB     R2,R0
        BPL     0
        ADD     R3,R1
        ADC     R0
        ADD     R2,R0
        BR      1
0:      BIS     #1,R5
1:      ASL     R1
        ROL     R0
        SOB     R4,8

FDIMU:  MOV     (SP)+,R0
        MOV     R5,R1
        MOV     (SP)+,R5
        MOV     (SP)+,R4

FMUDI:  TST     R0
        BNE     0
        TST     R1
        BNE     0
        BIS     #4,@REVI0
        BR      QMUDI
0:      BIT     #1000,R0
        BEQ     1
3:      ASR     R0
        ROR     R1
        INC     R4
1:      BIT     #400,R0
        BNE     2
        ASL     R1
        ROL     R0
        DEC     R4
        BR      1
2:      ADD     #1,R1
        ADC     R0
        BIT     #1000,R0
        BNE     3
        ASR     R0
        ROR     R1
        BIC     #200,R0
        ADD     R4,R5
        ADD     #200,R5
        BPL     4
        BIS     #12,@REVI0
        BR      ERROR
4:      CMP     #377,R5
        BPL     5
        BIS     #2,@REVI0
        BR      ERROR
5:      SWAB    R5
        ROR     R5
        BIS     R5,R0
        BIT     #1,REVI2
        BEQ     QMUDI
        BIS     #100000,R0
        BIS     #10,@REVI0
QMUDI:  MOV     REVI4,R4
        ADD     #4,(R4)
        MOV     (R4),R4
        MOV     R0,(R4)+
        MOV     R1,(R4)+
        BR      E000
ERROR:  MOV     (SP)+,R0
        MOV     (SP)+,R1
        MOV     (SP)+,R2
        MOV     (SP)+,R3
        MOV     (SP)+,R4
        MOV     (SP)+,R5
        MOV     @#244,(SP)
        CCC
        RETURN

ASH:    BIC     #777,R1
        CMP     #72000,R1
        BNE     ASHC
        CALL    ARGUM
        CALL    ORGIS
        BEQ     0
        BPL     1
        NEG     R5
2:      ASR     R3
        SOB     R5,2
        BR      0
1:      ASL     R3
        SOB     R5,1
0:      CALL    PRIZN
        MOV     R3,(R1)
        BNE     E001
        BIS     #4,(R0)
E000:   BR      E001

ASHC:   CMP     #73000,R1
        BNE     MUL
        CALL    ARGUM
        CALL    ORGIS
        BEQ     0
        BPL     1
        NEG     R5
2:      ASR     R3
        ROR     R4
        SOB     R5,2
        BR      0
1:      ASL     R4
        ROL     R3
        SOB     R5,1
0:      CALL    PRIZN
        MOV     R3,(R1)
        BNE     3
        MOV     R4,(R2)
        BNE     E002
        BIS     #4,(R0)
3:      MOV     R4,(R2)
E001:   BR      E002

MUL:    CMP     #70000,R1
        BNE     DIV
        CALL    ARGUM
        CLR     -(SP)
        MOV     (R1),R2
        BPL     0
        NEG     R2
        COM     (SP)
0:      TST     R5
        BPL     1
        NEG     R5
        COM     (SP)
1:      MOV     #20,R3
        CLR     R4
3:      CLC
        ROR     R4
        ROR     R5
        BCC     2
        ADD     R2,R4
2:      SOB     R3,3
        MOV     REVI0,R3
        TST     R4
        BNE     4
        TST     R5
        BNE     5
        BIS     #4,(R3)
        TST     (SP)+
        BR      6
4:      BIS     #1,(R3)
5:      ROR     R4
        ROR     R5
        TST     (SP)+
        BEQ     6
        BIS     #10,(R3)
        COM     R4
        COM     R5
        ADD     #1,R5
        ADC     R4
6:      BIT     #100,(R0)
        BNE     7
        MOV     R4,(R1)+
7:      MOV     R5,(R1)
E002:   BR      E003

DIV:    CMP     #71000,R1
        BNE     QUITS
        CALL    ARGUM
        CLR     -(SP)
        BIT     #100,(R0)
        BNE     0
        MOV     (R1)+,R2
        MOV     (R1)+,R3
        TST     R5
        BGT     1
        BNE     2
        BIS     #1,(R4)
        BR      0
2:      INC     (SP)
        NEG     R5
1:      TST     R2
        BPL     3
        ADD     #11,(SP)
        COM     R2
        COM     R3
        ADD     #1,R3
        ADC     R2
3:      ASL     R3
        ROL     R2
        CMP     R5,R2
        BHI     4
0:      BIS     #2,(R4)
        TST     (SP)+
        BR      E003
4:      MOV     #17,R4
        MOV     #1,R0
6:      ASL     R3
        ROL     R2
        SUB     R5,R2
        BCC     5
        ADD     R5,R2
        SOB     R4,6
        BR      7
5:      BIS     R0,R3
        SOB     R4,6
7:      MOV     REVI0,R4
        TST     R3
        BNE     8
        BIS     #4,(R4)
        BR      9
8:      BIT     #1,(SP)
        BEQ     9
        NEG     R3
        BIS     #10,(R4)
9:      BIT     #10,(SP)+
        BEQ     10
        ADC     R2
10:     MOV     R2,-(R1)
        MOV     R3,-(R1)
E003:   BR      E004

QUITS:  MOV     (SP)+,R0
        MOV     (SP)+,R1
        MOV     (SP)+,R2
        MOV     (SP)+,R3
        MOV     (SP)+,R4
        MOV     (SP)+,R5
        MOV     @#4,(SP)
        CCC
        RETURN

E004:   MOV     (SP)+,R0
        MOV     (SP)+,R1
        MOV     (SP)+,R2
        MOV     (SP)+,R3
        MOV     (SP)+,R4
        MOV     (SP)+,REVI4
        MOV     (SP)+,R5
        MOV     2(SP),-(R5)
        MOV     (SP),-(R5)
        MOV     R5,SP
        MOV     REVI4,R5
        RTI

FLACS:  MOV     (R1)+,R0
        MOV     (R1),R1
        MOV     R0,R4
        ROL     R4
        SWAB    R4
        BIC     #177400,R4
        BEQ     0
        BIS     #200,R0
0:      SUB     #200,R4
        ASL     R1
        ROL     R0
        BIC     #177000,R0
        RETURN

SBPSW:  MOV     SP,R4
        ADD     #24,R4
        BIC     #17,(R4)
        MOV     R4,REVI0
        RETURN

REGIS:  ASL     R1
        BIC     #-17,R1
        ADD     SP,R1
        ADD     #4,R1
        RETURN

WODFL:  CLR     REVI2
        CALL    SBPSW
        MOV     (R0),R1
        CALL    REGIS
        MOV     R1,REVI4
        MOV     (R1),R1
        MOV     R1,R2
        CALL    FLACS
        BCC     0
        ADD     #101,REVI2
0:      MOV     R1,R3
        MOV     R2,R1
        MOV     R0,R2
        MOV     R4,R5
        ADD     #4,R1
        CALL    FLACS
        BCC     1
        ADD     #11,REVI2
1:      RETURN

ORGIS:  MOV     R1,R2
        BIT     #100,(R0)
        BNE     0
        TST     (R2)+
0:      MOV     (R1),R3
        MOV     (R2),R4
        MOV     REVI0,R0
        BIC     #177700,R5
        BIT     #40,R5
        BEQ     1
        BIS     #177700,R5
1:      TST     R5
        RETURN

ARGUM:  MOV     (R0),R1
        CALL    REGIS
        MOV     (R0),R5
        BIC     #177717,R5
        CMP     #20,R5
        BEQ     0
        CMP     #40,R5
        BEQ     1
        CMP     #60,R5
        BEQ     2
        MOV     (R1),R5
        BR      3
0:      MOV     (R1),R5
        ADD     #2,(R1)
        BR      4
1:      SUB     #2,(R1)
        MOV     (R1),R5
        BR      4
2:      MOV     @20(SP),R5
        ADD     #2,20(SP)
        ADD     (R1),R5
4:      MOV     (R5),R5
3:      BIT     #10,(R0)
        BEQ     5
        MOV     (R5),R5
5:      MOV     (R0),R1
        ROL     R1
        ROL     R1
        SWAB    R1
        CALL    REGIS
        CALL    SBPSW
        RETURN

PRIZN:  BCC     0
        BIS     #1,(R0)
0:      MOV     R3,R4
        BPL     1
        BIS     #10,(R0)
1:      XOR     R5,(R1)
        BPL     2
        BIS     #2,(R0)
2:      RETURN
        .WORD   0,0,0,0
REVI0:  .WORD   0
REVI2:  .WORD   0
REVI4:  .WORD   0

        .END


