    .extern ISR_Handler_Basic
    .section .bss
    .align 2
    .global KEY_CURR
    .global KEY_PREV
    .type KEY_CURR %object
    .type KEY_PREV %object
KEY_CURR:
    .zero 2
    .size KEY_CURR, 2
KEY_PREV:
    .zero 2
    .size KEY_PREV, 2

    .section .ewram,"ax",%progbits
    .thumb_func
    .align 2
    .global Vsync
    .type Vsync %function
Vsync:
    SVC 0x05
    BX lr
    .size Vsync, .-Vsync

    .thumb_func
    .align 2
    .global IRQ_Sync
    .type IRQ_Sync %function
IRQ_Sync:
    MOV r1, r0
    MOV r0, #1
    SVC 0x04
    BX lr
    .size IRQ_Sync, .-IRQ_Sync

    .section .iwram,"ax",%progbits
    .arm
    .align 2
    .global ChessGameloop_ISR_Handler
    .type ChessGameloop_ISR_Handler %function
    /* When ISR_Handler_Basic returns, it will have */
ChessGameloop_ISR_Handler:
    STMFD sp!, { lr }
    BL ISR_Handler_Basic
    LDMFD sp!, { lr }
    TST r1, #0x1000  /* Flagbit for Keypad IRQ firing in REG_IE */
    BXEQ lr
    LDR r2, =KEY_CURR
    LDRH r1, [r2]
    STRH r1, [r2, #2]
    MOV r0, #0x04000000
    ORR r0, r0, #0x130
    LDRH r1, [r0]
    MVN r1, r1
    STRH r1, [r2]
    BX lr
    .size ChessGameloop_ISR_Handler, .-ChessGameloop_ISR_Handler

