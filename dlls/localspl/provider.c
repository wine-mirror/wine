/*
 * Implementation of the Local Printprovider
 *
 * Copyright 2006-2009 Detlef Riekenberg
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
#include <stdlib.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winnls.h"
#include "winspool.h"
#include "winuser.h"
#include "ddk/winddiui.h"
#include "ddk/winsplp.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "localspl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(localspl);

static const struct builtin_form
{
    const WCHAR *name;
    SIZEL size;
    DWORD res_id;
} builtin_forms[] =
{
    { L"Letter", { 215900, 279400 }, IDS_FORM_LETTER },
    { L"Letter Small", { 215900, 279400 }, IDS_FORM_LETTER_SMALL },
    { L"Tabloid", { 279400, 431800 }, IDS_FORM_TABLOID },
    { L"Ledger", { 431800, 279400 }, IDS_FORM_LEDGER },
    { L"Legal", { 215900, 355600 }, IDS_FORM_LEGAL },
    { L"Statement", { 139700, 215900 }, IDS_FORM_STATEMENT },
    { L"Executive", { 184150, 266700 }, IDS_FORM_EXECUTIVE },
    { L"A3", { 297000, 420000 }, IDS_FORM_A3 },
    { L"A4", { 210000, 297000 }, IDS_FORM_A4 },
    { L"A4 Small", { 210000, 297000 }, IDS_FORM_A4_SMALL },
    { L"A5", { 148000, 210000 }, IDS_FORM_A5 },
    { L"B4 (JIS)", { 257000, 364000 }, IDS_FORM_B4_JIS },
    { L"B5 (JIS)", { 182000, 257000 }, IDS_FORM_B5_JIS },
    { L"Folio", { 215900, 330200 }, IDS_FORM_FOLIO },
    { L"Quarto", { 215000, 275000 }, IDS_FORM_QUARTO },
    { L"10x14", { 254000, 355600 }, IDS_FORM_10x14 },
    { L"11x17", { 279400, 431800 }, IDS_FORM_11x17 },
    { L"Note", { 215900, 279400 }, IDS_FORM_NOTE },
    { L"Envelope #9", { 98425, 225425 }, IDS_FORM_ENVELOPE_9 },
    { L"Envelope #10", { 104775, 241300 }, IDS_FORM_ENVELOPE_10 },
    { L"Envelope #11", { 114300, 263525 }, IDS_FORM_ENVELOPE_11 },
    { L"Envelope #12", { 120650, 279400 }, IDS_FORM_ENVELOPE_12 },
    { L"Envelope #14", { 127000, 292100 }, IDS_FORM_ENVELOPE_14 },
    { L"C size sheet", { 431800, 558800 }, IDS_FORM_C_SIZE_SHEET },
    { L"D size sheet", { 558800, 863600 }, IDS_FORM_D_SIZE_SHEET },
    { L"E size sheet", { 863600, 1117600 }, IDS_FORM_E_SIZE_SHEET },
    { L"Envelope DL", { 110000, 220000 }, IDS_FORM_ENVELOPE_DL },
    { L"Envelope C5", { 162000, 229000 }, IDS_FORM_ENVELOPE_C5 },
    { L"Envelope C3", { 324000, 458000 }, IDS_FORM_ENVELOPE_C3 },
    { L"Envelope C4", { 229000, 324000 }, IDS_FORM_ENVELOPE_C4 },
    { L"Envelope C6", { 114000, 162000 }, IDS_FORM_ENVELOPE_C6 },
    { L"Envelope C65", { 114000, 229000 }, IDS_FORM_ENVELOPE_C65 },
    { L"Envelope B4", { 250000, 353000 }, IDS_FORM_ENVELOPE_B4 },
    { L"Envelope B5", { 176000, 250000 }, IDS_FORM_ENVELOPE_B5 },
    { L"Envelope B6", { 176000, 125000 }, IDS_FORM_ENVELOPE_B6 },
    { L"Envelope", { 110000, 230000 }, IDS_FORM_ENVELOPE },
    { L"Envelope Monarch", { 98425, 190500 }, IDS_FORM_ENVELOPE_MONARCH },
    { L"6 3/4 Envelope", { 92075, 165100 }, IDS_FORM_6_34_ENVELOPE },
    { L"US Std Fanfold", { 377825, 279400 }, IDS_FORM_US_STD_FANFOLD },
    { L"German Std Fanfold", { 215900, 304800 }, IDS_FORM_GERMAN_STD_FANFOLD },
    { L"German Legal Fanfold", { 215900, 330200 }, IDS_FORM_GERMAN_LEGAL_FANFOLD },
    { L"B4 (ISO)", { 250000, 353000 }, IDS_FORM_B4_ISO },
    { L"Japanese Postcard", { 100000, 148000 }, IDS_FORM_JAPANESE_POSTCARD },
    { L"9x11", { 228600, 279400 }, IDS_FORM_9x11 },
    { L"10x11", { 254000, 279400 }, IDS_FORM_10x11 },
    { L"15x11", { 381000, 279400 }, IDS_FORM_15x11 },
    { L"Envelope Invite", { 220000, 220000 }, IDS_FORM_ENVELOPE_INVITE },
    { L"Letter Extra", { 241300, 304800 }, IDS_FORM_LETTER_EXTRA },
    { L"Legal Extra", { 241300, 381000 }, IDS_FORM_LEGAL_EXTRA },
    { L"Tabloid Extra", { 304800, 457200 }, IDS_FORM_TABLOID_EXTRA },
    { L"A4 Extra", { 235458, 322326 }, IDS_FORM_A4_EXTRA },
    { L"Letter Transverse", { 215900, 279400 }, IDS_FORM_LETTER_TRANSVERSE },
    { L"A4 Transverse", { 210000, 297000 }, IDS_FORM_A4_TRANSVERSE },
    { L"Letter Extra Transverse", { 241300, 304800 }, IDS_FORM_LETTER_EXTRA_TRANSVERSE },
    { L"Super A", { 227000, 356000 }, IDS_FORM_SUPER_A },
    { L"Super B", { 305000, 487000 }, IDS_FORM_SUPER_B },
    { L"Letter Plus", { 215900, 322326 }, IDS_FORM_LETTER_PLUS },
    { L"A4 Plus", { 210000, 330000 }, IDS_FORM_A4_PLUS },
    { L"A5 Transverse", { 148000, 210000 }, IDS_FORM_A5_TRANSVERSE },
    { L"B5 (JIS) Transverse", { 182000, 257000 }, IDS_FORM_B5_JIS_TRANSVERSE },
    { L"A3 Extra", { 322000, 445000 }, IDS_FORM_A3_EXTRA },
    { L"A5 Extra", { 174000, 235000 }, IDS_FORM_A5_EXTRA },
    { L"B5 (ISO) Extra", { 201000, 276000 }, IDS_FORM_B5_ISO_EXTRA },
    { L"A2", { 420000, 594000 }, IDS_FORM_A2 },
    { L"A3 Transverse", { 297000, 420000 }, IDS_FORM_A3_TRANSVERSE },
    { L"A3 Extra Transverse", { 322000, 445000 }, IDS_FORM_A3_EXTRA_TRANSVERSE },
    { L"Japanese Double Postcard", { 200000, 148000 }, IDS_FORM_JAPANESE_DOUBLE_POSTCARD },
    { L"A6", { 105000, 148000 }, IDS_FORM_A6 },
    { L"Japanese Envelope Kaku #2", { 240000, 332000 }, IDS_FORM_JAPANESE_ENVELOPE_KAKU_2 },
    { L"Japanese Envelope Kaku #3", { 216000, 277000 }, IDS_FORM_JAPANESE_ENVELOPE_KAKU_3 },
    { L"Japanese Envelope Chou #3", { 120000, 235000 }, IDS_FORM_JAPANESE_ENVELOPE_CHOU_3 },
    { L"Japanese Envelope Chou #4", { 90000, 205000 }, IDS_FORM_JAPANESE_ENVELOPE_CHOU_4 },
    { L"Letter Rotated", { 279400, 215900 }, IDS_FORM_LETTER_ROTATED },
    { L"A3 Rotated", { 420000, 297000 }, IDS_FORM_A3_ROTATED },
    { L"A4 Rotated", { 297000, 210000 }, IDS_FORM_A4_ROTATED },
    { L"A5 Rotated", { 210000, 148000 }, IDS_FORM_A5_ROTATED },
    { L"B4 (JIS) Rotated", { 364000, 257000 }, IDS_FORM_B4_JIS_ROTATED },
    { L"B5 (JIS) Rotated", { 257000, 182000 }, IDS_FORM_B5_JIS_ROTATED },
    { L"Japanese Postcard Rotated", { 148000, 100000 }, IDS_FORM_JAPANESE_POSTCARD_ROTATED },
    { L"Double Japan Postcard Rotated", { 148000, 200000 }, IDS_FORM_DOUBLE_JAPAN_POSTCARD_ROTATED },
    { L"A6 Rotated", { 148000, 105000 }, IDS_FORM_A6_ROTATED },
    { L"Japan Envelope Kaku #2 Rotated", { 332000, 240000 }, IDS_FORM_JAPAN_ENVELOPE_KAKU_2_ROTATED },
    { L"Japan Envelope Kaku #3 Rotated", { 277000, 216000 }, IDS_FORM_JAPAN_ENVELOPE_KAKU_3_ROTATED },
    { L"Japan Envelope Chou #3 Rotated", { 235000, 120000 }, IDS_FORM_JAPAN_ENVELOPE_CHOU_3_ROTATED },
    { L"Japan Envelope Chou #4 Rotated", { 205000, 90000 }, IDS_FORM_JAPAN_ENVELOPE_CHOU_4_ROTATED },
    { L"B6 (JIS)", { 128000, 182000 }, IDS_FORM_B6_JIS },
    { L"B6 (JIS) Rotated", { 182000, 128000 }, IDS_FORM_B6_JIS_ROTATED },
    { L"12x11", { 304932, 279521 }, IDS_FORM_12x11 },
    { L"Japan Envelope You #4", { 105000, 235000 }, IDS_FORM_JAPAN_ENVELOPE_YOU_4 },
    { L"Japan Envelope You #4 Rotated", { 235000, 105000 }, IDS_FORM_JAPAN_ENVELOPE_YOU_4_ROTATED },
    { L"PRC 16K", { 188000, 260000 }, IDS_FORM_PRC_16K },
    { L"PRC 32K", { 130000, 184000 }, IDS_FORM_PRC_32K },
    { L"PRC 32K(Big)", { 140000, 203000 }, IDS_FORM_PRC_32K_BIG },
    { L"PRC Envelope #1", { 102000, 165000 }, IDS_FORM_PRC_ENVELOPE_1 },
    { L"PRC Envelope #2", { 102000, 176000 }, IDS_FORM_PRC_ENVELOPE_2 },
    { L"PRC Envelope #3", { 125000, 176000 }, IDS_FORM_PRC_ENVELOPE_3 },
    { L"PRC Envelope #4", { 110000, 208000 }, IDS_FORM_PRC_ENVELOPE_4 },
    { L"PRC Envelope #5", { 110000, 220000 }, IDS_FORM_PRC_ENVELOPE_5 },
    { L"PRC Envelope #6", { 120000, 230000 }, IDS_FORM_PRC_ENVELOPE_6 },
    { L"PRC Envelope #7", { 160000, 230000 }, IDS_FORM_PRC_ENVELOPE_7 },
    { L"PRC Envelope #8", { 120000, 309000 }, IDS_FORM_PRC_ENVELOPE_8 },
    { L"PRC Envelope #9", { 229000, 324000 }, IDS_FORM_PRC_ENVELOPE_9 },
    { L"PRC Envelope #10", { 324000, 458000 }, IDS_FORM_PRC_ENVELOPE_10 },
    { L"PRC 16K Rotated", { 260000, 188000 }, IDS_FORM_PRC_16K_ROTATED },
    { L"PRC 32K Rotated", { 184000, 130000 }, IDS_FORM_PRC_32K_ROTATED },
    { L"PRC 32K(Big) Rotated", { 203000, 140000 }, IDS_FORM_PRC_32K_BIG_ROTATED },
    { L"PRC Envelope #1 Rotated", { 165000, 102000 }, IDS_FORM_PRC_ENVELOPE_1_ROTATED },
    { L"PRC Envelope #2 Rotated", { 176000, 102000 }, IDS_FORM_PRC_ENVELOPE_2_ROTATED },
    { L"PRC Envelope #3 Rotated", { 176000, 125000 }, IDS_FORM_PRC_ENVELOPE_3_ROTATED },
    { L"PRC Envelope #4 Rotated", { 208000, 110000 }, IDS_FORM_PRC_ENVELOPE_4_ROTATED },
    { L"PRC Envelope #5 Rotated", { 220000, 110000 }, IDS_FORM_PRC_ENVELOPE_5_ROTATED },
    { L"PRC Envelope #6 Rotated", { 230000, 120000 }, IDS_FORM_PRC_ENVELOPE_6_ROTATED },
    { L"PRC Envelope #7 Rotated", { 230000, 160000 }, IDS_FORM_PRC_ENVELOPE_7_ROTATED },
    { L"PRC Envelope #8 Rotated", { 309000, 120000 }, IDS_FORM_PRC_ENVELOPE_8_ROTATED },
    { L"PRC Envelope #9 Rotated", { 324000, 229000 }, IDS_FORM_PRC_ENVELOPE_9_ROTATED },
    { L"PRC Envelope #10 Rotated", { 458000, 324000 }, IDS_FORM_PRC_ENVELOPE_10_ROTATED }
};


/* ############################### */

static CRITICAL_SECTION monitor_handles_cs;
static CRITICAL_SECTION_DEBUG monitor_handles_cs_debug =
{
    0, 0, &monitor_handles_cs,
    { &monitor_handles_cs_debug.ProcessLocksList, &monitor_handles_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": monitor_handles_cs") }
};
static CRITICAL_SECTION monitor_handles_cs = { &monitor_handles_cs_debug, -1, 0, 0, 0, 0 };

/* ############################### */

typedef struct {
    WCHAR   src[MAX_PATH+MAX_PATH];
    WCHAR   dst[MAX_PATH+MAX_PATH];
    DWORD   srclen;
    DWORD   dstlen;
    DWORD   copyflags;
    BOOL    lazy;
} apd_data_t;

typedef struct {
    struct list     entry;
    LPWSTR          name;
    LPWSTR          dllname;
    PMONITORUI      monitorUI;
    MONITOR2        monitor;
    BOOL (WINAPI *old_EnumPorts)(LPWSTR,DWORD,LPBYTE,DWORD,LPDWORD,LPDWORD);
    BOOL (WINAPI *old_OpenPort)(LPWSTR,PHANDLE);
    BOOL (WINAPI *old_OpenPortEx)(LPWSTR,LPWSTR,PHANDLE,struct _MONITOR *);
    BOOL (WINAPI *old_AddPort)(LPWSTR,HWND,LPWSTR);
    BOOL (WINAPI *old_AddPortEx)(LPWSTR,DWORD,LPBYTE,LPWSTR);
    BOOL (WINAPI *old_ConfigurePort)(LPWSTR,HWND,LPWSTR);
    BOOL (WINAPI *old_DeletePort)(LPWSTR,HWND,LPWSTR);
    BOOL (WINAPI *old_XcvOpenPort)(LPCWSTR,ACCESS_MASK,PHANDLE);
    HANDLE          hmon;
    HMODULE         hdll;
    DWORD           refcount;
} monitor_t;

typedef struct {
    LPCWSTR  envname;
    LPCWSTR  subdir;
    DWORD    driverversion;
    LPCWSTR  versionregpath;
    LPCWSTR  versionsubdir;
} printenv_t;

typedef struct {
    LPWSTR name;
    LPWSTR printername;
    monitor_t * pm;
    HANDLE hXcv;
} printer_t;

/* ############################### */

static struct list monitor_handles = LIST_INIT( monitor_handles );
static monitor_t * pm_localport;

static const WCHAR fmt_driversW[] =
    L"System\\CurrentControlSet\\control\\Print\\Environments\\%s\\Drivers%s";
static const WCHAR fmt_printprocessorsW[] =
    L"System\\CurrentControlSet\\Control\\Print\\Environments\\%s\\Print Processors";
static const WCHAR monitorsW[] = L"System\\CurrentControlSet\\Control\\Print\\Monitors\\";
static const WCHAR printersW[] = L"System\\CurrentControlSet\\Control\\Print\\Printers";
static const WCHAR winnt_cv_portsW[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Ports";
static const WCHAR x86_envnameW[] = L"Windows NT x86";


static const printenv_t env_ia64 =  {L"Windows IA64", L"ia64", 3,
                                     L"\\Version-3", L"\\3"};

static const printenv_t env_x86 =   {x86_envnameW, L"w32x86", 3,
                                     L"\\Version-3", L"\\3"};

static const printenv_t env_x64 =   {L"Windows x64", L"x64", 3,
                                     L"\\Version-3", L"\\3"};

static const printenv_t env_arm =   {L"Windows ARM", L"arm", 3,
                                     L"\\Version-3", L"\\3"};

static const printenv_t env_arm64 = {L"Windows ARM64", L"arm64", 3,
                                     L"\\Version-3", L"\\3"};

static const printenv_t env_win40 = {L"Windows 4.0", L"win40", 0,
                                     L"\\Version-0", L"\\0"};

static const printenv_t * const all_printenv[] = {&env_x86, &env_x64, &env_ia64, &env_arm, &env_arm64, &env_win40};

#ifdef __i386__
#define env_arch env_x86
#elif defined __x86_64__
#define env_arch env_x64
#elif defined __arm__
#define env_arch env_arm
#elif defined __aarch64__
#define env_arch env_arm64
#else
#error not defined for this cpu
#endif


static const DWORD di_sizeof[] = {0, sizeof(DRIVER_INFO_1W), sizeof(DRIVER_INFO_2W),
                                     sizeof(DRIVER_INFO_3W), sizeof(DRIVER_INFO_4W),
                                     sizeof(DRIVER_INFO_5W), sizeof(DRIVER_INFO_6W),
                                  0, sizeof(DRIVER_INFO_8W)};


/******************************************************************
 *  apd_copyfile [internal]
 *
 * Copy a file from the driverdirectory to the versioned directory
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL apd_copyfile( WCHAR *pathname, WCHAR *file_part, apd_data_t *apd )
{
    WCHAR *srcname;
    BOOL res;

    apd->src[apd->srclen] = '\0';
    apd->dst[apd->dstlen] = '\0';

    if (!pathname || !pathname[0]) {
        /* nothing to copy */
        return TRUE;
    }

    if (apd->copyflags & APD_COPY_FROM_DIRECTORY)
        srcname = pathname;
    else
    {
        srcname = apd->src;
        lstrcatW( srcname, file_part );
    }
    lstrcatW( apd->dst, file_part );

    TRACE("%s => %s\n", debugstr_w(srcname), debugstr_w(apd->dst));

    /* FIXME: handle APD_COPY_NEW_FILES */
    res = CopyFileW(srcname, apd->dst, FALSE);
    TRACE("got %d with %lu\n", res, GetLastError());

    return apd->lazy || res;
}

/******************************************************************
 * copy_servername_from_name  (internal)
 *
 * for an external server, the serverpart from the name is copied.
 *
 * RETURNS
 *  the length (in WCHAR) of the serverpart (0 for the local computer)
 *  (-length), when the name is too long
 *
 */
static LONG copy_servername_from_name(LPCWSTR name, LPWSTR target)
{
    LPCWSTR server;
    LPWSTR  ptr;
    WCHAR   buffer[MAX_COMPUTERNAME_LENGTH +1];
    DWORD   len;
    DWORD   serverlen;

    if (target) *target = '\0';

    if (name == NULL) return 0;
    if ((name[0] != '\\') || (name[1] != '\\')) return 0;

    server = &name[2];
    /* skip over both backslash, find separator '\' */
    ptr = wcschr(server, '\\');
    serverlen = (ptr) ? ptr - server : wcslen(server);

    /* servername is empty */
    if (serverlen == 0) return 0;

    TRACE("found %s\n", debugstr_wn(server, serverlen));

    if (serverlen > MAX_COMPUTERNAME_LENGTH) return -serverlen;

    if (target) {
        memcpy(target, server, serverlen * sizeof(WCHAR));
        target[serverlen] = '\0';
    }

    len = ARRAY_SIZE(buffer);
    if (GetComputerNameW(buffer, &len)) {
        if ((serverlen == len) && (wcsnicmp(server, buffer, len) == 0)) {
            /* The requested Servername is our computername */
            return 0;
        }
    }
    return serverlen;
}

/******************************************************************
 * get_basename_from_name  (internal)
 *
 * skip over the serverpart from the full name
 *
 */
static LPCWSTR get_basename_from_name(LPCWSTR name)
{
    if (name == NULL)  return NULL;
    if ((name[0] == '\\') && (name[1] == '\\')) {
        /* skip over the servername and search for the following '\'  */
        name = wcschr(&name[2], '\\');
        if ((name) && (name[1])) {
            /* found a separator ('\') followed by a name:
               skip over the separator and return the rest */
            name++;
        }
        else
        {
            /* no basename present (we found only a servername) */
            return NULL;
        }
    }
    return name;
}

/******************************************************************
 * monitor_unload [internal]
 *
 * release a printmonitor and unload it from memory, when needed
 *
 */
static void monitor_unload(monitor_t * pm)
{
    if (pm == NULL) return;
    TRACE("%p (refcount: %ld) %s\n", pm, pm->refcount, debugstr_w(pm->name));

    EnterCriticalSection(&monitor_handles_cs);

    if (pm->refcount) pm->refcount--;

    if (pm->refcount == 0) {
        list_remove(&pm->entry);

        if (pm->monitor.pfnShutdown)
            pm->monitor.pfnShutdown(pm->hmon);

        FreeLibrary(pm->hdll);
        free(pm->name);
        free(pm->dllname);
        free(pm);
    }
    LeaveCriticalSection(&monitor_handles_cs);
}

/******************************************************************
 * monitor_unloadall [internal]
 *
 * release all registered printmonitors and unload them from memory, when needed
 *
 */

static void monitor_unloadall(void)
{
    monitor_t * pm;
    monitor_t * next;

    EnterCriticalSection(&monitor_handles_cs);
    /* iterate through the list, with safety against removal */
    LIST_FOR_EACH_ENTRY_SAFE(pm, next, &monitor_handles, monitor_t, entry)
    {
        /* skip monitorui dlls */
        if (pm->monitor.cbSize) monitor_unload(pm);
    }
    LeaveCriticalSection(&monitor_handles_cs);
}

static LONG WINAPI CreateKey(HANDLE hcKey, LPCWSTR pszSubKey, DWORD dwOptions,
                REGSAM samDesired, PSECURITY_ATTRIBUTES pSecurityAttributes,
                PHANDLE phckResult, PDWORD pdwDisposition, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI OpenKey(HANDLE hcKey, LPCWSTR pszSubKey, REGSAM samDesired,
                PHANDLE phkResult, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI CloseKey(HANDLE hcKey, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI DeleteKey(HANDLE hcKey, LPCWSTR pszSubKey, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI EnumKey(HANDLE hcKey, DWORD dwIndex, LPWSTR pszName,
                PDWORD pcchName, PFILETIME pftLastWriteTime, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI QueryInfoKey(HANDLE hcKey, PDWORD pcSubKeys, PDWORD pcbKey,
                PDWORD pcValues, PDWORD pcbValue, PDWORD pcbData,
                PDWORD pcbSecurityDescriptor, PFILETIME pftLastWriteTime,
                HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI SetValue(HANDLE hcKey, LPCWSTR pszValue, DWORD dwType,
                const BYTE* pData, DWORD cbData, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI DeleteValue(HANDLE hcKey, LPCWSTR pszValue, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI EnumValue(HANDLE hcKey, DWORD dwIndex, LPWSTR pszValue,
                PDWORD pcbValue, PDWORD pType, PBYTE pData, PDWORD pcbData,
                HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static LONG WINAPI QueryValue(HANDLE hcKey, LPCWSTR pszValue, PDWORD pType,
                PBYTE pData, PDWORD pcbData, HANDLE hSpooler)
{
    FIXME("stub\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

static MONITORREG monreg =
{
    sizeof(MONITORREG),
    CreateKey,
    OpenKey,
    CloseKey,
    DeleteKey,
    EnumKey,
    QueryInfoKey,
    SetValue,
    DeleteValue,
    EnumValue,
    QueryValue
};

/******************************************************************
 * monitor_load [internal]
 *
 * load a printmonitor, get the dllname from the registry, when needed
 * initialize the monitor and dump found function-pointers
 *
 * On failure, SetLastError() is called and NULL is returned
 */

static monitor_t * monitor_load(LPCWSTR name, LPWSTR dllname)
{
    LPMONITOR2  (WINAPI *pInitializePrintMonitor2) (PMONITORINIT, LPHANDLE);
    PMONITORUI  (WINAPI *pInitializePrintMonitorUI)(VOID);
    LPMONITOREX (WINAPI *pInitializePrintMonitor)  (LPWSTR);
    DWORD (WINAPI *pInitializeMonitorEx)(LPWSTR, LPMONITOR);
    DWORD (WINAPI *pInitializeMonitor)  (LPWSTR);

    monitor_t * pm = NULL;
    monitor_t * cursor;
    LPWSTR  regroot = NULL;
    LPWSTR  driver = dllname;
    HKEY    hroot = 0;

    TRACE("(%s, %s)\n", debugstr_w(name), debugstr_w(dllname));
    /* Is the Monitor already loaded? */
    EnterCriticalSection(&monitor_handles_cs);

    if (name) {
        LIST_FOR_EACH_ENTRY(cursor, &monitor_handles, monitor_t, entry)
        {
            if (cursor->name && (lstrcmpW(name, cursor->name) == 0)) {
                pm = cursor;
                break;
            }
        }
    }

    if (pm == NULL) {
        pm = calloc(1, sizeof(monitor_t));
        if (pm == NULL) goto cleanup;
        list_add_tail(&monitor_handles, &pm->entry);
    }
    pm->refcount++;

    if (pm->name == NULL) {
        /* Load the monitor */
        DWORD   len;

        if (name) {
            len = wcslen(monitorsW) + wcslen(name) + 2;
            regroot = malloc(len * sizeof(WCHAR));
        }

        if (regroot) {
            lstrcpyW(regroot, monitorsW);
            lstrcatW(regroot, name);
            if (RegOpenKeyW(HKEY_LOCAL_MACHINE, regroot, &hroot) == ERROR_SUCCESS) {
                /* Get the Driver from the Registry */
                if (driver == NULL) {
                    DWORD   namesize;
                    if (RegQueryValueExW(hroot, L"Driver", NULL, NULL, NULL,
                                        &namesize) == ERROR_SUCCESS) {
                        driver = malloc(namesize);
                        RegQueryValueExW(hroot, L"Driver", NULL, NULL, (BYTE*)driver, &namesize);
                    }
                }
            }
            else
                WARN("%s not found\n", debugstr_w(regroot));
        }

        pm->name = wcsdup(name);
        pm->dllname = wcsdup(driver);

        if ((name && (!regroot || !pm->name)) || !pm->dllname) {
            monitor_unload(pm);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            pm = NULL;
            goto cleanup;
        }

        pm->hdll = LoadLibraryW(driver);
        TRACE("%p: LoadLibrary(%s) => %ld\n", pm->hdll, debugstr_w(driver), GetLastError());

        if (pm->hdll == NULL) {
            monitor_unload(pm);
            SetLastError(ERROR_MOD_NOT_FOUND);
            pm = NULL;
            goto cleanup;
        }

        pInitializePrintMonitor2  = (void *)GetProcAddress(pm->hdll, "InitializePrintMonitor2");
        pInitializePrintMonitorUI = (void *)GetProcAddress(pm->hdll, "InitializePrintMonitorUI");
        pInitializePrintMonitor   = (void *)GetProcAddress(pm->hdll, "InitializePrintMonitor");
        pInitializeMonitorEx = (void *)GetProcAddress(pm->hdll, "InitializeMonitorEx");
        pInitializeMonitor   = (void *)GetProcAddress(pm->hdll, "InitializeMonitor");


        TRACE("%p: %s,pInitializePrintMonitor2\n", pInitializePrintMonitor2, debugstr_w(driver));
        TRACE("%p: %s,pInitializePrintMonitorUI\n", pInitializePrintMonitorUI, debugstr_w(driver));
        TRACE("%p: %s,pInitializePrintMonitor\n", pInitializePrintMonitor, debugstr_w(driver));
        TRACE("%p: %s,pInitializeMonitorEx\n", pInitializeMonitorEx, debugstr_w(driver));
        TRACE("%p: %s,pInitializeMonitor\n", pInitializeMonitor, debugstr_w(driver));

        if (pInitializePrintMonitorUI  != NULL) {
            pm->monitorUI = pInitializePrintMonitorUI();
            TRACE("%p: MONITORUI from %s,InitializePrintMonitorUI()\n", pm->monitorUI, debugstr_w(driver));
            if (pm->monitorUI) {
                TRACE("0x%08lx: dwMonitorSize (%ld)\n",
                        pm->monitorUI->dwMonitorUISize, pm->monitorUI->dwMonitorUISize);

            }
        }

        if (pInitializePrintMonitor2 && hroot) {
            MONITORINIT init;
            MONITOR2 *monitor2;
            HANDLE hmon;

            memset(&init, 0, sizeof(init));
            init.cbSize = sizeof(init);
            init.hckRegistryRoot = hroot;
            init.pMonitorReg = &monreg;
            init.bLocal = TRUE;

            monitor2 = pInitializePrintMonitor2(&init, &hmon);
            TRACE("%p: MONITOR2 from %s,InitializePrintMonitor2(%s)\n",
                    monitor2, debugstr_w(driver), debugstr_w(regroot));
            if (monitor2)
            {
                memcpy(&pm->monitor, monitor2, min(monitor2->cbSize, sizeof(pm->monitor)));
                pm->hmon = hmon;
            }
        }
        else if (pInitializePrintMonitor && regroot) {
            MONITOREX *pmonitorEx;

            pmonitorEx = pInitializePrintMonitor(regroot);
            TRACE("%p: LPMONITOREX from %s,InitializePrintMonitor(%s)\n",
                    pmonitorEx, debugstr_w(driver), debugstr_w(regroot));
            if (pmonitorEx)
            {
                /* Layout of MONITOREX and MONITOR2 mostly matches */
                memcpy(&pm->monitor, pmonitorEx, min(pmonitorEx->dwMonitorSize + sizeof(void *), sizeof(pm->monitor)));
                /* MONITOREX.dwMonitorSize doesn't include the size field, while MONITOR2.cbSize does */
                pm->monitor.cbSize += sizeof(void *);

                pm->old_EnumPorts = pmonitorEx->Monitor.pfnEnumPorts;
                pm->old_OpenPort = pmonitorEx->Monitor.pfnOpenPort;
                pm->old_OpenPortEx = pmonitorEx->Monitor.pfnOpenPortEx;
                pm->old_AddPort = pmonitorEx->Monitor.pfnAddPort;
                pm->old_AddPortEx = pmonitorEx->Monitor.pfnAddPortEx;
                pm->old_ConfigurePort = pmonitorEx->Monitor.pfnConfigurePort;
                pm->old_DeletePort = pmonitorEx->Monitor.pfnDeletePort;
                pm->old_XcvOpenPort = pmonitorEx->Monitor.pfnXcvOpenPort;

                pm->monitor.pfnEnumPorts = NULL;
                pm->monitor.pfnOpenPort = NULL;
                pm->monitor.pfnOpenPortEx = NULL;
                pm->monitor.pfnAddPort = NULL;
                pm->monitor.pfnAddPortEx = NULL;
                pm->monitor.pfnConfigurePort = NULL;
                pm->monitor.pfnDeletePort = NULL;
                pm->monitor.pfnXcvOpenPort = NULL;
            }
        }

        if (!pm->monitor.cbSize && regroot) {
            if (pInitializeMonitorEx != NULL) {
                FIXME("%s,InitializeMonitorEx not implemented\n", debugstr_w(driver));
            }
            if (pInitializeMonitor != NULL) {
                FIXME("%s,InitializeMonitor not implemented\n", debugstr_w(driver));
            }
        }
        if (!pm->monitor.cbSize && !pm->monitorUI) {
            monitor_unload(pm);
            SetLastError(ERROR_PROC_NOT_FOUND);
            pm = NULL;
        }
    }
cleanup:
    if ((pm_localport ==  NULL) && (pm != NULL) && (lstrcmpW(pm->name, L"Local Port") == 0)) {
        pm->refcount++;
        pm_localport = pm;
    }
    LeaveCriticalSection(&monitor_handles_cs);
    if (driver != dllname) free(driver);
    if (hroot) RegCloseKey(hroot);
    free(regroot);
    TRACE("=> %p\n", pm);
    return pm;
}

/******************************************************************
 * monitor_loadall [internal]
 *
 * Load all registered monitors
 *
 */
static DWORD monitor_loadall(void)
{
    monitor_t * pm;
    DWORD   registered = 0;
    DWORD   loaded = 0;
    HKEY    hmonitors;
    WCHAR   buffer[MAX_PATH];
    DWORD   id = 0;

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hmonitors) == ERROR_SUCCESS) {
        RegQueryInfoKeyW(hmonitors, NULL, NULL, NULL, &registered, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL);

        TRACE("%ld monitors registered\n", registered);

        while (id < registered) {
            buffer[0] = '\0';
            RegEnumKeyW(hmonitors, id, buffer, MAX_PATH);
            pm = monitor_load(buffer, NULL);
            if (pm) loaded++;
            id++;
        }
        RegCloseKey(hmonitors);
    }
    TRACE("%ld monitors loaded\n", loaded);
    return loaded;
}

/******************************************************************
 * monitor_loadui [internal]
 *
 * load the userinterface-dll for a given portmonitor
 *
 * On failure, NULL is returned
 */
static monitor_t * monitor_loadui(monitor_t * pm)
{
    monitor_t * pui = NULL;
    WCHAR   buffer[MAX_PATH];
    HANDLE  hXcv;
    DWORD   len;
    DWORD   res = 0;

    if (pm == NULL) return NULL;
    TRACE("(%p) => dllname: %s\n", pm, debugstr_w(pm->dllname));

    /* Try the Portmonitor first; works for many monitors */
    if (pm->monitorUI) {
        EnterCriticalSection(&monitor_handles_cs);
        pm->refcount++;
        LeaveCriticalSection(&monitor_handles_cs);
        return pm;
    }

    /* query the userinterface-dllname from the Portmonitor */
    /* building (",XcvMonitor %s",pm->name) not needed yet */
    if (pm->monitor.pfnXcvOpenPort)
        res = pm->monitor.pfnXcvOpenPort(pm->hmon, L"", SERVER_ACCESS_ADMINISTER, &hXcv);
    else if (pm->old_XcvOpenPort)
        res = pm->old_XcvOpenPort(L"", SERVER_ACCESS_ADMINISTER, &hXcv);
    TRACE("got %lu with %p\n", res, hXcv);
    if (res) {
        res = pm->monitor.pfnXcvDataPort(hXcv, L"MonitorUI", NULL, 0, (BYTE *) buffer, sizeof(buffer), &len);
        TRACE("got %lu with %s\n", res, debugstr_w(buffer));
        if (res == ERROR_SUCCESS) pui = monitor_load(NULL, buffer);
        pm->monitor.pfnXcvClosePort(hXcv);
    }
    return pui;
}

/******************************************************************
 * monitor_load_by_port [internal]
 *
 * load a printmonitor for a given port
 *
 * On failure, NULL is returned
 */

static monitor_t * monitor_load_by_port(LPCWSTR portname)
{
    HKEY    hroot;
    HKEY    hport;
    LPWSTR  buffer;
    monitor_t * pm = NULL;
    DWORD   registered = 0;
    DWORD   id = 0;
    DWORD   len;

    TRACE("(%s)\n", debugstr_w(portname));

    /* Try the Local Monitor first */
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, winnt_cv_portsW, &hroot) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hroot, portname, NULL, NULL, NULL, &len) == ERROR_SUCCESS) {
            /* found the portname */
            RegCloseKey(hroot);
            return monitor_load(L"Local Port", NULL);
        }
        RegCloseKey(hroot);
    }

    len = MAX_PATH + wcslen(L"\\Ports\\") + wcslen(portname) + 1;
    buffer = malloc(len * sizeof(WCHAR));
    if (buffer == NULL) return NULL;

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hroot) == ERROR_SUCCESS) {
        EnterCriticalSection(&monitor_handles_cs);
        RegQueryInfoKeyW(hroot, NULL, NULL, NULL, &registered, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        while ((pm == NULL) && (id < registered)) {
            buffer[0] = '\0';
            RegEnumKeyW(hroot, id, buffer, MAX_PATH);
            TRACE("testing %s\n", debugstr_w(buffer));
            len = wcslen(buffer);
            lstrcatW(buffer, L"\\Ports\\");
            lstrcatW(buffer, portname);
            if (RegOpenKeyW(hroot, buffer, &hport) == ERROR_SUCCESS) {
                RegCloseKey(hport);
                buffer[len] = '\0';             /* use only the Monitor-Name */
                pm = monitor_load(buffer, NULL);
            }
            id++;
        }
        LeaveCriticalSection(&monitor_handles_cs);
        RegCloseKey(hroot);
    }
    free(buffer);
    return pm;
}

/******************************************************************
 * Return the number of bytes for an multi_sz string.
 * The result includes all \0s
 * (specifically the extra \0, that is needed as multi_sz terminator).
 */
static int multi_sz_lenW(const WCHAR *str)
{
    const WCHAR *ptr = str;
    if (!str) return 0;
    do
    {
        ptr += wcslen(ptr) + 1;
    } while (*ptr);

    return (ptr - str + 1) * sizeof(WCHAR);
}

/******************************************************************
 * validate_envW [internal]
 *
 * validate the user-supplied printing-environment
 *
 * PARAMS
 *  env  [I] PTR to Environment-String or NULL
 *
 * RETURNS
 *  Success:  PTR to printenv_t
 *  Failure:  NULL and ERROR_INVALID_ENVIRONMENT
 *
 * NOTES
 *  An empty string is handled the same way as NULL.
 *
 */

static const  printenv_t * validate_envW(LPCWSTR env)
{
    const printenv_t *result = NULL;
    unsigned int i;

    TRACE("(%s)\n", debugstr_w(env));
    if (env && env[0])
    {
        for (i = 0; i < ARRAY_SIZE(all_printenv); i++)
        {
            if (lstrcmpiW(env, all_printenv[i]->envname) == 0)
            {
                result = all_printenv[i];
                break;
            }
        }
        if (result == NULL) {
            FIXME("unsupported Environment: %s\n", debugstr_w(env));
            SetLastError(ERROR_INVALID_ENVIRONMENT);
        }
        /* on win9x, only "Windows 4.0" is allowed, but we ignore this */
    }
    else
    {
        result = (GetVersion() & 0x80000000) ? &env_win40 : &env_arch;
    }

    TRACE("=> using %p: %s\n", result, debugstr_w(result ? result->envname : NULL));
    return result;
}

/*****************************************************************************
 * enumerate the local monitors (INTERNAL)
 *
 * returns the needed size (in bytes) for pMonitors
 * and  *lpreturned is set to number of entries returned in pMonitors
 *
 * Language-Monitors are also installed in the same Registry-Location but
 * they are filtered in Windows (not returned by EnumMonitors).
 * We do no filtering to simplify our Code.
 *
 */
static DWORD get_local_monitors(DWORD level, LPBYTE pMonitors, DWORD cbBuf, LPDWORD lpreturned)
{
    HKEY    hroot = NULL;
    HKEY    hentry = NULL;
    LPWSTR  ptr;
    LPMONITOR_INFO_2W mi;
    WCHAR   buffer[MAX_PATH];
    WCHAR   dllname[MAX_PATH];
    DWORD   dllsize;
    DWORD   len;
    DWORD   index = 0;
    DWORD   needed = 0;
    DWORD   numentries;
    DWORD   entrysize;

    entrysize = (level == 1) ? sizeof(MONITOR_INFO_1W) : sizeof(MONITOR_INFO_2W);

    numentries = *lpreturned;       /* this is 0, when we scan the registry */
    len = entrysize * numentries;
    ptr = (LPWSTR) &pMonitors[len];

    numentries = 0;
    len = ARRAY_SIZE(buffer);
    buffer[0] = '\0';

    /* Windows creates the "Monitors"-Key on reboot / start "spooler" */
    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hroot) == ERROR_SUCCESS) {
        /* Scan all Monitor-Registry-Keys */
        while (RegEnumKeyExW(hroot, index, buffer, &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            TRACE("Monitor_%ld: %s\n", numentries, debugstr_w(buffer));
            dllsize = sizeof(dllname);
            dllname[0] = '\0';

            /* The Monitor must have a Driver-DLL */
            if (RegOpenKeyExW(hroot, buffer, 0, KEY_READ, &hentry) == ERROR_SUCCESS) {
                if (RegQueryValueExW(hentry, L"Driver", NULL, NULL, (BYTE*)dllname, &dllsize) == ERROR_SUCCESS) {
                    /* We found a valid DLL for this Monitor. */
                    TRACE("using Driver: %s\n", debugstr_w(dllname));
                }
                RegCloseKey(hentry);
            }

            /* Windows returns only Port-Monitors here, but to simplify our code,
               we do no filtering for Language-Monitors */
            if (dllname[0]) {
                numentries++;
                needed += entrysize;
                needed += (len+1) * sizeof(WCHAR);  /* len is wcslen(monitorname) */
                if (level > 1) {
                    /* we install and return only monitors for "Windows NT x86" */
                    needed += (wcslen(x86_envnameW) +1) * sizeof(WCHAR);
                    needed += dllsize;
                }

                /* required size is calculated. Now fill the user-buffer */
                if (pMonitors && (cbBuf >= needed)){
                    mi = (LPMONITOR_INFO_2W) pMonitors;
                    pMonitors += entrysize;

                    TRACE("%p: writing MONITOR_INFO_%ldW #%ld\n", mi, level, numentries);
                    mi->pName = ptr;
                    lstrcpyW(ptr, buffer);      /* Name of the Monitor */
                    ptr += (len+1);               /* len is wcslen(monitorname) */
                    if (level > 1) {
                        mi->pEnvironment = ptr;
                        lstrcpyW(ptr, x86_envnameW); /* fixed to "Windows NT x86" */
                        ptr += (wcslen(x86_envnameW)+1);

                        mi->pDLLName = ptr;
                        lstrcpyW(ptr, dllname);         /* Name of the Driver-DLL */
                        ptr += (dllsize / sizeof(WCHAR));
                    }
                }
            }
            index++;
            len = ARRAY_SIZE(buffer);
            buffer[0] = '\0';
        }
        RegCloseKey(hroot);
    }
    *lpreturned = numentries;
    TRACE("need %ld byte for %ld entries\n", needed, numentries);
    return needed;
}

/*****************************************************************************
 * enumerate the local print processors (INTERNAL)
 *
 * returns the needed size (in bytes) for pPPInfo
 * and  *lpreturned is set to number of entries returned in pPPInfo
 *
 */
static DWORD get_local_printprocessors(LPWSTR regpathW, LPBYTE pPPInfo, DWORD cbBuf, LPDWORD lpreturned)
{
    HKEY    hroot = NULL;
    HKEY    hentry = NULL;
    LPWSTR  ptr;
    PPRINTPROCESSOR_INFO_1W ppi;
    WCHAR   buffer[MAX_PATH];
    WCHAR   dllname[MAX_PATH];
    DWORD   dllsize;
    DWORD   len;
    DWORD   index = 0;
    DWORD   needed = 0;
    DWORD   numentries;

    numentries = *lpreturned;       /* this is 0, when we scan the registry */
    len = numentries * sizeof(PRINTPROCESSOR_INFO_1W);
    ptr = (LPWSTR) &pPPInfo[len];

    numentries = 0;
    len = ARRAY_SIZE(buffer);
    buffer[0] = '\0';

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, regpathW, &hroot) == ERROR_SUCCESS) {
        /* add "winprint" first */
        numentries++;
        needed = sizeof(PRINTPROCESSOR_INFO_1W) + sizeof(L"winprint");
        if (pPPInfo && (cbBuf >= needed)){
            ppi = (PPRINTPROCESSOR_INFO_1W) pPPInfo;
            pPPInfo += sizeof(PRINTPROCESSOR_INFO_1W);

            TRACE("%p: writing PRINTPROCESSOR_INFO_1W #%ld\n", ppi, numentries);
            ppi->pName = ptr;
            lstrcpyW(ptr, L"winprint");      /* Name of the Print Processor */
            ptr += ARRAY_SIZE(L"winprint");
        }

        /* Scan all Printprocessor Keys */
        while ((RegEnumKeyExW(hroot, index, buffer, &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) &&
            (lstrcmpiW(buffer, L"winprint") != 0)) {
            TRACE("PrintProcessor_%ld: %s\n", numentries, debugstr_w(buffer));
            dllsize = sizeof(dllname);
            dllname[0] = '\0';

            /* The Print Processor must have a Driver-DLL */
            if (RegOpenKeyExW(hroot, buffer, 0, KEY_READ, &hentry) == ERROR_SUCCESS) {
                if (RegQueryValueExW(hentry, L"Driver", NULL, NULL, (BYTE*)dllname, &dllsize) == ERROR_SUCCESS) {
                    /* We found a valid DLL for this Print Processor */
                    TRACE("using Driver: %s\n", debugstr_w(dllname));
                }
                RegCloseKey(hentry);
            }

            if (dllname[0]) {
                numentries++;
                needed += sizeof(PRINTPROCESSOR_INFO_1W);
                needed += (len+1) * sizeof(WCHAR);  /* len is wcslen(printprocessor name) */

                /* required size is calculated. Now fill the user-buffer */
                if (pPPInfo && (cbBuf >= needed)){
                    ppi = (PPRINTPROCESSOR_INFO_1W) pPPInfo;
                    pPPInfo += sizeof(PRINTPROCESSOR_INFO_1W);

                    TRACE("%p: writing PRINTPROCESSOR_INFO_1W #%ld\n", ppi, numentries);
                    ppi->pName = ptr;
                    lstrcpyW(ptr, buffer);      /* Name of the Print Processor */
                    ptr += (len+1);             /* len is wcslen(printprosessor name) */
                }
            }
            index++;
            len = ARRAY_SIZE(buffer);
            buffer[0] = '\0';
        }
        RegCloseKey(hroot);
    }
    *lpreturned = numentries;
    TRACE("need %ld byte for %ld entries\n", needed, numentries);
    return needed;
}

static BOOL wrap_EnumPorts(monitor_t *pm, LPWSTR name, DWORD level, LPBYTE buffer,
                           DWORD size, LPDWORD needed, LPDWORD returned)
{
    if (pm->monitor.pfnEnumPorts)
        return pm->monitor.pfnEnumPorts(pm->hmon, name, level, buffer, size, needed, returned);

    if (pm->old_EnumPorts)
        return pm->old_EnumPorts(name, level, buffer, size, needed, returned);

    WARN("EnumPorts is not implemented by monitor\n");
    return FALSE;
}

/******************************************************************
 * enumerate the local Ports from all loaded monitors (internal)
 *
 * returns the needed size (in bytes) for pPorts
 * and  *lpreturned is set to number of entries returned in pPorts
 *
 */
static DWORD get_ports_from_all_monitors(DWORD level, LPBYTE pPorts, DWORD cbBuf, LPDWORD lpreturned)
{
    monitor_t * pm;
    LPWSTR      ptr;
    LPPORT_INFO_2W cache;
    LPPORT_INFO_2W out;
    LPBYTE  pi_buffer = NULL;
    DWORD   pi_allocated = 0;
    DWORD   pi_needed;
    DWORD   pi_index;
    DWORD   pi_returned;
    DWORD   res;
    DWORD   outindex = 0;
    DWORD   needed;
    DWORD   numentries;
    DWORD   entrysize;

    TRACE("(%ld, %p, %ld, %p)\n", level, pPorts, cbBuf, lpreturned);
    entrysize = (level == 1) ? sizeof(PORT_INFO_1W) : sizeof(PORT_INFO_2W);

    numentries = *lpreturned;       /* this is 0, when we scan the registry */
    needed = entrysize * numentries;
    ptr = (LPWSTR) &pPorts[needed];

    numentries = 0;
    needed = 0;

    LIST_FOR_EACH_ENTRY(pm, &monitor_handles, monitor_t, entry)
    {
        if (pm->monitor.pfnEnumPorts || pm->old_EnumPorts) {
            pi_needed = 0;
            pi_returned = 0;
            res = wrap_EnumPorts(pm, NULL, level, pi_buffer, pi_allocated, &pi_needed, &pi_returned);
            if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                /* Do not use realloc (we do not need the old data in the buffer) */
                free(pi_buffer);
                pi_buffer = malloc(pi_needed);
                pi_allocated = (pi_buffer) ? pi_needed : 0;
                res = wrap_EnumPorts(pm, NULL, level, pi_buffer, pi_allocated, &pi_needed, &pi_returned);
            }
            TRACE("(%s) got %ld with %ld (need %ld byte for %ld entries)\n",
                  debugstr_w(pm->name), res, GetLastError(), pi_needed, pi_returned);

            numentries += pi_returned;
            needed += pi_needed;

            /* fill the output-buffer (pPorts), if we have one */
            if (pPorts && (cbBuf >= needed ) && pi_buffer) {
                pi_index = 0;
                while (pi_returned > pi_index) {
                    cache = (LPPORT_INFO_2W) &pi_buffer[pi_index * entrysize];
                    out = (LPPORT_INFO_2W) &pPorts[outindex * entrysize];
                    out->pPortName = ptr;
                    lstrcpyW(ptr, cache->pPortName);
                    ptr += (wcslen(ptr)+1);
                    if (level > 1) {
                        out->pMonitorName = ptr;
                        lstrcpyW(ptr,  cache->pMonitorName);
                        ptr += (wcslen(ptr)+1);

                        out->pDescription = ptr;
                        lstrcpyW(ptr,  cache->pDescription);
                        ptr += (wcslen(ptr)+1);
                        out->fPortType = cache->fPortType;
                        out->Reserved = cache->Reserved;
                    }
                    pi_index++;
                    outindex++;
                }
            }
        }
    }
    /* the temporary portinfo-buffer is no longer needed */
    free(pi_buffer);

    *lpreturned = numentries;
    TRACE("need %ld byte for %ld entries\n", needed, numentries);
    return needed;
}


/*****************************************************************************
 * open_driver_reg [internal]
 *
 * opens the registry for the printer drivers depending on the given input
 * variable pEnvironment
 *
 * RETURNS:
 *    Success: the opened hkey
 *    Failure: NULL
 */
static HKEY open_driver_reg(LPCWSTR pEnvironment)
{
    HKEY  retval = NULL;
    LPWSTR buffer;
    const printenv_t * env;

    TRACE("(%s)\n", debugstr_w(pEnvironment));

    env = validate_envW(pEnvironment);
    if (!env) return NULL;

    buffer = malloc(sizeof(fmt_driversW) +
            (wcslen(env->envname) + wcslen(env->versionregpath)) * sizeof(WCHAR));

    if (buffer) {
        wsprintfW(buffer, fmt_driversW, env->envname, env->versionregpath);
        RegCreateKeyW(HKEY_LOCAL_MACHINE, buffer, &retval);
        free(buffer);
    }
    return retval;
}

/*****************************************************************************
 * fpGetPrinterDriverDirectory [exported through PRINTPROVIDOR]
 *
 * Return the PATH for the Printer-Drivers
 *
 * PARAMS
 *   pName            [I] Servername (NT only) or NULL (local Computer)
 *   pEnvironment     [I] Printing-Environment (see below) or NULL (Default)
 *   Level            [I] Structure-Level (must be 1)
 *   pDriverDirectory [O] PTR to Buffer that receives the Result
 *   cbBuf            [I] Size of Buffer at pDriverDirectory
 *   pcbNeeded        [O] PTR to DWORD that receives the size in Bytes used /
 *                        required for pDriverDirectory
 *
 * RETURNS
 *   Success: TRUE  and in pcbNeeded the Bytes used in pDriverDirectory
 *   Failure: FALSE and in pcbNeeded the Bytes required for pDriverDirectory,
 *            if cbBuf is too small
 *
 *   Native Values returned in pDriverDirectory on Success:
 *|  NT(Windows NT x86):  "%winsysdir%\\spool\\DRIVERS\\w32x86"
 *|  NT(Windows 4.0):     "%winsysdir%\\spool\\DRIVERS\\win40"
 *|  win9x(Windows 4.0):  "%winsysdir%"
 *
 *   "%winsysdir%" is the Value from GetSystemDirectoryW()
 *
 */
static BOOL WINAPI fpGetPrinterDriverDirectory(LPWSTR pName, LPWSTR pEnvironment,
            DWORD Level, LPBYTE pDriverDirectory, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD needed;
    const printenv_t * env;
    WCHAR * const dir = (WCHAR *)pDriverDirectory;

    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", debugstr_w(pName),
          debugstr_w(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);

    if (pName != NULL && pName[0]) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    env = validate_envW(pEnvironment);
    if (!env) return FALSE;  /* pEnvironment invalid or unsupported */


    /* GetSystemDirectoryW returns number of WCHAR including the '\0' */
    needed = GetSystemDirectoryW(NULL, 0);
    /* add the Size for the Subdirectories */
    needed += wcslen(L"\\spool");
    needed += wcslen(L"\\drivers\\");
    needed += wcslen(env->subdir);
    needed *= sizeof(WCHAR);  /* return-value is size in Bytes */

    *pcbNeeded = needed;

    if (needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (dir == NULL) {
        /* ERROR_INVALID_USER_BUFFER is NT, ERROR_INVALID_PARAMETER is win9x */
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    GetSystemDirectoryW( dir, cbBuf / sizeof(WCHAR) );
    /* add the Subdirectories */
    lstrcatW( dir, L"\\spool" );
    CreateDirectoryW( dir, NULL );
    lstrcatW( dir, L"\\drivers\\" );
    CreateDirectoryW( dir, NULL );
    lstrcatW( dir, env->subdir );
    CreateDirectoryW( dir, NULL );

    TRACE( "=> %s\n", debugstr_w( dir ) );
    return TRUE;
}

/******************************************************************
 * driver_load [internal]
 *
 * load a driver user interface dll
 *
 * On failure, NULL is returned
 *
 */

static HMODULE driver_load(const printenv_t * env, LPWSTR dllname)
{
    WCHAR fullname[MAX_PATH];
    HMODULE hui;
    DWORD len;

    TRACE("(%p, %s)\n", env, debugstr_w(dllname));

    /* build the driverdir */
    len = sizeof(fullname) -
          (wcslen(env->versionsubdir) + 1 + wcslen(dllname) + 1) * sizeof(WCHAR);

    if (!fpGetPrinterDriverDirectory(NULL, (LPWSTR) env->envname, 1,
                                     (LPBYTE) fullname, len, &len)) {
        /* Should never fail */
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return NULL;
    }

    lstrcatW(fullname, env->versionsubdir);
    lstrcatW(fullname, L"\\");
    lstrcatW(fullname, dllname);

    hui = LoadLibraryW(fullname);
    TRACE("%p: LoadLibrary(%s) %ld\n", hui, debugstr_w(fullname), GetLastError());

    return hui;
}

/******************************************************************
 *  printer_free
 *  free the data pointer of an opened printer
 */
static VOID printer_free(printer_t * printer)
{
    if (printer->hXcv)
    {
        if (printer->pm->monitor.pfnXcvClosePort)
            printer->pm->monitor.pfnXcvClosePort(printer->hXcv);
    }

    monitor_unload(printer->pm);

    free(printer->printername);
    free(printer->name);
    free(printer);
}

/******************************************************************
 *  printer_alloc_handle
 *  alloc a printer handle and remember the data pointer in the printer handle table
 *
 */
static HANDLE printer_alloc_handle(LPCWSTR name, LPPRINTER_DEFAULTSW pDefault)
{
    WCHAR servername[MAX_COMPUTERNAME_LENGTH + 1];
    printer_t *printer = NULL;
    LPCWSTR printername;
    HKEY    hkeyPrinters;
    HKEY    hkeyPrinter;
    DWORD   len;

    if (copy_servername_from_name(name, servername)) {
        FIXME("server %s not supported\n", debugstr_w(servername));
        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return NULL;
    }

    printername = get_basename_from_name(name);
    if (name != printername) TRACE("converted %s to %s\n", debugstr_w(name), debugstr_w(printername));

    /* an empty printername is invalid */
    if (printername && (!printername[0])) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    printer = calloc(1, sizeof(printer_t));
    if (!printer) goto end;

    /* clone the base name. This is NULL for the printserver */
    printer->printername = wcsdup(printername);

    /* clone the full name */
    printer->name = wcsdup(name);
    if (name && (!printer->name)) {
        printer_free(printer);
        printer = NULL;
    }
    if (printername) {
        len = ARRAY_SIZE(L",XcvMonitor ") - 1;
        if (wcsncmp(printername, L",XcvMonitor ", len) == 0) {
            /* OpenPrinter(",XcvMonitor ", ...) detected */
            TRACE(",XcvMonitor: %s\n", debugstr_w(&printername[len]));
            printer->pm = monitor_load(&printername[len], NULL);
            if (printer->pm == NULL) {
                printer_free(printer);
                SetLastError(ERROR_UNKNOWN_PORT);
                printer = NULL;
                goto end;
            }
        }
        else
        {
            len = ARRAY_SIZE(L",XcvPort ") - 1;
            if (wcsncmp( printername, L",XcvPort ", len) == 0) {
                /* OpenPrinter(",XcvPort ", ...) detected */
                TRACE(",XcvPort: %s\n", debugstr_w(&printername[len]));
                printer->pm = monitor_load_by_port(&printername[len]);
                if (printer->pm == NULL) {
                    printer_free(printer);
                    SetLastError(ERROR_UNKNOWN_PORT);
                    printer = NULL;
                    goto end;
                }
            }
        }

        if (printer->pm) {
            if (printer->pm->monitor.pfnXcvOpenPort)
                printer->pm->monitor.pfnXcvOpenPort(printer->pm->hmon, &printername[len],
                                                   pDefault ? pDefault->DesiredAccess : 0,
                                                   &printer->hXcv);
            else if (printer->pm->old_XcvOpenPort)
                printer->pm->old_XcvOpenPort(&printername[len],
                                                   pDefault ? pDefault->DesiredAccess : 0,
                                                   &printer->hXcv);
            if (printer->hXcv == NULL) {
                printer_free(printer);
                SetLastError(ERROR_INVALID_PARAMETER);
                printer = NULL;
                goto end;
            }
        }
        else
        {
            /* Does the Printer exist? */
            if (RegCreateKeyW(HKEY_LOCAL_MACHINE, printersW, &hkeyPrinters) != ERROR_SUCCESS) {
                ERR("Can't create Printers key\n");
                printer_free(printer);
                SetLastError(ERROR_INVALID_PRINTER_NAME);
                printer = NULL;
                goto end;
            }
            if (RegOpenKeyW(hkeyPrinters, printername, &hkeyPrinter) != ERROR_SUCCESS) {
                WARN("Printer not found in Registry: %s\n", debugstr_w(printername));
                RegCloseKey(hkeyPrinters);
                printer_free(printer);
                SetLastError(ERROR_INVALID_PRINTER_NAME);
                printer = NULL;
                goto end;
            }
            RegCloseKey(hkeyPrinter);
            RegCloseKey(hkeyPrinters);
        }
    }
    else
    {
        TRACE("using the local printserver\n");
    }

end:

    TRACE("==> %p\n", printer);
    return (HANDLE)printer;
}

static inline WCHAR *get_file_part( WCHAR *name )
{
    WCHAR *ptr = wcsrchr( name, '\\' );
    if (ptr) return ptr + 1;
    return name;
}

/******************************************************************************
 *  myAddPrinterDriverEx [internal]
 *
 * Install a Printer Driver with the Option to upgrade / downgrade the Files
 * and a special mode with lazy error checking.
 *
 */
static BOOL myAddPrinterDriverEx(DWORD level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags, BOOL lazy)
{
    const printenv_t *env;
    apd_data_t apd;
    DRIVER_INFO_8W di;
    BOOL    (WINAPI *pDrvDriverEvent)(DWORD, DWORD, LPBYTE, LPARAM);
    HMODULE hui;
    WCHAR *file;
    HKEY    hroot;
    HKEY    hdrv;
    DWORD   disposition;
    DWORD   len;
    LONG    lres;
    BOOL    res;

    /* we need to set all entries in the Registry, independent from the Level of
       DRIVER_INFO, that the caller supplied */

    ZeroMemory(&di, sizeof(di));
    if (pDriverInfo && (level < ARRAY_SIZE(di_sizeof))) {
        memcpy(&di, pDriverInfo, di_sizeof[level]);
    }

    /* dump the most used infos */
    TRACE("%p: .cVersion    : 0x%lx/%ld\n", pDriverInfo, di.cVersion, di.cVersion);
    TRACE("%p: .pName       : %s\n", di.pName, debugstr_w(di.pName));
    TRACE("%p: .pEnvironment: %s\n", di.pEnvironment, debugstr_w(di.pEnvironment));
    TRACE("%p: .pDriverPath : %s\n", di.pDriverPath, debugstr_w(di.pDriverPath));
    TRACE("%p: .pDataFile   : %s\n", di.pDataFile, debugstr_w(di.pDataFile));
    TRACE("%p: .pConfigFile : %s\n", di.pConfigFile, debugstr_w(di.pConfigFile));
    TRACE("%p: .pHelpFile   : %s\n", di.pHelpFile, debugstr_w(di.pHelpFile));
    /* dump only the first of the additional Files */
    TRACE("%p: .pDependentFiles: %s\n", di.pDependentFiles, debugstr_w(di.pDependentFiles));


    /* check environment */
    env = validate_envW(di.pEnvironment);
    if (env == NULL) return FALSE;        /* ERROR_INVALID_ENVIRONMENT */

    /* fill the copy-data / get the driverdir */
    len = sizeof(apd.src) - sizeof(L"\\3") - sizeof(WCHAR);
    if (!fpGetPrinterDriverDirectory(NULL, (LPWSTR) env->envname, 1,
                                    (LPBYTE) apd.src, len, &len)) {
        /* Should never fail */
        return FALSE;
    }
    memcpy(apd.dst, apd.src, len);
    lstrcatW(apd.src, L"\\");
    apd.srclen = wcslen(apd.src);
    lstrcatW(apd.dst, env->versionsubdir);
    lstrcatW(apd.dst, L"\\");
    apd.dstlen = wcslen(apd.dst);
    apd.copyflags = dwFileCopyFlags;
    apd.lazy = lazy;
    CreateDirectoryW(apd.src, NULL);
    CreateDirectoryW(apd.dst, NULL);

    hroot = open_driver_reg(env->envname);
    if (!hroot) {
        ERR("Can't create Drivers key\n");
        return FALSE;
    }

    /* Fill the Registry for the Driver */
    if ((lres = RegCreateKeyExW(hroot, di.pName, 0, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_WRITE | KEY_QUERY_VALUE, NULL,
                                &hdrv, &disposition)) != ERROR_SUCCESS) {

        ERR("can't create driver %s: %lu\n", debugstr_w(di.pName), lres);
        RegCloseKey(hroot);
        SetLastError(lres);
        return FALSE;
    }
    RegCloseKey(hroot);

    /* Verified with the Adobe PS Driver, that w2k does not use di.Version */
    RegSetValueExW(hdrv, L"Version", 0, REG_DWORD, (const BYTE*) &env->driverversion,
                   sizeof(DWORD));

    file = get_file_part( di.pDriverPath );
    RegSetValueExW( hdrv, L"Driver", 0, REG_SZ, (BYTE*)file, (wcslen( file ) + 1) * sizeof(WCHAR) );
    apd_copyfile( di.pDriverPath, file, &apd );

    file = get_file_part( di.pDataFile );
    RegSetValueExW( hdrv, L"Data File", 0, REG_SZ, (BYTE*)file, (wcslen( file ) + 1) * sizeof(WCHAR) );
    apd_copyfile( di.pDataFile, file, &apd );

    file = get_file_part( di.pConfigFile );
    RegSetValueExW( hdrv, L"Configuration File", 0, REG_SZ, (BYTE*)file, (wcslen( file ) + 1) * sizeof(WCHAR) );
    apd_copyfile( di.pConfigFile, file, &apd );

    /* settings for level 3 */
    if (di.pHelpFile)
    {
        file = get_file_part( di.pHelpFile );
        RegSetValueExW( hdrv, L"Help File", 0, REG_SZ, (BYTE*)file, (wcslen( file ) + 1) * sizeof(WCHAR) );
        apd_copyfile( di.pHelpFile, file, &apd );
    }
    else
        RegSetValueExW( hdrv, L"Help File", 0, REG_SZ, (const BYTE*)L"", sizeof(L"") );

    if (di.pDependentFiles && *di.pDependentFiles)
    {
        WCHAR *reg, *reg_ptr, *in_ptr;
        reg = reg_ptr = malloc( multi_sz_lenW( di.pDependentFiles ) );

        for (in_ptr = di.pDependentFiles; *in_ptr; in_ptr += wcslen( in_ptr ) + 1)
        {
            file = get_file_part( in_ptr );
            len = wcslen( file ) + 1;
            memcpy( reg_ptr, file, len * sizeof(WCHAR) );
            reg_ptr += len;
            apd_copyfile( in_ptr, file, &apd );
        }
        *reg_ptr = 0;

        RegSetValueExW( hdrv, L"Dependent Files", 0, REG_MULTI_SZ, (BYTE*)reg, (reg_ptr - reg + 1) * sizeof(WCHAR) );
        free( reg );
    }
    else
        RegSetValueExW(hdrv, L"Dependent Files", 0, REG_MULTI_SZ, (const BYTE*)L"", sizeof(L""));

    /* The language-Monitor was already copied by the caller to "%SystemRoot%\system32" */
    if (di.pMonitorName)
        RegSetValueExW(hdrv, L"Monitor", 0, REG_SZ, (BYTE*)di.pMonitorName,
                       (wcslen(di.pMonitorName)+1)* sizeof(WCHAR));
    else
        RegSetValueExW(hdrv, L"Monitor", 0, REG_SZ, (const BYTE*)L"", sizeof(L""));

    if (di.pDefaultDataType)
        RegSetValueExW(hdrv, L"Datatype", 0, REG_SZ, (BYTE*)di.pDefaultDataType,
                       (wcslen(di.pDefaultDataType)+1)* sizeof(WCHAR));
    else
        RegSetValueExW(hdrv, L"Datatype", 0, REG_SZ, (const BYTE*)L"", sizeof(L""));

    /* settings for level 4 */
    if (di.pszzPreviousNames)
        RegSetValueExW(hdrv, L"Previous Names", 0, REG_MULTI_SZ, (BYTE*)di.pszzPreviousNames,
                       multi_sz_lenW(di.pszzPreviousNames));
    else
        RegSetValueExW(hdrv, L"Previous Names", 0, REG_MULTI_SZ, (const BYTE*)L"", sizeof(L""));

    if (level > 5) TRACE("level %lu for Driver %s is incomplete\n", level, debugstr_w(di.pName));

    RegCloseKey(hdrv);
    hui = driver_load(env, di.pConfigFile);
    pDrvDriverEvent = (void *)GetProcAddress(hui, "DrvDriverEvent");
    if (hui && pDrvDriverEvent) {

        /* Support for DrvDriverEvent is optional */
        TRACE("DRIVER_EVENT_INITIALIZE for %s (%s)\n", debugstr_w(di.pName), debugstr_w(di.pConfigFile));
        /* MSDN: level for DRIVER_INFO is 1 to 3 */
        res = pDrvDriverEvent(DRIVER_EVENT_INITIALIZE, 3, (LPBYTE) &di, 0);
        TRACE("got %d from DRIVER_EVENT_INITIALIZE\n", res);
    }
    FreeLibrary(hui);

    TRACE("=> TRUE with %lu\n", GetLastError());
    return TRUE;

}

/******************************************************************************
 * fpAddMonitor [exported through PRINTPROVIDOR]
 *
 * Install a Printmonitor
 *
 * PARAMS
 *  pName       [I] Servername or NULL (local Computer)
 *  Level       [I] Structure-Level (Must be 2)
 *  pMonitors   [I] PTR to MONITOR_INFO_2
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  All Files for the Monitor must already be copied to %winsysdir% ("%SystemRoot%\system32")
 *
 */
static BOOL WINAPI fpAddMonitor(LPWSTR pName, DWORD Level, LPBYTE pMonitors)
{
    const printenv_t * env;
    monitor_t * pm = NULL;
    LPMONITOR_INFO_2W mi2w;
    HKEY    hroot = NULL;
    HKEY    hentry = NULL;
    DWORD   disposition;
    BOOL    res = FALSE;

    mi2w = (LPMONITOR_INFO_2W) pMonitors;
    TRACE("(%s, %ld, %p): %s %s %s\n", debugstr_w(pName), Level, pMonitors,
        debugstr_w(mi2w->pName), debugstr_w(mi2w->pEnvironment), debugstr_w(mi2w->pDLLName));

    if (copy_servername_from_name(pName, NULL)) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (!mi2w->pName || (! mi2w->pName[0])) {
        WARN("pName not valid : %s\n", debugstr_w(mi2w->pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    env = validate_envW(mi2w->pEnvironment);
    if (!env)
        return FALSE;   /* ERROR_INVALID_ENVIRONMENT */

    if (!mi2w->pDLLName || (! mi2w->pDLLName[0])) {
        WARN("pDLLName not valid : %s\n", debugstr_w(mi2w->pDLLName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hroot) != ERROR_SUCCESS) {
        ERR("unable to create key %s\n", debugstr_w(monitorsW));
        return FALSE;
    }

    if (RegCreateKeyExW(hroot, mi2w->pName, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_QUERY_VALUE, NULL, &hentry,
                        &disposition) == ERROR_SUCCESS) {

        /* Some installers set options for the port before calling AddMonitor.
           We query the "Driver" entry to verify that the monitor is installed,
           before we return an error.
           When a user installs two print monitors at the same time with the
           same name, a race condition is possible but silently ignored. */

        DWORD   namesize = 0;

        if ((disposition == REG_OPENED_EXISTING_KEY) &&
            (RegQueryValueExW(hentry, L"Driver", NULL, NULL, NULL,
                              &namesize) == ERROR_SUCCESS)) {
            TRACE("monitor %s already exists\n", debugstr_w(mi2w->pName));
            /* 9x use ERROR_ALREADY_EXISTS */
            SetLastError(ERROR_PRINT_MONITOR_ALREADY_INSTALLED);
        }
        else
        {
            INT len;
            len = (wcslen(mi2w->pDLLName) +1) * sizeof(WCHAR);
            res = (RegSetValueExW(hentry, L"Driver", 0, REG_SZ,
                    (LPBYTE) mi2w->pDLLName, len) == ERROR_SUCCESS);

            /* Load and initialize the monitor. SetLastError() is called on failure */
            if ((pm = monitor_load(mi2w->pName, mi2w->pDLLName)) == NULL)
            {
                RegDeleteKeyW(hroot, mi2w->pName);
                res = FALSE;
            }
            else
                SetLastError(ERROR_SUCCESS); /* Monitor installer depends on this */
        }

        RegCloseKey(hentry);
    }

    RegCloseKey(hroot);
    return (res);
}

static BOOL wrap_AddPort(monitor_t *pm, LPWSTR name, HWND hwnd, LPWSTR monitor_name)
{
    if (pm->monitor.pfnAddPort)
        return pm->monitor.pfnAddPort(pm->hmon, name, hwnd, monitor_name);

    if (pm->old_AddPort)
        return pm->old_AddPort(name, hwnd, monitor_name);

    WARN("AddPort is not implemented by monitor\n");
    return FALSE;
}

/******************************************************************************
 * fpAddPort [exported through PRINTPROVIDOR]
 *
 * Add a Port for a specific Monitor
 *
 * PARAMS
 *  pName        [I] Servername or NULL (local Computer)
 *  hWnd         [I] Handle to parent Window for the Dialog-Box
 *  pMonitorName [I] Name of the Monitor that manage the Port
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpAddPort(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName)
{
    monitor_t * pm;
    monitor_t * pui;
    LONG        lres;
    DWORD       res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pMonitorName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* an empty Monitorname is Invalid */
    if (!pMonitorName[0]) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pm = monitor_load(pMonitorName, NULL);
    if (pm && (pm->monitor.pfnAddPort || pm->old_AddPort)) {
        res = wrap_AddPort(pm, pName, hWnd, pMonitorName);
        TRACE("got %ld with %lu (%s)\n", res, GetLastError(), debugstr_w(pm->dllname));
    }
    else
    {
        pui = monitor_loadui(pm);
        if (pui && pui->monitorUI && pui->monitorUI->pfnAddPortUI) {
            res = pui->monitorUI->pfnAddPortUI(pName, hWnd, pMonitorName, NULL);
            TRACE("got %ld with %lu (%s)\n", res, GetLastError(), debugstr_w(pui->dllname));
        }
        else
        {
            FIXME("not implemented for %s (monitor %p: %s / monitorui %p: %s)\n",
                    debugstr_w(pMonitorName), pm, debugstr_w(pm ? pm->dllname : NULL),
                    pui, debugstr_w(pui ? pui->dllname : NULL));

            SetLastError(ERROR_NOT_SUPPORTED);
            res = FALSE;
        }
        monitor_unload(pui);
    }
    monitor_unload(pm);

    TRACE("returning %ld with %lu\n", res, GetLastError());
    return res;
}

static BOOL wrap_AddPortEx(monitor_t *pm, LPWSTR name, DWORD level, LPBYTE buffer, LPWSTR monitor_name)
{
    if (pm->monitor.pfnAddPortEx)
        return pm->monitor.pfnAddPortEx(pm->hmon, name, level, buffer, monitor_name);

    if (pm->old_AddPortEx)
        return pm->old_AddPortEx(name, level, buffer, monitor_name);

    WARN("AddPortEx is not implemented by monitor\n");
    return FALSE;
}

/******************************************************************************
 * fpAddPortEx [exported through PRINTPROVIDOR]
 *
 * Add a Port for a specific Monitor, without presenting a user interface
 *
 * PARAMS
 *  pName         [I] Servername or NULL (local Computer)
 *  level         [I] Structure-Level (1 or 2) for pBuffer
 *  pBuffer       [I] PTR to: PORT_INFO_1 or PORT_INFO_2
 *  pMonitorName  [I] Name of the Monitor that manage the Port
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpAddPortEx(LPWSTR pName, DWORD level, LPBYTE pBuffer, LPWSTR pMonitorName)
{
    PORT_INFO_2W * pi2;
    monitor_t * pm;
    DWORD lres;
    DWORD res;

    pi2 = (PORT_INFO_2W *) pBuffer;

    TRACE("(%s, %ld, %p, %s): %s %s %s\n", debugstr_w(pName), level, pBuffer,
            debugstr_w(pMonitorName), debugstr_w(pi2 ? pi2->pPortName : NULL),
            debugstr_w(((level > 1) && pi2) ? pi2->pMonitorName : NULL),
            debugstr_w(((level > 1) && pi2) ? pi2->pDescription : NULL));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((level < 1) || (level > 2)) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if ((!pi2) || (!pMonitorName) || (!pMonitorName[0])) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* load the Monitor */
    pm = monitor_load(pMonitorName, NULL);
    if (pm && (pm->monitor.pfnAddPortEx || pm->old_AddPortEx))
    {
        res = wrap_AddPortEx(pm, pName, level, pBuffer, pMonitorName);
        TRACE("got %ld with %lu (%s)\n", res, GetLastError(), debugstr_w(pm->dllname));
    }
    else
    {
        FIXME("not implemented for %s (monitor %p: %s)\n",
            debugstr_w(pMonitorName), pm, pm ? debugstr_w(pm->dllname) : "(null)");
            SetLastError(ERROR_INVALID_PARAMETER);
            res = FALSE;
    }
    monitor_unload(pm);
    return res;
}

/******************************************************************************
 * fpAddPrinterDriverEx [exported through PRINTPROVIDOR]
 *
 * Install a Printer Driver with the Option to upgrade / downgrade the Files
 *
 * PARAMS
 *  pName           [I] Servername or NULL (local Computer)
 *  level           [I] Level for the supplied DRIVER_INFO_*W struct
 *  pDriverInfo     [I] PTR to DRIVER_INFO_*W struct with the Driver Parameter
 *  dwFileCopyFlags [I] How to Copy / Upgrade / Downgrade the needed Files
 *
 * RESULTS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpAddPrinterDriverEx(LPWSTR pName, DWORD level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    LONG lres;

    TRACE("(%s, %ld, %p, 0x%lx)\n", debugstr_w(pName), level, pDriverInfo, dwFileCopyFlags);
    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if ((dwFileCopyFlags & ~APD_COPY_FROM_DIRECTORY) != APD_COPY_ALL_FILES) {
        TRACE("Flags 0x%lx ignored (using APD_COPY_ALL_FILES)\n", dwFileCopyFlags & ~APD_COPY_FROM_DIRECTORY);
    }

    return myAddPrinterDriverEx(level, pDriverInfo, dwFileCopyFlags, TRUE);
}

/******************************************************************************
 * fpClosePrinter [exported through PRINTPROVIDOR]
 *
 * Close a printer handle and free associated resources
 *
 * PARAMS
 *  hPrinter [I] Printerhandle to close
 *
 * RESULTS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpClosePrinter(HANDLE hPrinter)
{
    printer_t *printer = (printer_t *) hPrinter;

    TRACE("(%p)\n", hPrinter);

    if (printer) {
        printer_free(printer);
        return TRUE;
    }
    return FALSE;
}

static BOOL wrap_ConfigurePort(monitor_t *pm, LPWSTR name, HWND hwnd, LPWSTR port_name)
{
    if (pm->monitor.pfnConfigurePort)
        return pm->monitor.pfnConfigurePort(pm->hmon, name, hwnd, port_name);

    if (pm->old_ConfigurePort)
        return pm->old_ConfigurePort(name, hwnd, port_name);

    WARN("ConfigurePort is not implemented by monitor\n");
    return FALSE;
}

/******************************************************************************
 * fpConfigurePort [exported through PRINTPROVIDOR]
 *
 * Display the Configuration-Dialog for a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window for the Dialog-Box
 *  pPortName [I] Name of the Port, that should be configured
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpConfigurePort(LPWSTR pName, HWND hWnd, LPWSTR pPortName)
{
    monitor_t * pm;
    monitor_t * pui;
    LONG        lres;
    DWORD       res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /* an empty Portname is Invalid, but can popup a Dialog */
    if (!pPortName[0]) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pm = monitor_load_by_port(pPortName);
    if (pm && (pm->monitor.pfnConfigurePort || pm->old_ConfigurePort))
    {
        TRACE("use %s for %s (monitor %p: %s)\n", debugstr_w(pm->name),
                debugstr_w(pPortName), pm, debugstr_w(pm->dllname));
        res = wrap_ConfigurePort(pm, pName, hWnd, pPortName);
        TRACE("got %ld with %lu\n", res, GetLastError());
    }
    else
    {
        pui = monitor_loadui(pm);
        if (pui && pui->monitorUI && pui->monitorUI->pfnConfigurePortUI) {
            TRACE("use %s for %s (monitorui %p: %s)\n", debugstr_w(pui->name),
                        debugstr_w(pPortName), pui, debugstr_w(pui->dllname));
            res = pui->monitorUI->pfnConfigurePortUI(pName, hWnd, pPortName);
            TRACE("got %ld with %lu\n", res, GetLastError());
        }
        else
        {
            FIXME("not implemented for %s (monitor %p: %s / monitorui %p: %s)\n",
                    debugstr_w(pPortName), pm, debugstr_w(pm ? pm->dllname : NULL),
                    pui, debugstr_w(pui ? pui->dllname : NULL));

            SetLastError(ERROR_NOT_SUPPORTED);
            res = FALSE;
        }
        monitor_unload(pui);
    }
    monitor_unload(pm);

    TRACE("returning %ld with %lu\n", res, GetLastError());
    return res;
}

/******************************************************************
 * fpDeleteMonitor [exported through PRINTPROVIDOR]
 *
 * Delete a specific Printmonitor from a Printing-Environment
 *
 * PARAMS
 *  pName        [I] Servername or NULL (local Computer)
 *  pEnvironment [I] Printing-Environment of the Monitor or NULL (Default)
 *  pMonitorName [I] Name of the Monitor, that should be deleted
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  pEnvironment is ignored in Windows for the local Computer.
 *
 */

static BOOL WINAPI fpDeleteMonitor(LPWSTR pName, LPWSTR pEnvironment, LPWSTR pMonitorName)
{
    monitor_t *pm;
    HKEY    hroot = NULL;
    LONG    lres;

    TRACE("(%s, %s, %s)\n",debugstr_w(pName),debugstr_w(pEnvironment),
           debugstr_w(pMonitorName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /*  pEnvironment is ignored in Windows for the local Computer */
    if (!pMonitorName || !pMonitorName[0]) {
        TRACE("pMonitorName %s is invalid\n", debugstr_w(pMonitorName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Unload the monitor if it's loaded */
    EnterCriticalSection(&monitor_handles_cs);
    LIST_FOR_EACH_ENTRY(pm, &monitor_handles, monitor_t, entry)
    {
        if (pm->name && !lstrcmpW(pMonitorName, pm->name))
        {
            monitor_unload(pm);
            break;
        }
    }
    LeaveCriticalSection(&monitor_handles_cs);

    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hroot) != ERROR_SUCCESS) {
        ERR("unable to create key %s\n", debugstr_w(monitorsW));
        return FALSE;
    }

    if(RegDeleteTreeW(hroot, pMonitorName) == ERROR_SUCCESS) {
        TRACE("%s deleted\n", debugstr_w(pMonitorName));
        RegCloseKey(hroot);
        return TRUE;
    }

    TRACE("%s does not exist\n", debugstr_w(pMonitorName));
    RegCloseKey(hroot);

    /* NT: ERROR_UNKNOWN_PRINT_MONITOR (3000), 9x: ERROR_INVALID_PARAMETER (87) */
    SetLastError(ERROR_UNKNOWN_PRINT_MONITOR);
    return FALSE;
}

static BOOL wrap_DeletePort(monitor_t *pm, LPWSTR name, HWND hwnd, LPWSTR port_name)
{
    if (pm->monitor.pfnDeletePort)
        return pm->monitor.pfnDeletePort(pm->hmon, name, hwnd, port_name);

    if (pm->old_ConfigurePort)
        return pm->old_DeletePort(name, hwnd, port_name);

    WARN("DeletePort is not implemented by monitor\n");
    return FALSE;
}

/*****************************************************************************
 * fpDeletePort [exported through PRINTPROVIDOR]
 *
 * Delete a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window for the Dialog-Box
 *  pPortName [I] Name of the Port, that should be deleted
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpDeletePort(LPWSTR pName, HWND hWnd, LPWSTR pPortName)
{
    monitor_t * pm;
    monitor_t * pui;
    LONG        lres;
    DWORD       res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /* an empty Portname is Invalid */
    if (!pPortName[0]) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pm = monitor_load_by_port(pPortName);
    if (pm && (pm->monitor.pfnDeletePort || pm->old_DeletePort))
    {
        TRACE("use %s for %s (monitor %p: %s)\n", debugstr_w(pm->name),
                debugstr_w(pPortName), pm, debugstr_w(pm->dllname));
        res = wrap_DeletePort(pm, pName, hWnd, pPortName);
        TRACE("got %ld with %lu\n", res, GetLastError());
    }
    else
    {
        pui = monitor_loadui(pm);
        if (pui && pui->monitorUI && pui->monitorUI->pfnDeletePortUI) {
            TRACE("use %s for %s (monitorui %p: %s)\n", debugstr_w(pui->name),
                        debugstr_w(pPortName), pui, debugstr_w(pui->dllname));
            res = pui->monitorUI->pfnDeletePortUI(pName, hWnd, pPortName);
            TRACE("got %ld with %lu\n", res, GetLastError());
        }
        else
        {
            FIXME("not implemented for %s (monitor %p: %s / monitorui %p: %s)\n",
                    debugstr_w(pPortName), pm, debugstr_w(pm ? pm->dllname : NULL),
                    pui, debugstr_w(pui ? pui->dllname : NULL));

            SetLastError(ERROR_NOT_SUPPORTED);
            res = FALSE;
        }
        monitor_unload(pui);
    }
    monitor_unload(pm);

    TRACE("returning %ld with %lu\n", res, GetLastError());
    return res;
}

/*****************************************************************************
 * fpEnumMonitors [exported through PRINTPROVIDOR]
 *
 * Enumerate available Port-Monitors
 *
 * PARAMS
 *  pName      [I] Servername or NULL (local Computer)
 *  Level      [I] Structure-Level (1:Win9x+NT or 2:NT only)
 *  pMonitors  [O] PTR to Buffer that receives the Result
 *  cbBuf      [I] Size of Buffer at pMonitors
 *  pcbNeeded  [O] PTR to DWORD that receives the size in Bytes used / required for pMonitors
 *  pcReturned [O] PTR to DWORD that receives the number of Monitors in pMonitors
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pMonitors, if cbBuf is too small
 *
 * NOTES
 *  Windows reads the Registry once and cache the Results.
 *
 */
static BOOL WINAPI fpEnumMonitors(LPWSTR pName, DWORD Level, LPBYTE pMonitors, DWORD cbBuf,
                                  LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD   numentries = 0;
    DWORD   needed = 0;
    LONG    lres;
    BOOL    res = FALSE;

    TRACE("(%s, %ld, %p, %ld, %p, %p)\n", debugstr_w(pName), Level, pMonitors,
          cbBuf, pcbNeeded, pcReturned);

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        goto em_cleanup;
    }

    if (!Level || (Level > 2)) {
        WARN("level (%ld) is ignored in win9x\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    /* Scan all Monitor-Keys */
    numentries = 0;
    needed = get_local_monitors(Level, NULL, 0, &numentries);

    /* we calculated the needed buffersize. now do more error-checks */
    if (cbBuf < needed) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto em_cleanup;
    }

    /* fill the Buffer with the Monitor-Keys */
    needed = get_local_monitors(Level, pMonitors, cbBuf, &numentries);
    res = TRUE;

em_cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = numentries;

    TRACE("returning %d with %ld (%ld byte for %ld entries)\n",
            res, GetLastError(), needed, numentries);

    return (res);
}

/******************************************************************************
 * fpEnumPorts [exported through PRINTPROVIDOR]
 *
 * Enumerate available Ports
 *
 * PARAMS
 *  pName      [I] Servername or NULL (local Computer)
 *  Level      [I] Structure-Level (1 or 2)
 *  pPorts     [O] PTR to Buffer that receives the Result
 *  cbBuf      [I] Size of Buffer at pPorts
 *  pcbNeeded  [O] PTR to DWORD that receives the size in Bytes used / required for pPorts
 *  pcReturned [O] PTR to DWORD that receives the number of Ports in pPorts
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPorts, if cbBuf is too small
 *
 */
static BOOL WINAPI fpEnumPorts(LPWSTR pName, DWORD Level, LPBYTE pPorts, DWORD cbBuf,
                               LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD   needed = 0;
    DWORD   numentries = 0;
    LONG    lres;
    BOOL    res = FALSE;

    TRACE("(%s, %ld, %p, %ld, %p, %p)\n", debugstr_w(pName), Level, pPorts,
          cbBuf, pcbNeeded, pcReturned);

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        goto emP_cleanup;
    }

    if (!Level || (Level > 2)) {
        SetLastError(ERROR_INVALID_LEVEL);
        goto emP_cleanup;
    }

    if (!pcbNeeded || (!pPorts && (cbBuf > 0))) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        goto emP_cleanup;
    }

    EnterCriticalSection(&monitor_handles_cs);
    monitor_loadall();

    /* Scan all local Ports */
    numentries = 0;
    needed = get_ports_from_all_monitors(Level, NULL, 0, &numentries);

    /* we calculated the needed buffersize. now do the error-checks */
    if (cbBuf < needed) {
        monitor_unloadall();
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto emP_cleanup_cs;
    }
    else if (!pPorts || !pcReturned) {
        monitor_unloadall();
        SetLastError(RPC_X_NULL_REF_POINTER);
        goto emP_cleanup_cs;
    }

    /* Fill the Buffer */
    needed = get_ports_from_all_monitors(Level, pPorts, cbBuf, &numentries);
    res = TRUE;
    monitor_unloadall();

emP_cleanup_cs:
    LeaveCriticalSection(&monitor_handles_cs);

emP_cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = (res) ? numentries : 0;

    TRACE("returning %d with %ld (%ld byte for %ld of %ld entries)\n",
          (res), GetLastError(), needed, (res) ? numentries : 0, numentries);

    return (res);
}

/*****************************************************************************
 * fpEnumPrintProcessors [exported through PRINTPROVIDOR]
 *
 * Enumerate available Print Processors
 *
 * PARAMS
 *  pName        [I] Servername or NULL (local Computer)
 *  pEnvironment [I] Printing-Environment or NULL (Default)
 *  Level        [I] Structure-Level (Only 1 is allowed)
 *  pPPInfo      [O] PTR to Buffer that receives the Result
 *  cbBuf        [I] Size of Buffer at pMonitors
 *  pcbNeeded    [O] PTR to DWORD that receives the size in Bytes used / required for pPPInfo
 *  pcReturned   [O] PTR to DWORD that receives the number of Print Processors in pPPInfo
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPPInfo, if cbBuf is too small
 *
 */
static BOOL WINAPI fpEnumPrintProcessors(LPWSTR pName, LPWSTR pEnvironment, DWORD Level,
                            LPBYTE pPPInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    const printenv_t * env;
    LPWSTR  regpathW = NULL;
    DWORD   numentries = 0;
    DWORD   needed = 0;
    LONG    lres;
    BOOL    res = FALSE;

    TRACE("(%s, %s, %ld, %p, %ld, %p, %p)\n", debugstr_w(pName), debugstr_w(pEnvironment),
                                Level, pPPInfo, cbBuf, pcbNeeded, pcReturned);

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        goto epp_cleanup;
    }

    if (Level != 1) {
        SetLastError(ERROR_INVALID_LEVEL);
        goto epp_cleanup;
    }

    env = validate_envW(pEnvironment);
    if (!env)
        goto epp_cleanup;   /* ERROR_INVALID_ENVIRONMENT */

    regpathW = malloc(sizeof(fmt_printprocessorsW) +
            (wcslen(env->envname) * sizeof(WCHAR)));

    if (!regpathW)
        goto epp_cleanup;

    wsprintfW(regpathW, fmt_printprocessorsW, env->envname);

    /* Scan all Printprocessor-Keys */
    numentries = 0;
    needed = get_local_printprocessors(regpathW, NULL, 0, &numentries);

    /* we calculated the needed buffersize. now do more error-checks */
    if (cbBuf < needed) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto epp_cleanup;
    }

    /* fill the Buffer with the Printprocessor Infos */
    needed = get_local_printprocessors(regpathW, pPPInfo, cbBuf, &numentries);
    res = TRUE;

epp_cleanup:
    free(regpathW);
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = numentries;

    TRACE("returning %d with %ld (%ld byte for %ld entries)\n",
            res, GetLastError(), needed, numentries);

    return (res);
}

/******************************************************************************
 * fpGetPrintProcessorDirectory [exported through PRINTPROVIDOR]
 *
 * Return the PATH for the Print-Processors
 *
 * PARAMS
 *  pName        [I] Servername or NULL (this computer)
 *  pEnvironment [I] Printing-Environment or NULL (Default)
 *  level        [I] Structure-Level (must be 1)
 *  pPPInfo      [O] PTR to Buffer that receives the Result
 *  cbBuf        [I] Size of Buffer at pPPInfo
 *  pcbNeeded    [O] PTR to DWORD that receives the size in Bytes used / required for pPPInfo
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPPInfo, if cbBuf is too small
 *
 *  Native Values returned in pPPInfo on Success for this computer:
 *| NT(Windows x64):    "%winsysdir%\\spool\\PRTPROCS\\x64"
 *| NT(Windows NT x86): "%winsysdir%\\spool\\PRTPROCS\\w32x86"
 *| NT(Windows 4.0):    "%winsysdir%\\spool\\PRTPROCS\\win40"
 *
 *  "%winsysdir%" is the Value from GetSystemDirectoryW()
 *
 */
static BOOL WINAPI fpGetPrintProcessorDirectory(LPWSTR pName, LPWSTR pEnvironment, DWORD level,
                                                LPBYTE pPPInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    const printenv_t * env;
    DWORD needed;
    LONG  lres;

    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", debugstr_w(pName), debugstr_w(pEnvironment),
                                        level, pPPInfo, cbBuf, pcbNeeded);

    *pcbNeeded = 0;
    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(RPC_S_SERVER_UNAVAILABLE);
        return FALSE;
    }

    env = validate_envW(pEnvironment);
    if (!env)
        return FALSE;   /* ERROR_INVALID_ENVIRONMENT */

    /* GetSystemDirectoryW returns number of WCHAR including the '\0' */
    needed = GetSystemDirectoryW(NULL, 0);
    /* add the Size for the Subdirectories */
    needed += wcslen(L"\\spool\\prtprocs\\");
    needed += wcslen(env->subdir);
    needed *= sizeof(WCHAR);  /* return-value is size in Bytes */

    *pcbNeeded = needed;

    if (needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    GetSystemDirectoryW((LPWSTR) pPPInfo, cbBuf/sizeof(WCHAR));
    /* add the Subdirectories */
    lstrcatW((LPWSTR) pPPInfo, L"\\spool\\prtprocs\\");
    lstrcatW((LPWSTR) pPPInfo, env->subdir);
    TRACE("==> %s\n", debugstr_w((LPWSTR) pPPInfo));
    return TRUE;
}

/******************************************************************************
 * fpOpenPrinter [exported through PRINTPROVIDOR]
 *
 * Open a Printer / Printserver or a Printer-Object
 *
 * PARAMS
 *  lpPrinterName [I] Name of Printserver, Printer, or Printer-Object
 *  pPrinter      [O] The resulting Handle is stored here
 *  pDefaults     [I] PTR to Default Printer Settings or NULL
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  lpPrinterName is one of:
 *|  Printserver (NT only): "Servername" or NULL for the local Printserver
 *|  Printer: "PrinterName"
 *|  Printer-Object: "PrinterName,Job xxx"
 *|  XcvMonitor: "Servername,XcvMonitor MonitorName"
 *|  XcvPort: "Servername,XcvPort PortName"
 *
 *
 */
static BOOL WINAPI fpOpenPrinter(LPWSTR lpPrinterName, HANDLE *pPrinter,
                                 LPPRINTER_DEFAULTSW pDefaults)
{

    TRACE("(%s, %p, %p)\n", debugstr_w(lpPrinterName), pPrinter, pDefaults);

    *pPrinter = printer_alloc_handle(lpPrinterName, pDefaults);

    return (*pPrinter != 0);
}

/******************************************************************************
 * fpXcvData [exported through PRINTPROVIDOR]
 *
 * Execute commands in the Printmonitor DLL
 *
 * PARAMS
 *  hXcv            [i] Handle from fpOpenPrinter (with XcvMonitor or XcvPort)
 *  pszDataName     [i] Name of the command to execute
 *  pInputData      [i] Buffer for extra Input Data (needed only for some commands)
 *  cbInputData     [i] Size in Bytes of Buffer at pInputData
 *  pOutputData     [o] Buffer to receive additional Data (needed only for some commands)
 *  cbOutputData    [i] Size in Bytes of Buffer at pOutputData
 *  pcbOutputNeeded [o] PTR to receive the minimal Size in Bytes of the Buffer at pOutputData
 *  pdwStatus       [o] PTR to receive the win32 error code from the Printmonitor DLL
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Returning "TRUE" does mean, that the Printmonitor DLL was called successful.
 *  The execution of the command can still fail (check pdwStatus for ERROR_SUCCESS).
 *
 *  Minimal List of commands, that a Printmonitor DLL should support:
 *
 *| "MonitorUI" : Return the Name of the Userinterface-DLL as WSTR in pOutputData
 *| "AddPort"   : Add a Port
 *| "DeletePort": Delete a Port
 *
 *  Many Printmonitors support additional commands. Examples for localspl.dll:
 *  "GetDefaultCommConfig", "SetDefaultCommConfig",
 *  "GetTransmissionRetryTimeout", "ConfigureLPTPortCommandOK"
 *
 */
static BOOL WINAPI fpXcvData(HANDLE hXcv, LPCWSTR pszDataName, PBYTE pInputData,
                    DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData,
                    PDWORD pcbOutputNeeded, PDWORD pdwStatus)
{
    printer_t *printer = (printer_t * ) hXcv;

    TRACE("(%p, %s, %p, %ld, %p, %ld, %p, %p)\n", hXcv, debugstr_w(pszDataName),
          pInputData, cbInputData, pOutputData,
          cbOutputData, pcbOutputNeeded, pdwStatus);

    if (!printer || (!printer->hXcv)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!pcbOutputNeeded) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pszDataName || !pdwStatus || (!pOutputData && (cbOutputData > 0))) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    *pcbOutputNeeded = 0;

    if (printer->pm->monitor.pfnXcvDataPort)
        *pdwStatus = printer->pm->monitor.pfnXcvDataPort(printer->hXcv, pszDataName,
            pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded);

    return TRUE;
}

static inline size_t form_struct_size( DWORD level )
{
    if (level == 1) return sizeof(FORM_INFO_1W);
    if (level == 2) return sizeof(FORM_INFO_2W);

    SetLastError( ERROR_INVALID_LEVEL );
    return 0;
}

static void fill_builtin_form_info( BYTE **base, WCHAR **strings, const struct builtin_form *form, DWORD level,
                                    DWORD size, DWORD *used )
{
    FORM_INFO_2W *info = *(FORM_INFO_2W**)base;
    DWORD name_len = wcslen( form->name ) + 1, res_len, keyword_len, total_size;
    static const WCHAR dll_name[] = L"localspl.dll";
    const WCHAR *resource;

    total_size = name_len * sizeof(WCHAR);

    if (level > 1)
    {
        keyword_len = WideCharToMultiByte( CP_ACP, 0, form->name, -1, NULL, 0, NULL, NULL );
        keyword_len = (keyword_len + 1) & ~1;
        total_size += keyword_len;
        res_len = LoadStringW( localspl_instance, form->res_id, (WCHAR *)&resource, 0 );
        if (res_len && resource[res_len - 1]) res_len++;
        total_size += (res_len + ARRAY_SIZE(dll_name)) * sizeof(WCHAR);
    }

    if (*used + total_size <= size)
    {
        info->Flags = FORM_BUILTIN;
        info->pName = memcpy( *strings, form->name, name_len * sizeof(WCHAR) );
        *strings += name_len;
        info->Size = form->size;
        info->ImageableArea.left = info->ImageableArea.top = 0;
        info->ImageableArea.right = info->Size.cx;
        info->ImageableArea.bottom = info->Size.cy;
        if (level > 1)
        {
            info->pKeyword = (char *)*strings;
            WideCharToMultiByte( CP_ACP, 0, form->name, -1, (char *)info->pKeyword, keyword_len, NULL, NULL );
            *strings += keyword_len / sizeof(WCHAR);
            info->StringType = STRING_MUIDLL;
            info->pMuiDll = memcpy( *strings, dll_name, sizeof(dll_name) );
            *strings += ARRAY_SIZE(dll_name);
            info->dwResourceId = form->res_id;
            if (res_len)
            {
                info->StringType |= STRING_LANGPAIR;
                info->pDisplayName = memcpy( *strings, resource, (res_len - 1) * sizeof(WCHAR) );
                info->pDisplayName[res_len - 1] = '\0';
                *strings += res_len;
                info->wLangId = GetUserDefaultLangID();
            }
            else
            {
                info->pDisplayName = NULL;
                info->wLangId = 0;
            }
        }
    }

    *base += form_struct_size( level );
    *used += total_size;
}

static BOOL WINAPI fpAddForm( HANDLE printer, DWORD level, BYTE *form )
{
    FIXME( "(%p, %ld, %p): stub\n", printer, level, form );
    return TRUE;
}

static BOOL WINAPI fpDeleteForm( HANDLE printer, WCHAR *name )
{
    FIXME( "(%p, %s): stub\n", printer, debugstr_w( name ) );
    return TRUE;
}

static BOOL WINAPI fpGetForm( HANDLE printer, WCHAR *name, DWORD level, BYTE *form, DWORD size, DWORD *needed )
{
    size_t struct_size = form_struct_size( level );
    const struct builtin_form *builtin = NULL;
    WCHAR *strings = NULL;
    BYTE *base = form;
    DWORD i;

    TRACE( "(%p, %s, %ld, %p, %ld, %p)\n", printer, debugstr_w( name ), level, form, size, needed );

    *needed = 0;

    if (!struct_size) return FALSE;

    for (i = 0; i < ARRAY_SIZE(builtin_forms); i++)
    {
        if (!wcscmp( name, builtin_forms[i].name ))
        {
            builtin = builtin_forms + i;
            break;
        }
    }

    if (!builtin)
    {
        SetLastError( ERROR_INVALID_FORM_NAME );
        return FALSE;
    }

    *needed = struct_size;
    if (*needed < size) strings = (WCHAR *)(form + *needed);

    fill_builtin_form_info( &base, &strings, builtin, level, size, needed );

    if (*needed > size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    return TRUE;
}

static BOOL WINAPI fpSetForm( HANDLE printer, WCHAR *name, DWORD level, BYTE *form )
{
    FIXME( "(%p, %s, %ld, %p): stub\n", printer, debugstr_w( name ), level, form );
    return FALSE;
}

static BOOL WINAPI fpEnumForms( HANDLE printer, DWORD level, BYTE *form, DWORD size, DWORD *needed, DWORD *count )
{
    DWORD num = ARRAY_SIZE(builtin_forms), i;
    WCHAR *strings = NULL;
    BYTE *base = form;
    size_t struct_size = form_struct_size( level );

    TRACE( "(%p, %ld, %p, %ld, %p, %p)\n", printer, level, form, size, needed, count );

    *count = *needed = 0;

    if (!struct_size) return FALSE;

    *needed = num * struct_size;
    if (*needed < size) strings = (WCHAR *)(form + *needed);

    for (i = 0; i < ARRAY_SIZE(builtin_forms); i++)
        fill_builtin_form_info( &base, &strings, builtin_forms + i, level, size, needed );

    if (*needed > size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *count = i;
    return TRUE;
}

static const PRINTPROVIDOR backend = {
        fpOpenPrinter,
        NULL,   /* fpSetJob */
        NULL,   /* fpGetJob */
        NULL,   /* fpEnumJobs */
        NULL,   /* fpAddPrinter */
        NULL,   /* fpDeletePrinter */
        NULL,   /* fpSetPrinter */
        NULL,   /* fpGetPrinter */
        NULL,   /* fpEnumPrinters */
        NULL,   /* fpAddPrinterDriver */
        NULL,   /* fpEnumPrinterDrivers */
        NULL,   /* fpGetPrinterDriver */
        fpGetPrinterDriverDirectory,
        NULL,   /* fpDeletePrinterDriver */
        NULL,   /* fpAddPrintProcessor */
        fpEnumPrintProcessors,
        fpGetPrintProcessorDirectory,
        NULL,   /* fpDeletePrintProcessor */
        NULL,   /* fpEnumPrintProcessorDatatypes */
        NULL,   /* fpStartDocPrinter */
        NULL,   /* fpStartPagePrinter */
        NULL,   /* fpWritePrinter */
        NULL,   /* fpEndPagePrinter */
        NULL,   /* fpAbortPrinter */
        NULL,   /* fpReadPrinter */
        NULL,   /* fpEndDocPrinter */
        NULL,   /* fpAddJob */
        NULL,   /* fpScheduleJob */
        NULL,   /* fpGetPrinterData */
        NULL,   /* fpSetPrinterData */
        NULL,   /* fpWaitForPrinterChange */
        fpClosePrinter,
        fpAddForm,
        fpDeleteForm,
        fpGetForm,
        fpSetForm,
        fpEnumForms,
        fpEnumMonitors,
        fpEnumPorts,
        fpAddPort,
        fpConfigurePort,
        fpDeletePort,
        NULL,   /* fpCreatePrinterIC */
        NULL,   /* fpPlayGdiScriptOnPrinterIC */
        NULL,   /* fpDeletePrinterIC */
        NULL,   /* fpAddPrinterConnection */
        NULL,   /* fpDeletePrinterConnection */
        NULL,   /* fpPrinterMessageBox */
        fpAddMonitor,
        fpDeleteMonitor,
        NULL,   /* fpResetPrinter */
        NULL,   /* fpGetPrinterDriverEx */
        NULL,   /* fpFindFirstPrinterChangeNotification */
        NULL,   /* fpFindClosePrinterChangeNotification */
        fpAddPortEx,
        NULL,   /* fpShutDown */
        NULL,   /* fpRefreshPrinterChangeNotification */
        NULL,   /* fpOpenPrinterEx */
        NULL,   /* fpAddPrinterEx */
        NULL,   /* fpSetPort */
        NULL,   /* fpEnumPrinterData */
        NULL,   /* fpDeletePrinterData */
        NULL,   /* fpClusterSplOpen */
        NULL,   /* fpClusterSplClose */
        NULL,   /* fpClusterSplIsAlive */
        NULL,   /* fpSetPrinterDataEx */
        NULL,   /* fpGetPrinterDataEx */
        NULL,   /* fpEnumPrinterDataEx */
        NULL,   /* fpEnumPrinterKey */
        NULL,   /* fpDeletePrinterDataEx */
        NULL,   /* fpDeletePrinterKey */
        NULL,   /* fpSeekPrinter */
        NULL,   /* fpDeletePrinterDriverEx */
        NULL,   /* fpAddPerMachineConnection */
        NULL,   /* fpDeletePerMachineConnection */
        NULL,   /* fpEnumPerMachineConnections */
        fpXcvData,
        fpAddPrinterDriverEx,
        NULL,   /* fpSplReadPrinter */
        NULL,   /* fpDriverUnloadComplete */
        NULL,   /* fpGetSpoolFileInfo */
        NULL,   /* fpCommitSpoolData */
        NULL,   /* fpCloseSpoolFileHandle */
        NULL,   /* fpFlushPrinter */
        NULL,   /* fpSendRecvBidiData */
        NULL    /* fpAddDriverCatalog */
};

/*****************************************************
 * InitializePrintProvidor     (localspl.@)
 *
 * Initialize the Printprovider
 *
 * PARAMS
 *  pPrintProvidor    [I] Buffer to fill with a struct PRINTPROVIDOR
 *  cbPrintProvidor   [I] Size of Buffer in Bytes
 *  pFullRegistryPath [I] Registry-Path for the Printprovidor
 *
 * RETURNS
 *  Success: TRUE and pPrintProvidor filled
 *  Failure: FALSE
 *
 * NOTES
 *  The RegistryPath should be:
 *  "System\CurrentControlSet\Control\Print\Providers\<providername>",
 *  but this Parameter is ignored in "localspl.dll".
 *
 */

BOOL WINAPI InitializePrintProvidor(LPPRINTPROVIDOR pPrintProvidor,
                                    DWORD cbPrintProvidor, LPWSTR pFullRegistryPath)
{

    TRACE("(%p, %lu, %s)\n", pPrintProvidor, cbPrintProvidor, debugstr_w(pFullRegistryPath));
    memcpy(pPrintProvidor, &backend,
          (cbPrintProvidor < sizeof(PRINTPROVIDOR)) ? cbPrintProvidor : sizeof(PRINTPROVIDOR));

    return TRUE;
}
