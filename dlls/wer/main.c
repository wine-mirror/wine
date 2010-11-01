/*
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

HRESULT WINAPI WerAddExcludedApplication(PCWSTR exeName, BOOL allUsers)
{
    FIXME("(%s, %d) stub\n",debugstr_w(exeName), allUsers);
    return E_NOTIMPL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

/***********************************************************************
 * WerRemoveExcludedApplication (wer.@)
 *
 * remove an application from the exclusion list
 *
 * PARAMS
 *  exeName  [i] The application name
 *  allUsers [i] for all users (TRUE) or for the current user (FALSE)
 *
 * RESULTS
 *  SUCCESS  S_OK
 *  FAILURE  A HRESULT error code
 *
 */
HRESULT WINAPI WerRemoveExcludedApplication(PCWSTR exeName, BOOL allUsers)
{
    FIXME("(%s, %d) :stub\n",debugstr_w(exeName), allUsers);
    return E_NOTIMPL;
}

/***********************************************************************
 * WerReportCloseHandle (wer.@)
 *
 * Close an error reporting handle and free associated resources
 *
 * PARAMS
 *  hreport [i] error reporting handle to close
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 */
HRESULT WINAPI WerReportCloseHandle(HREPORT hreport)
{
    FIXME("(%p) :stub\n", hreport);

    return E_NOTIMPL;
}

/***********************************************************************
 * WerReportCreate (wer.@)
 *
 * Create an error report in memory and return a related HANDLE
 *
 * PARAMS
 *  eventtype  [i] a name for the event type
 *  reporttype [i] what type of report should be created
 *  reportinfo [i] NULL or a ptr to a struct with some detailed information
 *  phandle    [o] ptr, where the resulting handle should be saved
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 * NOTES
 *  The event type must be registered at microsoft. Predefined types are
 *  "APPCRASH" as the default on Windows, "Crash32" and "Crash64"
 *
 */
HRESULT WINAPI WerReportCreate(PCWSTR eventtype, WER_REPORT_TYPE reporttype, PWER_REPORT_INFORMATION reportinfo, HREPORT *phandle)
{

    FIXME("(%s, %d, %p, %p) :stub\n", debugstr_w(eventtype), reporttype, reportinfo, phandle);
    if (reportinfo) {
        TRACE(".wzFriendlyEventName: %s\n", debugstr_w(reportinfo->wzFriendlyEventName));
        TRACE(".wzApplicationName: %s\n", debugstr_w(reportinfo->wzApplicationName));
    }

    if (phandle)  *phandle = NULL;
    if (!eventtype || !eventtype[0] || !phandle) {
        return E_INVALIDARG;
    }

    return E_NOTIMPL;
}
