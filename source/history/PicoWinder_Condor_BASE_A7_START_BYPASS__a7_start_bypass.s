.syntax unified
.cpu cortex-m0plus
.thumb
.section .text.start_bypass,"ax",%progbits
.global a7_bypass_op
.type a7_bypass_op,%function
.thumb_func
a7_bypass_op:
    cmp     r0, #0x20
    beq     1f
    ldr     r3, =0x100062b5
    bx      r3
1:
    bx      lr
.size a7_bypass_op, .-a7_bypass_op
