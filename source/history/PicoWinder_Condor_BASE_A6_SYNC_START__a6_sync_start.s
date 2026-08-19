.syntax unified
.cpu cortex-m0plus
.thumb
.section .text.sync_start,"ax",%progbits
.global a6_sync_op
.type a6_sync_op,%function
.thumb_func
a6_sync_op:
    cmp     r0, #0x20
    beq     1f
    ldr     r3, =0x100062b5
    bx      r3
1:
    push    {r4,r5,r6,lr}
    mov     r5, r1
    movs    r6, #0x7f
    ands    r5, r6

    /* Drain the software FIFO completely. */
    ldr     r4, =0x20010000
2:
    ldrh    r0, [r4, #0]
    ldrh    r1, [r4, #2]
    cmp     r0, r1
    beq     3f
    ldr     r3, =0x10005a3d
    blx     r3
    b       2b

    /* Wait until UART0 has physically transmitted everything. */
3:
    ldr     r4, =0x40034000
4:
    ldr     r0, [r4, #0x18]
    movs    r1, #0x80       /* TXFE */
    tst     r0, r1
    beq     4b
    movs    r1, #0x08       /* BUSY */
    tst     r0, r1
    bne     4b

    /* Give the SideWinder 2 ms to process the preceding DEFINE/MODIFY stream. */
    ldr     r4, =0x40054028 /* TIMERAWL */
    ldr     r0, [r4]
    ldr     r2, =2000
5:
    ldr     r1, [r4]
    subs    r1, r1, r0
    cmp     r1, r2
    blo     5b

    /* Send START directly, synchronously: B5 20 <effect id>. */
    ldr     r4, =0x40034000
6:
    ldr     r0, [r4, #0x18]
    movs    r1, #0x20       /* TXFF */
    tst     r0, r1
    bne     6b
    movs    r0, #0xb5
    str     r0, [r4]
7:
    ldr     r0, [r4, #0x18]
    movs    r1, #0x20
    tst     r0, r1
    bne     7b
    movs    r0, #0x20
    str     r0, [r4]
8:
    ldr     r0, [r4, #0x18]
    movs    r1, #0x20
    tst     r0, r1
    bne     8b
    str     r5, [r4]

    /* Do not return to USB until the START itself is on the wire. */
9:
    ldr     r0, [r4, #0x18]
    movs    r1, #0x80
    tst     r0, r1
    beq     9b
    movs    r1, #0x08
    tst     r0, r1
    bne     9b

    pop     {r4,r5,r6,pc}
.size a6_sync_op, .-a6_sync_op
