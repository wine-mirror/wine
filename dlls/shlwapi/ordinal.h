/*
 * Internal structures (read "undocumented") used by the
 * ordinal entry points.
 *
 * Determined by experimentation.
 *
 * Copyright 2001 Guy Albertelli
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

typedef struct {
    INT     size;      /* [in]  (always 0x18)                       */
    LPCSTR  ap1;       /* [out] start of scheme                     */
    INT     sizep1;    /* [out] size of scheme (until colon)        */
    LPCSTR  ap2;       /* [out] pointer following first colon       */
    INT     sizep2;    /* [out] size of remainder                   */
    INT     fcncde;    /* [out] function match of p1 (0 if unknown) */
} UNKNOWN_SHLWAPI_1;

DWORD WINAPI SHLWAPI_1(LPCSTR x, UNKNOWN_SHLWAPI_1 *y);

typedef struct {
    INT     size;      /* [in]  (always 0x18)                       */
    LPCWSTR ap1;       /* [out] start of scheme                     */
    INT     sizep1;    /* [out] size of scheme (until colon)        */
    LPCWSTR ap2;       /* [out] pointer following first colon       */
    INT     sizep2;    /* [out] size of remainder                   */
    INT     fcncde;    /* [out] function match of p1 (0 if unknown) */
} UNKNOWN_SHLWAPI_2;

DWORD WINAPI SHLWAPI_2(LPCWSTR x, UNKNOWN_SHLWAPI_2 *y);

/* Macro to get function pointer for a module*/
#define GET_FUNC(func, module, name, fail) \
  do { \
    if (!func) { \
      if (!SHLWAPI_h##module && !(SHLWAPI_h##module = LoadLibraryA(#module ".dll"))) return fail; \
      if (!(func = (void*)GetProcAddress(SHLWAPI_h##module, name))) return fail; \
    } \
  } while (0)

/* SHLWAPI_266 definitions and structures */
#define SHELL_NO_POLICY -1

typedef struct tagPOLICYDATA
{
  DWORD policy;        /* flags value passed to SHRestricted */
  LPCWSTR appstr;      /* application str such as "Explorer" */
  LPCWSTR keystr;      /* name of the actual registry key / policy */
} POLICYDATA, *LPPOLICYDATA;


extern HMODULE SHLWAPI_hshell32;

/* Shared internal functions */
BOOL WINAPI SHLWAPI_PathFindLocalExeA(LPSTR lpszPath, DWORD dwWhich);
BOOL WINAPI SHLWAPI_PathFindLocalExeW(LPWSTR lpszPath, DWORD dwWhich);
BOOL WINAPI SHLWAPI_PathFindOnPathExA(LPSTR lpszFile,LPCSTR *lppszOtherDirs,DWORD dwWhich);
BOOL WINAPI SHLWAPI_PathFindOnPathExW(LPWSTR lpszFile,LPCWSTR *lppszOtherDirs,DWORD dwWhich);
