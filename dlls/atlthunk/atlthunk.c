/*
 * Copyright 2019 Jacek Caban for CodeWeavers
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

#include "windows.h"
#include "atlthunk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(atlthunk);

#if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__)

struct AtlThunkData_t {
    struct thunk_pool *pool;
    WNDPROC proc;
    SIZE_T arg;
};

/* Thunk replaces the first argument and jumps to provided proc. */
#include "pshpack1.h"
struct thunk_code
{
#if defined(__x86_64__)
    BYTE  mov_rip_rcx[3];     /* mov mov_offset(%rip), %rcx */
    DWORD mov_offset;
    WORD  jmp_rip;            /* jmp *jmp_offset(%rip) */
    DWORD jmp_offset;
#elif defined(__i386__)
    BYTE  mov_data_addr_eax;  /* movl data_addr, %eax */
    DWORD data_addr;
    DWORD mov_eax_esp;        /* movl %eax, 4(%esp) */
    WORD  jmp;
    DWORD jmp_addr;           /* jmp *jmp_addr */
#elif defined(__aarch64__)
    DWORD ldr_x0;             /* ldr x0,data_addr */
    DWORD ldr_x16;            /* ldr x16,proc_addr */
    DWORD br_x16;             /* br x16 */
    DWORD pad;
#endif
};
#include "poppack.h"

#define THUNK_POOL_SIZE (4096 / sizeof(struct thunk_code))

struct thunk_pool
{
    struct thunk_code thunks[THUNK_POOL_SIZE];
    LONG first_free;
    LONG free_count;
    AtlThunkData_t data[THUNK_POOL_SIZE];
};

C_ASSERT(FIELD_OFFSET(struct thunk_pool, first_free) == 4096);

static struct thunk_pool *alloc_thunk_pool(void)
{
    struct thunk_pool *thunks;
    DWORD old_protect;
    unsigned i;

    if (!(thunks = VirtualAlloc(NULL, sizeof(*thunks), MEM_COMMIT, PAGE_READWRITE)))
        return NULL;

    for (i = 0; i < ARRAY_SIZE(thunks->thunks); i++)
    {
        struct thunk_code *thunk = &thunks->thunks[i];
#if defined(__x86_64__)
        thunk->mov_rip_rcx[0]    = 0x48;    /* mov mov_offset(%rip), %rcx */
        thunk->mov_rip_rcx[1]    = 0x8b;
        thunk->mov_rip_rcx[2]    = 0x0d;
        thunk->mov_offset = (const BYTE *)&thunks->data[i].arg - (const BYTE *)(&thunk->mov_offset + 1);
        thunk->jmp_rip           = 0x25ff;  /* jmp *jmp_offset(%rip) */
        thunk->jmp_offset = (const BYTE *)&thunks->data[i].proc - (const BYTE *)(&thunk->jmp_offset + 1);
#elif defined(__i386__)
        thunk->mov_data_addr_eax = 0xa1;       /* movl data_addr, %eax */
        thunk->data_addr         = (DWORD)&thunks->data[i].arg;
        thunk->mov_eax_esp       = 0x04244489; /* movl %eax, 4(%esp) */
        thunk->jmp               = 0x25ff;     /* jmp *jmp_addr */
        thunk->jmp_addr          = (DWORD)&thunks->data[i].proc;
#elif defined(__aarch64__)
        thunk->ldr_x0            = 0x58000000 | (((DWORD *)&thunks->data[i].arg - &thunk->ldr_x0) << 5);
        thunk->ldr_x16           = 0x58000010 | (((DWORD *)&thunks->data[i].proc - &thunk->ldr_x16) << 5);
        thunk->br_x16            = 0xd61f0200;
#endif
    }
    VirtualProtect(thunks->thunks, FIELD_OFFSET(struct thunk_pool, first_free), PAGE_EXECUTE_READ, &old_protect);
    thunks->first_free = 0;
    thunks->free_count = 0;
    return thunks;
}

static struct thunk_pool *current_pool;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(instance);
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        if (current_pool && current_pool->free_count == current_pool->first_free)
            VirtualFree(current_pool, sizeof(*current_pool), MEM_RELEASE);
        break;
    }
    return TRUE;
}

static CRITICAL_SECTION thunk_alloc_cs;
static CRITICAL_SECTION_DEBUG thunk_alloc_cs_debug = {
    0, 0, &thunk_alloc_cs,
    { &thunk_alloc_cs_debug.ProcessLocksList,
      &thunk_alloc_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": thunk_alloc_cs") }
};
static CRITICAL_SECTION thunk_alloc_cs = { &thunk_alloc_cs_debug, -1, 0, 0, 0, 0 };

AtlThunkData_t* WINAPI AtlThunk_AllocateData(void)
{
    AtlThunkData_t *thunk = NULL;

    EnterCriticalSection(&thunk_alloc_cs);

    if (!current_pool) current_pool = alloc_thunk_pool();
    if (current_pool)
    {
        thunk = &current_pool->data[current_pool->first_free];
        thunk->pool = current_pool;
        thunk->proc = NULL;
        thunk->arg  = 0;
        if (++current_pool->first_free == ARRAY_SIZE(current_pool->data))
            current_pool = NULL;
    }

    LeaveCriticalSection(&thunk_alloc_cs);
    return thunk;
}

WNDPROC WINAPI AtlThunk_DataToCode(AtlThunkData_t *thunk)
{
    WNDPROC code = (WNDPROC)&thunk->pool->thunks[thunk - thunk->pool->data];
    TRACE("(%p) -> %p\n", thunk, code);
    return code;
}

void WINAPI AtlThunk_FreeData(AtlThunkData_t *thunk)
{
    if (InterlockedIncrement(&thunk->pool->free_count) == ARRAY_SIZE(thunk->pool->thunks))
        VirtualFree(thunk->pool, sizeof(*thunk->pool), MEM_RELEASE);
}

void WINAPI AtlThunk_InitData(AtlThunkData_t *thunk, void *proc, SIZE_T arg)
{
    thunk->proc = proc;
    thunk->arg = arg;
}

#else  /* __i386__ || __x86_64__ || __aarch64__ */

AtlThunkData_t* WINAPI AtlThunk_AllocateData(void)
{
    FIXME("Unsupported architecture.\n");
    return NULL;
}

WNDPROC WINAPI AtlThunk_DataToCode(AtlThunkData_t *thunk)
{
    return NULL;
}

void WINAPI AtlThunk_FreeData(AtlThunkData_t *thunk)
{
}

void WINAPI AtlThunk_InitData(AtlThunkData_t *thunk, void *proc, SIZE_T arg)
{
}

#endif  /* __i386__ || __x86_64__ || __aarch64__ */
