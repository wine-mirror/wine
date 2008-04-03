/*
 * Implementation of the Local Printmonitor
 *
 * Copyright 2006-2008 Detlef Riekenberg
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winspool.h"
#include "ddk/winsplp.h"
#include "winuser.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "localspl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(localspl);

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
    LPCWSTR  envname;
    LPCWSTR  subdir;
    DWORD    driverversion;
    LPCWSTR  versionregpath;
    LPCWSTR  versionsubdir;
} printenv_t;

/* ############################### */

HINSTANCE LOCALSPL_hInstance = NULL;

static const PRINTPROVIDOR * pp = NULL;


static const WCHAR backslashW[] = {'\\',0};
static const WCHAR configuration_fileW[] = {'C','o','n','f','i','g','u','r','a','t','i','o','n',' ','F','i','l','e',0};
static const WCHAR datatypeW[] = {'D','a','t','a','t','y','p','e',0};
static const WCHAR data_fileW[] = {'D','a','t','a',' ','F','i','l','e',0};
static const WCHAR default_devmodeW[] = {'D','e','f','a','u','l','t',' ','D','e','v','M','o','d','e',0};
static const WCHAR dependent_filesW[] = {'D','e','p','e','n','d','e','n','t',' ','F','i','l','e','s',0};
static const WCHAR descriptionW[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR driverW[] = {'D','r','i','v','e','r',0};
static const WCHAR fmt_driversW[] = { 'S','y','s','t','e','m','\\',
                                  'C','u', 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'c','o','n','t','r','o','l','\\',
                                  'P','r','i','n','t','\\',
                                  'E','n','v','i','r','o','n','m','e','n','t','s','\\',
                                  '%','s','\\','D','r','i','v','e','r','s','%','s',0 };
static const WCHAR hardwareidW[] = {'H','a','r','d','w','a','r','e','I','D',0};
static const WCHAR help_fileW[] = {'H','e','l','p',' ','F','i','l','e',0};
static const WCHAR locationW[] = {'L','o','c','a','t','i','o','n',0};
static const WCHAR manufacturerW[] = {'M','a','n','u','f','a','c','t','u','r','e','r',0};
static const WCHAR monitorW[] = {'M','o','n','i','t','o','r',0};
static const WCHAR monitorUIW[] = {'M','o','n','i','t','o','r','U','I',0};
static const WCHAR nameW[] = {'N','a','m','e',0};
static const WCHAR oem_urlW[] = {'O','E','M',' ','U','r','l',0};
static const WCHAR parametersW[] = {'P','a','r','a','m','e','t','e','r','s',0};
static const WCHAR portW[] = {'P','o','r','t',0};
static const WCHAR previous_namesW[] = {'P','r','e','v','i','o','u','s',' ','N','a','m','e','s',0};
static const WCHAR spooldriversW[] = {'\\','s','p','o','o','l','\\','d','r','i','v','e','r','s','\\',0};
static const WCHAR versionW[] = {'V','e','r','s','i','o','n',0};

static const WCHAR win40_envnameW[] = {'W','i','n','d','o','w','s',' ','4','.','0',0};
static const WCHAR win40_subdirW[] = {'w','i','n','4','0',0};
static const WCHAR version0_regpathW[] = {'\\','V','e','r','s','i','o','n','-','0',0};
static const WCHAR version0_subdirW[] = {'\\','0',0};

static const WCHAR x86_envnameW[] = {'W','i','n','d','o','w','s',' ','N','T',' ','x','8','6',0};
static const WCHAR x86_subdirW[] = {'w','3','2','x','8','6',0};
static const WCHAR version3_regpathW[] = {'\\','V','e','r','s','i','o','n','-','3',0};
static const WCHAR version3_subdirW[] = {'\\','3',0};


static const printenv_t env_x86 =   {x86_envnameW, x86_subdirW, 3,
                                     version3_regpathW, version3_subdirW};

static const printenv_t env_win40 = {win40_envnameW, win40_subdirW, 0,
                                     version0_regpathW, version0_subdirW};

static const printenv_t * const all_printenv[] = {&env_x86, &env_win40};


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
static BOOL apd_copyfile(LPWSTR filename, apd_data_t *apd)
{
    LPWSTR  ptr;
    LPWSTR  srcname;
    DWORD   res;

    apd->src[apd->srclen] = '\0';
    apd->dst[apd->dstlen] = '\0';

    if (!filename || !filename[0]) {
        /* nothing to copy */
        return TRUE;
    }

    ptr = strrchrW(filename, '\\');
    if (ptr) {
        ptr++;
    }
    else
    {
        ptr = filename;
    }

    if (apd->copyflags & APD_COPY_FROM_DIRECTORY) {
        /* we have an absolute Path */
        srcname = filename;
    }
    else
    {
        srcname = apd->src;
        lstrcatW(srcname, ptr);
    }
    lstrcatW(apd->dst, ptr);

    TRACE("%s => %s\n", debugstr_w(filename), debugstr_w(apd->dst));

    /* FIXME: handle APD_COPY_NEW_FILES */
    res = CopyFileW(srcname, apd->dst, FALSE);
    TRACE("got %u with %u\n", res, GetLastError());

    return (apd->lazy) ? TRUE : res;
}

/******************************************************************
 * copy_servername_from_name  (internal)
 *
 * for an external server, the serverpart from the name is copied.
 *
 * RETURNS
 *  the length (in WCHAR) of the serverpart (0 for the local computer)
 *  (-length), when the name is to long
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
    ptr = strchrW(server, '\\');
    serverlen = (ptr) ? ptr - server : lstrlenW(server);

    /* servername is empty or to long */
    if (serverlen == 0) return 0;

    TRACE("found %s\n", debugstr_wn(server, serverlen));

    if (serverlen > MAX_COMPUTERNAME_LENGTH) return -serverlen;

    len = sizeof(buffer) / sizeof(buffer[0]);
    if (GetComputerNameW(buffer, &len)) {
        if ((serverlen == len) && (strncmpiW(server, buffer, len) == 0)) {
            /* The requested Servername is our computername */
            if (target) {
                memcpy(target, server, serverlen * sizeof(WCHAR));
                target[serverlen] = '\0';
            }
            return serverlen;
        }
    }
    return 0;
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
        ptr += lstrlenW(ptr) + 1;
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
        for (i = 0; i < sizeof(all_printenv)/sizeof(all_printenv[0]); i++)
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
        result = (GetVersion() & 0x80000000) ? &env_win40 : &env_x86;
    }

    TRACE("=> using %p: %s\n", result, debugstr_w(result ? result->envname : NULL));
    return result;
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

    buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(fmt_driversW) +
                (lstrlenW(env->envname) + lstrlenW(env->versionregpath)) * sizeof(WCHAR));

    if (buffer) {
        wsprintfW(buffer, fmt_driversW, env->envname, env->versionregpath);
        RegCreateKeyW(HKEY_LOCAL_MACHINE, buffer, &retval);
        HeapFree(GetProcessHeap(), 0, buffer);
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

    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_w(pName),
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
    needed += lstrlenW(spooldriversW);
    needed += lstrlenW(env->subdir);
    needed *= sizeof(WCHAR);  /* return-value is size in Bytes */

    *pcbNeeded = needed;

    if (needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (pDriverDirectory == NULL) {
        /* ERROR_INVALID_USER_BUFFER is NT, ERROR_INVALID_PARAMETER is win9x */
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    GetSystemDirectoryW((LPWSTR) pDriverDirectory, cbBuf/sizeof(WCHAR));
    /* add the Subdirectories */
    lstrcatW((LPWSTR) pDriverDirectory, spooldriversW);
    lstrcatW((LPWSTR) pDriverDirectory, env->subdir);

    TRACE("=> %s\n", debugstr_w((LPWSTR) pDriverDirectory));
    return TRUE;
}

/******************************************************************************
 *  myAddPrinterDriverEx [internal]
 *
 * Install a Printer Driver with the Option to upgrade / downgrade the Files
 * and a special mode with lazy error checking.
 *
 */
static BOOL WINAPI myAddPrinterDriverEx(DWORD level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags, BOOL lazy)
{
    const printenv_t *env;
    apd_data_t apd;
    DRIVER_INFO_8W di;
    LPWSTR  ptr;
    HKEY    hroot;
    HKEY    hdrv;
    DWORD   disposition;
    DWORD   len;
    LONG    lres;

    /* we need to set all entries in the Registry, independent from the Level of
       DRIVER_INFO, that the caller supplied */

    ZeroMemory(&di, sizeof(di));
    if (pDriverInfo && (level < (sizeof(di_sizeof) / sizeof(di_sizeof[0])))) {
        memcpy(&di, pDriverInfo, di_sizeof[level]);
    }

    /* dump the most used infos */
    TRACE("%p: .cVersion    : 0x%x/%d\n", pDriverInfo, di.cVersion, di.cVersion);
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
    len = sizeof(apd.src) - sizeof(version3_subdirW) - sizeof(WCHAR);
    if (!fpGetPrinterDriverDirectory(NULL, (LPWSTR) env->envname, 1,
                                    (LPBYTE) apd.src, len, &len)) {
        /* Should never Fail */
        return FALSE;
    }
    memcpy(apd.dst, apd.src, len);
    lstrcatW(apd.src, backslashW);
    apd.srclen = lstrlenW(apd.src);
    lstrcatW(apd.dst, env->versionsubdir);
    lstrcatW(apd.dst, backslashW);
    apd.dstlen = lstrlenW(apd.dst);
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

        ERR("can't create driver %s: %u\n", debugstr_w(di.pName), lres);
        RegCloseKey(hroot);
        SetLastError(lres);
        return FALSE;
    }
    RegCloseKey(hroot);

    if (disposition == REG_OPENED_EXISTING_KEY) {
        TRACE("driver %s already installed\n", debugstr_w(di.pName));
        RegCloseKey(hdrv);
        SetLastError(ERROR_PRINTER_DRIVER_ALREADY_INSTALLED);
        return FALSE;
    }

    /* Verified with the Adobe PS Driver, that w2k does not use di.Version */
    RegSetValueExW(hdrv, versionW, 0, REG_DWORD, (LPBYTE) &env->driverversion,
                   sizeof(DWORD));

    RegSetValueExW(hdrv, driverW, 0, REG_SZ, (LPBYTE) di.pDriverPath,
                   (lstrlenW(di.pDriverPath)+1)* sizeof(WCHAR));
    apd_copyfile(di.pDriverPath, &apd);

    RegSetValueExW(hdrv, data_fileW, 0, REG_SZ, (LPBYTE) di.pDataFile,
                   (lstrlenW(di.pDataFile)+1)* sizeof(WCHAR));
    apd_copyfile(di.pDataFile, &apd);

    RegSetValueExW(hdrv, configuration_fileW, 0, REG_SZ, (LPBYTE) di.pConfigFile,
                   (lstrlenW(di.pConfigFile)+1)* sizeof(WCHAR));
    apd_copyfile(di.pConfigFile, &apd);

    /* settings for level 3 */
    RegSetValueExW(hdrv, help_fileW, 0, REG_SZ, (LPBYTE) di.pHelpFile,
                   di.pHelpFile ? (lstrlenW(di.pHelpFile)+1)* sizeof(WCHAR) : 0);
    apd_copyfile(di.pHelpFile, &apd);


    ptr = di.pDependentFiles;
    RegSetValueExW(hdrv, dependent_filesW, 0, REG_MULTI_SZ, (LPBYTE) di.pDependentFiles,
                   di.pDependentFiles ? multi_sz_lenW(di.pDependentFiles) : 0);
    while ((ptr != NULL) && (ptr[0])) {
        if (apd_copyfile(ptr, &apd)) {
            ptr += lstrlenW(ptr) + 1;
        }
        else
        {
            WARN("Failed to copy %s\n", debugstr_w(ptr));
            ptr = NULL;
        }
    }
    /* The language-Monitor was already copied by the caller to "%SystemRoot%\system32" */
    RegSetValueExW(hdrv, monitorW, 0, REG_SZ, (LPBYTE) di.pMonitorName,
                   di.pMonitorName ? (lstrlenW(di.pMonitorName)+1)* sizeof(WCHAR) : 0);

    RegSetValueExW(hdrv, datatypeW, 0, REG_SZ, (LPBYTE) di.pDefaultDataType,
                   di.pDefaultDataType ? (lstrlenW(di.pDefaultDataType)+1)* sizeof(WCHAR) : 0);

    /* settings for level 4 */
    RegSetValueExW(hdrv, previous_namesW, 0, REG_MULTI_SZ, (LPBYTE) di.pszzPreviousNames,
                   di.pszzPreviousNames ? multi_sz_lenW(di.pszzPreviousNames) : 0);

    if (level > 5) TRACE("level %u for Driver %s is incomplete\n", level, debugstr_w(di.pName));

    RegCloseKey(hdrv);
    TRACE("### DrvDriverEvent(...,DRIVEREVENT_INITIALIZE) not implemented yet\n");

    TRACE("=> TRUE with %u\n", GetLastError());
    return TRUE;

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

    TRACE("(%s, %d, %p, 0x%x)\n", debugstr_w(pName), level, pDriverInfo, dwFileCopyFlags);
    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if ((dwFileCopyFlags & ~APD_COPY_FROM_DIRECTORY) != APD_COPY_ALL_FILES) {
        TRACE("Flags 0x%x ignored (using APD_COPY_ALL_FILES)\n", dwFileCopyFlags & ~APD_COPY_FROM_DIRECTORY);
    }

    return myAddPrinterDriverEx(level, pDriverInfo, dwFileCopyFlags, TRUE);
}

/*****************************************************
 *  get_backend [internal]
 */
static const PRINTPROVIDOR * get_backend(void)
{
    static const PRINTPROVIDOR backend = {
        NULL,   /* fpOpenPrinter */
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
        NULL,   /* fpEnumPrintProcessors */
        NULL,   /* fpGetPrintProcessorDirectory */
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
        NULL,   /* fpClosePrinter */
        NULL,   /* fpAddForm */
        NULL,   /* fpDeleteForm */
        NULL,   /* fpGetForm */
        NULL,   /* fpSetForm */
        NULL,   /* fpEnumForms */
        NULL,   /* fpEnumMonitors */
        NULL,   /* fpEnumPorts */
        NULL,   /* fpAddPort */
        NULL,   /* fpConfigurePort */
        NULL,   /* fpDeletePort */
        NULL,   /* fpCreatePrinterIC */
        NULL,   /* fpPlayGdiScriptOnPrinterIC */
        NULL,   /* fpDeletePrinterIC */
        NULL,   /* fpAddPrinterConnection */
        NULL,   /* fpDeletePrinterConnection */
        NULL,   /* fpPrinterMessageBox */
        NULL,   /* fpAddMonitor */
        NULL,   /* fpDeleteMonitor */
        NULL,   /* fpResetPrinter */
        NULL,   /* fpGetPrinterDriverEx */
        NULL,   /* fpFindFirstPrinterChangeNotification */
        NULL,   /* fpFindClosePrinterChangeNotification */
        NULL,   /* fpAddPortEx */
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
        NULL,   /* fpXcvData */
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
    TRACE("=> %p\n", &backend);
    return &backend;

}

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;           /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinstDLL );
            LOCALSPL_hInstance = hinstDLL;
            pp = get_backend();
            break;
    }
    return TRUE;
}


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

    TRACE("(%p, %u, %s)\n", pPrintProvidor, cbPrintProvidor, debugstr_w(pFullRegistryPath));
    memcpy(pPrintProvidor, pp, (cbPrintProvidor < sizeof(PRINTPROVIDOR)) ? cbPrintProvidor : sizeof(PRINTPROVIDOR));

    return TRUE;
}
