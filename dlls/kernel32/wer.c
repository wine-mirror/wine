/*
 * Windows Error Reporting functions
 *
 * Copyright 2010 Louis Lenders
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "werapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wer);

/***********************************************************************
 * WerRegisterFile (KERNEL32.@)
 *
 * Register a file to be included in WER error report.
 */
HRESULT WINAPI WerRegisterFile(PCWSTR file, WER_REGISTER_FILE_TYPE regfiletype, DWORD flags)
{
    FIXME("(%s, %d, %d) stub!\n", debugstr_w(file), regfiletype, flags);
    return E_NOTIMPL;
}

/***********************************************************************
 * WerRegisterRuntimeExceptionModule (KERNEL32.@)
 *
 * Register a custom runtime exception handler.
 */

HRESULT WINAPI WerRegisterRuntimeExceptionModule(PCWSTR callbackdll, PVOID context)
{
    FIXME("(%s, %p) stub!\n", debugstr_w(callbackdll), context);
    return S_OK;
}

/***********************************************************************
 * WerSetFlags (KERNEL32.@)
 *
 * Sets error reporting flags for the current process.
 */

HRESULT WINAPI WerSetFlags(DWORD flags)
{
    FIXME("(%d) stub!\n", flags);
    return E_NOTIMPL;
}

/***********************************************************************
 * WerRegisterMemoryBlock (KERNEL32.@)
 */
HRESULT WINAPI WerRegisterMemoryBlock(void *block, DWORD size)
{
    FIXME("(%p %d) stub\n", block, size);
    return E_NOTIMPL;
}

/***********************************************************************
 * WerUnregisterMemoryBlock (KERNEL32.@)
 */
HRESULT WINAPI WerUnregisterMemoryBlock(void *block)
{
    FIXME("(%p) stub\n", block);
    return E_NOTIMPL;
}
