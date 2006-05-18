/*
 * Activation contexts
 *
 * Copyright 2004 Jon Griffiths
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

/***********************************************************************
 * CreateActCtxA (KERNEL32.@)
 *
 * Create an activation context.
 */
HANDLE WINAPI CreateActCtxA(PCACTCTXA pActCtx)
{
  FIXME("%p %08lx\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

  if (!pActCtx)
    return INVALID_HANDLE_VALUE;
  if (pActCtx->cbSize != sizeof *pActCtx)
    return INVALID_HANDLE_VALUE;
  if (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL)
    return INVALID_HANDLE_VALUE;
  return ACTCTX_FAKE_HANDLE;
}

/***********************************************************************
 * CreateActCtxW (KERNEL32.@)
 *
 * Create an activation context.
 */
HANDLE WINAPI CreateActCtxW(PCACTCTXW pActCtx)
{
  FIXME("%p %08lx\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

  if (!pActCtx)
    return INVALID_HANDLE_VALUE;
  if (pActCtx->cbSize != sizeof *pActCtx)
    return INVALID_HANDLE_VALUE;
  if (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL)
    return INVALID_HANDLE_VALUE;
  return ACTCTX_FAKE_HANDLE;
}

/***********************************************************************
 * ActivateActCtx (KERNEL32.@)
 *
 * Activate an activation context.
 */
BOOL WINAPI ActivateActCtx(HANDLE hActCtx, ULONG_PTR *ulCookie)
{
  FIXME("%p %p\n", hActCtx, ulCookie );
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
  FIXME("%08lx %08lx\n", dwFlags, ulCookie);
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
  FIXME("%p\n", hActCtx);
}

/***********************************************************************
 * ReleaseActCtx (KERNEL32.@)
 *
 * Release a reference to an activation context.
 */
void WINAPI ReleaseActCtx(HANDLE hActCtx)
{
  FIXME("%p\n", hActCtx);
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
  FIXME("stub!\n");
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
  FIXME("stub!\n");
  return FALSE;
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
  FIXME("stub!\n");
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
  FIXME("stub!\n");
  /* this makes Adobe Photoshop 7.0 happy */
  SetLastError( ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
