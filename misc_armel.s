@ vim:filetype=armasm

@ Generic memory routines.
@ (c) Copyright 2007, Grazvydas "notaz" Ignotas

.text

.global memcpy32 @ int *dest, int *src, int count
.type memcpy32, function

memcpy32:
    stmfd   sp!, {r4,lr}

    subs    r2, r2, #4
    bmi     mcp32_fin

mcp32_loop:
    ldmia   r1!, {r3,r4,r12,lr}
    subs    r2, r2, #4
    stmia   r0!, {r3,r4,r12,lr}
    bpl     mcp32_loop

mcp32_fin:
    tst     r2, #3
    ldmeqfd sp!, {r4,pc}
    tst     r2, #1
    ldrne   r3, [r1], #4
    strne   r3, [r0], #4

mcp32_no_unal1:
    tst     r2, #2
    ldmneia r1!, {r3,r12}
    ldmfd   sp!, {r4,lr}
    stmneia r0!, {r3,r12}
    bx      lr

.size memcpy32, .-memcpy32

.global memset32 @ int *dest, int c, int count
.type memset32, function

memset32:
    stmfd   sp!, {lr}

    mov     r3, r1
    subs    r2, r2, #4
    bmi     mst32_fin

    mov     r12,r1
    mov     lr, r1

mst32_loop:
    subs    r2, r2, #4
    stmia   r0!, {r1,r3,r12,lr}
    bpl     mst32_loop

mst32_fin:
    tst     r2, #1
    strne   r1, [r0], #4

    tst     r2, #2
    stmneia r0!, {r1,r3}

    ldmfd   sp!, {lr}
    bx      lr

.size memset32, .-memset32

