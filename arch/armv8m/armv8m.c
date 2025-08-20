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
 * @brief Collect ARM Cortex-M33 registers from RT-Thread stack frame
 *
 * This function extracts register values from the stack frame created by
 * RT-Thread's HardFault_Handler for ARMv8-M architecture.
 * 
 * ARMv8-M Stack Frame Layout (based on HardFault_Handler in context_gcc.S):
 * +-------------------+  <- stack_top (input parameter)
 * | EXC_RETURN        |  (4 bytes, contains exception return information)
 * +-------------------+
 * | tz                |  (4 bytes, TrustZone context)
 * | lr                |  (4 bytes, saved link register)
 * | psplim            |  (4 bytes, stack pointer limit)
 * | control           |  (4 bytes, control register)
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
 * | FPU d8-d15        |  (64 bytes, if FPU context active)
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
 * | FPU d0-d7         |  (64 bytes, if FPU context active)
 * | FPSCR             |  (4 bytes, if FPU context active)
 * | NO_NAME           |  (4 bytes, if FPU context active)
 * +-------------------+  <- current SP after context save
 *
 * @param stack_top Pointer to the beginning of the stack frame (EXC_RETURN position)
 * @param core_regset Pointer to structure for storing ARM core registers
 * @param fp_regset Pointer to structure for storing FPU registers
 */
void collect_registers_armv8m(uint32_t *stack_top,
                                    core_regset_type *core_regset,
                                    fp_regset_type *fp_regset)
{    
    /* 
     * ARMv8-M has two different stack layouts:
     * 1. Exception context (HardFault_Handler): [EXC_RETURN] -> [tz,lr,psplim,control] -> [r4-r11] -> [FPU d8-d15] -> [exception frame] -> [FPU d0-d7,FPSCR]
     * 2. Normal thread context (PendSV_Handler): [tz,lr,psplim,control] -> [r4-r11] -> [hardware exception frame]
     */
    uint32_t *current_ptr = stack_top;
    
    /* Clear both register sets first to ensure clean state */ 
    mcd_memset(core_regset, 0, sizeof(core_regset_type));
    mcd_memset(fp_regset, 0, sizeof(fp_regset_type));

    /* Read first word to determine context type */
    uint32_t first_word = *current_ptr;
    uint32_t fpu_flag = 0;  // Default to no FPU context
    
    /* Check if this is a valid EXC_RETURN value (starts with 0xFF) */
    if ((first_word & 0xFF000000) == 0xFF000000) 
    {
        /* Valid EXC_RETURN - this is an exception context */
        uint32_t exc_return = *current_ptr++;
        fpu_flag = !(exc_return & 0x10);
        /* Skip tz, lr, psplim, control fields (4 words) */
        current_ptr += 4;
    }
    else 
    {
        /* Not a valid EXC_RETURN - this is normal thread context from PendSV_Handler */
        fpu_flag = 0;
        /* For normal thread context, stack_top points to [tz,lr,psplim,control] */
        /* Skip tz, lr, psplim, control fields (4 words) */
        current_ptr += 4;
    }

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
    /* If FPU context is active, d8-d15 registers are saved after r4-r11 */
    if (fpu_flag)
    {
        /* Copy FPU d8-d15 registers (software saved by HardFault_Handler) */
        /* Each double precision register is 64 bits = 2 words */
        uint64_t *fp_d_ptr = (uint64_t *)current_ptr;
        for (int i = 0; i < 8; i++)  /* d8-d15 = 8 registers */
        {
            (&fp_regset->d8)[i] = *fp_d_ptr++;
        }
        current_ptr = (uint32_t *)fp_d_ptr;
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
    /* If FPU context is active, d0-d7 and FPSCR are saved after exception frame */
    if (fpu_flag)
    {
        /* Copy FPU d0-d7 registers (hardware saved by ARM Cortex-M) */
        /* Each double precision register is 64 bits = 2 words */
        uint64_t *fp_d_ptr = (uint64_t *)current_ptr;
        for (int i = 0; i < 8; i++)  /* d0-d7 = 8 registers */
        {
            (&fp_regset->d0)[i] = *fp_d_ptr++;
        }
        current_ptr = (uint32_t *)fp_d_ptr;
        
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
 * @brief ARM Cortex-M33 specific hard fault exception handler for MCoreDump
 * 
 * This function handles ARM Cortex-M33 specific stack frame processing when a 
 * hard fault occurs. It follows the armv7m approach for simple and reliable
 * stack pointer calculation.
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
 * +-------------------+  <- Target position for collect_registers_armv8m
 *
 * @param context Pointer to exception_stack_frame from HardFault_Handler
 * @return int Always returns 0 after processing coredump
 */
int armv8m_hard_fault_exception_hook(void *context)
{
    /* Add debug output to confirm we reach this function */
    mcd_print("armv8m_hard_fault_exception_hook called, context = 0x%08x\n", (uint32_t)context);
    
    /* 
     * Based on HardFault_Handler implementation in context_gcc.S:
     * 
     * For EXCEPTION context (HardFault_Handler):
     * Stack layout from context (exception_stack_frame) backwards:
     * context -> [r0,r1,r2,r3,r12,lr,pc,xpsr] (exception frame)
     * context-32 -> [r11,r10,r9,r8,r7,r6,r5,r4] (8 words, STMFD r0!, {r4-r11})
     * context-48 -> [tz,lr,psplim,control] (4 words, STMFD r0!, {r2-r5})  
     * context-52 -> [EXC_RETURN] (1 word, STMFD r0!, {lr})
     * 
     * We need to point to EXC_RETURN for collect_registers_armv8m
     */

    uint32_t *stack_ptr = (uint32_t *)context;
    
    /* Move backwards to find EXC_RETURN position */
    stack_ptr -= 8;  /* r4-r11 (8 registers) */
    stack_ptr -= 4;  /* tz, lr, psplim, control (4 fields) */  
    stack_ptr -= 1;  /* EXC_RETURN */

#ifdef RT_USING_FINSH
    extern long list_thread(void);
    list_thread();
#endif

    /* Now stack_ptr points to EXC_RETURN, which is what collect_registers_armv8m expects */
    collect_registers_armv8m(stack_ptr,
                           get_cur_core_regset_address(),
                           get_cur_fp_regset_address());

    /* Generate coredump using memory mode */
    mcd_faultdump(MCD_OUTPUT_MEMORY);

    return 0;
}
