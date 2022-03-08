/*
 * Unit test suite for vcomp110
 *
 * Copyright 2021 Paul Gofman for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include "wine/test.h"

static HMODULE vcomp_handle;

static void  (WINAPIV *pC2VectParallel)(int start, int end, int step, BOOL end_included, int thread_count,
        BOOL dynamic_distribution, void *function, int nargs, ...);
static int   (CDECL   *p_vcomp_get_thread_num)(void);
static int   (CDECL   *pomp_get_num_threads)(void);
static void  (CDECL   *pomp_set_num_threads)(int num_threads);
static void  (CDECL   *p_vcomp_set_num_threads)(int num_threads);

#define VCOMP_GET_PROC(func) \
    do \
    { \
        p ## func = (void *)GetProcAddress(vcomp_handle, #func); \
        if (!p ## func) trace("Failed to get address for %s\n", #func); \
    } \
    while (0)

static BOOL init_vcomp(void)
{
    vcomp_handle = LoadLibraryA("vcomp110.dll");
    if (!vcomp_handle)
    {
        win_skip("vcomp110.dll is not installed.\n");
        return FALSE;
    }

    VCOMP_GET_PROC(C2VectParallel);
    VCOMP_GET_PROC(_vcomp_get_thread_num);
    VCOMP_GET_PROC(omp_get_num_threads);
    VCOMP_GET_PROC(omp_set_num_threads);
    VCOMP_GET_PROC(_vcomp_set_num_threads);

    return TRUE;
}

#define TEST_C2VECTPARALLEL_MAX_THREADS 256
#define TEST_C2VECTPARALLEL_MAX_VALUES 256
#define TEST_C2VECTPARALLEL_MAX_CALLS 256

struct C2VectParallel_test_data
{
    int start;
    int end;
    int step;
    BOOL end_included;
    int thread_count;
    unsigned int expected_call_count;
    unsigned int expected_dynamic_call_count;
};

static volatile LONG test_C2VectParallel_call_count;

static void CDECL test_C2VectParallel_payload(int start, int end, void *test_parameter,
        const struct C2VectParallel_test_data *test, unsigned int test_index, BOOL dynamic_distribution)
{
    int thread_count = test->thread_count ? test->thread_count : 5;
    int thread = p_vcomp_get_thread_num();
    int expected_start, expected_end;
    int omp_num_threads;
    int step_sign;

    step_sign = test->step > 0 ? 1 : -1;

    omp_num_threads = pomp_get_num_threads();

    InterlockedIncrement(&test_C2VectParallel_call_count);

    if ((step_sign > 0 && test->end < test->start) || (step_sign < 0 && test->end > test->start)
            || (test->end - test->start) / test->step < 2)
    {
        ok(omp_num_threads == 1, "Got unexpected omp_num_threads %u, thread_count %u, test %u.\n",
                omp_num_threads, thread_count, test_index);
        ok(!thread, "Got unexpected thread %d, test %u.\n", thread, test_index);
        ok(start == test->start, "Got unexpected start %d, expected %d, thread %d, test %u.\n",
                start, test->start, thread, test_index);
        ok(end == test->end, "Got unexpected end %d, expected %d, thread %d, test %u.\n",
                end, test->end, thread, test_index);
        return;
    }

    ok(thread_count == omp_num_threads, "Got unexpected omp_num_threads %u, thread_count %u, test %u.\n",
            omp_num_threads, thread_count, test_index);

    ok(test_parameter == (void *)0xdeadbeef, "Got unexpected test_parameter %p.\n", test_parameter);

    if (dynamic_distribution)
    {
        unsigned int remaining_count;
        unsigned int loop_count = 0;

        expected_start = test->start;
        do
        {
            ++loop_count;
            remaining_count = test->end - expected_start + test->step - step_sign * !test->end_included;
            if (test->step < 0)
                remaining_count = (unsigned int)-remaining_count / -test->step;
            else
                remaining_count /= test->step;

            expected_end = expected_start + (remaining_count + thread_count - 1) / thread_count * test->step
                    + step_sign * !test->end_included;

            if ((expected_end - test->end) * step_sign > 0)
                expected_end = test->end;
            else
                expected_end -= test->step;

            if (expected_start == start)
                break;

            expected_start = expected_end - step_sign * !test->end_included + test->step;
        }
        while (loop_count < 100);
        ok(loop_count < 100, "Could not match start %d after %u iterations, test %u.\n",
                start, loop_count, test_index);
    }
    else
    {
        unsigned int step_count, steps_per_call, remainder;

        step_count = test->end - test->start + test->step - step_sign * !test->end_included;
        if (test->step < 0)
            step_count = (unsigned int)-step_count / -test->step;
        else
            step_count /= test->step;

        steps_per_call = step_count / thread_count;
        remainder = step_count % thread_count;

        if (thread < remainder)
        {
            expected_start = thread * (steps_per_call + 1);
            expected_end = expected_start + steps_per_call + 1;
        }
        else if (thread < step_count)
        {
            expected_start = remainder + steps_per_call * thread;
            expected_end = expected_start + steps_per_call;
        }
        else
        {
            expected_start = expected_end = 0;
        }

        expected_start = test->start + expected_start * test->step;
        expected_end = test->start + expected_end * test->step;

        expected_end -= test->step;
        if (!test->end_included)
            expected_end += step_sign;
    }

    ok(start == expected_start, "Got unexpected start %d, expected %d, thread %d, test %u.\n",
            start, expected_start, thread, test_index);
    ok(end == expected_end, "Got unexpected end %d, expected %d, start %d, thread %d, dynamic %#x, test %u.\n",
            end, expected_end, start, thread, dynamic_distribution, test_index);
}

#if 0
static void CDECL test_C2VectParallel_single_argument(int start)
{
    ok(0, "Should not get here.\n");
}
#endif

static void test_C2VectParallel(void)
{
    static const struct C2VectParallel_test_data tests[] =
    {
        {28, 28, 1, 0, 6, 1, 1},
        {28, 28, 1, 1, 6, 1, 1},
        {28, 29, 1, 0, 6, 1, 1},
        {28, 29, 1, 1, 6, 1, 1},

        /* Native generates calls from excessive threads with empty range here. */
        {28, 30, 1, 0, 6, 6, 2},
        {28, 31, 1, 0, 6, 6, 3},
        {28, 33, 1, 0, 6, 6, 5},
        {0, -2, -1, 0, 3, 3, 2},

        /* But not in such cases. */
        {28, 30, 1, 1, 6, 3, 3},
        {28, 31, 2, 0, 6, 1, 1},
        {28, 32, 2, 0, 6, 2, 2},

        {28, 57, 1, 0, 6, 6, 13},
        {28, 57, 1, 1, 6, 6, 13},
        {28, 57, 4, 1, 6, 6, 7},
        {28, 36, 4, 0, 6, 2, 2},
        {28, 36, 4, 1, 6, 3, 3},
        {36, 28, 4, 0, 6, 1, 1},
        {36, 28, 4, 1, 6, 1, 1},
        {57, 28, -1, 0, 6, 6, 13},
        {57, 28, -1, 1, 6, 6, 13},
        {36, 28, -4, 0, 6, 2, 2},
        {36, 28, -4, 1, 6, 3, 3},
        {28, 57, 1, 0, 0, 5, 11},
        {-10, -2, 2, 1, 2, 2, 3},
        {0x7ffffffd, 0x7fffffff, 1, 1, 2, 2, 2},
        {-1, 0x7fffffff, 0x10000000, 1, 2, 1, 1},
        {0, 0x7fffffff, 1, 0, 2, 2, 31},
        {0, 0x7fffffff, 1, 1, 2, 2, 32},
        {-10000, 0x7fffffff, 1, 1, 2, 1, 1},
        {-10000, 0x7fffffff, -1, 1, 2, 1, 1},
        {-10000, 0x7fffffff, 2, 1, 2, 1, 1},
        {-10000, 0x7ffffff0, 2, 1, 2, 1, 1},
        {-10000, 0x7fffffff - 10000, 1, 0, 1, 1, 1},
        {-10000, 0x7fffffff - 10000, 1, 1, 1, 1, 1},
        {-10000, 0x7fffffff - 10000, 1, 0, 2, 2, 31},
        {-10000, 0x7fffffff - 10000, 1, 1, 2, 2, 32},
        {-10000, 0x7fffffff - 10000, 2, 0, 2, 2, 31},
        {-10000, 0x7fffffff - 10000, 2, 1, 2, 2, 31},
        {0x80000001, 0, 2, 0, 3, 3, 51},
        {0x80000001, 0, 2, 1, 3, 3, 51},
        {0x80000000, 0x7fffffff, 1, 0, 3, 1, 1},
        {0x80000000, 0x7fffffff, 1, 1, 3, 1, 1},
        {0x80000000, 0x7fffffff, 2, 0, 3, 1, 1},
        {0x80000000, 0x7fffffff, 2, 1, 3, 1, 1},
        {0x80000001, 1, 1, 0, 3, 1, 1},
        {0x80000001, 1, 1, 1, 3, 1, 1},
        {0x80000001, 0, 1, 0, 3, 3, 52},
        {0x80000001, 0, 1, 1, 3, 3, 52},
        {28, 28, 1, 0, -1, 1, 1},
#if 0
        /* Results in division by zero on Windows. */
        {28, 57, 0, 0, 6, 6},
#endif
    };
    int num_threads;
    unsigned int i;

    pomp_set_num_threads(5);
    p_vcomp_set_num_threads(3);
    num_threads = pomp_get_num_threads();
    ok(num_threads == 1, "Got unexpected num_threads %u.\n", num_threads);

#if 0
    /* Results in stack overflow on Windows. */
    pC2VectParallel(28, 57, 1, 0, 2, FALSE, test_C2VectParallel_single_argument, 1);
#endif

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const struct C2VectParallel_test_data *test = &tests[i];

        test_C2VectParallel_call_count = 0;
        pC2VectParallel(test->start, test->end, test->step, test->end_included, test->thread_count,
                FALSE, test_C2VectParallel_payload, 6, (void *)0xdeadbeef, test, i, FALSE);
        ok(test_C2VectParallel_call_count == test->expected_call_count,
                "Got unexpected call count %lu, expected %u, test %u.\n",
                test_C2VectParallel_call_count, test->expected_call_count, i);

        test_C2VectParallel_call_count = 0;
        pC2VectParallel(test->start, test->end, test->step, test->end_included, test->thread_count,
                ~0u, test_C2VectParallel_payload, 6, (void *)0xdeadbeef, test, i, TRUE);
        ok(test_C2VectParallel_call_count == test->expected_dynamic_call_count,
                "Got unexpected call count %lu, expected %u, test %u.\n",
                test_C2VectParallel_call_count, test->expected_dynamic_call_count, i);
    }
}

START_TEST(vcomp110)
{
    if (!init_vcomp())
        return;

    test_C2VectParallel();
}
