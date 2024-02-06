; Загрузчик сдаточных тестов

        mov     #100,@#177660
        mov     #100000,R0      ;сперва переместим перемещатель
        mov     #mover_end,R1   ;в верхние адреса
1$:     mov     -(R1),-(R0)
        cmp     R1,#mover_begin
        bhi     1$
        jmp     (R0)            ;запустим перемещатель

mover_begin:
        mov     #TST_BEGIN,R1   ;начало массива тестов
        mov     #TST_END,R2     ;конец массива тестов
        clr     R0              ;адрес, куда переместить массив
1$:     mov     (R1)+,(R0)+
        cmp     R1,R2
        blo     1$
        mov     @#24,R0         ;берём точку входа
        jmp     (R0)            ;запускаем тест
        mov     #2$, R0
        mov     2$(R0),R0
        .blkb   10
2$:
mover_end:
TST_BEGIN:
        .org 1400
TST_END:
        .end
