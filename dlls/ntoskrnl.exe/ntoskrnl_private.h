/*
 * ntoskrnl.exe implementation
 *
 * Copyright (C) 2007 Alexandre Julliard
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

#ifndef __WINE_NTOSKRNL_PRIVATE_H
#define __WINE_NTOSKRNL_PRIVATE_H

struct _OBJECT_TYPE
{
    const WCHAR *name;            /* object type name used for type validation */
    void *(*constructor)(HANDLE); /* used for creating an object from server handle */
    void (*release)(void*);       /* called when the last reference is released */
};

struct _EPROCESS
{
    DISPATCHER_HEADER header;
    PROCESS_BASIC_INFORMATION info;
};

struct _KTHREAD
{
    DISPATCHER_HEADER header;
    PEPROCESS process;
    CLIENT_ID id;
};

struct _ETHREAD
{
    struct _KTHREAD kthread;
};

void *alloc_kernel_object( POBJECT_TYPE type, HANDLE handle, SIZE_T size, LONG ref ) DECLSPEC_HIDDEN;
NTSTATUS kernel_object_from_handle( HANDLE handle, POBJECT_TYPE type, void **ret ) DECLSPEC_HIDDEN;

extern POBJECT_TYPE ExEventObjectType;
extern POBJECT_TYPE ExSemaphoreObjectType;
extern POBJECT_TYPE IoDeviceObjectType;
extern POBJECT_TYPE IoDriverObjectType;
extern POBJECT_TYPE IoFileObjectType;
extern POBJECT_TYPE PsProcessType;
extern POBJECT_TYPE PsThreadType;
extern POBJECT_TYPE SeTokenObjectType;


#ifdef __i386__
#define DEFINE_FASTCALL1_WRAPPER(func) \
    __ASM_STDCALL_FUNC( __fastcall_ ## func, 4, \
                       "popl %eax\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME(#func) __ASM_STDCALL(4) )
#define DEFINE_FASTCALL_WRAPPER(func,args) \
    __ASM_STDCALL_FUNC( __fastcall_ ## func, args, \
                       "popl %eax\n\t" \
                       "pushl %edx\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )
#else
#define DEFINE_FASTCALL1_WRAPPER(func) /* nothing */
#define DEFINE_FASTCALL_WRAPPER(func,args) /* nothing */
#endif

#endif
