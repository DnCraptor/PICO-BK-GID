mov     #0xdead, R0
mov     #0b1111000011110000, R0
mov     #^xfaad, R0

.word   ^hFFFF, ^b1110001111000010
.word   ^o777, ^rONE

mov     #^f256.0, R1

mov     #^c101, R1
mov     #^c 0x85, R1

.word   ^c 25. + ^c "YE


mov     #-0xdead, R0
mov     #-0b1111000011110000, R0
mov     #-^xfaad, R0

.word   -^hFFFF, -^b1110001111000010
.word   -^o777, -^rONE

mov     #-^f256.0, R1

mov     #-^c101, R1
mov     #-^c 0x85, R1

.word   ^c-25. + -^c "YE

.end
