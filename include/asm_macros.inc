#if !__ASSEMBLER__
    #error "Only ASM files can include this header."
#endif

.equ VIDEO_MODE3, 3

.equ VRAM_BASE, 0x06000000

.equ DSTAT_VBLANK_IRQ_BIT, 8
.equ IE_VBLANK_IRQ_BIT, 1

.equ SCRN_WIDTH, 240
.equ SCRN_HEIGHT, 160


.equ KEY_A, 1
.equ KEY_B, 2
.equ KEY_SELECT, 4
.equ KEY_START, 8
.equ KEY_RIGHT, 16
.equ KEY_LEFT, 32
.equ KEY_UP, 64
.equ KEY_DOWN, 128
.equ KEY_R, 256
.equ KEY_L, 512

#define SVC_VSYNC 0x05

#define SVC_THUMB(callidx) SVC callidx
#define SVC_ARM(callidx) SVC callidx##0000



#define SIZEOF(type) SIZE_OF_##type

.equ SIZE_OF_STRUCT_RECT, 16

.macro LD_RECT_X, rd, rs
    LDR \rd, [\rs]
.endm
.macro LD_RECT_Y, rd, rs
    LDR \rd, [\rs, #4]
.endm
.macro LD_RECT_WIDTH, rd, rs
    LDR \rd, [\rs, #8]
.endm
.macro LD_RECT_HEIGHT, rd, rs
    LDR \rd, [\rs, #12]
.endm

.macro ST_RECT_X, rd, rs
    STR \rs, [\rd]
.endm
.macro ST_RECT_Y, rd, rs
    STR \rs, [\rd, #4]
.endm
.macro ST_RECT_WIDTH, rd, rs
    STR \rs, [\rd, #8]
.endm
.macro ST_RECT_HEIGHT, rd, rs
    STR \rs, [\rd, #12]
.endm

.macro MOV_NEGATIVE, cond, rd, imm
    MVN\cond \rd, #(\imm-1)
.endm

#define PUSH_RECT_RANGE(rlo, rhi) PUSH { rlo-rhi }
#define PUSH_RECT(ra, ...) PUSH { ra, __VA_ARGS__ }
#define POP_RECT_RANGE(rlo, rhi) POP { rlo-rhi }
#define POP_RECT(ra, ...) POP { ra, __VA_ARGS__ }

#define VEL_TO_DISPLACEMENT_SHAMT 6
#define ACCEL_TO_VELOCITY_SHAMT 0

#define PADDLE_ACCEL_FRICTION_AMT 80
#define BALL_MAX_VEL 256
#define BALL_MAX_ACCEL 128

#define PADDLE_MAX_VEL 128
#define PADDLE_MAX_ACCEL 64

#define PADDLE_GOAL_DIST 16

#define BALL_DIMS 8

#define BALL_INIT_X ((SCRN_WIDTH-BALL_DIMS)/2)
#define BALL_Y ((SCRN_HEIGHT-BALL_DIMS)/2)

#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT 16

#define PADDLE_Y ((SCRN_HEIGHT - PADDLE_HEIGHT)/2)

#define PADDLE0_INIT_X PADDLE_GOAL_DIST
#define PADDLE1_INIT_X (SCRN_WIDTH - PADDLE_GOAL_DIST - PADDLE_WIDTH)



