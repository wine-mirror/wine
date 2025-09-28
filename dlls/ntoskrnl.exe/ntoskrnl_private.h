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

#include <stdarg.h>
#include <stdbool.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winioctl.h"
#include "winbase.h"
#include "winsvc.h"
#include "winternl.h"
#include "ddk/ntifs.h"
#include "ddk/wdm.h"

#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wine/rbtree.h"

static inline LPCSTR debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn( us->Buffer, us->Length / sizeof(WCHAR) );
}

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
    BOOL wow64;
};

struct _KTHREAD
{
    DISPATCHER_HEADER header;
    PEPROCESS process;
    CLIENT_ID id;
    unsigned int critical_region;
    KAFFINITY user_affinity;
};

struct _ETHREAD
{
    struct _KTHREAD kthread;
};

void *alloc_kernel_object( POBJECT_TYPE type, HANDLE handle, SIZE_T size, LONG ref );
NTSTATUS kernel_object_from_handle( HANDLE handle, POBJECT_TYPE type, void **ret );

extern POBJECT_TYPE ExEventObjectType;
extern POBJECT_TYPE ExSemaphoreObjectType;
extern POBJECT_TYPE IoDeviceObjectType;
extern POBJECT_TYPE IoDriverObjectType;
extern POBJECT_TYPE IoFileObjectType;
extern POBJECT_TYPE PsProcessType;
extern POBJECT_TYPE PsThreadType;
extern POBJECT_TYPE SeTokenObjectType;

#define DECLARE_CRITICAL_SECTION(cs) \
    static CRITICAL_SECTION cs; \
    static CRITICAL_SECTION_DEBUG cs##_debug = \
    { 0, 0, &cs, { &cs##_debug.ProcessLocksList, &cs##_debug.ProcessLocksList }, \
      0, 0, { (DWORD_PTR)(__FILE__ ": " # cs) }}; \
    static CRITICAL_SECTION cs = { &cs##_debug, -1, 0, 0, 0, 0 };

struct wine_driver
{
    DRIVER_OBJECT driver_obj;
    DRIVER_EXTENSION driver_extension;
    SERVICE_STATUS_HANDLE service_handle;
    struct wine_rb_entry entry;
    struct list root_pnp_devices;
};

void ObReferenceObject( void *obj );

void pnp_manager_start(void);
void pnp_manager_stop_driver( struct wine_driver *driver );
void pnp_manager_stop(void);

void CDECL wine_enumerate_root_devices( const WCHAR *driver_name );

struct wine_driver *get_driver( const WCHAR *name );

static const WCHAR servicesW[] = {'\\','R','e','g','i','s','t','r','y',
                                  '\\','M','a','c','h','i','n','e',
                                  '\\','S','y','s','t','e','m',
                                  '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
                                  '\\','S','e','r','v','i','c','e','s',
                                  '\\',0};

struct wine_device
{
    DEVICE_OBJECT device_obj;
    DEVICE_RELATIONS *children;
    HKEY dyn_data_key;
};
#endif
