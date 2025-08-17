/*
 * Copyright (c) 2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-08-16     Rbb666       first version
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "coredump.h"

typedef void (*fault_func)(float);

static int mcd_test(int argc, char **argv)
{
    int x, y;
    const char * sx = "84597";
    const char * sy = "35268";

    float a, b, c;
    const char * fsa = "1.1322";
    const char * fsb = "45.2547";
    const char * fsc = "7854.2";

    if (argc < 2)
    {
        rt_kprintf("Please input 'mcd_test <COREDUMP|ASSERT|FAULT>' \n");
        return 0;
    }

    if (!strcmp(argv[1], "COREDUMP"))
    {
        mcd_faultdump(MCD_OUTPUT_SERIAL);
        return 0;
    }
    else if (!strcmp(argv[1], "ASSERT"))
    {
        a = atof(&fsa[0]);
        b = atof(&fsb[0]);
        c = atof(&fsc[0]);

        x = atoi(&sx[0]);
        y = atoi(&sy[0]);

        mcd_assert(x * a == y * b * c);
        return 0;
    }
    else if (!strcmp(argv[1], "FAULT"))
    {
        fault_func func = (fault_func)0xFFFF0000;
        
        a = atof(&fsa[0]);
        b = atof(&fsb[0]);
        c = atof(&fsc[0]);

        x = atoi(&sx[0]);
        y = atoi(&sy[0]);

        func(x * a + y * b * c);
        return 0;
    }
    
    return 0;
}
MCD_CMD_EXPORT(mcd_test, mCoreDump test: mcd_test <COREDUMP|FLOAT|ASSERT|FAULT>);
