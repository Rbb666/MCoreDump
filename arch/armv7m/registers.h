/*
 * Copyright (c) 2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-16     Rbb666       first version
 */

#ifndef __REGISTERS_H__
#define __REGISTERS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct armv7m_core_regset
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
};

struct arm_vfpv2_regset
{
    uint64_t d0;
    uint64_t d1;
    uint64_t d2;
    uint64_t d3;
    uint64_t d4;
    uint64_t d5;
    uint64_t d6;
    uint64_t d7;
    uint64_t d8;
    uint64_t d9;
    uint64_t d10;
    uint64_t d11;
    uint64_t d12;
    uint64_t d13;
    uint64_t d14;
    uint64_t d15;
    uint32_t fpscr;
};

typedef struct armv7m_core_regset    core_regset_type;
typedef struct arm_vfpv2_regset      fp_regset_type;

/**
 * @brief RT-Thread basic exception stack frame (hardware auto-saved)
 */
struct rt_exception_stack_frame
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
};

/**
 * @brief RT-Thread exception stack frame with FPU context (hardware auto-saved)
 */
struct rt_exception_stack_frame_fpu
{
#if MCD_FPU_SUPPORT
    /* FPU register s0-s15 (hardware auto-saved) */
    uint32_t S0;
    uint32_t S1;
    uint32_t S2;
    uint32_t S3;
    uint32_t S4;
    uint32_t S5;
    uint32_t S6;
    uint32_t S7;
    uint32_t S8;
    uint32_t S9;
    uint32_t S10;
    uint32_t S11;
    uint32_t S12;
    uint32_t S13;
    uint32_t S14;
    uint32_t S15;
    uint32_t FPSCR;
    uint32_t NO_NAME;  // Reserved
#endif
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
};

/**
 * @brief RT-Thread stack frame (software + hardware saved)
 */
struct rt_stack_frame
{
#if MCD_FPU_SUPPORT
    uint32_t flag;

    /* FPU register s16-s31 (software saved when FPU context active) */
    uint32_t s16;
    uint32_t s17;
    uint32_t s18;
    uint32_t s19;
    uint32_t s20;
    uint32_t s21;
    uint32_t s22;
    uint32_t s23;
    uint32_t s24;
    uint32_t s25;
    uint32_t s26;
    uint32_t s27;
    uint32_t s28;
    uint32_t s29;
    uint32_t s30;
    uint32_t s31;
#endif /* MCD_FPU_SUPPORT */

    /* r4 ~ r11 register (software saved) */
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;

    struct rt_exception_stack_frame exception_stack_frame;
};

/**
 * @brief RT-Thread complete exception information structure
 */
struct rt_exception_info
{
    uint32_t exc_return;
    struct rt_stack_frame stack_frame;
};

#ifdef __cplusplus
}
#endif

#endif /* __MCD_REGISTERS_H__ */
