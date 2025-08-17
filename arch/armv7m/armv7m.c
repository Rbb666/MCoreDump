/*
 * Copyright (c) 2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-16     Rbb666       first version
 */

#include "coredump.h"
#include "registers.h"

#define FPU_CPACR 0xE000ED88

int is_vfp_addressable(void)
{
    uint32_t reg_cpacr = *((volatile uint32_t *)FPU_CPACR);
    if (reg_cpacr & 0x00F00000)
        return 1;
    else
        return 0;
}

#if defined(__CC_ARM)

/* clang-format off */
__asm void mcd_mini_dump()
{
    extern get_cur_core_regset_address;
    extern get_cur_fp_regset_address;
    extern mcd_mini_dump_ops;
    extern mcd_gen_coredump;
    extern is_vfp_addressable;

    PRESERVE8

    push {r7, lr}
    sub sp, sp, #24
    add r7, sp, #0

get_regset
    bl get_cur_core_regset_address
    str r0, [r0, #0]
    add r0, r0, #4
    stmia r0!, {r1 - r12}
    mov r1, sp
    add r1, #32
    str r1, [r0, #0]
    ldr r1, [sp, #28]
    str r1, [r0, #4]
    mov r1, pc
    str r1, [r0, #8]
    mrs r1, xpsr
    str r1, [r0, #12]

    bl is_vfp_addressable
    cmp r0, #0
    beq get_reg_done

    bl get_cur_fp_regset_address
    vstmia r0!, {d0 - d15}
    vmrs r1, fpscr
    str r1, [r0, #0]

get_reg_done
    mov r0, r7
    bl mcd_mini_dump_ops
    mov r0, r7
    bl mcd_gen_coredump
    nop
    adds r7, r7, #24
    mov sp, r7
    pop {r7, pc}
    nop
    nop
}

__asm void mcd_multi_dump(void)
{
    extern get_cur_core_regset_address;
    extern get_cur_fp_regset_address;
    extern mcd_rtos_thread_ops;
    extern mcd_gen_coredump;
    extern is_vfp_addressable;

    PRESERVE8

    push {r7, lr}
    sub sp, sp, #24
    add r7, sp, #0

get_regset1
    bl get_cur_core_regset_address
    str r0, [r0, #0]
    add r0, r0, #4
    stmia r0!, {r1 - r12}
    mov r1, sp
    add r1, #32
    str r1, [r0, #0]
    ldr r1, [sp, #28]
    str r1, [r0, #4]
    mov r1, pc
    str r1, [r0, #8]
    mrs r1, xpsr
    str r1, [r0, #12]

    bl is_vfp_addressable
    cmp r0, #0
    beq get_reg_done

    bl get_cur_fp_regset_address
    vstmia r0!, {d0 - d15}
    vmrs r1, fpscr
    str r1, [r0, #0]

get_reg_done1
    mov r0, r7
    bl mcd_rtos_thread_ops
    mov r0, r7
    bl mcd_gen_coredump
    nop
    adds r7, r7, #24
    mov sp, r7
    pop {r7, pc}
    nop
    nop
}

#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) || defined(__GNUC__)

#define mcd_get_regset(regset)                                              \
    __asm volatile("  mov r0, %0                \n"                         \
                   "  str r0, [r0 , #0]         \n"                         \
                   "  add r0, r0, #4            \n"                         \
                   "  stmia  r0!, {r1 - r12}    \n"                         \
                   "  mov r1, sp                \n"                         \
                   "  str  r1, [r0, #0]         \n"                         \
                   "  mov r1, lr                \n"                         \
                   "  str  r1, [r0, #4]         \n"                         \
                   "  mov r1, pc                \n"                         \
                   "  str  r1, [r0, #8]         \n"                         \
                   "  mrs r1, xpsr              \n"                         \
                   "  str  r1, [r0, #12]        \n" ::"r"(regset)           \
                   : "memory", "cc");

#define mcd_get_fpregset(regset)                                            \
    __asm volatile(" mov r0, %0                 \n"                         \
                   " vstmia r0!, {d0 - d15}     \n"                         \
                   " vmrs r1, fpscr             \n"                         \
                   " str  r1, [r0, #0]          \n" ::"r"(regset)           \
                   : "memory", "cc");

void mcd_mini_dump(void)
{
    struct thread_info_ops ops;

    mcd_get_regset((uint32_t *)get_cur_core_regset_address());

#if MCD_FPU_SUPPORT
    if (is_vfp_addressable())
        mcd_get_fpregset((uint32_t *)get_cur_fp_regset_address());
#endif

    mcd_mini_dump_ops(&ops);
    mcd_gen_coredump(&ops);
}

void mcd_multi_dump(void)
{
    struct thread_info_ops ops;

    mcd_get_regset((uint32_t *)get_cur_core_regset_address());

#if MCD_FPU_SUPPORT
    if (is_vfp_addressable())
        mcd_get_fpregset((uint32_t *)get_cur_fp_regset_address());
#endif

    mcd_rtos_thread_ops(&ops);
    mcd_gen_coredump(&ops);
}

#endif

/**
 * @brief Collect ARM Cortex-M4 registers from RT-Thread stack frame
 *
 * This function extracts register values from the stack frame created by
 * RT-Thread's context switch mechanism (PendSV_Handler) or exception handling.
 * 
 * RT-Thread Stack Frame Layout (from low to high memory address):
 * +-------------------+  <- stack_top (input parameter)
 * | FPU flag          |  (4 bytes, if MCD_FPU_SUPPORT enabled)
 * +-------------------+
 * | r4                |  (4 bytes, software saved)
 * | r5                |  (4 bytes, software saved)
 * | r6                |  (4 bytes, software saved)
 * | r7                |  (4 bytes, software saved)
 * | r8                |  (4 bytes, software saved)
 * | r9                |  (4 bytes, software saved)
 * | r10               |  (4 bytes, software saved)
 * | r11               |  (4 bytes, software saved)
 * +-------------------+
 * | FPU s16-s31       |  (64 bytes, if FPU context active)
 * +-------------------+
 * | r0                |  (4 bytes, hardware saved)
 * | r1                |  (4 bytes, hardware saved)
 * | r2                |  (4 bytes, hardware saved)
 * | r3                |  (4 bytes, hardware saved)
 * | r12               |  (4 bytes, hardware saved)
 * | lr                |  (4 bytes, hardware saved)
 * | pc                |  (4 bytes, hardware saved)
 * | xpsr              |  (4 bytes, hardware saved)
 * +-------------------+
 * | FPU s0-s15        |  (64 bytes, if FPU context active)
 * | FPSCR             |  (4 bytes, if FPU context active)
 * | NO_NAME           |  (4 bytes, if FPU context active)
 * +-------------------+  <- current SP after context save
 *
 * @param stack_top Pointer to the beginning of the stack frame (FPU flag position)
 * @param core_regset Pointer to structure for storing ARM core registers
 * @param fp_regset Pointer to structure for storing FPU registers
 */
void collect_registers_armv7m(uint32_t *stack_top,
                                    core_regset_type *core_regset,
                                    fp_regset_type *fp_regset)
{    
    /* 
     * This function uses the same stack frame parsing approach as collect_registers_armv7ms
     * for consistency. Both PendSV_Handler and HardFault_Handler now use identical
     * stacking order after the modifications.
     * 
     * Expected stack layout starting from stack_top:
     * [FPU flag] -> [r4-r11] -> [FPU s16-s31] -> [exception frame] -> [FPU s0-s15,FPSCR]
     */
    uint32_t *current_ptr = stack_top;
    
    /* Clear both register sets first to ensure clean state */ 
    mcd_memset(core_regset, 0, sizeof(core_regset_type));
    mcd_memset(fp_regset, 0, sizeof(fp_regset_type));

#if MCD_FPU_SUPPORT
    /* Read FPU flag first - indicates if FPU context was saved */
    uint32_t fpu_flag = *current_ptr++;
#endif

    /* Extract core registers r4-r11 (software saved by RT-Thread) */
    core_regset->r4 = *current_ptr++;
    core_regset->r5 = *current_ptr++;
    core_regset->r6 = *current_ptr++;
    core_regset->r7 = *current_ptr++;
    core_regset->r8 = *current_ptr++;
    core_regset->r9 = *current_ptr++;
    core_regset->r10 = *current_ptr++;
    core_regset->r11 = *current_ptr++;

#if MCD_FPU_SUPPORT
    /* If FPU context is active, s16-s31 registers are saved after r4-r11 */
    if (fpu_flag)
    {
        /* Copy FPU s16-s31 registers (software saved by RT-Thread) */
        for (int i = 16; i < 32; i++)
        {
            ((uint32_t *)fp_regset)[i] = *current_ptr++;
        }
    }
#endif

    /* Extract hardware exception frame (automatically saved by ARM Cortex-M) */
    core_regset->r0 = *current_ptr++;
    core_regset->r1 = *current_ptr++;
    core_regset->r2 = *current_ptr++;
    core_regset->r3 = *current_ptr++;
    core_regset->r12 = *current_ptr++;
    core_regset->lr = *current_ptr++;
    core_regset->pc = *current_ptr++;
    core_regset->xpsr = *current_ptr++;

#if MCD_FPU_SUPPORT
    /* If FPU context is active, s0-s15 and FPSCR are saved after exception frame */
    if (fpu_flag)
    {
        /* Copy FPU s0-s15 registers (hardware saved by ARM Cortex-M) */
        for (int i = 0; i < 16; i++)
        {
            ((uint32_t *)fp_regset)[i] = *current_ptr++;
        }
        
        /* Copy FPSCR register (FPU status and control) */
        fp_regset->fpscr = *current_ptr++;
        
        /* Skip NO_NAME field (reserved/alignment) */
        current_ptr++;
    }
#endif

    /* SP should point to the current stack pointer position after all saved data */
    core_regset->sp = (uintptr_t)current_ptr;
}

/**
 * @brief ARM Cortex-M specific hard fault exception handler for MCoreDump
 * 
 * This function handles ARM Cortex-M specific stack frame processing when a 
 * hard fault occurs. It calculates the proper stack pointer position and
 * extracts register context for coredump generation.
 * 
 * HardFault Stack Frame Layout (created by HardFault_Handler):
 * +-------------------+  <- Exception occurs here
 * | Hardware Exception|  (32 bytes: r0,r1,r2,r3,r12,lr,pc,xpsr)
 * | Stack Frame       |  (+ optional 72 bytes FPU: s0-s15,FPSCR,NO_NAME)
 * +-------------------+  <- context parameter points here
 * | r11               |  (4 bytes, software saved in HardFault_Handler)
 * | r10               |  (4 bytes, software saved in HardFault_Handler)
 * | ...               |  (...)
 * | r4                |  (4 bytes, software saved in HardFault_Handler)
 * +-------------------+
 * | FPU s31           |  (4 bytes, if FPU context active)
 * | FPU s30           |  (4 bytes, if FPU context active)
 * | ...               |  (...)
 * | FPU s16           |  (4 bytes, if FPU context active)
 * +-------------------+
 * | FPU flag          |  (4 bytes, if MCD_FPU_SUPPORT enabled)
 * +-------------------+
 * | EXC_RETURN        |  (4 bytes, contains exception return information)
 * +-------------------+  <- Final stack pointer position
 *
 * @param context Pointer to exception_stack_frame from HardFault_Handler
 * @return int Always returns -1 to indicate fault condition
 */
int armv7m_hard_fault_exception_hook(void *context)
{
    /* 
     * context points to exception_stack_frame created by HardFault_Handler.
     * We need to calculate the complete stack frame position to extract all registers.
     * Since HardFault_Handler now uses the same stacking order as PendSV_Handler,
     * we can directly use collect_registers_armv7m function.
     */

    struct exception_stack_frame *exception_stack = (struct exception_stack_frame *)context;

    /* Calculate stack pointer to the beginning of the saved context */
    uint32_t *stack_ptr = (uint32_t *)exception_stack;

#ifdef RT_USING_FINSH
    extern long list_thread(void);
    list_thread();
#endif

    /* 
     * Move backward through the stack to reach the beginning of saved context.
     * Stack layout (working backwards from exception_stack):
     * exception_stack -> r4-r11 (8 words) -> [s16-s31] -> [fpu_flag] -> [exc_return]
     */
    stack_ptr -= 8; /* Move backward 8 uint32_t positions to reach r4 */

#if MCD_FPU_SUPPORT
    /* Point to FPU flag position (collect_registers_armv7m expects this as start) */
    stack_ptr -= 1; /* fpu flag */
#else
    /* If no FPU support, skip EXC_RETURN and point to r4 directly */
    stack_ptr -= 1; /* exc_return */
#endif

    /* 
     * Now stack_ptr points to where collect_registers_armv7m expects:
     * - With FPU: points to FPU flag (first field to be read)
     * - Without FPU: points to r4 (first register to be read)
     */
    collect_registers_armv7m(stack_ptr,
                           get_cur_core_regset_address(),
                           get_cur_fp_regset_address());

    /* Generate coredump using memory mode */
    mcd_faultdump(MCD_OUTPUT_MEMORY);

    return 0;
}
