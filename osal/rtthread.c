/*
 * Copyright (c) 2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-16     Rbb666       first version
 */

#include <rtthread.h>
#include "coredump.h"
#include "mcd_arch_interface.h"

/*
 * RT-Thread OS abstraction layer implementation for MCoreDump
 */
typedef struct
{
    rt_int32_t thr_cnts;
    rt_int32_t cur_idx;
} rtthread_ti_priv_t;

/* Architecture-specific exception hook function */
#ifdef PKG_USING_MCOREDUMP_ARCH_ARMV7M
#define arch_hard_fault_exception_hook armv7m_hard_fault_exception_hook
#else
#error "MCoredump does not support this architecture"
#endif

static rt_int32_t is_thread_object(rt_thread_t thread)
{
    /* Check if the object is a thread (both static and dynamic) */
    return ((thread->parent.type & ~RT_Object_Class_Static) == RT_Object_Class_Thread);
}

static rt_int32_t rtthread_thr_cnts(struct thread_info_ops *ops)
{
    rtthread_ti_priv_t *priv = (rtthread_ti_priv_t *)ops->priv;
    rt_int32_t idx = 0;
    struct rt_object_information *information;
    struct rt_object *object;
    struct rt_list_node *node;
    rt_thread_t current_thread;

    if (-1 == priv->thr_cnts)
    {
        information = rt_object_get_information(RT_Object_Class_Thread);
        mcd_assert(information != RT_NULL);
        
        current_thread = rt_thread_self();
        priv->cur_idx = -1;  /* Initialize current thread index */

        for (node = information->object_list.next;
                node != &(information->object_list);
                node = node->next)
        {
            object = rt_list_entry(node, struct rt_object, list);
            rt_thread_t thread = (rt_thread_t)object;

            if (is_thread_object(thread))
            {
                /* Check if this is the current thread */
                if (thread == current_thread)
                {
                    priv->cur_idx = idx;
                }
                idx++;
            }
        }

        priv->thr_cnts = idx;
        
        /* If current thread not found, default to 0 */
        if (priv->cur_idx == -1)
        {
            priv->cur_idx = 0;
            mcd_print("MCD DEBUG: Current thread not found, defaulting to index 0\n");
        }
    }

    return priv->thr_cnts;
}

static rt_int32_t rtthread_cur_idx(struct thread_info_ops *ops)
{
    rtthread_ti_priv_t *priv = (rtthread_ti_priv_t *)ops->priv;

    if (-1 == priv->cur_idx)
        rtthread_thr_cnts(ops);

    return priv->cur_idx;
}

static void rtthread_thr_rset(struct thread_info_ops *ops, rt_int32_t idx,
                              core_regset_type *core_regset,
                              fp_regset_type *fp_regset)
{
    rt_int32_t idx_l = 0;
    rt_int32_t current_idx = rtthread_cur_idx(ops);
    struct rt_object_information *information;
    struct rt_object *object;
    struct rt_list_node *node;

    information = rt_object_get_information(RT_Object_Class_Thread);
    mcd_assert(information != RT_NULL);

    for (node = information->object_list.next;
            node != &(information->object_list);
            node = node->next)
    {
        object = rt_list_entry(node, struct rt_object, list);
        rt_thread_t thread = (rt_thread_t)object;

        if (is_thread_object(thread))
        {
            if (idx == idx_l)
            {
                /* If this is the current thread, use current registers */
                if (idx_l == current_idx)
                {
                    mcd_memcpy(core_regset, get_cur_core_regset_address(), sizeof(core_regset_type));
                    mcd_memcpy(fp_regset, get_cur_fp_regset_address(), sizeof(fp_regset_type));
                }
                else
                {
                    /* Debug: Dynamic thread for comparison */
                    collect_registers((rt_uint32_t *)thread->sp, core_regset, fp_regset);
                }
                return;
            }
            idx_l++;
        }
    }
}

static rt_int32_t rtthread_get_mem_cnts(struct thread_info_ops *ops)
{
    return rtthread_thr_cnts(ops);
}

static rt_int32_t rtthread_get_memarea(struct thread_info_ops *ops, rt_int32_t idx,
                                       rt_uint32_t *addr, rt_uint32_t *memlen)
{
    rt_int32_t         idx_l = 0;
    rt_int32_t current_idx = rtthread_cur_idx(ops);
    struct rt_object_information *information;
    struct rt_object *object;
    struct rt_list_node *node;

    information = rt_object_get_information(RT_Object_Class_Thread);
    mcd_assert(information != RT_NULL);

    for (node = information->object_list.next;
            node != &(information->object_list);
            node = node->next)
    {
        object = rt_list_entry(node, struct rt_object, list);
        rt_thread_t thread = (rt_thread_t)object;

        if (is_thread_object(thread))
        {
            if (idx == idx_l)
            {
                /* If this is the current thread, use current stack pointer */
                if (idx_l == current_idx)
                {
                    *addr = get_cur_core_regset_address()->sp;
                    *memlen = 1024;
                }
                else
                {
                    *addr = (rt_uint32_t)thread->sp;
                    *memlen = (rt_uint32_t)thread->stack_addr + thread->stack_size - (rt_uint32_t)thread->sp;
                }
                return 0;
            }
            idx_l++;
        }
    }
    return 0;
}

void mcd_rtos_thread_ops(struct thread_info_ops *ops)
{
    static rtthread_ti_priv_t priv;
    ops->get_threads_count = rtthread_thr_cnts;
    ops->get_current_thread_idx = rtthread_cur_idx;
    ops->get_thread_regset = rtthread_thr_rset;
    ops->get_memarea_count = rtthread_get_mem_cnts;
    ops->get_memarea = rtthread_get_memarea;
    ops->priv = &priv;
    priv.cur_idx = -1;
    priv.thr_cnts = -1;
}

MCD_WEAK rt_err_t rtt_hard_fault_exception_hook(void *context)
{
    arch_hard_fault_exception_hook(context);
    return -RT_ERROR;
}

MCD_WEAK void rtt_assert_hook(const char *ex, const char *func, rt_size_t line)
{
    volatile uint8_t _continue = 1;

    rt_interrupt_enter();

    mcd_print("(%s) has assert failed at %s:%ld.\n", ex, func, line);

    mcd_faultdump(MCD_OUTPUT_SERIAL);

    rt_interrupt_leave();

    while (_continue == 1);
}

static int mcd_coredump_init(void) 
{
    static mcd_bool_t is_init = MCD_FALSE;

    if (is_init)
    {
        return 0;
    }

    mcd_print_memoryinfo();
    
    rt_hw_exception_install(rtt_hard_fault_exception_hook);

    rt_assert_set_hook(rtt_assert_hook);
    
    is_init = MCD_TRUE;
    return RT_EOK;
}
INIT_DEVICE_EXPORT(mcd_coredump_init);
