/*
 * Copyright 2014 Hans Leidekker for CodeWeavers
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
#include "windef.h"
#include "winbase.h"
#include "vss.h"
#include "vswriter.h"
#include "vsbackup.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( vssapi );

struct CVssWriter
{
    void **vtable;
};

/******************************************************************
 *  ??0CVssWriter@@QAE@XZ (VSSAPI.@)
 */
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_default_ctor, 4 )
struct CVssWriter * __thiscall VSSAPI_CVssWriter_default_ctor( struct CVssWriter *writer )
{
    FIXME( "%p\n", writer );
    writer->vtable = NULL;
    return writer;
}

/******************************************************************
 *  ??1CVssWriter@@UAE@XZ (VSSAPI.@)
 */
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_dtor, 4 )
void __thiscall VSSAPI_CVssWriter_dtor( struct CVssWriter *writer )
{
    FIXME( "%p\n", writer );
}

/******************************************************************
 *  ?Initialize@CVssWriter@@QAGJU_GUID@@PBGW4VSS_USAGE_TYPE@@W4VSS_SOURCE_TYPE@@W4_VSS_APPLICATION_LEVEL@@KW4VSS_ALTERNATE_WRITER_STATE@@_N@Z
 */
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_Initialize, 52 )
HRESULT __thiscall VSSAPI_CVssWriter_Initialize( struct CVssWriter *writer, VSS_ID id,
    LPCWSTR name, VSS_USAGE_TYPE usage_type, VSS_SOURCE_TYPE source_type,
    VSS_APPLICATION_LEVEL level, DWORD timeout, VSS_ALTERNATE_WRITER_STATE alt_writer_state,
    BOOL throttle, LPCWSTR instance )
{
    FIXME( "%p, %s, %s, %u, %u, %u, %lu, %u, %d, %s\n", writer, debugstr_guid(&id),
           debugstr_w(name), usage_type, source_type, level, timeout, alt_writer_state,
           throttle, debugstr_w(instance) );
    return S_OK;
}

/******************************************************************
 *  ?Subscribe@CVssWriter@@QAGJK@Z
 */
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_Subscribe, 8 )
HRESULT __thiscall VSSAPI_CVssWriter_Subscribe( struct CVssWriter *writer, DWORD flags )
{
    FIXME( "%p, %lx\n", writer, flags );
    return S_OK;
}

/******************************************************************
 *  ?Unsubscribe@CVssWriter@@QAGJXZ
 */
DEFINE_THISCALL_WRAPPER( VSSAPI_CVssWriter_Unsubscribe, 4 )
HRESULT __thiscall VSSAPI_CVssWriter_Unsubscribe( struct CVssWriter *writer )
{
    FIXME( "%p\n", writer );
    return S_OK;
}

HRESULT WINAPI CreateVssBackupComponentsInternal(IVssBackupComponents **backup)
{
    FIXME("%p\n", backup);
    return E_NOTIMPL;
}

/******************************************************************
 *  ?CreateVssBackupComponents@@YGJPAPAVIVssBackupComponents@@@Z
 */
HRESULT WINAPI VSSAPI_CreateVssBackupComponents( IVssBackupComponents **backup )
{
    FIXME( "%p\n", backup );

    return CreateVssBackupComponentsInternal(backup);
}
