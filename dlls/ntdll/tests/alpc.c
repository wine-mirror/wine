/*
 * Advanced Local Procedure Call tests
 *
 * Copyright 2026 Zhiyi Zhang for CodeWeavers
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

#include <stdarg.h>
#include <windef.h>
#include <winternl.h>
#include <ntstatus.h>
#include "wine/test.h"

#define DECL_FUNCPTR(f) static typeof(f) *p##f;

DECL_FUNCPTR(AlpcGetHeaderSize)
DECL_FUNCPTR(AlpcGetMessageAttribute)
DECL_FUNCPTR(AlpcInitializeMessageAttribute)

#undef DECL_FUNCPTR

static void init_functions(void)
{
    HMODULE ntdll;

    ntdll = GetModuleHandleA("ntdll.dll");
    ok(ntdll != NULL, "GetModuleHandleA failed.\n");

#define LOAD_FUNCPTR(f) p##f = (void *)GetProcAddress(ntdll, #f);

    LOAD_FUNCPTR(AlpcGetHeaderSize)
    LOAD_FUNCPTR(AlpcGetMessageAttribute)
    LOAD_FUNCPTR(AlpcInitializeMessageAttribute)

#undef LOAD_FUNCPTR
}

static void test_AlpcGetHeaderSize(void)
{
    unsigned int i, j;
    SIZE_T size;

    static const struct
    {
        ULONG attribute;
        SIZE_T size;
    }
    tests[] =
    {
        {0, 0},
        {1u << 1, 0},
        {1u << 2, 0},
        {1u << 3, 0},
        {1u << 4, 0},
        {1u << 5, 0},
        {1u << 6, 0},
        {1u << 7, 0},
        {1u << 8, 0},
        {1u << 9, 0},
        {1u << 10, 0},
        {1u << 11, 0},
        {1u << 12, 0},
        {1u << 13, 0},
        {1u << 14, 0},
        {1u << 15, 0},
        {1u << 16, 0},
        {1u << 17, 0},
        {1u << 18, 0},
        {1u << 19, 0},
        {1u << 20, 0},
        {1u << 21, 0},
        {1u << 22, 0},
        {1u << 23, 0},
        {1u << 24, 0},
        {1u << 25, sizeof(ALPC_WORK_ON_BEHALF_ATTR)},   /* ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE */
        {1u << 26, sizeof(ALPC_DIRECT_ATTR)},           /* ALPC_MESSAGE_DIRECT_ATTRIBUTE */
        {1u << 27, sizeof(ALPC_TOKEN_ATTR)},            /* ALPC_MESSAGE_TOKEN_ATTRIBUTE */
        {1u << 28, sizeof(ALPC_HANDLE_ATTR)},           /* ALPC_MESSAGE_HANDLE_ATTRIBUTE */
        {1u << 29, sizeof(ALPC_CONTEXT_ATTR)},          /* ALPC_MESSAGE_CONTEXT_ATTRIBUTE */
        {1u << 30, sizeof(ALPC_VIEW_ATTR)},             /* ALPC_MESSAGE_VIEW_ATTRIBUTE */
        {1u << 31, sizeof(ALPC_SECURITY_ATTR)},         /* ALPC_MESSAGE_SECURITY_ATTRIBUTE */
        {0xffffffff, sizeof(ALPC_WORK_ON_BEHALF_ATTR) +
                     sizeof(ALPC_DIRECT_ATTR) +
                     sizeof(ALPC_TOKEN_ATTR) +
                     sizeof(ALPC_HANDLE_ATTR) +
                     sizeof(ALPC_CONTEXT_ATTR) +
                     sizeof(ALPC_VIEW_ATTR) +
                     sizeof(ALPC_SECURITY_ATTR)},       /* All attributes */
    };

    if (!pAlpcGetHeaderSize)
    {
        win_skip("AlpcGetHeaderSize is unavailable.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context("i %d", i);

        /* Single attribute */
        size = pAlpcGetHeaderSize(tests[i].attribute);
        ok(size == sizeof(ALPC_MESSAGE_ATTRIBUTES) + tests[i].size, "Got unexpected size %Ix.\n", size);

        /* Two attributes */
        for (j = 0; j < ARRAY_SIZE(tests); j++)
        {
            if (tests[i].attribute & tests[j].attribute)
                continue;

            winetest_push_context("j %d", j);

            size = pAlpcGetHeaderSize(tests[i].attribute | tests[j].attribute);
            ok(size == sizeof(ALPC_MESSAGE_ATTRIBUTES) + tests[i].size + tests[j].size,
               "Got unexpected size %Ix.\n", size);

            winetest_pop_context();
        }
        winetest_pop_context();
    }
}

static void test_AlpcGetMessageAttribute(void)
{
    unsigned char buffer[1024];
    ALPC_MESSAGE_ATTRIBUTES *attr = (ALPC_MESSAGE_ATTRIBUTES *)buffer;
    unsigned int i, j;
    void *ptr;

    static const ULONG attributes[] =
    {
        0,
        ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE,
        ALPC_MESSAGE_DIRECT_ATTRIBUTE,
        ALPC_MESSAGE_TOKEN_ATTRIBUTE,
        ALPC_MESSAGE_HANDLE_ATTRIBUTE,
        ALPC_MESSAGE_CONTEXT_ATTRIBUTE,
        ALPC_MESSAGE_VIEW_ATTRIBUTE,
        ALPC_MESSAGE_SECURITY_ATTRIBUTE,
    };

    if (!pAlpcGetMessageAttribute)
    {
        todo_wine
        win_skip("AlpcGetMessageAttribute is unavailable.\n");
        return;
    }

    /* Invalid flag */
    attr->AllocatedAttributes = ALPC_MESSAGE_ATTRIBUTE_ALL;
    attr->ValidAttributes = ALPC_MESSAGE_ATTRIBUTE_ALL;
    ptr = pAlpcGetMessageAttribute(attr, 0x1);
    ok(ptr == NULL, "Got unexpected ptr.\n");

    for (i = 0; i < ARRAY_SIZE(attributes); i++)
    {
        winetest_push_context("%#lx", attributes[i]);

        /* The message has no allocated attributes */
        attr->AllocatedAttributes = 0;
        attr->ValidAttributes = attributes[i];
        ptr = pAlpcGetMessageAttribute(attr, attributes[i]);
        ok(ptr == NULL, "Got unexpected ptr.\n");

        /* The message has no valid attributes */
        attr->AllocatedAttributes = attributes[i];
        attr->ValidAttributes = 0;
        ptr = pAlpcGetMessageAttribute(attr, attributes[i]);
        if (attributes[i])
            ok(ptr == ((unsigned char *)attr + sizeof(*attr)), "Got unexpected ptr.\n");
        else
            ok(ptr == NULL, "Got unexpected ptr.\n");

        /* Normal calls */
        attr->AllocatedAttributes = attributes[i];
        attr->ValidAttributes = attributes[i];
        ptr = pAlpcGetMessageAttribute(attr, attributes[i]);
        if (attributes[i])
            ok(ptr == ((unsigned char *)attr + sizeof(*attr)), "Got unexpected ptr.\n");
        else
            ok(ptr == NULL, "Got unexpected ptr.\n");

        /* The message has full attributes */
        attr->AllocatedAttributes = ALPC_MESSAGE_ATTRIBUTE_ALL;
        attr->ValidAttributes = ALPC_MESSAGE_ATTRIBUTE_ALL;
        ptr = pAlpcGetMessageAttribute(attr, attributes[i]);
        if (attributes[i])
            ok(ptr == ((unsigned char *)attr + pAlpcGetHeaderSize(attr->AllocatedAttributes & ~(attributes[i] | (attributes[i] - 1)))),
               "Got unexpected ptr.\n");
        else
            ok(ptr == NULL, "Got unexpected ptr.\n");

        /* The message has some valid attributes */
        for (j = 0; j < ARRAY_SIZE(attributes); j++)
        {
            attr->AllocatedAttributes = ALPC_MESSAGE_ATTRIBUTE_ALL;
            attr->ValidAttributes = ALPC_MESSAGE_ATTRIBUTE_ALL & ~attributes[j];
            ptr = pAlpcGetMessageAttribute(attr, attributes[i]);
            if (attributes[i])
                ok(ptr == ((unsigned char *)attr + pAlpcGetHeaderSize(attr->AllocatedAttributes & ~(attributes[i] | (attributes[i] - 1)))),
                   "Got unexpected ptr.\n");
            else
                ok(ptr == NULL, "Got unexpected ptr.\n");
        }

        /* Two attributes are passed to AlpcGetMessageAttribute() */
        for (j = 0; j < ARRAY_SIZE(attributes); j++)
        {
            if (!attributes[i] || !attributes[j] || attributes[i] == attributes[j])
                continue;

            attr->AllocatedAttributes = ALPC_MESSAGE_ATTRIBUTE_ALL;
            attr->ValidAttributes = ALPC_MESSAGE_ATTRIBUTE_ALL;
            ptr = pAlpcGetMessageAttribute(attr, attributes[i] | attributes[j]);
            ok(ptr == NULL, "Got unexpected ptr.\n");
        }

        winetest_pop_context();
    }
}

static void test_AlpcInitializeMessageAttribute(void)
{
    unsigned char buffer[1024];
    ALPC_MESSAGE_ATTRIBUTES *attr = (ALPC_MESSAGE_ATTRIBUTES *)buffer;
    SIZE_T required_size, size;
    unsigned int i, j, k;
    NTSTATUS status;

    static const ULONG attributes[] =
    {
        0,
        ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE,
        ALPC_MESSAGE_DIRECT_ATTRIBUTE,
        ALPC_MESSAGE_TOKEN_ATTRIBUTE,
        ALPC_MESSAGE_HANDLE_ATTRIBUTE,
        ALPC_MESSAGE_CONTEXT_ATTRIBUTE,
        ALPC_MESSAGE_VIEW_ATTRIBUTE,
        ALPC_MESSAGE_SECURITY_ATTRIBUTE,
        0xffffffff,
    };

    if (!pAlpcInitializeMessageAttribute)
    {
        win_skip("AlpcInitializeMessageAttribute is unavailable.\n");
        return;
    }

    /* Check parameters */
    for (i = 0; i < ARRAY_SIZE(attributes); i++)
    {
        winetest_push_context("i %d", i);

        size = pAlpcGetHeaderSize(attributes[i]);

        /* Null message pointer */
        required_size = 0;
        status = pAlpcInitializeMessageAttribute(attributes[i], NULL, size, &required_size);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        ok(required_size == size, "Expected %Ix, got %Ix.\n", size, required_size);

        /* Buffer too small */
        attr->AllocatedAttributes = 0xdeadbeef;
        attr->ValidAttributes = 0xdeadbeef;
        required_size = 0;
        status = pAlpcInitializeMessageAttribute(attributes[i], attr, size - 1, &required_size);
        ok(status == STATUS_BUFFER_TOO_SMALL, "Got unexpected status %#lx.\n", status);
        ok(attr->AllocatedAttributes == 0xdeadbeef, "Got unexpected %#lx.\n", attr->AllocatedAttributes);
        ok(attr->ValidAttributes == 0xdeadbeef, "Got unexpected %#lx.\n", attr->ValidAttributes);
        ok(required_size == size, "Expected %Ix, got %Ix.\n", size, required_size);

        /* Buffer too large */
        attr->AllocatedAttributes = 0xdeadbeef;
        attr->ValidAttributes = 0xdeadbeef;
        required_size = 0;
        status = pAlpcInitializeMessageAttribute(attributes[i], attr, size + 1, &required_size);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        ok(attr->AllocatedAttributes == attributes[i], "Got unexpected %#lx.\n", attr->AllocatedAttributes);
        ok(attr->ValidAttributes == 0, "Got unexpected %#lx.\n", attr->ValidAttributes);
        ok(required_size == size, "Expected %Ix, got %Ix.\n", size, required_size);

        /* Correct buffer size */
        memset(buffer, 0xa1, sizeof(buffer));
        required_size = 0;
        status = pAlpcInitializeMessageAttribute(attributes[i], attr, size, &required_size);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        ok(attr->AllocatedAttributes == attributes[i], "Got unexpected %#lx.\n", attr->AllocatedAttributes);
        ok(attr->ValidAttributes == 0, "Got unexpected %#lx.\n", attr->ValidAttributes);
        ok(required_size == size, "Expected %Ix, got %Ix.\n", size, required_size);
        /* Test that AlpcInitializeMessageAttribute() only sets AllocatedAttributes and ValidAttributes */
        for (k = sizeof(*attr); k < sizeof(buffer); k++)
        {
            if (buffer[k] != 0xa1)
            {
                ok(0, "Marker got overwritten at %d.\n", k);
                break;
            }
        }

        /* Two attributes */
        for (j = 0; j < ARRAY_SIZE(attributes); j++)
        {
            if (attributes[i] & attributes[j])
                continue;

            winetest_push_context("j %d", j);

            memset(buffer, 0xb2, sizeof(buffer));
            size = pAlpcGetHeaderSize(attributes[i] | attributes[j]);
            required_size = 0;
            status = pAlpcInitializeMessageAttribute(attributes[i] | attributes[j], attr, size, &required_size);
            ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
            ok(attr->AllocatedAttributes == (attributes[i] | attributes[j]),
               "Got unexpected %#lx.\n", attr->AllocatedAttributes);
            ok(attr->ValidAttributes == 0, "Got unexpected %#lx.\n", attr->ValidAttributes);
            ok(required_size == size, "Expected %Ix, got %Ix.\n", size, required_size);
            /* Test that AlpcInitializeMessageAttribute() only sets AllocatedAttributes and ValidAttributes */
            for (k = sizeof(*attr); k < sizeof(buffer); k++)
            {
                if (buffer[k] != 0xb2)
                {
                    ok(0, "Marker got overwritten at %d.\n", k);
                    break;
                }
            }

            winetest_pop_context();
        }
        winetest_pop_context();
    }
}

START_TEST(alpc)
{
    init_functions();

    test_AlpcGetHeaderSize();
    test_AlpcGetMessageAttribute();
    test_AlpcInitializeMessageAttribute();
}
