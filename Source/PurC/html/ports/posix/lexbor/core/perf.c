/**
 * @file perf.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of performance.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "html/core/perf.h"


#ifdef PCHTML_WITH_PERF

#ifdef PCHTML_OS_DARWIN
    #include <sys/sysctl.h>
#elif PCHTML_OS_LINUX
#endif


static unsigned long long
pchtml_perf_clock(void);

static unsigned long long
pchtml_perf_frequency(void);


typedef struct pchtml_perf {
    unsigned long long start;
    unsigned long long end;
    unsigned long long freq;
}
pchtml_perf_t;


void *
pchtml_perf_create(void)
{
    pchtml_perf_t *perf = pchtml_calloc(1, sizeof(pchtml_perf_t));
    if (perf == NULL) {
        return NULL;
    }

    perf->freq = pchtml_perf_frequency();

    return perf;
}

void
pchtml_perf_clean(void *perf)
{
    memset(perf, 0, sizeof(pchtml_perf_t));
}

void
pchtml_perf_destroy(void *perf)
{
    if (perf != NULL) {
        pchtml_free(perf);
    }
}

unsigned int
pchtml_perf_begin(void *perf)
{
    ((pchtml_perf_t *) (perf))->start = pchtml_perf_clock();

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_perf_end(void *perf)
{
    ((pchtml_perf_t *) (perf))->end = pchtml_perf_clock();

    return PCHTML_STATUS_OK;
}

double
pchtml_perf_in_sec(void *perf)
{
    pchtml_perf_t *obj_perf = (pchtml_perf_t *) perf;

    if (obj_perf->freq != 0) {
        return ((double) (obj_perf->end - obj_perf->start)
                / (double)obj_perf->freq);
    }

    return 0.0f;
}

static unsigned long long
pchtml_perf_clock(void)
{
    unsigned long long x;

     /*
      * cpuid serializes any out-of-order prefetches
      * before executing rdtsc (clobbers ebx, ecx, edx).
      */
    __asm__ volatile (
                      "cpuid\n\t"
                      "rdtsc\n\t"
                      "shl $32, %%rdx\n\t"
                      "or %%rdx, %%rax"
                      : "=a" (x)
                      :
                      : "rdx", "ebx", "ecx");

    return x;
}

static unsigned long long
pchtml_perf_frequency(void)
{
    unsigned long long freq = 0;

#if defined(PCHTML_OS_DARWIN) && defined(CTL_HW) && defined(HW_CPU_FREQ)

    /* OSX kernel: sysctl(CTL_HW | HW_CPU_FREQ) */
    size_t len = sizeof(freq);
    int mib[2] = {CTL_HW, HW_CPU_FREQ};

    if(sysctl(mib, 2, &freq, &len, NULL, 0)) {
        return 0;
    }

    return freq;

#elif defined(PCHTML_OS_LINUX)

    char buf[1024] = {0};
    double fval = 0.0;

    /* Use procfs on linux */
    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        return 0;
    }

    /* Find 'CPU MHz :' */
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (sscanf(buf, "cpu MHz : %lf\n", &fval) == 1) {
            freq = (unsigned long long)(fval * 1000000ull);

            break;
        }
    }

    fclose(fp);

    return freq;

#else

    return freq;

#endif /* PCHTML_OS_DARWIN || PCHTML_OS_LINUX */
}

#endif /* PCHTML_WITH_PERF */
