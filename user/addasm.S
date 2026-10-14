.data
fmt_result: .asciz "%lu\n"

.bss
buffer: .space 64

.text
.global main

main:
    # save data
    addi sp, sp, -24
    sd s1, 0(sp)
    sd s2, 8(sp)
    sd s3, 16(sp)

    # gets
    la a0, buffer
    li a1, 63
    call gets

    # convert to number
    call atoi
    mv s1, a0

    # gets
    la a0, buffer
    li a1, 63
    call gets

    # convert to number
    call atoi
    mv s2, a0

    # sum
    add s3, s1, s2

    # printf
    la a0, fmt_result
    mv a1, s3
    call printf

    # restore data
    ld s1, 0(sp)
    ld s2, 8(sp)
    ld s3, 16(sp)
    addi sp, sp, 24

    li a7, 2
    li a0, 0
    ecall