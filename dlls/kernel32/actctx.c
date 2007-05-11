/*
 * Activation contexts
 *
 * Copyright 2004 Jon Griffiths
 * Copyright 2007 Eric Pouech
 * Copyright 2007 Jacek Caban for CodeWeavers
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
#include "wine/port.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(actctx);


#define ACTCTX_FLAGS_ALL (\
 ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID |\
 ACTCTX_FLAG_LANGID_VALID |\
 ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID |\
 ACTCTX_FLAG_RESOURCE_NAME_VALID |\
 ACTCTX_FLAG_SET_PROCESS_DEFAULT |\
 ACTCTX_FLAG_APPLICATION_NAME_VALID |\
 ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF |\
 ACTCTX_FLAG_HMODULE_VALID )

#define ACTCTX_FAKE_HANDLE ((HANDLE) 0xf00baa)
#define ACTCTX_FAKE_COOKIE ((ULONG_PTR) 0xf00bad)

#define ACTCTX_MAGIC       0xC07E3E11

struct actctx
{
    ULONG               magic;
    LONG                ref_count;
};

/***********************************************************************
 * CreateActCtxA (KERNEL32.@)
 *
 * Create an activation context.
 */
HANDLE WINAPI CreateActCtxA(PCACTCTXA pActCtx)
{
    ACTCTXW     actw;
    SIZE_T      len;
    HANDLE      ret = INVALID_HANDLE_VALUE;
    LPWSTR      src = NULL, assdir = NULL, resname = NULL, appname = NULL;

    TRACE("%p %08x\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

    if (!pActCtx || pActCtx->cbSize != sizeof(*pActCtx) ||
        (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    actw.cbSize = sizeof(actw);
    actw.dwFlags = pActCtx->dwFlags;
    if (pActCtx->lpSource)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pActCtx->lpSource, -1, NULL, 0);
        src = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!src) return INVALID_HANDLE_VALUE;
        MultiByteToWideChar(CP_ACP, 0, pActCtx->lpSource, -1, src, len);
    }
    actw.lpSource = src;

    if (actw.dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID)
        actw.wProcessorArchitecture = pActCtx->wProcessorArchitecture;
    if (actw.dwFlags & ACTCTX_FLAG_LANGID_VALID)
        actw.wLangId = pActCtx->wLangId;
    if (actw.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pActCtx->lpAssemblyDirectory, -1, NULL, 0);
        assdir = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!assdir) goto done;
        MultiByteToWideChar(CP_ACP, 0, pActCtx->lpAssemblyDirectory, -1, assdir, len);
        actw.lpAssemblyDirectory = assdir;
    }
    if (actw.dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID)
    {
        if ((ULONG_PTR)pActCtx->lpResourceName >> 16)
        {
            len = MultiByteToWideChar(CP_ACP, 0, pActCtx->lpResourceName, -1, NULL, 0);
            resname = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            if (!resname) goto done;
            MultiByteToWideChar(CP_ACP, 0, pActCtx->lpResourceName, -1, resname, len);
            actw.lpResourceName = resname;
        }
        else actw.lpResourceName = (LPWSTR)pActCtx->lpResourceName;
    }
    if (actw.dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pActCtx->lpApplicationName, -1, NULL, 0);
        appname = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!appname) goto done;
        MultiByteToWideChar(CP_ACP, 0, pActCtx->lpApplicationName, -1, appname, len);
        actw.lpApplicationName = appname;
    }
    if (actw.dwFlags & ACTCTX_FLAG_HMODULE_VALID)
        actw.hModule = pActCtx->hModule;

    ret = CreateActCtxW(&actw);

done:
    HeapFree(GetProcessHeap(), 0, src);
    HeapFree(GetProcessHeap(), 0, assdir);
    HeapFree(GetProcessHeap(), 0, resname);
    HeapFree(GetProcessHeap(), 0, appname);
    return ret;
}

/***********************************************************************
 * CreateActCtxW (KERNEL32.@)
 *
 * Create an activation context.
 */
HANDLE WINAPI CreateActCtxW(PCACTCTXW pActCtx)
{
    struct actctx*      actctx;
    DWORD               ret = ERROR_SUCCESS;

    TRACE("%p %08x\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

    if (!pActCtx || pActCtx->cbSize != sizeof(*pActCtx) ||
        (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    actctx = HeapAlloc(GetProcessHeap(), 0, sizeof(*actctx));
    if (!actctx) return INVALID_HANDLE_VALUE;

    actctx->magic = ACTCTX_MAGIC;
    actctx->ref_count = 1;

    if (ret == ERROR_SUCCESS)
    {
        return (HANDLE)actctx;
    }

    ReleaseActCtx((HANDLE)actctx);
    SetLastError(ret);
    return INVALID_HANDLE_VALUE;
}

static struct actctx* check_actctx(HANDLE h)
{
    struct actctx*      actctx = (struct actctx*)h;

    switch (actctx->magic)
    {
    case ACTCTX_MAGIC: return actctx;
    default:
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
}

/***********************************************************************
 * ActivateActCtx (KERNEL32.@)
 *
 * Activate an activation context.
 */
BOOL WINAPI ActivateActCtx(HANDLE hActCtx, ULONG_PTR *ulCookie)
{
  static BOOL reported = FALSE;

  if (reported)
    TRACE("%p %p\n", hActCtx, ulCookie);
  else
  {
    FIXME("%p %p\n", hActCtx, ulCookie);
    reported = TRUE;
  }

  if (ulCookie)
    *ulCookie = ACTCTX_FAKE_COOKIE;
  return TRUE;
}

/***********************************************************************
 * DeactivateActCtx (KERNEL32.@)
 *
 * Deactivate an activation context.
 */
BOOL WINAPI DeactivateActCtx(DWORD dwFlags, ULONG_PTR ulCookie)
{
  static BOOL reported = FALSE;

  if (reported)
    TRACE("%08x %08lx\n", dwFlags, ulCookie);
  else
  {
    FIXME("%08x %08lx\n", dwFlags, ulCookie);
    reported = TRUE;
  }

  if (ulCookie != ACTCTX_FAKE_COOKIE)
    return FALSE;
  return TRUE;
}

/***********************************************************************
 * GetCurrentActCtx (KERNEL32.@)
 *
 * Get the current activation context.
 */
BOOL WINAPI GetCurrentActCtx(HANDLE* phActCtx)
{
  FIXME("%p\n", phActCtx);
  *phActCtx = ACTCTX_FAKE_HANDLE;
  return TRUE;
}

/***********************************************************************
 * AddRefActCtx (KERNEL32.@)
 *
 * Add a reference to an activation context.
 */
void WINAPI AddRefActCtx(HANDLE hActCtx)
{
    struct actctx*      actctx;

    TRACE("%p\n", hActCtx);

    if ((actctx = check_actctx(hActCtx)))
        InterlockedIncrement( &actctx->ref_count );
}

/***********************************************************************
 * ReleaseActCtx (KERNEL32.@)
 *
 * Release a reference to an activation context.
 */
void WINAPI ReleaseActCtx(HANDLE hActCtx)
{
    struct actctx*      actctx;

    TRACE("%p\n", hActCtx);

    if ((actctx = check_actctx(hActCtx)))
    {
        if (!InterlockedDecrement( &actctx->ref_count ))
        {
            actctx->magic = 0;
            HeapFree(GetProcessHeap(), 0, actctx);
        }
    }
}

/***********************************************************************
 * ZombifyActCtx (KERNEL32.@)
 *
 * Release a reference to an activation context.
 */
BOOL WINAPI ZombifyActCtx(HANDLE hActCtx)
{
  FIXME("%p\n", hActCtx);
  if (hActCtx != ACTCTX_FAKE_HANDLE)
    return FALSE;
  return TRUE;
}

/***********************************************************************
 * FindActCtxSectionStringA (KERNEL32.@)
 *
 * Find information about a GUID in an activation context.
 */
BOOL WINAPI FindActCtxSectionStringA(DWORD dwFlags, const GUID* lpExtGuid,
                                    ULONG ulId, LPCSTR lpSearchStr,
                                    PACTCTX_SECTION_KEYED_DATA pInfo)
{
  FIXME("%08x %s %u %s %p\n", dwFlags, debugstr_guid(lpExtGuid),
       ulId, debugstr_a(lpSearchStr), pInfo);
  SetLastError( ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 * FindActCtxSectionStringW (KERNEL32.@)
 *
 * Find information about a GUID in an activation context.
 */
BOOL WINAPI FindActCtxSectionStringW(DWORD dwFlags, const GUID* lpExtGuid,
                                    ULONG ulId, LPCWSTR lpSearchStr,
                                    PACTCTX_SECTION_KEYED_DATA pInfo)
{
  FIXME("%08x %s %u %s %p\n", dwFlags, debugstr_guid(lpExtGuid),
        ulId, debugstr_w(lpSearchStr), pInfo);

  if (lpExtGuid)
  {
    FIXME("expected lpExtGuid == NULL\n");
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  if (dwFlags & ~FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX)
  {
    FIXME("unknown dwFlags %08x\n", dwFlags);
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  if (!pInfo || pInfo->cbSize < sizeof (ACTCTX_SECTION_KEYED_DATA))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  pInfo->ulDataFormatVersion = 1;
  pInfo->lpData = NULL;
  pInfo->lpSectionGlobalData = NULL;
  pInfo->ulSectionGlobalDataLength = 0;
  pInfo->lpSectionBase = NULL;
  pInfo->ulSectionTotalLength = 0;
  pInfo->hActCtx = ACTCTX_FAKE_HANDLE;
  pInfo->ulAssemblyRosterIndex = 0;

  return TRUE;
}

/***********************************************************************
 * FindActCtxSectionGuid (KERNEL32.@)
 *
 * Find information about a GUID in an activation context.
 */
BOOL WINAPI FindActCtxSectionGuid(DWORD dwFlags, const GUID* lpExtGuid,
                                  ULONG ulId, const GUID* lpSearchGuid,
                                  PACTCTX_SECTION_KEYED_DATA pInfo)
{
  FIXME("%08x %s %u %s %p\n", dwFlags, debugstr_guid(lpExtGuid),
       ulId, debugstr_guid(lpSearchGuid), pInfo);
  SetLastError( ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 * QueryActCtxW (KERNEL32.@)
 *
 * Get information about an activation context.
 */
BOOL WINAPI QueryActCtxW(DWORD dwFlags, HANDLE hActCtx, PVOID pvSubInst,
                         ULONG ulClass, PVOID pvBuff, SIZE_T cbBuff,
                         SIZE_T *pcbLen)
{
  FIXME("%08x %p %p %u %p %ld %p\n", dwFlags, hActCtx,
       pvSubInst, ulClass, pvBuff, cbBuff, pcbLen);
  /* this makes Adobe Photoshop 7.0 happy */
  SetLastError( ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
