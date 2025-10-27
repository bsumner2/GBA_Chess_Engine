/** (C) 7 of September, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */

#ifndef _GBADEV_FUNCTIONS_
#define _GBADEV_FUNCTIONS_

#include <GBAdev_types.h>
#ifdef __cplusplus
extern "C" {
#endif  /* C++ Name mangler guard */

#ifdef __thumb__
#define SUPERVISOR_CALL(idx) __asm volatile ("SVC\t" #idx ::: "r0", "r1", "r2", "r3")
#else
#define SUPERVISOR_CALL(idx) __asm volatile ("SVC\t" #idx "<<16" ::: "r0", "r1", "r2", "r3")
#endif
BOOL SRAM_Write(const void *src, u32 size, u32 offset);
BOOL SRAM_Read(void *dst, u32 size, u32 offset);

IWRAM_CODE void Fast_Memcpy32(void *dst, const void *src, u32 word_ct);
IWRAM_CODE void Fast_Memset32(void *dst, u32 fill_word, u32 word_ct);

BOOL OAM_Init(Obj_Attr_t *obj_attrs, u32 count);

BOOL Obj_Affine_Transform_Copy(Obj_Affine_Transform_t *dst, 
    const Obj_Affine_Transform_t *src, 
    u32 count);

/**
 * @brief Offers a basic ISR callback function, that simply acknowledges 
 * incoming interrupts before exiting.
 * */
IWRAM_CODE void ISR_Handler_Basic(void);


/**
 * @brief Offers the most complex of the three options. This ISR callback will
 * acknowledge the interrupt, and will call whichever callback function has been
 * registered to the respective interrupt sender. In addition to calling 
 * callbacks, this ISR allows for nesting of interrupts (e.g.: vblank interrupt
 * can be triggered during another IRQ's callback function execution).
 * */
IWRAM_CODE void ISR_Handler_Nested_Switchboard(void);

IWRAM_CODE void ISR_Handler_Switchboard(void);

void IRQ_Add_Callback(void (*)(void));

INLN void OAM_Copy(Obj_Attr_t *dst, const Obj_Attr_t *src, u32 count);


INLN void OAM_Copy(Obj_Attr_t *dst, const Obj_Attr_t *src, u32 count) {
  Fast_Memcpy32(dst, src, count<<1);
}





#ifdef __cplusplus
}
#endif  /* C++ Name mangler guard */

#endif  /* _GBADEV_FUNCTIONS_ */
