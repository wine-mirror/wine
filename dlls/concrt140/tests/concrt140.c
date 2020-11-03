/*
 * Copyright 2020 Daniel Lehman
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

#include "wine/test.h"

typedef void (*vtable_ptr)(void);

typedef struct {
    const vtable_ptr *vtable;
} Context;

typedef struct {
    Context *ctx;
} _Context;

static Context* (__cdecl *p_Context_CurrentContext)(void);
static _Context* (__cdecl *p__Context__CurrentContext)(_Context*);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(module,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("concrt140.dll");
    if (!module)
    {
        win_skip("concrt140.dll not installed\n");
        return FALSE;
    }

    SET(p__Context__CurrentContext,
        "?_CurrentContext@_Context@details@Concurrency@@SA?AV123@XZ");
    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_Context_CurrentContext,
                "?CurrentContext@Context@Concurrency@@SAPEAV12@XZ");
    } else {
        SET(p_Context_CurrentContext,
                "?CurrentContext@Context@Concurrency@@SAPAV12@XZ");
    }

    return TRUE;
}

static void test_CurrentContext(void)
{
    _Context _ctx, *ret;
    Context *ctx;

    ctx = p_Context_CurrentContext();
    ok(!!ctx, "got NULL\n");

    memset(&_ctx, 0xcc, sizeof(_ctx));
    ret = p__Context__CurrentContext(&_ctx);
    ok(_ctx.ctx == ctx, "expected %p, got %p\n", ctx, _ctx.ctx);
    ok(ret == &_ctx, "expected %p, got %p\n", &_ctx, ret);
}

START_TEST(concrt140)
{
    if (!init()) return;
    test_CurrentContext();
}
