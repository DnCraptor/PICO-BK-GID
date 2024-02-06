; ������: �������� 440�� � 0 �������� ����� �������.
;  Cnt = 1000000 / 440 = ~2273
.la 400
cr = 12;
1$:     mov     #177714, r5         ; ���� ����������
        mov     #10000, r3          ; �����
        mov     #300 + 10 + 1, r4   ; ��. ����������.cfg
        call    MenestrelOut
        mov     #2273., r2          ; 440Hz
        com     r2                  ; ������ � ��������� ����
        mov     #0 + 1400, r1       ; ���� � ��� 0� ��������
        mov     r1, r4              ; ���������� ������� ����
        bisb    r2, r4
        call    MenestrelOut
        swab    r2
        mov     r1, r4              ; ���������� ������� ����
        bisb    r2, r4
        call    MenestrelOut        ; ���� ������ ������ ������
        halt
        jmp     1$
        .addr   r0, Buff

MenestrelOut:
        mov     r4, (r5)            ; ���������� ������ �� ������ ���������
        bis     r3, r4
        mov     r4, (r5)            ; �����
        mov     (r6)+, r7           ; �����. ���� ������� ��� rts pc
1$:
        .word   .
        .word   2$
        .asciz  <cr>"test1"<cr+3><cr>
        .even
        .word   @MenestrelOut
        .word   MenestrelOut-.
        .word   1$
        .word   <1$ - MenestrelOut>/2
        .word   [[[1+3]*[4+6]] +5] << [4-2] ; ����� ����� ������, ����������� ����������� ������ ���������������� ��� shr
        .word   <7*5-6/2> ^ [2|1]
2$:
Buff:
buffer2 = Buff + 4000

.end
