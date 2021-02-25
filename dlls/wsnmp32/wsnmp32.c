/*
 * Copyright 2012 Stefan Leichter
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
#include "winsnmp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsnmp32);

SNMPAPI_STATUS WINAPI SnmpCleanup( void )
{
    FIXME( "\n" );
    return SNMPAPI_SUCCESS;
}

HSNMP_SESSION WINAPI SnmpOpen( HWND hwnd, UINT msg )
{
    FIXME( "%p %u\n", hwnd, msg );
    return SNMPAPI_FAILURE;
}

SNMPAPI_STATUS WINAPI SnmpSetRetransmitMode( smiUINT32 retransmit_mode )
{
    FIXME( "%u\n", retransmit_mode );
    return SNMPAPI_SUCCESS;
}

SNMPAPI_STATUS WINAPI SnmpSetTranslateMode( smiUINT32 translate_mode )
{
    FIXME( "%u\n", translate_mode );
    return SNMPAPI_SUCCESS;
}

SNMPAPI_STATUS WINAPI SnmpStartup( smiLPUINT32 major, smiLPUINT32 minor, smiLPUINT32 level,
                                   smiLPUINT32 translate_mode, smiLPUINT32 retransmit_mode )
{
    FIXME( "%p, %p, %p, %p, %p\n", major, minor, level, translate_mode, retransmit_mode );

    if (major) *major = 2;
    if (minor) *minor = 0;
    if (level) *level = SNMPAPI_V2_SUPPORT;
    if (translate_mode) *translate_mode = SNMPAPI_UNTRANSLATED_V1;
    if (retransmit_mode) *retransmit_mode = SNMPAPI_ON;
    return SNMPAPI_SUCCESS;
}
