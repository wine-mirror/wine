/*
 * File         : memory_lim.c
 * Author       : Mirco Miranda
 * Created on   : 2026/5/26
 * Description  :
 */

#include "memory_lim.h"

#if defined(__STDC_VERSION__) && !defined(__STDC_NO_ATOMICS__) && (__STDC_VERSION__ >= 201112L)
#include <stdatomic.h>
static atomic_size_t CB_PKAlloc_MaxBytes = 0;
#else
static size_t CB_PKAlloc_MaxBytes = 0;
#endif

void alloc_set_lim(size_t max_size)
{
    CB_PKAlloc_MaxBytes = max_size;
}

size_t alloc_get_lim()
{
    return CB_PKAlloc_MaxBytes;
}

void *malloc_lim(size_t size)
{
    size_t cbMax = alloc_get_lim();
    if(cbMax && size > cbMax) {
        return NULL;
    }
    return malloc(size);
}

void *calloc_lim(size_t count, size_t size)
{
    size_t cbMax = alloc_get_lim();
    // Use division instead of multiplication to avoid a size_t overflow in the limit check.
    if(cbMax && count > 0 && size > cbMax / count) {
        return NULL;
    }
    return calloc(count, size);
}
