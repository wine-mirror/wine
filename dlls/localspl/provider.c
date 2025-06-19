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

static CRITICAL_SECTION printers_cs;
static CRITICAL_SECTION_DEBUG printers_cs_debug =
{
    0, 0, &printers_cs,
    { &printers_cs_debug.ProcessLocksList, &printers_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": printers_cs") }
};
static CRITICAL_SECTION printers_cs = { &printers_cs_debug, -1, 0, 0, 0, 0 };

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
    const MONITOREX *monitorex;
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

#define MAX_JOB_ID 99999

typedef struct {
    struct list entry;
    DWORD id;
    WCHAR *filename;
    WCHAR *port;
    WCHAR *datatype;
    WCHAR *document_title;
    DEVMODEW *devmode;
    HANDLE hf;
} job_info_t;

typedef struct {
    WCHAR *name;
    struct list entry;
    LONG ref;

    WCHAR *port;
    WCHAR *print_proc;
    WCHAR *datatype;
    DWORD attributes;

    CRITICAL_SECTION jobs_cs;
    struct list jobs;
} printer_info_t;

typedef struct {
    enum
    {
        HANDLE_SERVER,
        HANDLE_PRINTER,
        HANDLE_XCV,
        HANDLE_PORT,
        HANDLE_JOB,
    } type;
} handle_header_t;

typedef handle_header_t server_t;

typedef struct {
    handle_header_t header;
    monitor_t *pm;
    HANDLE hxcv;
} xcv_t;

typedef struct {
    handle_header_t header;
    monitor_t *mon;
    HANDLE hport;
} port_t;

typedef struct {
    handle_header_t header;
    HANDLE hf;
} job_t;

typedef struct {
    handle_header_t header;
    printer_info_t *info;
    WCHAR *name;
    WCHAR *print_proc;
    WCHAR *datatype;
    DEVMODEW *devmode;
    job_info_t *doc;
} printer_t;

/* ############################### */

static struct list monitor_handles = LIST_INIT( monitor_handles );
static monitor_t * pm_localport;

static struct list printers = LIST_INIT(printers);
static LONG last_job_id;

static const WCHAR fmt_driversW[] =
    L"System\\CurrentControlSet\\control\\Print\\Environments\\%s\\Drivers%s";
static const WCHAR fmt_printprocessorsW[] =
    L"System\\CurrentControlSet\\Control\\Print\\Environments\\%s\\Print Processors\\";
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

static BOOL WINAPI fpClosePrinter(HANDLE);

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

static printer_info_t *find_printer_info(const WCHAR *name, unsigned int len)
{
    printer_info_t *info;

    EnterCriticalSection(&printers_cs);
    LIST_FOR_EACH_ENTRY(info, &printers, printer_info_t, entry)
    {
        if (!wcsncmp(info->name, name, len) && (len == -1 || !info->name[len]))
        {
            InterlockedIncrement(&info->ref);
            LeaveCriticalSection(&printers_cs);
            return info;
        }
    }
    LeaveCriticalSection(&printers_cs);
    return NULL;
}

static WCHAR * reg_query_value(HKEY key, const WCHAR *name)
{
    DWORD size, type;
    WCHAR *ret;

    if (RegQueryValueExW(key, name, 0, &type, NULL, &size) != ERROR_SUCCESS
            || type != REG_SZ)
        return NULL;

    ret = malloc(size);
    if (!ret)
        return NULL;

    if (RegQueryValueExW(key, name, 0, NULL, (BYTE *)ret, &size) != ERROR_SUCCESS)
    {
        free(ret);
        return NULL;
    }
    return ret;
}

static DWORD reg_query_dword(HKEY hkey, const WCHAR *name)
{
    DWORD type, val, size = sizeof(size);

    if (RegQueryValueExW(hkey, name, 0, &type, (BYTE*)&val, &size))
        return 0;
    if (type != REG_DWORD)
        return 0;
    return val;
}

static printer_info_t* get_printer_info(const WCHAR *name)
{
    HKEY hkey, hprinter = NULL;
    printer_info_t *info;
    LSTATUS ret;

    EnterCriticalSection(&printers_cs);
    info = find_printer_info(name, -1);
    if (info)
    {
        LeaveCriticalSection(&printers_cs);
        return info;
    }

    ret = RegCreateKeyW(HKEY_LOCAL_MACHINE, printersW, &hkey);
    if (ret == ERROR_SUCCESS)
        ret = RegOpenKeyW(hkey, name, &hprinter);
    RegCloseKey(hkey);
    if (ret != ERROR_SUCCESS)
    {
        LeaveCriticalSection(&printers_cs);
        return NULL;
    }

    info = calloc(1, sizeof(*info));
    if (!info)
    {
        LeaveCriticalSection(&printers_cs);
        RegCloseKey(hprinter);
        return NULL;
    }

    info->name = wcsdup(name);
    info->port = reg_query_value(hprinter, L"Port");
    info->print_proc = reg_query_value(hprinter, L"Print Processor");
    info->datatype = reg_query_value(hprinter, L"Datatype");
    info->attributes = reg_query_dword(hprinter, L"Attributes");
    RegCloseKey(hprinter);

    if (!info->name || !info->port || !info->print_proc || !info->datatype)
    {
        free(info->name);
        free(info->port);
        free(info->print_proc);
        free(info->datatype);
        free(info);

        LeaveCriticalSection(&printers_cs);
        return NULL;
    }

    info->ref = 1;
    list_add_head(&printers, &info->entry);
    InitializeCriticalSection(&info->jobs_cs);
    list_init(&info->jobs);

    LeaveCriticalSection(&printers_cs);
    return info;
}

static void free_job(job_info_t *job)
{
    list_remove(&job->entry);
    free(job->filename);
    free(job->port);
    free(job->datatype);
    free(job->document_title);
    free(job->devmode);
    CloseHandle(job->hf);
    free(job);
}

static void release_printer_info(printer_info_t *info)
{
    if (!info)
        return;

    if (!InterlockedDecrement(&info->ref))
    {
        EnterCriticalSection(&printers_cs);
        list_remove(&info->entry);
        LeaveCriticalSection(&printers_cs);

        free(info->name);
        free(info->port);
        free(info->print_proc);
        free(info->datatype);
        DeleteCriticalSection(&info->jobs_cs);
        while (!list_empty(&info->jobs))
        {
            job_info_t *job = LIST_ENTRY(list_head(&info->jobs), job_info_t, entry);
            free_job(job);
        }
        free(info);
    }
}

static DEVMODEW * dup_devmode(const DEVMODEW *dm)
{
    DEVMODEW *ret;

    if (!dm) return NULL;
    ret = malloc(dm->dmSize + dm->dmDriverExtra);
    if (ret) memcpy(ret, dm, dm->dmSize + dm->dmDriverExtra);
    return ret;
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

static BOOL WINAPI monitor2_EnumPorts(HANDLE hmon, WCHAR *name, DWORD level,
        BYTE *buf, DWORD size, DWORD *needed, DWORD *count)
{
    return ((MONITOREX*)hmon)->Monitor.pfnEnumPorts(name, level,
            buf, size, needed, count);
}

static BOOL WINAPI monitor2_OpenPort(HANDLE hmon, WCHAR *name, HANDLE *hport)
{
    return ((MONITOREX*)hmon)->Monitor.pfnOpenPort(name, hport);
}

static BOOL WINAPI monitor2_AddPort(HANDLE hmon, WCHAR *name,
        HWND hwnd, WCHAR *monitor_name)
{
    return ((MONITOREX*)hmon)->Monitor.pfnAddPort(name, hwnd, monitor_name);
}

static BOOL WINAPI monitor2_AddPortEx(HANDLE hmon, WCHAR *name, DWORD level,
        BYTE *buf, WCHAR *monitor_name)
{
    return ((MONITOREX*)hmon)->Monitor.pfnAddPortEx(name, level, buf, monitor_name);
}

static BOOL WINAPI monitor2_ConfigurePort(HANDLE hmon, WCHAR *name,
        HWND hwnd, WCHAR *port_name)
{
    return ((MONITOREX*)hmon)->Monitor.pfnConfigurePort(name, hwnd, port_name);
}

static BOOL WINAPI monitor2_DeletePort(HANDLE hmon, WCHAR *name,
        HWND hwnd, WCHAR *port_name)
{
    return ((MONITOREX*)hmon)->Monitor.pfnDeletePort(name, hwnd, port_name);
}

static BOOL WINAPI monitor2_XcvOpenPort(HANDLE hmon, const WCHAR *obj,
        ACCESS_MASK granted_access, HANDLE *hxcv)
{
    return ((MONITOREX*)hmon)->Monitor.pfnXcvOpenPort(obj, granted_access, hxcv);
}

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
                if (!driver)
                    driver = reg_query_value(hroot, L"Driver");
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

            pm->monitorex = pInitializePrintMonitor(regroot);
            TRACE("%p: LPMONITOREX from %s,InitializePrintMonitor(%s)\n",
                    pm->monitorex, debugstr_w(driver), debugstr_w(regroot));
            if (pm->monitorex)
            {
                pm->hmon = (HANDLE)pm->monitorex;

                pm->monitor.cbSize = sizeof(pm->monitor);
                if (pm->monitorex->Monitor.pfnEnumPorts)
                    pm->monitor.pfnEnumPorts = monitor2_EnumPorts;
                if (pm->monitorex->Monitor.pfnOpenPort)
                    pm->monitor.pfnOpenPort = monitor2_OpenPort;
                pm->monitor.pfnStartDocPort = pm->monitorex->Monitor.pfnStartDocPort;
                pm->monitor.pfnWritePort = pm->monitorex->Monitor.pfnWritePort;
                pm->monitor.pfnReadPort = pm->monitorex->Monitor.pfnReadPort;
                pm->monitor.pfnEndDocPort = pm->monitorex->Monitor.pfnEndDocPort;
                pm->monitor.pfnClosePort = pm->monitorex->Monitor.pfnClosePort;
                if (pm->monitorex->Monitor.pfnAddPort)
                    pm->monitor.pfnAddPort = monitor2_AddPort;
                if (pm->monitorex->Monitor.pfnAddPortEx)
                    pm->monitor.pfnAddPortEx = monitor2_AddPortEx;
                if (pm->monitorex->Monitor.pfnConfigurePort)
                    pm->monitor.pfnConfigurePort = monitor2_ConfigurePort;
                if (pm->monitorex->Monitor.pfnDeletePort)
                    pm->monitor.pfnDeletePort = monitor2_DeletePort;
                pm->monitor.pfnGetPrinterDataFromPort =
                    pm->monitorex->Monitor.pfnGetPrinterDataFromPort;
                pm->monitor.pfnSetPortTimeOuts = pm->monitorex->Monitor.pfnSetPortTimeOuts;
                if (pm->monitorex->Monitor.pfnXcvOpenPort)
                    pm->monitor.pfnXcvOpenPort = monitor2_XcvOpenPort;
                pm->monitor.pfnXcvDataPort = pm->monitorex->Monitor.pfnXcvDataPort;
                pm->monitor.pfnXcvClosePort = pm->monitorex->Monitor.pfnXcvClosePort;
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

    /* wine specific ports */
    if (portname[0] == '|' || portname[0] == '/' ||
            !wcsncmp(portname, L"LPR:", 4) || !wcsncmp(portname, L"CUPS:", 5))
        return monitor_load(L"Local Port", NULL);

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
        if (pm->monitor.pfnEnumPorts) {
            pi_needed = 0;
            pi_returned = 0;
            res = pm->monitor.pfnEnumPorts(pm->hmon, NULL, level, pi_buffer,
                    pi_allocated, &pi_needed, &pi_returned);
            if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                /* Do not use realloc (we do not need the old data in the buffer) */
                free(pi_buffer);
                pi_buffer = malloc(pi_needed);
                pi_allocated = (pi_buffer) ? pi_needed : 0;
                res = pm->monitor.pfnEnumPorts(pm->hmon, NULL, level,
                        pi_buffer, pi_allocated, &pi_needed, &pi_returned);
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

static job_info_t * get_job(printer_info_t *info, DWORD job_id)
{
    job_info_t *job;

    LIST_FOR_EACH_ENTRY(job, &info->jobs, job_info_t, entry)
    {
        if(job->id == job_id)
            return job;
    }
    return NULL;
}

static HANDLE server_alloc_handle(const WCHAR *name, BOOL *stop_search)
{
    server_t *server;

    *stop_search = FALSE;
    if (name)
        return NULL;

    server = malloc(sizeof(*server));
    if (!server)
    {
        *stop_search = TRUE;
        return NULL;
    }
    server->type = HANDLE_SERVER;
    return (HANDLE)server;
}

static HANDLE xcv_alloc_handle(const WCHAR *name, PRINTER_DEFAULTSW *def, BOOL *stop_search)
{
    static const WCHAR xcv_monitor[] = L"XcvMonitor ";
    static const WCHAR xcv_port[] = L"XcvPort ";
    BOOL mon;
    xcv_t *xcv;

    *stop_search = FALSE;
    if (name[0] != ',')
        return NULL;

    name++;
    while (*name == ' ')
        name++;

    mon = !wcsncmp(name, xcv_monitor, ARRAY_SIZE(xcv_monitor) - 1);
    if (mon)
    {
        name += ARRAY_SIZE(xcv_monitor) - 1;
    }
    else
    {
        if (wcsncmp(name, xcv_port, ARRAY_SIZE(xcv_port) - 1)) return NULL;
        name += ARRAY_SIZE(xcv_port) - 1;
    }

    *stop_search = TRUE;
    xcv = calloc(1, sizeof(*xcv));
    if (!xcv)
        return NULL;
    xcv->header.type = HANDLE_XCV;

    if (mon)
        xcv->pm = monitor_load(name, NULL);
    else
        xcv->pm = monitor_load_by_port(name);
    if (!xcv->pm)
    {
        free(xcv);
        SetLastError(ERROR_UNKNOWN_PORT);
        return NULL;
    }

    if (xcv->pm->monitor.pfnXcvOpenPort)
    {
        xcv->pm->monitor.pfnXcvOpenPort(xcv->pm->hmon, name,
                def ? def->DesiredAccess : 0, &xcv->hxcv);
    }
    if (!xcv->hxcv)
    {
        fpClosePrinter((HANDLE)xcv);
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    return (HANDLE)xcv;
}

static HANDLE port_alloc_handle(const WCHAR *name, BOOL *stop_search)
{
    static const WCHAR portW[] = L"Port";

    unsigned int i, name_len;
    WCHAR *port_name;
    port_t *port;

    *stop_search = FALSE;
    for (name_len = 0; name[name_len] != ','; name_len++)
    {
        if (!name[name_len])
            return NULL;
    }

    for (i = name_len + 1; name[i] == ' '; i++);
    if (wcscmp(name + i, portW))
        return NULL;

    *stop_search = TRUE;
    port_name = malloc((name_len + 1) * sizeof(WCHAR));
    if (!port_name)
        return NULL;
    memcpy(port_name, name, name_len * sizeof(WCHAR));
    port_name[name_len] = 0;

    port = calloc(1, sizeof(*port));
    if (!port)
    {
        free(port_name);
        return NULL;
    }
    port->header.type = HANDLE_PORT;

    port->mon = monitor_load_by_port(port_name);
    if (!port->mon)
    {
        free(port_name);
        free(port);
        return NULL;
    }
    if (!port->mon->monitor.pfnOpenPort || !port->mon->monitor.pfnWritePort
            || !port->mon->monitor.pfnClosePort || !port->mon->monitor.pfnStartDocPort
            || !port->mon->monitor.pfnEndDocPort)
    {
        FIXME("port not supported: %s\n", debugstr_w(name));
        free(port_name);
        fpClosePrinter((HANDLE)port);
        return NULL;
    }

    port->mon->monitor.pfnOpenPort(port->mon->hmon, port_name, &port->hport);
    free(port_name);
    if (!port->hport)
    {
        fpClosePrinter((HANDLE)port);
        return NULL;
    }
    return (HANDLE)port;
}

static HANDLE job_alloc_handle(const WCHAR *name, BOOL *stop_search)
{
    static const WCHAR jobW[] = L"Job ";

    unsigned int name_len, job_id;
    printer_info_t *printer_info;
    job_info_t *job_info;
    job_t *job;

    *stop_search = FALSE;
    for (name_len = 0; name[name_len] != ','; name_len++)
    {
        if (!name[name_len])
            return NULL;
    }

    for (job_id = name_len + 1; name[job_id] == ' '; job_id++);
    if (!name[job_id])
        return NULL;

    if (wcsncmp(name + job_id, jobW, ARRAY_SIZE(jobW) - 1))
        return NULL;

    *stop_search = TRUE;
    job_id += ARRAY_SIZE(jobW) - 1;
    job_id = wcstoul(name + job_id, NULL, 10);

    printer_info = find_printer_info(name, name_len);
    if (!printer_info)
    {
        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return NULL;
    }

    EnterCriticalSection(&printer_info->jobs_cs);

    job_info = get_job(printer_info, job_id);
    if (!job_info)
    {
        LeaveCriticalSection(&printer_info->jobs_cs);
        release_printer_info(printer_info);
        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return NULL;
    }

    job = malloc(sizeof(*job));
    if (!job)
    {
        LeaveCriticalSection(&printer_info->jobs_cs);
        release_printer_info(printer_info);
        return NULL;
    }
    job->header.type = HANDLE_JOB;
    job->hf = CreateFileW(job_info->filename, GENERIC_READ,
            FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, 0, NULL);

    LeaveCriticalSection(&printer_info->jobs_cs);
    release_printer_info(printer_info);

    if (job->hf == INVALID_HANDLE_VALUE)
    {
        free(job);
        return NULL;
    }
    return (HANDLE)job;
}

static HANDLE printer_alloc_handle(const WCHAR *name, const WCHAR *basename,
                                   PRINTER_DEFAULTSW *def)
{
    printer_t *printer;
    HKEY hroot, hkey;
    LSTATUS status;

    printer = calloc(1, sizeof(*printer));
    if (!printer)
        return NULL;
    printer->header.type = HANDLE_PRINTER;

    /* clone the full name */
    printer->name = wcsdup(name);
    if (name && !printer->name)
    {
        fpClosePrinter((HANDLE)printer);
        return NULL;
    }

    printer->info = get_printer_info(basename);
    if (!printer->info)
    {
        fpClosePrinter((HANDLE)printer);
        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return NULL;
    }

    if (def && def->pDatatype)
        printer->datatype = wcsdup(def->pDatatype);
    if (def && def->pDevMode)
        printer->devmode = dup_devmode(def->pDevMode);

    hroot = open_driver_reg(env_arch.envname);
    if (hroot)
    {
        status = RegOpenKeyW(hroot, name, &hkey);
        RegCloseKey(hroot);
        if (status == ERROR_SUCCESS)
        {
            printer->print_proc = reg_query_value(hkey, L"Print Processor");
            RegCloseKey(hkey);
        }
    }
    if (!printer->print_proc)
        printer->print_proc = wcsdup(L"winprint");

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

    if (di.pszPrintProcessor)
        RegSetValueExW(hdrv, L"Print Processor", 0, REG_SZ, (BYTE*)di.pszPrintProcessor,
                       (wcslen(di.pszPrintProcessor) + 1) * sizeof(WCHAR));
    else
        RegSetValueExW(hdrv, L"Print Processor", 0, REG_SZ, (const BYTE*)L"winprint", sizeof(L"winprint"));

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
    if (pm && pm->monitor.pfnAddPort) {
        res = pm->monitor.pfnAddPort(pm->hmon, pName, hWnd, pMonitorName);
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
    if (pm && pm->monitor.pfnAddPortEx)
    {
        res = pm->monitor.pfnAddPortEx(pm->hmon, pName, level, pBuffer, pMonitorName);
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
    if (pm && pm->monitor.pfnConfigurePort)
    {
        TRACE("use %s for %s (monitor %p: %s)\n", debugstr_w(pm->name),
                debugstr_w(pPortName), pm, debugstr_w(pm->dllname));
        res = pm->monitor.pfnConfigurePort(pm->hmon, pName, hWnd, pPortName);
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

static BOOL WINAPI fpResetPrinter(HANDLE hprinter, PRINTER_DEFAULTSW *def)
{
    printer_t *printer = (printer_t *)hprinter;

    if (!printer || printer->header.type != HANDLE_PRINTER)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!def)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    free(printer->datatype);
    if (def->pDatatype)
        printer->datatype = wcsdup(def->pDatatype);
    else
        printer->datatype = NULL;

    free(printer->devmode);
    if (def->pDevMode)
        printer->devmode = dup_devmode(def->pDevMode);
    else
        printer->devmode = NULL;
    return TRUE;
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
    if (pm && pm->monitor.pfnDeletePort)
    {
        TRACE("use %s for %s (monitor %p: %s)\n", debugstr_w(pm->name),
                debugstr_w(pPortName), pm, debugstr_w(pm->dllname));
        res = pm->monitor.pfnDeletePort(pm->hmon, pName, hWnd, pPortName);
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

static BOOL WINAPI fpAddPrintProcessor(WCHAR *name, WCHAR *environment, WCHAR *path,
        WCHAR *print_proc)
{
    const printenv_t * env;
    HKEY hroot = NULL;
    WCHAR *regpath;
    LSTATUS s;

    TRACE("(%s, %s, %s, %s)\n", debugstr_w(name), debugstr_w(environment),
            debugstr_w(path), debugstr_w(print_proc));

    if (!path || !print_proc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (name && name[0])
    {
        FIXME("server %s not supported\n", debugstr_w(name));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    env = validate_envW(environment);
    if (!env)
        return FALSE;

    regpath = malloc(sizeof(fmt_printprocessorsW) +
            wcslen(env->envname) * sizeof(WCHAR));
    if (!regpath)
        return FALSE;
    wsprintfW(regpath, fmt_printprocessorsW, env->envname);

    s = RegCreateKeyW(HKEY_LOCAL_MACHINE, regpath, &hroot);
    free(regpath);
    if (!s)
    {
        s = RegSetKeyValueW(hroot, print_proc, L"Driver", REG_SZ, path,
                (wcslen(path) + 1) * sizeof(WCHAR));
    }
    RegCloseKey(hroot);
    if (s)
    {
        SetLastError(s);
        return FALSE;
    }

    return TRUE;
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
static BOOL WINAPI fpOpenPrinter(WCHAR *name, HANDLE *hprinter,
                                 PRINTER_DEFAULTSW *def)
{
    WCHAR servername[MAX_COMPUTERNAME_LENGTH + 1];
    const WCHAR *basename;
    BOOL stop_search;

    TRACE("(%s, %p, %p)\n", debugstr_w(name), hprinter, def);

    if (copy_servername_from_name(name, servername))
    {
        FIXME("server %s not supported\n", debugstr_w(servername));
        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return FALSE;
    }

    basename = get_basename_from_name(name);
    if (name != basename) TRACE("converted %s to %s\n",
            debugstr_w(name), debugstr_w(basename));

    /* an empty basename is invalid */
    if (basename && (!basename[0]))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *hprinter = server_alloc_handle(basename, &stop_search);
    if (!*hprinter && !stop_search)
        *hprinter = xcv_alloc_handle(basename, def, &stop_search);
    if (!*hprinter && !stop_search)
        *hprinter = port_alloc_handle(basename, &stop_search);
    if (!*hprinter && !stop_search)
        *hprinter = job_alloc_handle(basename, &stop_search);
    if (!*hprinter && !stop_search)
        *hprinter = printer_alloc_handle(name, basename, def);

    TRACE("==> %p\n", *hprinter);
    return *hprinter != 0;
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
    xcv_t *xcv = (xcv_t *)hXcv;

    TRACE("(%p, %s, %p, %ld, %p, %ld, %p, %p)\n", hXcv, debugstr_w(pszDataName),
          pInputData, cbInputData, pOutputData,
          cbOutputData, pcbOutputNeeded, pdwStatus);

    if (!xcv || xcv->header.type != HANDLE_XCV) {
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

    if (xcv->pm->monitor.pfnXcvDataPort)
        *pdwStatus = xcv->pm->monitor.pfnXcvDataPort(xcv->hxcv, pszDataName,
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
    DWORD name_len = wcslen( form->name ) + 1, res_len = 0, keyword_len = 0, total_size;
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

static size_t get_spool_filename(DWORD job_id, WCHAR *buf, size_t len)
{
    static const WCHAR spool_path[] = L"spool\\PRINTERS\\";
    size_t ret;

    ret = GetSystemDirectoryW(NULL, 0) + ARRAY_SIZE(spool_path) + 10;
    if (len < ret)
        return ret;

    ret = GetSystemDirectoryW(buf, ret);
    if (buf[ret - 1] != '\\')
        buf[ret++] = '\\';
    memcpy(buf + ret, spool_path, sizeof(spool_path));
    ret += ARRAY_SIZE(spool_path) - 1;
    swprintf(buf + ret, 10, L"%05d.SPL", job_id);
    ret += 10;
    return ret;
}

static job_info_t* add_job(printer_t *printer, DOC_INFO_1W *info, BOOL create)
{
    DWORD job_id, last_id;
    size_t len;
    job_info_t *job;

    job = calloc(1, sizeof(*job));
    if (!job)
        return NULL;
    len = get_spool_filename(0, NULL, 0);
    job->filename = malloc(len * sizeof(WCHAR));
    if (!job->filename)
    {
        free(job);
        return NULL;
    }
    job->port = wcsdup(info->pOutputFile);
    if (info->pOutputFile && !job->port)
    {
        free(job->filename);
        free(job);
        return NULL;
    }

    while (1)
    {
        last_id = last_job_id;
        job_id = last_id < MAX_JOB_ID ? last_id + 1 : 1;
        if (InterlockedCompareExchange(&last_job_id, job_id, last_id) == last_id)
            break;
    }

    job->id = job_id;
    get_spool_filename(job_id, job->filename, len);
    if (create)
    {
        job->hf = CreateFileW(job->filename, GENERIC_WRITE, FILE_SHARE_READ,
                NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (job->hf == INVALID_HANDLE_VALUE)
        {
            free(job->filename);
            free(job);
            return NULL;
        }
    }
    else
    {
        job->hf = NULL;
    }
    job->document_title = wcsdup(info->pDocName);
    job->datatype = wcsdup(info->pDatatype);
    job->devmode = dup_devmode(printer->devmode);

    EnterCriticalSection(&printer->info->jobs_cs);
    list_add_tail(&printer->info->jobs, &job->entry);
    LeaveCriticalSection(&printer->info->jobs_cs);
    return job;
}

static BOOL WINAPI fpAddJob(HANDLE hprinter, DWORD level, BYTE *data, DWORD size, DWORD *needed)
{
    ADDJOB_INFO_1W *addjob = (ADDJOB_INFO_1W *)data;
    printer_t *printer = (printer_t *)hprinter;
    DOC_INFO_1W doc_info;
    job_info_t *job;
    size_t len;

    TRACE("(%p %ld %p %ld %p)\n", hprinter, level, data, size, needed);

    if (!printer || printer->header.type != HANDLE_PRINTER)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (level != 1)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!needed)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = get_spool_filename(0, NULL, 0);
    *needed = sizeof(*addjob) + len * sizeof(WCHAR);
    if (size < *needed)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    memset(&doc_info, 0, sizeof(doc_info));
    doc_info.pDocName = (WCHAR *)L"Local Downlevel Document";
    job = add_job(printer, &doc_info, FALSE);
    if (!job)
        return FALSE;

    addjob->JobId = job->id;
    addjob->Path = (WCHAR *)(addjob + 1);
    memcpy(addjob->Path, job->filename, len * sizeof(WCHAR));
    return TRUE;
}

typedef struct {
    HMODULE hmod;
    WCHAR *name;
    BOOL (WINAPI *enum_datatypes)(WCHAR *, WCHAR *, DWORD,
            BYTE *, DWORD, DWORD *, DWORD *);
    HANDLE (WINAPI *open)(WCHAR *, PRINTPROCESSOROPENDATA *);
    BOOL (WINAPI *print)(HANDLE, WCHAR *);
    BOOL (WINAPI *close)(HANDLE);
} printproc_t;

static printproc_t * print_proc_load(const WCHAR *name)
{
    WCHAR *reg_path, path[2 * MAX_PATH];
    printproc_t *ret;
    DWORD size, len;
    LSTATUS status;
    HKEY hkey;

    size = sizeof(fmt_printprocessorsW) +
        (wcslen(env_arch.envname) + wcslen(name)) * sizeof(WCHAR);
    reg_path = malloc(size);
    if (!reg_path)
        return NULL;
    swprintf(reg_path, size / sizeof(WCHAR), fmt_printprocessorsW, env_arch.envname);
    wcscat(reg_path, name);

    status = RegOpenKeyW(HKEY_LOCAL_MACHINE, reg_path, &hkey);
    free(reg_path);
    if (status != ERROR_SUCCESS)
        return NULL;

    if (!fpGetPrintProcessorDirectory(NULL, NULL, 1, (BYTE *)path, sizeof(path), &size))
    {
        RegCloseKey(hkey);
        return NULL;
    }
    len = size / sizeof(WCHAR);
    path[len - 1] = '\\';

    size = sizeof(path) - len * sizeof(WCHAR);
    status = RegQueryValueExW(hkey, L"Driver", NULL, NULL, (BYTE *)(path + len), &size);
    RegCloseKey(hkey);
    if (status != ERROR_SUCCESS)
        return NULL;

    ret = malloc(sizeof(*ret));
    if (!ret)
        return NULL;

    TRACE("loading print processor: %s\n", debugstr_w(path));

    ret->hmod = LoadLibraryW(path);
    if (!ret->hmod)
    {
        free(ret);
        return NULL;
    }

    ret->enum_datatypes = (void *)GetProcAddress(ret->hmod, "EnumPrintProcessorDatatypesW");
    ret->open = (void *)GetProcAddress(ret->hmod, "OpenPrintProcessor");
    ret->print = (void *)GetProcAddress(ret->hmod, "PrintDocumentOnPrintProcessor");
    ret->close = (void *)GetProcAddress(ret->hmod, "ClosePrintProcessor");
    if (!ret->enum_datatypes || !ret->open || !ret->print || !ret->close)
    {
        FreeLibrary(ret->hmod);
        free(ret);
        return NULL;
    }

    ret->name = wcsdup(name);
    return ret;
}

static BOOL print_proc_check_datatype(printproc_t *pp, const WCHAR *datatype)
{
    DATATYPES_INFO_1W *types;
    DWORD size, no, i;

    if (!datatype)
        return FALSE;

    pp->enum_datatypes(NULL, pp->name, 1, NULL, 0, &size, &no);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return FALSE;

    types = malloc(size);
    if (!types)
        return FALSE;

    if (!pp->enum_datatypes(NULL, pp->name, 1, (BYTE *)types, size, &size, &no))
    {
        free(types);
        return FALSE;
    }

    for (i = 0; i < no; i++)
    {
        if (!wcscmp(types[i].pName, datatype))
            break;
    }
    free(types);
    return i < no;
}

static void print_proc_unload(printproc_t *pp)
{
    FreeLibrary(pp->hmod);
    free(pp->name);
    free(pp);
}

static DWORD WINAPI fpStartDocPrinter(HANDLE hprinter, DWORD level, BYTE *doc_info)
{
    printer_t *printer = (printer_t *)hprinter;
    DOC_INFO_1W *info = (DOC_INFO_1W *)doc_info;
    BOOL datatype_valid = FALSE;
    WCHAR *datatype;
    printproc_t *pp;

    TRACE("(%p %ld %p {pDocName = %s, pOutputFile = %s, pDatatype = %s})\n",
            hprinter, level, doc_info, debugstr_w(info->pDocName),
            debugstr_w(info->pOutputFile), debugstr_w(info->pDatatype));

    if (!printer)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (printer->header.type == HANDLE_PORT)
    {
        port_t *port = (port_t *)hprinter;
        /* TODO: pass printer name and job_id */
        return port->mon->monitor.pfnStartDocPort(port->hport,
                NULL, 0, level, doc_info);
    }

    if (printer->header.type != HANDLE_PRINTER)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (level < 1 || level > 3)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return 0;
    }

    if (printer->doc)
    {
        SetLastError(ERROR_INVALID_PRINTER_STATE);
        return 0;
    }

    if (info->pDatatype)
        datatype = info->pDatatype;
    else if (printer->datatype)
        datatype = printer->datatype;
    else
        datatype = printer->info->datatype;

    if (!datatype || ((printer->info->attributes & PRINTER_ATTRIBUTE_RAW_ONLY) &&
                wcsicmp(datatype, L"RAW")))
    {
        TRACE("non RAW datatype specified on RAW-only printer (%s)\n", debugstr_w(datatype));
        SetLastError(ERROR_INVALID_DATATYPE);
        return 0;
    }

    pp = print_proc_load(printer->info->print_proc);
    if (!pp)
    {
        WARN("failed to load %s print processor\n", debugstr_w(printer->info->print_proc));
    }
    else
    {
        datatype_valid = print_proc_check_datatype(pp, datatype);
        print_proc_unload(pp);
    }

    if (!datatype_valid)
    {
        pp = print_proc_load(printer->print_proc);
        if (!pp)
            return 0;

        datatype_valid = print_proc_check_datatype(pp, datatype);
        print_proc_unload(pp);
    }

    if (!datatype_valid)
    {
        TRACE("%s datatype not supported by %s\n", debugstr_w(datatype),
                debugstr_w(printer->info->print_proc));
        SetLastError(ERROR_INVALID_DATATYPE);
        return 0;
    }

    printer->doc = add_job(printer, info, TRUE);
    return printer->doc ? printer->doc->id : 0;
}

static BOOL WINAPI fpWritePrinter(HANDLE hprinter, void *buf, DWORD size, DWORD *written)
{
    handle_header_t *header = (handle_header_t *)hprinter;

    TRACE("(%p, %p, %ld, %p)\n", hprinter, buf, size, written);

    if (!header)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (header->type == HANDLE_PORT)
    {
        port_t *port = (port_t *)hprinter;

        return port->mon->monitor.pfnWritePort(port->hport, buf, size, written);
    }

    if (header->type == HANDLE_PRINTER)
    {
        printer_t *printer = (printer_t *)hprinter;

        if (!printer->doc)
        {
            SetLastError(ERROR_SPL_NO_STARTDOC);
            return FALSE;
        }

        return WriteFile(printer->doc->hf, buf, size, written, NULL);
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}

static BOOL WINAPI fpSetJob(HANDLE hprinter, DWORD job_id,
        DWORD level, BYTE *data, DWORD command)
{
    printer_t *printer = (printer_t *)hprinter;
    BOOL ret = FALSE;
    job_info_t *job;

    TRACE("(%p, %ld, %ld, %p, %ld)\n", hprinter, job_id, level, data, command);
    FIXME("Ignoring everything other than document title\n");

    if (!printer || printer->header.type != HANDLE_PRINTER)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    EnterCriticalSection(&printer->info->jobs_cs);
    job = get_job(printer->info, job_id);
    if (!job)
    {
        LeaveCriticalSection(&printer->info->jobs_cs);
        return FALSE;
    }

    switch(level)
    {
    case 0:
        ret = TRUE;
        break;
    case 1:
    {
        JOB_INFO_1W *info1 = (JOB_INFO_1W *)data;
        WCHAR *title = wcsdup(info1->pDocument);

        if (title)
        {
            free(job->document_title);
            job->document_title = title;
            ret = TRUE;
        }
        break;
    }
    case 2:
    {
        JOB_INFO_2W *info2 = (JOB_INFO_2W *)data;
        WCHAR *title = wcsdup(info2->pDocument);
        DEVMODEW *devmode = dup_devmode(info2->pDevMode);

        if (!title || !devmode)
        {
            free(title);
            free(devmode);
        }
        else
        {
            free(job->document_title);
            free(job->devmode);
            job->document_title = title;
            job->devmode = devmode;
            ret = TRUE;
        }
        break;
      }
    case 3:
        FIXME("level 3 stub\n");
        ret = TRUE;
        break;
    default:
        SetLastError(ERROR_INVALID_LEVEL);
        break;
    }

    LeaveCriticalSection(&printer->info->jobs_cs);
    return ret;
}

static BOOL WINAPI fpGetJob(HANDLE hprinter, DWORD job_id, DWORD level,
        BYTE *data, DWORD size, DWORD *needed)
{
    printer_t *printer = (printer_t *)hprinter;
    BOOL ret = TRUE;
    DWORD s = 0;
    job_info_t *job;
    WCHAR *p;

    TRACE("%p %ld %ld %p %ld %p\n", hprinter, job_id, level, data, size, needed);

    if (!printer || printer->header.type != HANDLE_PRINTER)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!needed)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    EnterCriticalSection(&printer->info->jobs_cs);
    job = get_job(printer->info, job_id);
    if (!job)
    {
        LeaveCriticalSection(&printer->info->jobs_cs);
        return FALSE;
    }

    switch(level)
    {
    case 1:
        s = sizeof(JOB_INFO_1W);
        s += job->document_title ? (wcslen(job->document_title) + 1) * sizeof(WCHAR) : 0;
        s += printer->info->name ?
            (wcslen(printer->info->name) + 1) * sizeof(WCHAR) : 0;

        if (size >= s)
        {
            JOB_INFO_1W *info = (JOB_INFO_1W *)data;

            p = (WCHAR *)(info + 1);
            memset(info, 0, sizeof(*info));
            info->JobId = job->id;
            if (job->document_title)
            {
                info->pDocument = p;
                wcscpy(p, job->document_title);
                p += wcslen(job->document_title) + 1;
            }
            if (printer->info->name)
            {
                info->pPrinterName = p;
                wcscpy(p, printer->info->name);
            }
        }
        break;
    case 2:
        s = sizeof(JOB_INFO_2W);
        s += job->document_title ? (wcslen(job->document_title) + 1) * sizeof(WCHAR) : 0;
        s += printer->info->name ?
            (wcslen(printer->info->name) + 1) * sizeof(WCHAR) : 0;
        if (job->devmode)
        {
            /* align DEVMODE to a DWORD boundary */
            s += (4 - (s & 3)) & 3;
            s += job->devmode->dmSize + job->devmode->dmDriverExtra;
        }

        if (size >= s)
        {
            JOB_INFO_2W *info = (JOB_INFO_2W *)data;

            p = (WCHAR *)(info + 1);
            memset(info, 0, sizeof(*info));
            info->JobId = job->id;
            if (job->document_title)
            {
                info->pDocument = p;
                wcscpy(p, job->document_title);
                p += wcslen(job->document_title) + 1;
            }
            if (printer->info->name)
            {
                info->pPrinterName = p;
                wcscpy(p, printer->info->name);
                p += wcslen(printer->info->name) + 1;
            }
            if (job->devmode)
            {
                DEVMODEW *devmode = (DEVMODEW *)(data + s - job->devmode->dmSize
                        - job->devmode->dmDriverExtra);
                info->pDevMode = devmode;
                memcpy(devmode, job->devmode, job->devmode->dmSize + job->devmode->dmDriverExtra);
            }
        }
        break;
    case 3:
        FIXME("level 3 stub\n");
        s = sizeof(JOB_INFO_3);

        if (size >= s)
            memset(data, 0, sizeof(JOB_INFO_3));
        break;
    default:
        SetLastError(ERROR_INVALID_LEVEL);
        ret = FALSE;
        break;
    }

    LeaveCriticalSection(&printer->info->jobs_cs);

    *needed = s;
    if (size < s)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        ret = FALSE;
    }
    return ret;
}

static BOOL WINAPI fpScheduleJob(HANDLE hprinter, DWORD job_id)
{
    printer_t *printer = (printer_t *)hprinter;
    WCHAR output[1024], name[1024], *datatype;
    BOOL datatype_valid = FALSE, ret = FALSE;
    PRINTPROCESSOROPENDATA pp_data;
    const WCHAR *port_name, *port;
    job_info_t *job;
    printproc_t *pp;
    HANDLE hpp;
    HKEY hkey;

    TRACE("%p %ld\n", hprinter, job_id);

    if (!printer || printer->header.type != HANDLE_PRINTER)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    EnterCriticalSection(&printer->info->jobs_cs);
    job = get_job(printer->info, job_id);
    if (!job)
    {
        LeaveCriticalSection(&printer->info->jobs_cs);
        return FALSE;
    }

    port = job->port;
    if (!port || !*port)
        port = printer->info->port;
    TRACE("need to schedule job %ld filename %s to port %s\n", job->id,
            debugstr_w(job->filename), debugstr_w(port));

    port_name = port;
    if ((isalpha(port[0]) && port[1] == ':') ||
            !wcsncmp(port, L"FILE:", ARRAY_SIZE(L"FILE:") - 1))
    {
        port_name = L"FILE:";
    }
    else if (!RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Wine\\Printing\\Spooler", &hkey))
    {
        DWORD type, count = sizeof(output);
        if (!RegQueryValueExW(hkey, port, NULL, &type, (BYTE *)output, &count))
        {
            TRACE("overriding port %s -> %s\n", debugstr_w(port), debugstr_w(output));
            port_name = output;
        }
        RegCloseKey(hkey);
    }

    if (job->datatype)
        datatype = job->datatype;
    else if (printer->datatype)
        datatype = printer->datatype;
    else
        datatype = printer->info->datatype;

    pp = print_proc_load(printer->info->print_proc);
    if (!pp)
    {
        WARN("failed to load %s print processor\n", debugstr_w(printer->info->print_proc));
    }
    else
    {
        datatype_valid = print_proc_check_datatype(pp, datatype);
        if (!datatype_valid)
            print_proc_unload(pp);
    }

    if (!datatype_valid)
    {
        pp = print_proc_load(printer->print_proc);
        if (!pp) goto cleanup;

        datatype_valid = print_proc_check_datatype(pp, datatype);
    }

    if (!datatype_valid)
    {
        WARN("%s datatype not supported by %s\n", debugstr_w(datatype),
                debugstr_w(printer->info->print_proc));
        print_proc_unload(pp);
        goto cleanup;
    }

    swprintf(name, ARRAY_SIZE(name), L"%s, Port", port_name);
    pp_data.pDevMode = job->devmode;
    pp_data.pDatatype = datatype;
    pp_data.pParameters = NULL;
    pp_data.pDocumentName = job->document_title;
    pp_data.JobId = job->id;
    pp_data.pOutputFile = (WCHAR *)port;
    pp_data.pPrinterName = printer->name;
    hpp = pp->open(name, &pp_data);
    if (!hpp)
    {
        WARN("OpenPrintProcessor failed %ld\n", GetLastError());
        print_proc_unload(pp);
        goto cleanup;
    }

    swprintf(name, ARRAY_SIZE(name), L"%s, Job %d", printer->name, job->id);
    ret = pp->print(hpp, name);
    if (!ret)
        WARN("PrintDocumentOnPrintProcessor failed %ld\n", GetLastError());
    pp->close(hpp);
    print_proc_unload(pp);

    if (!(printer->info->attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS))
        DeleteFileW(job->filename);
    free_job(job);
cleanup:
    LeaveCriticalSection(&printer->info->jobs_cs);
    return ret;
}

static BOOL WINAPI fpAbortPrinter(HANDLE hprinter)
{
    printer_t *printer = (printer_t *)hprinter;
    job_info_t *job;

    TRACE("%p\n", hprinter);

    if (!printer)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (printer->header.type != HANDLE_PRINTER)
    {
        FIXME("%x handle not supported\n", printer->header.type);
        return FALSE;
    }

    if (!printer->doc)
    {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        return FALSE;
    }

    CloseHandle(printer->doc->hf);
    printer->doc->hf = NULL;

    EnterCriticalSection(&printer->info->jobs_cs);
    job = get_job(printer->info, printer->doc->id);
    if (job) free_job(job);
    LeaveCriticalSection(&printer->info->jobs_cs);

    printer->doc = NULL;
    return TRUE;
}

static BOOL WINAPI fpReadPrinter(HANDLE hprinter, void *buf, DWORD size, DWORD *bytes_read)
{
    job_t *job = (job_t *)hprinter;

    TRACE("%p %p %lu %p\n", hprinter, buf, size, bytes_read);

    if (!job || (job->header.type != HANDLE_JOB))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return ReadFile(job->hf, buf, size, bytes_read, NULL);
}

static BOOL WINAPI fpEndDocPrinter(HANDLE hprinter)
{
    printer_t *printer = (printer_t *)hprinter;
    BOOL ret;

    TRACE("%p\n", hprinter);

    if (!printer)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (printer->header.type == HANDLE_PORT)
    {
        port_t *port = (port_t *)hprinter;
        return port->mon->monitor.pfnEndDocPort(port->hport);
    }

    if (printer->header.type != HANDLE_PRINTER)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!printer->doc)
    {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        return FALSE;
    }

    CloseHandle(printer->doc->hf);
    printer->doc->hf = NULL;
    ret = fpScheduleJob(hprinter, printer->doc->id);
    printer->doc = NULL;
    return ret;
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
static BOOL WINAPI fpClosePrinter(HANDLE hprinter)
{
    handle_header_t *header = (handle_header_t *)hprinter;

    TRACE("(%p)\n", hprinter);

    if (!header)
        return FALSE;

    if (header->type == HANDLE_SERVER)
    {
        free(header);
    }
    else if (header->type == HANDLE_XCV)
    {
        xcv_t *xcv = (xcv_t *)hprinter;

        if (xcv->hxcv && xcv->pm->monitor.pfnXcvClosePort)
            xcv->pm->monitor.pfnXcvClosePort(xcv->hxcv);

        monitor_unload(xcv->pm);
        free(xcv);
    }
    else if (header->type == HANDLE_PORT)
    {
        port_t *port = (port_t *)hprinter;

        if (port->hport)
            port->mon->monitor.pfnClosePort(port->hport);
        if (port->mon)
            monitor_unload(port->mon);
        free(port);
    }
    else if (header->type == HANDLE_JOB)
    {
        job_t *job = (job_t *)hprinter;

        CloseHandle(job->hf);
        free(job);
    }
    else if (header->type == HANDLE_PRINTER)
    {
        printer_t *printer = (printer_t *)hprinter;

        if(printer->doc)
            fpEndDocPrinter(hprinter);

        release_printer_info(printer->info);
        free(printer->name);
        free(printer->print_proc);
        free(printer->datatype);
        free(printer->devmode);
        free(printer);
    }
    else
    {
        ERR("invalid handle type\n");
        return FALSE;
    }
    return TRUE;
}

static BOOL WINAPI fpSeekPrinter(HANDLE hprinter, LARGE_INTEGER distance,
        LARGE_INTEGER *pos, DWORD method, BOOL bwrite)
{
    job_t *job = (job_t *)hprinter;

    TRACE("(%p %I64d %p %lx %x)\n", hprinter, distance.QuadPart, pos, method, bwrite);

    if (!job)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (job->header.type != HANDLE_JOB)
    {
        FIXME("handle %x not supported\n", job->header.type);
        return FALSE;
    }

    if (bwrite)
    {
        if (pos)
            pos->QuadPart = 0;
        return TRUE;
    }

    return SetFilePointerEx(job->hf, distance, pos, method);
}

static const PRINTPROVIDOR backend = {
        fpOpenPrinter,
        fpSetJob,
        fpGetJob,
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
        fpAddPrintProcessor,
        fpEnumPrintProcessors,
        fpGetPrintProcessorDirectory,
        NULL,   /* fpDeletePrintProcessor */
        NULL,   /* fpEnumPrintProcessorDatatypes */
        fpStartDocPrinter,
        NULL,   /* fpStartPagePrinter */
        fpWritePrinter,
        NULL,   /* fpEndPagePrinter */
        fpAbortPrinter,
        fpReadPrinter,
        fpEndDocPrinter,
        fpAddJob,
        fpScheduleJob,
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
        fpResetPrinter,
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
        fpSeekPrinter,
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
