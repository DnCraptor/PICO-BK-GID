; Пример назначения синонимов регистрам на примере
; реальной проги "Загрузчик сдаточных тестов"

    %0 = AX
    %1 = BX
    
        mov     #100, @#177660
        mov     #100000, AX     ;сперва переместим перемещатель
        mov     #mover_end, BX  ;в верхние адреса
1$:     mov     -(BX), -(AX)
        cmp     BX, #mover_begin
        bhi     1$
        jmp     (AX)            ;запустим перемещатель

    %1 = SRC
    %0 = DST

mover_begin:
        mov     #TST_BEGIN, SRC ;начало массива тестов
        mov     #TST_END, %2    ;конец массива тестов
        clr     DST             ;адрес, куда переместить массив
1$:     mov     (SRC)+, (DST)+
        cmp     SRC, %2
        blo     1$
        mov     @#24, DST       ;берём точку входа
        jmp     (DST)           ;запускаем тест
        mov     #2$, DST
        mov     2$(DST), DST
        .blkb   10
2$:
mover_end:
TST_BEGIN:
        .org 1400
TST_END:
        .end
