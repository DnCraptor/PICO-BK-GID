; Пример: Запустим 440Гц в 0 счетчике обоих каналов.
;  Cnt = 1000000 / 440 = ~2273
.la 400
cr = 12;
1$:     mov     #177714, r5         ; Порт Менестреля
        mov     #10000, r3          ; Строб
        mov     #300 + 10 + 1, r4   ; см. Примечание.cfg
        call    MenestrelOut
        mov     #2273., r2          ; 440Hz
        com     r2                  ; данные в инверсном виде
        mov     #0 + 1400, r1       ; шлем в оба 0х счетчика
        mov     r1, r4              ; отправляем младший байт
        bisb    r2, r4
        call    MenestrelOut
        swab    r2
        mov     r1, r4              ; отправляем старший байт
        bisb    r2, r4
        call    MenestrelOut        ; звук пойдет только сейчас
        halt
        jmp     1$
        .addr   r0, Buff

MenestrelOut:
        mov     r4, (r5)            ; Подготовка данных на входах счетчиков
        bis     r3, r4
        mov     r4, (r5)            ; Строб
        mov     (r6)+, r7           ; Выход. Чуть быстрее чем rts pc
1$:
        .word   .
        .word   2$
        .asciz  <cr>"test1"<cr+3><cr>
        .even
        .word   @MenestrelOut
        .word   MenestrelOut-.
        .word   1$
        .word   <1$ - MenestrelOut>/2
        .word   [[[1+3]*[4+6]] +5] << [4-2] ; когда много скобок, треугольные закрывающие скобки интерпретируются как shr
        .word   <7*5-6/2> ^ [2|1]
2$:
Buff:
buffer2 = Buff + 4000

.end
