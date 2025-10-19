/*
 * Copyright (C) 2023 Mohamad Al-Jaf
 * Copyright (C) 2025 Vibhav Pant
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

#include <stddef.h>
#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "winuser.h"

#include "cfgmgr32.h"
#include "setupapi.h"
#include "dbt.h"
#include "devfiltertypes.h"
#include "devquery.h"

#include "wine/plugplay.h"
#include "wine/rbtree.h"
#include "wine/debug.h"

static inline const char *debugstr_DEVPROPKEY( const DEVPROPKEY *key )
{
    if (!key) return "(null)";
    return wine_dbg_sprintf( "{%s, %04lx}", debugstr_guid( &key->fmtid ), key->pid );
}

static inline const char *debugstr_DEVPROPCOMPKEY( const DEVPROPCOMPKEY *key )
{
    if (!key) return "(null)";
    return wine_dbg_sprintf( "{%s, %d, %s}", debugstr_DEVPROPKEY( &key->Key ), key->Store,
                             debugstr_w( key->LocaleName ) );
}

struct property
{
    BOOL ansi;
    DEVPROPKEY key;
    DEVPROPTYPE *type;
    DWORD *reg_type;
    void *buffer;
    DWORD *size;
};

struct device_interface
{
    GUID class_guid;
    WCHAR class[39];
    WCHAR name[MAX_PATH];
    WCHAR refstr[MAX_PATH];
};

extern LSTATUS init_device_interface( struct device_interface *iface, const WCHAR *name );
extern LSTATUS open_device_interface_key( const struct device_interface *iface, REGSAM access, BOOL open, HKEY *hkey );
