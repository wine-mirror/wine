/*
 * Copyright (C) 2015 Austin English
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

#ifndef _NTDEF_
#define _NTDEF_

#include <basetsd.h>
#include <specstrings.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

typedef enum _TIMER_TYPE {
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;

typedef enum _WAIT_TYPE {
    WaitAll,
    WaitAny,
    WaitNotification
} WAIT_TYPE;

#ifdef __cplusplus
}
#endif

#define NT_SUCCESS(status)      (((NTSTATUS)(status)) >= 0)
#define NT_INFORMATION(status)  ((((NTSTATUS)(status)) & 0xc0000000) == 0x40000000)
#define NT_WARNING(status)      ((((NTSTATUS)(status)) & 0xc0000000) == 0x80000000)
#define NT_ERROR(status)        ((((NTSTATUS)(status)) & 0xc0000000) == 0xc0000000)

#define InitializeObjectAttributes(p,n,a,r,s) \
    do { \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
        (p)->RootDirectory = r; \
        (p)->Attributes = a; \
        (p)->ObjectName = n; \
        (p)->SecurityDescriptor = s; \
        (p)->SecurityQualityOfService = NULL; \
    } while (0)

#ifndef BASETYPES
#define BASETYPES
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned short USHORT, *PUSHORT;
#if !defined(__LP64__) && !defined(WINE_NO_LONG_TYPES)
typedef unsigned long ULONG, *PULONG;
#else
typedef unsigned int ULONG, *PULONG;
#endif
#endif

typedef ULONG CLONG;
typedef CLONG *PCLONG;

typedef struct _RTL_BALANCED_NODE
{
    union
    {
        struct _RTL_BALANCED_NODE *Children[2];
        struct
        {
            struct _RTL_BALANCED_NODE *Left;
            struct _RTL_BALANCED_NODE *Right;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    union
    {
        UCHAR Red : 1;
        UCHAR Balance : 2;
        ULONG_PTR ParentValue;
    } DUMMYUNIONNAME2;
} RTL_BALANCED_NODE, *PRTL_BALANCED_NODE;

#define RTL_BALANCED_NODE_RESERVED_PARENT_MASK 3

typedef struct _RTL_RB_TREE
{
    RTL_BALANCED_NODE *root;
    RTL_BALANCED_NODE *min;
} RTL_RB_TREE, *PRTL_RB_TREE;

#define RTL_CONSTANT_STRING(s) { sizeof(s) - sizeof(s[0]), sizeof(s), (void*)s }

#endif /* _NTDEF_ */
