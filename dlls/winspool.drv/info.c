/*
 * WINSPOOL functions
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 * Copyright 1999, 2000 Huw D M Davies
 * Copyright 2001 Marcus Meissner
 * Copyright 2005, 2006, 2007 Detlef Riekenberg
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <signal.h>
#ifdef HAVE_CUPS_CUPS_H
# include <cups/cups.h>
# ifndef SONAME_LIBCUPS
#  define SONAME_LIBCUPS "libcups.so"
# endif
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "wine/library.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "winspool.h"
#include "winternl.h"
#include "wine/windef16.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "winnls.h"

#include "ddk/winsplp.h"
#include "wspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(winspool);

/* ############################### */

static CRITICAL_SECTION monitor_handles_cs;
static CRITICAL_SECTION_DEBUG monitor_handles_cs_debug = 
{
    0, 0, &monitor_handles_cs,
    { &monitor_handles_cs_debug.ProcessLocksList, &monitor_handles_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": monitor_handles_cs") }
};
static CRITICAL_SECTION monitor_handles_cs = { &monitor_handles_cs_debug, -1, 0, 0, 0, 0 };


static CRITICAL_SECTION printer_handles_cs;
static CRITICAL_SECTION_DEBUG printer_handles_cs_debug = 
{
    0, 0, &printer_handles_cs,
    { &printer_handles_cs_debug.ProcessLocksList, &printer_handles_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": printer_handles_cs") }
};
static CRITICAL_SECTION printer_handles_cs = { &printer_handles_cs_debug, -1, 0, 0, 0, 0 };

/* ############################### */

typedef struct {
    struct list     entry;
    LPWSTR          name;
    LPWSTR          dllname;
    PMONITORUI      monitorUI;
    LPMONITOR       monitor;
    HMODULE         hdll;
    DWORD           refcount;
    DWORD           dwMonitorSize;
} monitor_t;

typedef struct {
    DWORD job_id;
    HANDLE hf;
} started_doc_t;

typedef struct {
    struct list jobs;
    LONG ref;
} jobqueue_t;

typedef struct {
    LPWSTR name;
    LPWSTR printername;
    monitor_t *pm;
    HANDLE hXcv;
    jobqueue_t *queue;
    started_doc_t *doc;
} opened_printer_t;

typedef struct {
    struct list entry;
    DWORD job_id;
    WCHAR *filename;
    WCHAR *document_title;
} job_t;


typedef struct {
    LPCWSTR  envname;
    LPCWSTR  subdir;
    DWORD    driverversion;
    LPCWSTR  versionregpath;
    LPCWSTR  versionsubdir;
} printenv_t;

/* ############################### */

static struct list monitor_handles = LIST_INIT( monitor_handles );
static monitor_t * pm_localport;

static opened_printer_t **printer_handles;
static int nb_printer_handles;
static LONG next_job_id = 1;

static DWORD (WINAPI *GDI_CallDeviceCapabilities16)( LPCSTR lpszDevice, LPCSTR lpszPort,
                                                     WORD fwCapability, LPSTR lpszOutput,
                                                     LPDEVMODEA lpdm );
static INT (WINAPI *GDI_CallExtDeviceMode16)( HWND hwnd, LPDEVMODEA lpdmOutput,
                                              LPSTR lpszDevice, LPSTR lpszPort,
                                              LPDEVMODEA lpdmInput, LPSTR lpszProfile,
                                              DWORD fwMode );

static const WCHAR DriversW[] = { 'S','y','s','t','e','m','\\',
                                  'C','u', 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'c','o','n','t','r','o','l','\\',
                                  'P','r','i','n','t','\\',
                                  'E','n','v','i','r','o','n','m','e','n','t','s','\\',
                                  '%','s','\\','D','r','i','v','e','r','s','%','s',0 };

static const WCHAR MonitorsW[] =  { 'S','y','s','t','e','m','\\',
                                  'C','u', 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'C','o','n','t','r','o','l','\\',
                                  'P','r','i','n','t','\\',
                                  'M','o','n','i','t','o','r','s','\\',0};

static const WCHAR PrintersW[] = {'S','y','s','t','e','m','\\',
                                  'C','u', 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'C','o','n','t','r','o','l','\\',
                                  'P','r','i','n','t','\\',
                                  'P','r','i','n','t','e','r','s',0};

static const WCHAR LocalPortW[] = {'L','o','c','a','l',' ','P','o','r','t',0};

static const WCHAR user_default_reg_key[] = { 'S','o','f','t','w','a','r','e','\\',
                                              'M','i','c','r','o','s','o','f','t','\\',
                                              'W','i','n','d','o','w','s',' ','N','T','\\',
                                              'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                              'W','i','n','d','o','w','s',0};

static const WCHAR user_printers_reg_key[] = { 'S','o','f','t','w','a','r','e','\\',
                                               'M','i','c','r','o','s','o','f','t','\\',
                                               'W','i','n','d','o','w','s',' ','N','T','\\',
                                               'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                               'D','e','v','i','c','e','s',0};

static const WCHAR WinNT_CV_PortsW[] = {'S','o','f','t','w','a','r','e','\\',
                                        'M','i','c','r','o','s','o','f','t','\\',
                                        'W','i','n','d','o','w','s',' ','N','T','\\',
                                        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                        'P','o','r','t','s',0};

static const WCHAR DefaultEnvironmentW[] = {'W','i','n','e',0};
static const WCHAR envname_win40W[] = {'W','i','n','d','o','w','s',' ','4','.','0',0};
static const WCHAR envname_x86W[] =   {'W','i','n','d','o','w','s',' ','N','T',' ','x','8','6',0};
static const WCHAR subdir_win40W[] = {'w','i','n','4','0',0};
static const WCHAR subdir_x86W[] =   {'w','3','2','x','8','6',0};
static const WCHAR Version3_RegPathW[] = {'\\','V','e','r','s','i','o','n','-','3',0};
static const WCHAR Version3_SubdirW[] = {'\\','3',0};

static const WCHAR spooldriversW[] = {'\\','s','p','o','o','l','\\','d','r','i','v','e','r','s','\\',0};
static const WCHAR spoolprtprocsW[] = {'\\','s','p','o','o','l','\\','p','r','t','p','r','o','c','s','\\',0};

static const WCHAR Configuration_FileW[] = {'C','o','n','f','i','g','u','r','a','t',
				      'i','o','n',' ','F','i','l','e',0};
static const WCHAR DatatypeW[] = {'D','a','t','a','t','y','p','e',0};
static const WCHAR Data_FileW[] = {'D','a','t','a',' ','F','i','l','e',0};
static const WCHAR Default_DevModeW[] = {'D','e','f','a','u','l','t',' ','D','e','v',
				   'M','o','d','e',0};
static const WCHAR Dependent_FilesW[] = {'D','e','p','e','n','d','e','n','t',' ','F',
				   'i','l','e','s',0};
static const WCHAR DescriptionW[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR DriverW[] = {'D','r','i','v','e','r',0};
static const WCHAR Help_FileW[] = {'H','e','l','p',' ','F','i','l','e',0};
static const WCHAR LocationW[] = {'L','o','c','a','t','i','o','n',0};
static const WCHAR MonitorW[] = {'M','o','n','i','t','o','r',0};
static const WCHAR MonitorUIW[] = {'M','o','n','i','t','o','r','U','I',0};
static const WCHAR NameW[] = {'N','a','m','e',0};
static const WCHAR ParametersW[] = {'P','a','r','a','m','e','t','e','r','s',0};
static const WCHAR PortW[] = {'P','o','r','t',0};
static const WCHAR bs_Ports_bsW[] = {'\\','P','o','r','t','s','\\',0};
static const WCHAR Print_ProcessorW[] = {'P','r','i','n','t',' ','P','r','o','c','e',
				   's','s','o','r',0};
static const WCHAR Printer_DriverW[] = {'P','r','i','n','t','e','r',' ','D','r','i',
				  'v','e','r',0};
static const WCHAR PrinterDriverDataW[] = {'P','r','i','n','t','e','r','D','r','i',
				     'v','e','r','D','a','t','a',0};
static const WCHAR Separator_FileW[] = {'S','e','p','a','r','a','t','o','r',' ','F',
				  'i','l','e',0};
static const WCHAR Share_NameW[] = {'S','h','a','r','e',' ','N','a','m','e',0};
static const WCHAR WinPrintW[] = {'W','i','n','P','r','i','n','t',0};
static const WCHAR deviceW[]  = {'d','e','v','i','c','e',0};
static const WCHAR devicesW[] = {'d','e','v','i','c','e','s',0};
static const WCHAR windowsW[] = {'w','i','n','d','o','w','s',0};
static const WCHAR emptyStringW[] = {0};
static const WCHAR XcvMonitorW[] = {',','X','c','v','M','o','n','i','t','o','r',' ',0};
static const WCHAR XcvPortW[] = {',','X','c','v','P','o','r','t',' ',0};

static const WCHAR May_Delete_Value[] = {'W','i','n','e','M','a','y','D','e','l','e','t','e','M','e',0};

static const WCHAR CUPS_Port[] = {'C','U','P','S',':',0};
static const WCHAR FILE_Port[] = {'F','I','L','E',':',0};
static const WCHAR LPR_Port[] = {'L','P','R',':',0};

static const WCHAR default_doc_title[] = {'L','o','c','a','l',' ','D','o','w','n','l','e','v','e','l',' ',
                                          'D','o','c','u','m','e','n','t',0};


/*****************************************************************************
 *   WINSPOOL_SHRegDeleteKey
 *
 *   Recursively delete subkeys.
 *   Cut & paste from shlwapi.
 *
 */
static DWORD WINSPOOL_SHDeleteKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
  DWORD dwRet, dwKeyCount = 0, dwMaxSubkeyLen = 0, dwSize, i;
  WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
  HKEY hSubKey = 0;

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
  if(!dwRet)
  {
    /* Find how many subkeys there are */
    dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             &dwMaxSubkeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if(!dwRet)
    {
      dwMaxSubkeyLen++;
      if (dwMaxSubkeyLen > sizeof(szNameBuf)/sizeof(WCHAR))
        /* Name too big: alloc a buffer for it */
        lpszName = HeapAlloc(GetProcessHeap(), 0, dwMaxSubkeyLen*sizeof(WCHAR));

      if(!lpszName)
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
      else
      {
        /* Recursively delete all the subkeys */
        for(i = 0; i < dwKeyCount && !dwRet; i++)
        {
          dwSize = dwMaxSubkeyLen;
          dwRet = RegEnumKeyExW(hSubKey, i, lpszName, &dwSize, NULL, NULL, NULL, NULL);
          if(!dwRet)
            dwRet = WINSPOOL_SHDeleteKeyW(hSubKey, lpszName);
        }

        if (lpszName != szNameBuf)
          HeapFree(GetProcessHeap(), 0, lpszName); /* Free buffer if allocated */
      }
    }

    RegCloseKey(hSubKey);
    if(!dwRet)
      dwRet = RegDeleteKeyW(hKey, lpszSubKey);
  }
  return dwRet;
}


/******************************************************************
 *  validate the user-supplied printing-environment [internal]
 *
 * PARAMS
 *  env  [I] PTR to Environment-String or NULL
 *
 * RETURNS
 *  Failure:  NULL
 *  Success:  PTR to printenv_t
 *
 * NOTES
 *  An empty string is handled the same way as NULL.
 *  SetLastEror(ERROR_INVALID_ENVIRONMENT) is called on Failure
 *  
 */

static const  printenv_t * validate_envW(LPCWSTR env)
{
    static const printenv_t env_x86 =   {envname_x86W, subdir_x86W,
                                         3, Version3_RegPathW, Version3_SubdirW};
    static const printenv_t env_win40 = {envname_win40W, subdir_win40W,
                                         0, emptyStringW, emptyStringW};
    static const printenv_t * const all_printenv[]={&env_x86, &env_win40};

    const printenv_t *result = NULL;
    unsigned int i;

    TRACE("testing %s\n", debugstr_w(env));
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
    TRACE("using %p: %s\n", result, debugstr_w(result ? result->envname : NULL));

    return result;
}


/* RtlCreateUnicodeStringFromAsciiz will return an empty string in the buffer
   if passed a NULL string. This returns NULLs to the result. 
*/
static inline PWSTR asciitounicode( UNICODE_STRING * usBufferPtr, LPCSTR src )
{
    if ( (src) )
    {
        RtlCreateUnicodeStringFromAsciiz(usBufferPtr, src);
        return usBufferPtr->Buffer;
    }
    usBufferPtr->Buffer = NULL; /* so that RtlFreeUnicodeString won't barf */
    return NULL;
}
            
static LPWSTR strdupW(LPCWSTR p)
{
    LPWSTR ret;
    DWORD len;

    if(!p) return NULL;
    len = (strlenW(p) + 1) * sizeof(WCHAR);
    ret = HeapAlloc(GetProcessHeap(), 0, len);
    memcpy(ret, p, len);
    return ret;
}

static LPSTR strdupWtoA( LPCWSTR str )
{
    LPSTR ret;
    INT len;

    if (!str) return NULL;
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
    ret = HeapAlloc( GetProcessHeap(), 0, len );
    if(ret) WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

/* Returns the number of bytes in an ansi \0\0 terminated string (multi_sz).
   The result includes all \0s (specifically the last two). */
static int multi_sz_lenA(const char *str)
{
    const char *ptr = str;
    if(!str) return 0;
    do
    {
        ptr += lstrlenA(ptr) + 1;
    } while(*ptr);

    return ptr - str + 1;
}

static void
WINSPOOL_SetDefaultPrinter(const char *devname, const char *name, BOOL force) {
    char qbuf[200];

    /* If forcing, or no profile string entry for device yet, set the entry
     *
     * The always change entry if not WINEPS yet is discussable.
     */
    if (force								||
	!GetProfileStringA("windows","device","*",qbuf,sizeof(qbuf))	||
	!strcmp(qbuf,"*")						||
	!strstr(qbuf,"WINEPS.DRV")
    ) {
    	char *buf = HeapAlloc(GetProcessHeap(),0,strlen(name)+strlen(devname)+strlen(",WINEPS.DRV,LPR:")+1);
        HKEY hkey;

	sprintf(buf,"%s,WINEPS.DRV,LPR:%s",devname,name);
	WriteProfileStringA("windows","device",buf);
        if(RegCreateKeyW(HKEY_CURRENT_USER, user_default_reg_key, &hkey) == ERROR_SUCCESS) {
            RegSetValueExA(hkey, "Device", 0, REG_SZ, (LPBYTE)buf, strlen(buf) + 1);
            RegCloseKey(hkey);
        }
	HeapFree(GetProcessHeap(),0,buf);
    }
}

static BOOL add_printer_driver(const char *name)
{
    DRIVER_INFO_3A di3a;

    static char driver_path[]       = "wineps16",
                data_file[]         = "<datafile?>",
                config_file[]       = "wineps16",
                help_file[]         = "<helpfile?>",
                dep_file[]          = "<dependent files?>\0",
                monitor_name[]      = "<monitor name?>",
                default_data_type[] = "RAW";

    di3a.cVersion = (GetVersion() & 0x80000000) ? 0 : 3; /* FIXME: add 1, 2 */
    di3a.pName            = (char *)name;
    di3a.pEnvironment     = NULL;      /* NULL means auto */
    di3a.pDriverPath      = driver_path;
    di3a.pDataFile        = data_file;
    di3a.pConfigFile      = config_file;
    di3a.pHelpFile        = help_file;
    di3a.pDependentFiles  = dep_file;
    di3a.pMonitorName     = monitor_name;
    di3a.pDefaultDataType = default_data_type;

    if (!AddPrinterDriverA(NULL, 3, (LPBYTE)&di3a))
    {
        ERR("Failed adding driver (%d)\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

#ifdef HAVE_CUPS_CUPS_H
static typeof(cupsGetDests)  *pcupsGetDests;
static typeof(cupsGetPPD)    *pcupsGetPPD;
static typeof(cupsPrintFile) *pcupsPrintFile;
static void *cupshandle;

static BOOL CUPS_LoadPrinters(void)
{
    int	                  i, nrofdests;
    BOOL                  hadprinter = FALSE;
    cups_dest_t          *dests;
    PRINTER_INFO_2A       pinfo2a;
    char   *port,*devline;
    HKEY hkeyPrinter, hkeyPrinters, hkey;

    cupshandle = wine_dlopen(SONAME_LIBCUPS, RTLD_NOW, NULL, 0);
    if (!cupshandle) 
	return FALSE;
    TRACE("loaded %s\n", SONAME_LIBCUPS);

#define DYNCUPS(x) 					\
    	p##x = wine_dlsym(cupshandle, #x, NULL,0);	\
	if (!p##x) return FALSE;

    DYNCUPS(cupsGetPPD);
    DYNCUPS(cupsGetDests);
    DYNCUPS(cupsPrintFile);
#undef DYNCUPS

    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }

    nrofdests = pcupsGetDests(&dests);
    TRACE("Found %d CUPS %s:\n", nrofdests, (nrofdests == 1) ? "printer" : "printers");
    for (i=0;i<nrofdests;i++) {
        port = HeapAlloc(GetProcessHeap(),0,strlen("LPR:")+strlen(dests[i].name)+1);
        sprintf(port,"LPR:%s",dests[i].name);
	devline=HeapAlloc(GetProcessHeap(),0,sizeof("WINEPS.DRV,")+strlen(port));
	sprintf(devline,"WINEPS.DRV,%s",port);
	WriteProfileStringA("devices",dests[i].name,devline);
        if(RegCreateKeyW(HKEY_CURRENT_USER, user_printers_reg_key, &hkey) == ERROR_SUCCESS) {
            RegSetValueExA(hkey, dests[i].name, 0, REG_SZ, (LPBYTE)devline, strlen(devline) + 1);
            RegCloseKey(hkey);
        }
	HeapFree(GetProcessHeap(),0,devline);

        TRACE("Printer %d: %s\n", i, dests[i].name);
        if(RegOpenKeyA(hkeyPrinters, dests[i].name, &hkeyPrinter) == ERROR_SUCCESS) {
            /* Printer already in registry, delete the tag added in WINSPOOL_LoadSystemPrinters
               and continue */
            TRACE("Printer already exists\n");
            RegDeleteValueW(hkeyPrinter, May_Delete_Value);
            RegCloseKey(hkeyPrinter);
        } else {
            static CHAR data_type[] = "RAW",
                    print_proc[]    = "WinPrint",
                    comment[]       = "WINEPS Printer using CUPS",
                    location[]      = "<physical location of printer>",
                    params[]        = "<parameters?>",
                    share_name[]    = "<share name?>",
                    sep_file[]      = "<sep file?>";

            add_printer_driver(dests[i].name);

            memset(&pinfo2a,0,sizeof(pinfo2a));
            pinfo2a.pPrinterName    = dests[i].name;
            pinfo2a.pDatatype       = data_type;
            pinfo2a.pPrintProcessor = print_proc;
            pinfo2a.pDriverName     = dests[i].name;
            pinfo2a.pComment        = comment;
            pinfo2a.pLocation       = location;
            pinfo2a.pPortName       = port;
            pinfo2a.pParameters     = params;
            pinfo2a.pShareName      = share_name;
            pinfo2a.pSepFile        = sep_file;

            if (!AddPrinterA(NULL,2,(LPBYTE)&pinfo2a)) {
                if (GetLastError() != ERROR_PRINTER_ALREADY_EXISTS)
                    ERR("printer '%s' not added by AddPrinterA (error %d)\n",dests[i].name,GetLastError());
            }
        }
	HeapFree(GetProcessHeap(),0,port);

        hadprinter = TRUE;
        if (dests[i].is_default)
            WINSPOOL_SetDefaultPrinter(dests[i].name, dests[i].name, TRUE);
    }
    RegCloseKey(hkeyPrinters);
    return hadprinter;
}
#endif

static BOOL
PRINTCAP_ParseEntry(const char *pent, BOOL isfirst) {
    PRINTER_INFO_2A	pinfo2a;
    char		*e,*s,*name,*prettyname,*devname;
    BOOL		ret = FALSE, set_default = FALSE;
    char                *port,*devline,*env_default;
    HKEY                hkeyPrinter, hkeyPrinters, hkey;

    while (isspace(*pent)) pent++;
    s = strchr(pent,':');
    if(s) *s='\0';
    name = HeapAlloc(GetProcessHeap(), 0, strlen(pent) + 1);
    strcpy(name,pent);
    if(s) {
        *s=':';
        pent = s;
    } else
        pent = "";

    TRACE("name=%s entry=%s\n",name, pent);

    if(ispunct(*name)) { /* a tc entry, not a real printer */
        TRACE("skipping tc entry\n");
        goto end;
    }

    if(strstr(pent,":server")) { /* server only version so skip */
        TRACE("skipping server entry\n");
        goto end;
    }

    /* Determine whether this is a postscript printer. */

    ret = TRUE;
    env_default = getenv("PRINTER");
    prettyname = name;
    /* Get longest name, usually the one at the right for later display. */
    while((s=strchr(prettyname,'|'))) {
        *s = '\0';
        e = s;
        while(isspace(*--e)) *e = '\0';
        TRACE("\t%s\n", debugstr_a(prettyname));
        if(env_default && !strcasecmp(prettyname, env_default)) set_default = TRUE;
        for(prettyname = s+1; isspace(*prettyname); prettyname++)
            ;
    }
    e = prettyname + strlen(prettyname);
    while(isspace(*--e)) *e = '\0';
    TRACE("\t%s\n", debugstr_a(prettyname));
    if(env_default && !strcasecmp(prettyname, env_default)) set_default = TRUE;

    /* prettyname must fit into the dmDeviceName member of DEVMODE struct,
     * if it is too long, we use it as comment below. */
    devname = prettyname;
    if (strlen(devname)>=CCHDEVICENAME-1)
	 devname = name;
    if (strlen(devname)>=CCHDEVICENAME-1) {
        ret = FALSE;
        goto end;
    }

    port = HeapAlloc(GetProcessHeap(),0,strlen("LPR:")+strlen(name)+1);
    sprintf(port,"LPR:%s",name);

    devline=HeapAlloc(GetProcessHeap(),0,sizeof("WINEPS.DRV,")+strlen(port));
    sprintf(devline,"WINEPS.DRV,%s",port);
    WriteProfileStringA("devices",devname,devline);
    if(RegCreateKeyW(HKEY_CURRENT_USER, user_printers_reg_key, &hkey) == ERROR_SUCCESS) {
        RegSetValueExA(hkey, devname, 0, REG_SZ, (LPBYTE)devline, strlen(devline) + 1);
        RegCloseKey(hkey);
    }
    HeapFree(GetProcessHeap(),0,devline);
    
    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	ret = FALSE;
        goto end;
    }
    if(RegOpenKeyA(hkeyPrinters, devname, &hkeyPrinter) == ERROR_SUCCESS) {
        /* Printer already in registry, delete the tag added in WINSPOOL_LoadSystemPrinters
           and continue */
        TRACE("Printer already exists\n");
        RegDeleteValueW(hkeyPrinter, May_Delete_Value);
        RegCloseKey(hkeyPrinter);
    } else {
        static CHAR data_type[]   = "RAW",
                    print_proc[]  = "WinPrint",
                    comment[]     = "WINEPS Printer using LPR",
                    params[]      = "<parameters?>",
                    share_name[]  = "<share name?>",
                    sep_file[]    = "<sep file?>";

        add_printer_driver(devname);

        memset(&pinfo2a,0,sizeof(pinfo2a));
        pinfo2a.pPrinterName    = devname;
        pinfo2a.pDatatype       = data_type;
        pinfo2a.pPrintProcessor = print_proc;
        pinfo2a.pDriverName     = devname;
        pinfo2a.pComment        = comment;
        pinfo2a.pLocation       = prettyname;
        pinfo2a.pPortName       = port;
        pinfo2a.pParameters     = params;
        pinfo2a.pShareName      = share_name;
        pinfo2a.pSepFile        = sep_file;

        if (!AddPrinterA(NULL,2,(LPBYTE)&pinfo2a)) {
            if (GetLastError()!=ERROR_PRINTER_ALREADY_EXISTS)
                ERR("%s not added by AddPrinterA (%d)\n",name,GetLastError());
        }
    }
    RegCloseKey(hkeyPrinters);

    if (isfirst || set_default)
        WINSPOOL_SetDefaultPrinter(devname,name,TRUE);

    HeapFree(GetProcessHeap(), 0, port);
 end:
    HeapFree(GetProcessHeap(), 0, name);
    return ret;
}

static BOOL
PRINTCAP_LoadPrinters(void) {
    BOOL		hadprinter = FALSE;
    char		buf[200];
    FILE		*f;
    char *pent = NULL;
    BOOL had_bash = FALSE;

    f = fopen("/etc/printcap","r");
    if (!f)
	return FALSE;

    while(fgets(buf,sizeof(buf),f)) {
        char *start, *end;

        end=strchr(buf,'\n');
        if (end) *end='\0';
    
        start = buf;
        while(isspace(*start)) start++;
        if(*start == '#' || *start == '\0')
            continue;

        if(pent && !had_bash && *start != ':' && *start != '|') { /* start of new entry, parse the previous one */
	    hadprinter |= PRINTCAP_ParseEntry(pent,!hadprinter);
            HeapFree(GetProcessHeap(),0,pent);
            pent = NULL;
        }

        if (end && *--end == '\\') {
            *end = '\0';
            had_bash = TRUE;
        } else
            had_bash = FALSE;

        if (pent) {
            pent=HeapReAlloc(GetProcessHeap(),0,pent,strlen(pent)+strlen(start)+1);
            strcat(pent,start);
        } else {
            pent=HeapAlloc(GetProcessHeap(),0,strlen(start)+1);
            strcpy(pent,start);
        }

    }
    if(pent) {
        hadprinter |= PRINTCAP_ParseEntry(pent,!hadprinter);
        HeapFree(GetProcessHeap(),0,pent);
    }
    fclose(f);
    return hadprinter;
}

static inline DWORD set_reg_szW(HKEY hkey, const WCHAR *keyname, const WCHAR *value)
{
    if (value)
        return RegSetValueExW(hkey, keyname, 0, REG_SZ, (const BYTE*)value,
                              (lstrlenW(value) + 1) * sizeof(WCHAR));
    else
        return ERROR_FILE_NOT_FOUND;
}

/*****************************************************************************
 * enumerate the local monitors (INTERNAL)
 *
 * returns the needed size (in bytes) for pMonitors
 * and  *lpreturned is set to number of entries returned in pMonitors
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
    len = sizeof(buffer);
    buffer[0] = '\0';

    /* Windows creates the "Monitors"-Key on reboot / start "spooler" */
    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, MonitorsW, &hroot) == ERROR_SUCCESS) {
        /* Scan all Monitor-Registry-Keys */
        while (RegEnumKeyExW(hroot, index, buffer, &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            TRACE("Monitor_%d: %s\n", numentries, debugstr_w(buffer));
            dllsize = sizeof(dllname);
            dllname[0] = '\0';

            /* The Monitor must have a Driver-DLL */
            if (RegOpenKeyExW(hroot, buffer, 0, KEY_READ, &hentry) == ERROR_SUCCESS) {
                if (RegQueryValueExW(hentry, DriverW, NULL, NULL, (LPBYTE) dllname, &dllsize) == ERROR_SUCCESS) {
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
                needed += (len+1) * sizeof(WCHAR);  /* len is lstrlenW(monitorname) */
                if (level > 1) {
                    /* we install and return only monitors for "Windows NT x86" */
                    needed += (lstrlenW(envname_x86W) +1) * sizeof(WCHAR);
                    needed += dllsize;
                }

                /* required size is calculated. Now fill the user-buffer */
                if (pMonitors && (cbBuf >= needed)){
                    mi = (LPMONITOR_INFO_2W) pMonitors;
                    pMonitors += entrysize;

                    TRACE("%p: writing MONITOR_INFO_%dW #%d\n", mi, level, numentries);
                    mi->pName = ptr;
                    lstrcpyW(ptr, buffer);      /* Name of the Monitor */
                    ptr += (len+1);               /* len is lstrlenW(monitorname) */
                    if (level > 1) {
                        mi->pEnvironment = ptr;
                        lstrcpyW(ptr, envname_x86W); /* fixed to "Windows NT x86" */
                        ptr += (lstrlenW(envname_x86W)+1);

                        mi->pDLLName = ptr;
                        lstrcpyW(ptr, dllname);         /* Name of the Driver-DLL */
                        ptr += (dllsize / sizeof(WCHAR));
                    }
                }
            }
            index++;
            len = sizeof(buffer);
            buffer[0] = '\0';
        }
        RegCloseKey(hroot);
    }
    *lpreturned = numentries;
    TRACE("need %d byte for %d entries\n", needed, numentries);
    return needed;
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
    TRACE("%p (refcount: %d) %s\n", pm, pm->refcount, debugstr_w(pm->name));

    EnterCriticalSection(&monitor_handles_cs);

    if (pm->refcount) pm->refcount--;

    if (pm->refcount == 0) {
        list_remove(&pm->entry);
        FreeLibrary(pm->hdll);
        HeapFree(GetProcessHeap(), 0, pm->name);
        HeapFree(GetProcessHeap(), 0, pm->dllname);
        HeapFree(GetProcessHeap(), 0, pm);
    }
    LeaveCriticalSection(&monitor_handles_cs);
}

/******************************************************************
 * monitor_unloadall [internal]
 *
 * release all printmonitors and unload them from memory, when needed 
 */

static void monitor_unloadall(void)
{
    monitor_t * pm;
    monitor_t * next;

    EnterCriticalSection(&monitor_handles_cs);
    /* iterate through the list, with safety against removal */
    LIST_FOR_EACH_ENTRY_SAFE(pm, next, &monitor_handles, monitor_t, entry)
    {
        monitor_unload(pm);
    }
    LeaveCriticalSection(&monitor_handles_cs);
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
        pm = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(monitor_t));
        if (pm == NULL) goto cleanup;
        list_add_tail(&monitor_handles, &pm->entry);
    }
    pm->refcount++;

    if (pm->name == NULL) {
        /* Load the monitor */
        LPMONITOREX pmonitorEx;
        DWORD   len;

        if (name) {
            len = lstrlenW(MonitorsW) + lstrlenW(name) + 2;
            regroot = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        }

        if (regroot) {
            lstrcpyW(regroot, MonitorsW);
            lstrcatW(regroot, name);
            /* Get the Driver from the Registry */
            if (driver == NULL) {
                HKEY    hroot;
                DWORD   namesize;
                if (RegOpenKeyW(HKEY_LOCAL_MACHINE, regroot, &hroot) == ERROR_SUCCESS) {
                    if (RegQueryValueExW(hroot, DriverW, NULL, NULL, NULL,
                                        &namesize) == ERROR_SUCCESS) {
                        driver = HeapAlloc(GetProcessHeap(), 0, namesize);
                        RegQueryValueExW(hroot, DriverW, NULL, NULL, (LPBYTE) driver, &namesize) ;
                    }
                    RegCloseKey(hroot);
                }
            }
        }

        pm->name = strdupW(name);
        pm->dllname = strdupW(driver);

        if ((name && (!regroot || !pm->name)) || !pm->dllname) {
            monitor_unload(pm);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            pm = NULL;
            goto cleanup;
        }

        pm->hdll = LoadLibraryW(driver);
        TRACE("%p: LoadLibrary(%s) => %d\n", pm->hdll, debugstr_w(driver), GetLastError());

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
                TRACE(  "0x%08x: dwMonitorSize (%d)\n",
                        pm->monitorUI->dwMonitorUISize, pm->monitorUI->dwMonitorUISize );

            }
        }

        if (pInitializePrintMonitor && regroot) {
            pmonitorEx = pInitializePrintMonitor(regroot);
            TRACE(  "%p: LPMONITOREX from %s,InitializePrintMonitor(%s)\n",
                    pmonitorEx, debugstr_w(driver), debugstr_w(regroot));

            if (pmonitorEx) {
                pm->dwMonitorSize = pmonitorEx->dwMonitorSize;
                pm->monitor = &(pmonitorEx->Monitor);
            }
        }

        if (pm->monitor) {
            TRACE(  "0x%08x: dwMonitorSize (%d)\n", pm->dwMonitorSize, pm->dwMonitorSize );

        }

        if (!pm->monitor && regroot) {
            if (pInitializePrintMonitor2 != NULL) {
                FIXME("%s,InitializePrintMonitor2 not implemented\n", debugstr_w(driver));
            }
            if (pInitializeMonitorEx != NULL) {
                FIXME("%s,InitializeMonitorEx not implemented\n", debugstr_w(driver));
            }
            if (pInitializeMonitor != NULL) {
                FIXME("%s,InitializeMonitor not implemented\n", debugstr_w(driver));
            }
        }
        if (!pm->monitor && !pm->monitorUI) {
            monitor_unload(pm);
            SetLastError(ERROR_PROC_NOT_FOUND);
            pm = NULL;
        }
    }
cleanup:
    if ((pm_localport ==  NULL) && (pm != NULL) && (lstrcmpW(pm->name, LocalPortW) == 0)) {
        pm->refcount++;
        pm_localport = pm;
    }
    LeaveCriticalSection(&monitor_handles_cs);
    if (driver != dllname) HeapFree(GetProcessHeap(), 0, driver);
    HeapFree(GetProcessHeap(), 0, regroot);
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

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, MonitorsW, &hmonitors) == ERROR_SUCCESS) {
        RegQueryInfoKeyW(hmonitors, NULL, NULL, NULL, &registered, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL);

        TRACE("%d monitors registered\n", registered);

        EnterCriticalSection(&monitor_handles_cs);
        while (id < registered) {
            buffer[0] = '\0';
            RegEnumKeyW(hmonitors, id, buffer, MAX_PATH);
            pm = monitor_load(buffer, NULL);
            if (pm) loaded++;
            id++;
        }
        LeaveCriticalSection(&monitor_handles_cs);
        RegCloseKey(hmonitors);
    }
    TRACE("%d monitors loaded\n", loaded);
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
    LPWSTR  buffer[MAX_PATH];
    HANDLE  hXcv;
    DWORD   len;
    DWORD   res;

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
    if ((pm->monitor) && (pm->monitor->pfnXcvDataPort)) {
        /* building (",XcvMonitor %s",pm->name) not needed yet */
        res = pm->monitor->pfnXcvOpenPort(emptyStringW, SERVER_ACCESS_ADMINISTER, &hXcv);
        TRACE("got %u with %p\n", res, hXcv);
        if (res) {
            res = pm->monitor->pfnXcvDataPort(hXcv, MonitorUIW, NULL, 0, (BYTE *) buffer, sizeof(buffer), &len);
            TRACE("got %u with %s\n", res, debugstr_w((LPWSTR) buffer));
            if (res == ERROR_SUCCESS) pui = monitor_load(NULL, (LPWSTR) buffer);
            pm->monitor->pfnXcvClosePort(hXcv);
        }
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
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, WinNT_CV_PortsW, &hroot) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hroot, portname, NULL, NULL, NULL, &len) == ERROR_SUCCESS) {
            /* found the portname */
            RegCloseKey(hroot);
            return monitor_load(LocalPortW, NULL);
        }
        RegCloseKey(hroot);
    }

    len = MAX_PATH + lstrlenW(bs_Ports_bsW) + lstrlenW(portname) + 1;
    buffer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (buffer == NULL) return NULL;

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, MonitorsW, &hroot) == ERROR_SUCCESS) {
        EnterCriticalSection(&monitor_handles_cs);
        RegQueryInfoKeyW(hroot, NULL, NULL, NULL, &registered, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        while ((pm == NULL) && (id < registered)) {
            buffer[0] = '\0';
            RegEnumKeyW(hroot, id, buffer, MAX_PATH);
            TRACE("testing %s\n", debugstr_w(buffer));
            len = lstrlenW(buffer);
            lstrcatW(buffer, bs_Ports_bsW);
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
    HeapFree(GetProcessHeap(), 0, buffer);
    return pm;
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


    TRACE("(%d, %p, %d, %p)\n", level, pPorts, cbBuf, lpreturned);
    entrysize = (level == 1) ? sizeof(PORT_INFO_1W) : sizeof(PORT_INFO_2W);

    numentries = *lpreturned;       /* this is 0, when we scan the registry */
    needed = entrysize * numentries;
    ptr = (LPWSTR) &pPorts[needed];

    numentries = 0;
    needed = 0;

    LIST_FOR_EACH_ENTRY(pm, &monitor_handles, monitor_t, entry)
    {
        if ((pm->monitor) && (pm->monitor->pfnEnumPorts)) {
            pi_needed = 0;
            pi_returned = 0;
            res = pm->monitor->pfnEnumPorts(NULL, level, pi_buffer, pi_allocated, &pi_needed, &pi_returned);
            if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                /* Do not use HeapReAlloc (we do not need the old data in the buffer) */
                HeapFree(GetProcessHeap(), 0, pi_buffer);
                pi_buffer = HeapAlloc(GetProcessHeap(), 0, pi_needed);
                pi_allocated = (pi_buffer) ? pi_needed : 0;
                res = pm->monitor->pfnEnumPorts(NULL, level, pi_buffer, pi_allocated, &pi_needed, &pi_returned);
            }
            TRACE(  "(%s) got %d with %d (need %d byte for %d entries)\n",
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
                    ptr += (lstrlenW(ptr)+1);
                    if (level > 1) {
                        out->pMonitorName = ptr;
                        lstrcpyW(ptr,  cache->pMonitorName);
                        ptr += (lstrlenW(ptr)+1);

                        out->pDescription = ptr;
                        lstrcpyW(ptr,  cache->pDescription);
                        ptr += (lstrlenW(ptr)+1);
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
    HeapFree(GetProcessHeap(), 0, pi_buffer);

    *lpreturned = numentries;
    TRACE("need %d byte for %d entries\n", needed, numentries);
    return needed;
}

/******************************************************************
 * get_servername_from_name  (internal)
 *
 * for an external server, a copy of the serverpart from the full name is returned
 *
 */
static LPWSTR get_servername_from_name(LPCWSTR name)
{
    LPWSTR  server;
    LPWSTR  ptr;
    WCHAR   buffer[MAX_PATH];
    DWORD   len;

    if (name == NULL) return NULL;
    if ((name[0] != '\\') || (name[1] != '\\')) return NULL;

    server = strdupW(&name[2]);     /* skip over both backslash */
    if (server == NULL) return NULL;

    /* strip '\' and the printername */
    ptr = strchrW(server, '\\');
    if (ptr) ptr[0] = '\0';

    TRACE("found %s\n", debugstr_w(server));

    len = sizeof(buffer)/sizeof(buffer[0]);
    if (GetComputerNameW(buffer, &len)) {
        if (lstrcmpW(buffer, server) == 0) {
            /* The requested Servername is our computername */
            HeapFree(GetProcessHeap(), 0, server);
            return NULL;
        }
    }
    return server;
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
        name = strchrW(&name[2], '\\');
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
 *  get_opened_printer_entry
 *  Get the first place empty in the opened printer table
 *
 * ToDo:
 *  - pDefault is ignored
 */
static HANDLE get_opened_printer_entry(LPCWSTR name, LPPRINTER_DEFAULTSW pDefault)
{
    UINT_PTR handle = nb_printer_handles, i;
    jobqueue_t *queue = NULL;
    opened_printer_t *printer = NULL;
    LPWSTR  servername;
    LPCWSTR printername;
    HKEY    hkeyPrinters;
    HKEY    hkeyPrinter;
    DWORD   len;

    servername = get_servername_from_name(name);
    if (servername) {
        FIXME("server %s not supported\n", debugstr_w(servername));
        HeapFree(GetProcessHeap(), 0, servername);
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

    EnterCriticalSection(&printer_handles_cs);

    for (i = 0; i < nb_printer_handles; i++)
    {
        if (!printer_handles[i])
        {
            if(handle == nb_printer_handles)
                handle = i;
        }
        else
        {
            if(!queue && (name) && !lstrcmpW(name, printer_handles[i]->name))
                queue = printer_handles[i]->queue;
        }
    }

    if (handle >= nb_printer_handles)
    {
        opened_printer_t **new_array;
        if (printer_handles)
            new_array = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, printer_handles,
                                     (nb_printer_handles + 16) * sizeof(*new_array) );
        else
            new_array = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                   (nb_printer_handles + 16) * sizeof(*new_array) );

        if (!new_array)
        {
            handle = 0;
            goto end;
        }
        printer_handles = new_array;
        nb_printer_handles += 16;
    }

    if (!(printer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*printer))))
    {
        handle = 0;
        goto end;
    }


    /* clone the base name. This is NULL for the printserver */
    printer->printername = strdupW(printername);

    /* clone the full name */
    printer->name = strdupW(name);
    if (name && (!printer->name)) {
        handle = 0;
        goto end;
    }

    if (printername) {
        len = sizeof(XcvMonitorW)/sizeof(WCHAR) - 1;
        if (strncmpW(printername, XcvMonitorW, len) == 0) {
            /* OpenPrinter(",XcvMonitor " detected */
            TRACE(",XcvMonitor: %s\n", debugstr_w(&printername[len]));
            printer->pm = monitor_load(&printername[len], NULL);
            if (printer->pm == NULL) {
                SetLastError(ERROR_INVALID_PARAMETER);
                handle = 0;
                goto end;
            }
        }
        else
        {
            len = sizeof(XcvPortW)/sizeof(WCHAR) - 1;
            if (strncmpW( printername, XcvPortW, len) == 0) {
                /* OpenPrinter(",XcvPort " detected */
                TRACE(",XcvPort: %s\n", debugstr_w(&printername[len]));
                printer->pm = monitor_load_by_port(&printername[len]);
                if (printer->pm == NULL) {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    handle = 0;
                    goto end;
                }
            }
        }

        if (printer->pm) {
            if ((printer->pm->monitor) && (printer->pm->monitor->pfnXcvOpenPort)) {
                printer->pm->monitor->pfnXcvOpenPort(&printername[len], pDefault->DesiredAccess, &printer->hXcv);
            }
            if (printer->hXcv == NULL) {
                SetLastError(ERROR_INVALID_PARAMETER);
                handle = 0;
                goto end;
            }
        }
        else
        {
            /* Does the Printer exist? */
            if (RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) != ERROR_SUCCESS) {
                ERR("Can't create Printers key\n");
                handle = 0;
                goto end;
            }
            if (RegOpenKeyW(hkeyPrinters, printername, &hkeyPrinter) != ERROR_SUCCESS) {
                WARN("Printer not found in Registry: %s\n", debugstr_w(printername));
                RegCloseKey(hkeyPrinters);
                SetLastError(ERROR_INVALID_PRINTER_NAME);
                handle = 0;
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

    if(queue)
        printer->queue = queue;
    else
    {
        printer->queue = HeapAlloc(GetProcessHeap(), 0, sizeof(*queue));
        if (!printer->queue) {
            handle = 0;
            goto end;
        }
        list_init(&printer->queue->jobs);
        printer->queue->ref = 0;
    }
    InterlockedIncrement(&printer->queue->ref);

    printer_handles[handle] = printer;
    handle++;
end:
    LeaveCriticalSection(&printer_handles_cs);
    if (!handle && printer) {
        /* Something failed: Free all resources */
        if (printer->hXcv) printer->pm->monitor->pfnXcvClosePort(printer->hXcv);
        monitor_unload(printer->pm);
        HeapFree(GetProcessHeap(), 0, printer->printername);
        HeapFree(GetProcessHeap(), 0, printer->name);
        if (!queue) HeapFree(GetProcessHeap(), 0, printer->queue);
        HeapFree(GetProcessHeap(), 0, printer);
    }

    return (HANDLE)handle;
}

/******************************************************************
 *  get_opened_printer
 *  Get the pointer to the opened printer referred by the handle
 */
static opened_printer_t *get_opened_printer(HANDLE hprn)
{
    UINT_PTR idx = (UINT_PTR)hprn;
    opened_printer_t *ret = NULL;

    EnterCriticalSection(&printer_handles_cs);

    if ((idx <= 0) || (idx > nb_printer_handles))
        goto end;

    ret = printer_handles[idx - 1];
end:
    LeaveCriticalSection(&printer_handles_cs);
    return ret;
}

/******************************************************************
 *  get_opened_printer_name
 *  Get the pointer to the opened printer name referred by the handle
 */
static LPCWSTR get_opened_printer_name(HANDLE hprn)
{
    opened_printer_t *printer = get_opened_printer(hprn);
    if(!printer) return NULL;
    return printer->name;
}

/******************************************************************
 *  WINSPOOL_GetOpenedPrinterRegKey
 *
 */
static DWORD WINSPOOL_GetOpenedPrinterRegKey(HANDLE hPrinter, HKEY *phkey)
{
    LPCWSTR name = get_opened_printer_name(hPrinter);
    DWORD ret;
    HKEY hkeyPrinters;

    if(!name) return ERROR_INVALID_HANDLE;

    if((ret = RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters)) !=
       ERROR_SUCCESS)
        return ret;

    if(RegOpenKeyW(hkeyPrinters, name, phkey) != ERROR_SUCCESS)
    {
        ERR("Can't find opened printer %s in registry\n",
            debugstr_w(name));
        RegCloseKey(hkeyPrinters);
        return ERROR_INVALID_PRINTER_NAME; /* ? */
    }
    RegCloseKey(hkeyPrinters);
    return ERROR_SUCCESS;
}

void WINSPOOL_LoadSystemPrinters(void)
{
    HKEY                hkey, hkeyPrinters;
    HANDLE              hprn;
    DWORD               needed, num, i;
    WCHAR               PrinterName[256];
    BOOL                done = FALSE;

    /* This ensures that all printer entries have a valid Name value.  If causes
       problems later if they don't.  If one is found to be missed we create one
       and set it equal to the name of the key */
    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) == ERROR_SUCCESS) {
        if(RegQueryInfoKeyA(hkeyPrinters, NULL, NULL, NULL, &num, NULL, NULL,
                            NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            for(i = 0; i < num; i++) {
                if(RegEnumKeyW(hkeyPrinters, i, PrinterName, sizeof(PrinterName)) == ERROR_SUCCESS) {
                    if(RegOpenKeyW(hkeyPrinters, PrinterName, &hkey) == ERROR_SUCCESS) {
                        if(RegQueryValueExW(hkey, NameW, 0, 0, 0, &needed) == ERROR_FILE_NOT_FOUND) {
                            set_reg_szW(hkey, NameW, PrinterName);
                        }
                        RegCloseKey(hkey);
                    }
                }
            }
        }
        RegCloseKey(hkeyPrinters);
    }

    /* We want to avoid calling AddPrinter on printers as much as
       possible, because on cups printers this will (eventually) lead
       to a call to cupsGetPPD which takes forever, even with non-cups
       printers AddPrinter takes a while.  So we'll tag all printers that
       were automatically added last time around, if they still exist
       we'll leave them be otherwise we'll delete them. */
    EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &needed, &num);
    if(needed) {
        PRINTER_INFO_5A* pi = HeapAlloc(GetProcessHeap(), 0, needed);
        if(EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 5, (LPBYTE)pi, needed, &needed, &num)) {
            for(i = 0; i < num; i++) {
                if(pi[i].pPortName == NULL || !strncmp(pi[i].pPortName,"CUPS:", 5) || !strncmp(pi[i].pPortName, "LPR:", 4)) {
                    if(OpenPrinterA(pi[i].pPrinterName, &hprn, NULL)) {
                        if(WINSPOOL_GetOpenedPrinterRegKey(hprn, &hkey) == ERROR_SUCCESS) {
                            DWORD dw = 1;
                            RegSetValueExW(hkey, May_Delete_Value, 0, REG_DWORD, (LPBYTE)&dw, sizeof(dw));
                            RegCloseKey(hkey);
                        }
                        ClosePrinter(hprn);
                    }
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, pi);
    }


#ifdef HAVE_CUPS_CUPS_H
    done = CUPS_LoadPrinters();
#endif

    if(!done) /* If we have any CUPS based printers, skip looking for printcap printers */
        PRINTCAP_LoadPrinters();

    /* Now enumerate the list again and delete any printers that a still tagged */
    EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &needed, &num);
    if(needed) {
        PRINTER_INFO_5A* pi = HeapAlloc(GetProcessHeap(), 0, needed);
        if(EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 5, (LPBYTE)pi, needed, &needed, &num)) {
            for(i = 0; i < num; i++) {
                if(pi[i].pPortName == NULL || !strncmp(pi[i].pPortName,"CUPS:", 5) || !strncmp(pi[i].pPortName, "LPR:", 4)) {
                    if(OpenPrinterA(pi[i].pPrinterName, &hprn, NULL)) {
                        BOOL delete_driver = FALSE;
                        if(WINSPOOL_GetOpenedPrinterRegKey(hprn, &hkey) == ERROR_SUCCESS) {
                            DWORD dw, type, size = sizeof(dw);
                            if(RegQueryValueExW(hkey, May_Delete_Value, NULL, &type, (LPBYTE)&dw, &size) == ERROR_SUCCESS) {
                                TRACE("Deleting old printer %s\n", pi[i].pPrinterName);
                                DeletePrinter(hprn);
                                delete_driver = TRUE;
                            }
                            RegCloseKey(hkey);
                        }
                        ClosePrinter(hprn);
                        if(delete_driver)
                            DeletePrinterDriverExA(NULL, NULL, pi[i].pPrinterName, 0, 0);
                    }
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, pi);
    }

    return;

}

/******************************************************************
 *                  get_job
 *
 *  Get the pointer to the specified job.
 *  Should hold the printer_handles_cs before calling.
 */
static job_t *get_job(HANDLE hprn, DWORD JobId)
{
    opened_printer_t *printer = get_opened_printer(hprn);
    job_t *job;

    if(!printer) return NULL;
    LIST_FOR_EACH_ENTRY(job, &printer->queue->jobs, job_t, entry)
    {
        if(job->job_id == JobId)
            return job;
    }
    return NULL;
}

/***********************************************************
 *      DEVMODEcpyAtoW
 */
static LPDEVMODEW DEVMODEcpyAtoW(DEVMODEW *dmW, const DEVMODEA *dmA)
{
    BOOL Formname;
    ptrdiff_t off_formname = (const char *)dmA->dmFormName - (const char *)dmA;
    DWORD size;

    Formname = (dmA->dmSize > off_formname);
    size = dmA->dmSize + CCHDEVICENAME + (Formname ? CCHFORMNAME : 0);
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)dmA->dmDeviceName, -1,
                        dmW->dmDeviceName, CCHDEVICENAME);
    if(!Formname) {
      memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion,
	     dmA->dmSize - CCHDEVICENAME);
    } else {
      memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion,
	     off_formname - CCHDEVICENAME);
      MultiByteToWideChar(CP_ACP, 0, (LPCSTR)dmA->dmFormName, -1,
                          dmW->dmFormName, CCHFORMNAME);
      memcpy(&dmW->dmLogPixels, &dmA->dmLogPixels, dmA->dmSize -
	     (off_formname + CCHFORMNAME));
    }
    dmW->dmSize = size;
    memcpy((char *)dmW + dmW->dmSize, (const char *)dmA + dmA->dmSize,
	   dmA->dmDriverExtra);
    return dmW;
}

/***********************************************************
 *      DEVMODEdupWtoA
 * Creates an ascii copy of supplied devmode on heap
 */
static LPDEVMODEA DEVMODEdupWtoA(HANDLE heap, const DEVMODEW *dmW)
{
    LPDEVMODEA dmA;
    DWORD size;
    BOOL Formname;
    ptrdiff_t off_formname = (const char *)dmW->dmFormName - (const char *)dmW;

    if(!dmW) return NULL;
    Formname = (dmW->dmSize > off_formname);
    size = dmW->dmSize - CCHDEVICENAME - (Formname ? CCHFORMNAME : 0);
    dmA = HeapAlloc(heap, HEAP_ZERO_MEMORY, size + dmW->dmDriverExtra);
    WideCharToMultiByte(CP_ACP, 0, dmW->dmDeviceName, -1,
                        (LPSTR)dmA->dmDeviceName, CCHDEVICENAME, NULL, NULL);
    if(!Formname) {
      memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
	     dmW->dmSize - CCHDEVICENAME * sizeof(WCHAR));
    } else {
      memcpy(&dmA->dmSpecVersion, &dmW->dmSpecVersion,
	     off_formname - CCHDEVICENAME * sizeof(WCHAR));
      WideCharToMultiByte(CP_ACP, 0, dmW->dmFormName, -1,
                          (LPSTR)dmA->dmFormName, CCHFORMNAME, NULL, NULL);
      memcpy(&dmA->dmLogPixels, &dmW->dmLogPixels, dmW->dmSize -
	     (off_formname + CCHFORMNAME * sizeof(WCHAR)));
    }
    dmA->dmSize = size;
    memcpy((char *)dmA + dmA->dmSize, (const char *)dmW + dmW->dmSize,
	   dmW->dmDriverExtra);
    return dmA;
}

/***********************************************************
 *             PRINTER_INFO_2AtoW
 * Creates a unicode copy of PRINTER_INFO_2A on heap
 */
static LPPRINTER_INFO_2W PRINTER_INFO_2AtoW(HANDLE heap, LPPRINTER_INFO_2A piA)
{
    LPPRINTER_INFO_2W piW;
    UNICODE_STRING usBuffer;

    if(!piA) return NULL;
    piW = HeapAlloc(heap, 0, sizeof(*piW));
    memcpy(piW, piA, sizeof(*piW)); /* copy everything first */
    
    piW->pServerName = asciitounicode(&usBuffer,piA->pServerName);
    piW->pPrinterName = asciitounicode(&usBuffer,piA->pPrinterName);
    piW->pShareName = asciitounicode(&usBuffer,piA->pShareName);
    piW->pPortName = asciitounicode(&usBuffer,piA->pPortName);
    piW->pDriverName = asciitounicode(&usBuffer,piA->pDriverName);
    piW->pComment = asciitounicode(&usBuffer,piA->pComment);
    piW->pLocation = asciitounicode(&usBuffer,piA->pLocation);
    piW->pDevMode = piA->pDevMode ? GdiConvertToDevmodeW(piA->pDevMode) : NULL;
    piW->pSepFile = asciitounicode(&usBuffer,piA->pSepFile);
    piW->pPrintProcessor = asciitounicode(&usBuffer,piA->pPrintProcessor);
    piW->pDatatype = asciitounicode(&usBuffer,piA->pDatatype);
    piW->pParameters = asciitounicode(&usBuffer,piA->pParameters);
    return piW;
}

/***********************************************************
 *       FREE_PRINTER_INFO_2W
 * Free PRINTER_INFO_2W and all strings
 */
static void FREE_PRINTER_INFO_2W(HANDLE heap, LPPRINTER_INFO_2W piW)
{
    if(!piW) return;

    HeapFree(heap,0,piW->pServerName);
    HeapFree(heap,0,piW->pPrinterName);
    HeapFree(heap,0,piW->pShareName);
    HeapFree(heap,0,piW->pPortName);
    HeapFree(heap,0,piW->pDriverName);
    HeapFree(heap,0,piW->pComment);
    HeapFree(heap,0,piW->pLocation);
    HeapFree(heap,0,piW->pDevMode);
    HeapFree(heap,0,piW->pSepFile);
    HeapFree(heap,0,piW->pPrintProcessor);
    HeapFree(heap,0,piW->pDatatype);
    HeapFree(heap,0,piW->pParameters);
    HeapFree(heap,0,piW);
    return;
}

/******************************************************************
 *              DeviceCapabilities     [WINSPOOL.@]
 *              DeviceCapabilitiesA    [WINSPOOL.@]
 *
 */
INT WINAPI DeviceCapabilitiesA(LPCSTR pDevice,LPCSTR pPort, WORD cap,
			       LPSTR pOutput, LPDEVMODEA lpdm)
{
    INT ret;

    if (!GDI_CallDeviceCapabilities16)
    {
        GDI_CallDeviceCapabilities16 = (void*)GetProcAddress( GetModuleHandleA("gdi32"),
                                                              (LPCSTR)104 );
        if (!GDI_CallDeviceCapabilities16) return -1;
    }
    ret = GDI_CallDeviceCapabilities16(pDevice, pPort, cap, pOutput, lpdm);

    /* If DC_PAPERSIZE map POINT16s to POINTs */
    if(ret != -1 && cap == DC_PAPERSIZE && pOutput) {
        POINT16 *tmp = HeapAlloc( GetProcessHeap(), 0, ret * sizeof(POINT16) );
        POINT *pt = (POINT *)pOutput;
	INT i;
	memcpy(tmp, pOutput, ret * sizeof(POINT16));
	for(i = 0; i < ret; i++, pt++)
        {
            pt->x = tmp[i].x;
            pt->y = tmp[i].y;
        }
	HeapFree( GetProcessHeap(), 0, tmp );
    }
    return ret;
}


/*****************************************************************************
 *          DeviceCapabilitiesW        [WINSPOOL.@]
 *
 * Call DeviceCapabilitiesA since we later call 16bit stuff anyway
 *
 */
INT WINAPI DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort,
			       WORD fwCapability, LPWSTR pOutput,
			       const DEVMODEW *pDevMode)
{
    LPDEVMODEA dmA = DEVMODEdupWtoA(GetProcessHeap(), pDevMode);
    LPSTR pDeviceA = strdupWtoA(pDevice);
    LPSTR pPortA = strdupWtoA(pPort);
    INT ret;

    if(pOutput && (fwCapability == DC_BINNAMES ||
		   fwCapability == DC_FILEDEPENDENCIES ||
		   fwCapability == DC_PAPERNAMES)) {
      /* These need A -> W translation */
        INT size = 0, i;
	LPSTR pOutputA;
        ret = DeviceCapabilitiesA(pDeviceA, pPortA, fwCapability, NULL,
				  dmA);
	if(ret == -1)
	    return ret;
	switch(fwCapability) {
	case DC_BINNAMES:
	    size = 24;
	    break;
	case DC_PAPERNAMES:
	case DC_FILEDEPENDENCIES:
	    size = 64;
	    break;
	}
	pOutputA = HeapAlloc(GetProcessHeap(), 0, size * ret);
	ret = DeviceCapabilitiesA(pDeviceA, pPortA, fwCapability, pOutputA,
				  dmA);
	for(i = 0; i < ret; i++)
	    MultiByteToWideChar(CP_ACP, 0, pOutputA + (i * size), -1,
				pOutput + (i * size), size);
	HeapFree(GetProcessHeap(), 0, pOutputA);
    } else {
        ret = DeviceCapabilitiesA(pDeviceA, pPortA, fwCapability,
				  (LPSTR)pOutput, dmA);
    }
    HeapFree(GetProcessHeap(),0,pPortA);
    HeapFree(GetProcessHeap(),0,pDeviceA);
    HeapFree(GetProcessHeap(),0,dmA);
    return ret;
}

/******************************************************************
 *              DocumentPropertiesA   [WINSPOOL.@]
 *
 * FIXME: implement DocumentPropertiesA via DocumentPropertiesW, not vice versa
 */
LONG WINAPI DocumentPropertiesA(HWND hWnd,HANDLE hPrinter,
                                LPSTR pDeviceName, LPDEVMODEA pDevModeOutput,
				LPDEVMODEA pDevModeInput,DWORD fMode )
{
    LPSTR lpName = pDeviceName;
    static CHAR port[] = "LPT1:";
    LONG ret;

    TRACE("(%p,%p,%s,%p,%p,%d)\n",
	hWnd,hPrinter,pDeviceName,pDevModeOutput,pDevModeInput,fMode
    );

    if(!pDeviceName) {
        LPCWSTR lpNameW = get_opened_printer_name(hPrinter);
        if(!lpNameW) {
		ERR("no name from hPrinter?\n");
                SetLastError(ERROR_INVALID_HANDLE);
		return -1;
	}
	lpName = strdupWtoA(lpNameW);
    }

    if (!GDI_CallExtDeviceMode16)
    {
        GDI_CallExtDeviceMode16 = (void*)GetProcAddress( GetModuleHandleA("gdi32"),
                                                         (LPCSTR)102 );
        if (!GDI_CallExtDeviceMode16) {
		ERR("No CallExtDeviceMode16?\n");
		return -1;
	}
    }
    ret = GDI_CallExtDeviceMode16(hWnd, pDevModeOutput, lpName, port,
				  pDevModeInput, NULL, fMode);

    if(!pDeviceName)
        HeapFree(GetProcessHeap(),0,lpName);
    return ret;
}


/*****************************************************************************
 *          DocumentPropertiesW (WINSPOOL.@)
 *
 * FIXME: implement DocumentPropertiesA via DocumentPropertiesW, not vice versa
 */
LONG WINAPI DocumentPropertiesW(HWND hWnd, HANDLE hPrinter,
				LPWSTR pDeviceName,
				LPDEVMODEW pDevModeOutput,
				LPDEVMODEW pDevModeInput, DWORD fMode)
{

    LPSTR pDeviceNameA = strdupWtoA(pDeviceName);
    LPDEVMODEA pDevModeInputA = DEVMODEdupWtoA(GetProcessHeap(),pDevModeInput);
    LPDEVMODEA pDevModeOutputA = NULL;
    LONG ret;

    TRACE("(%p,%p,%s,%p,%p,%d)\n",
          hWnd,hPrinter,debugstr_w(pDeviceName),pDevModeOutput,pDevModeInput,
	  fMode);
    if(pDevModeOutput) {
        ret = DocumentPropertiesA(hWnd, hPrinter, pDeviceNameA, NULL, NULL, 0);
	if(ret < 0) return ret;
	pDevModeOutputA = HeapAlloc(GetProcessHeap(), 0, ret);
    }
    ret = DocumentPropertiesA(hWnd, hPrinter, pDeviceNameA, pDevModeOutputA,
			      pDevModeInputA, fMode);
    if(pDevModeOutput) {
        DEVMODEcpyAtoW(pDevModeOutput, pDevModeOutputA);
	HeapFree(GetProcessHeap(),0,pDevModeOutputA);
    }
    if(fMode == 0 && ret > 0)
        ret += (CCHDEVICENAME + CCHFORMNAME);
    HeapFree(GetProcessHeap(),0,pDevModeInputA);
    HeapFree(GetProcessHeap(),0,pDeviceNameA);
    return ret;
}

/******************************************************************
 *              OpenPrinterA        [WINSPOOL.@]
 *
 * See OpenPrinterW.
 *
 */
BOOL WINAPI OpenPrinterA(LPSTR lpPrinterName,HANDLE *phPrinter,
			 LPPRINTER_DEFAULTSA pDefault)
{
    UNICODE_STRING lpPrinterNameW;
    UNICODE_STRING usBuffer;
    PRINTER_DEFAULTSW DefaultW, *pDefaultW = NULL;
    PWSTR pwstrPrinterNameW;
    BOOL ret;

    pwstrPrinterNameW = asciitounicode(&lpPrinterNameW,lpPrinterName);

    if(pDefault) {
        DefaultW.pDatatype = asciitounicode(&usBuffer,pDefault->pDatatype);
	DefaultW.pDevMode = pDefault->pDevMode ? GdiConvertToDevmodeW(pDefault->pDevMode) : NULL;
	DefaultW.DesiredAccess = pDefault->DesiredAccess;
	pDefaultW = &DefaultW;
    }
    ret = OpenPrinterW(pwstrPrinterNameW, phPrinter, pDefaultW);
    if(pDefault) {
        RtlFreeUnicodeString(&usBuffer);
	HeapFree(GetProcessHeap(), 0, DefaultW.pDevMode);
    }
    RtlFreeUnicodeString(&lpPrinterNameW);
    return ret;
}

/******************************************************************
 *              OpenPrinterW        [WINSPOOL.@]
 *
 * Open a Printer / Printserver or a Printer-Object
 *
 * PARAMS
 *  lpPrinterName [I] Name of Printserver, Printer, or Printer-Object
 *  phPrinter     [O] The resulting Handle is stored here
 *  pDefault      [I] PTR to Default Printer Settings or NULL
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
 * BUGS
 *|  Printer-Object not supported
 *|  pDefaults is ignored
 *
 */
BOOL WINAPI OpenPrinterW(LPWSTR lpPrinterName,HANDLE *phPrinter, LPPRINTER_DEFAULTSW pDefault)
{

    TRACE("(%s, %p, %p)\n", debugstr_w(lpPrinterName), phPrinter, pDefault);
    if (pDefault) {
        FIXME("PRINTER_DEFAULTS ignored => %s,%p,0x%08x\n",
        debugstr_w(pDefault->pDatatype), pDefault->pDevMode, pDefault->DesiredAccess);
    }

    if(!phPrinter) {
        /* NT: FALSE with ERROR_INVALID_PARAMETER, 9x: TRUE */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Get the unique handle of the printer or Printserver */
    *phPrinter = get_opened_printer_entry(lpPrinterName, pDefault);
    TRACE("returning %d with %u and %p\n", *phPrinter != NULL, GetLastError(), *phPrinter);
    return (*phPrinter != 0);
}

/******************************************************************
 *              AddMonitorA        [WINSPOOL.@]
 *
 * See AddMonitorW.
 *
 */
BOOL WINAPI AddMonitorA(LPSTR pName, DWORD Level, LPBYTE pMonitors)
{
    LPWSTR  nameW = NULL;
    INT     len;
    BOOL    res;
    LPMONITOR_INFO_2A mi2a;
    MONITOR_INFO_2W mi2w;

    mi2a = (LPMONITOR_INFO_2A) pMonitors;
    TRACE("(%s, %d, %p) :  %s %s %s\n", debugstr_a(pName), Level, pMonitors, 
            mi2a ? debugstr_a(mi2a->pName) : NULL,
            mi2a ? debugstr_a(mi2a->pEnvironment) : NULL,
            mi2a ? debugstr_a(mi2a->pDLLName) : NULL);

    if  (Level != 2) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    /* XP: unchanged, win9x: ERROR_INVALID_ENVIRONMENT */
    if (mi2a == NULL) {
        return FALSE;
    }

    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    memset(&mi2w, 0, sizeof(MONITOR_INFO_2W));
    if (mi2a->pName) {
        len = MultiByteToWideChar(CP_ACP, 0, mi2a->pName, -1, NULL, 0);
        mi2w.pName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, mi2a->pName, -1, mi2w.pName, len);
    }
    if (mi2a->pEnvironment) {
        len = MultiByteToWideChar(CP_ACP, 0, mi2a->pEnvironment, -1, NULL, 0);
        mi2w.pEnvironment = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, mi2a->pEnvironment, -1, mi2w.pEnvironment, len);
    }
    if (mi2a->pDLLName) {
        len = MultiByteToWideChar(CP_ACP, 0, mi2a->pDLLName, -1, NULL, 0);
        mi2w.pDLLName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, mi2a->pDLLName, -1, mi2w.pDLLName, len);
    }

    res = AddMonitorW(nameW, Level, (LPBYTE) &mi2w);

    HeapFree(GetProcessHeap(), 0, mi2w.pName); 
    HeapFree(GetProcessHeap(), 0, mi2w.pEnvironment); 
    HeapFree(GetProcessHeap(), 0, mi2w.pDLLName); 

    HeapFree(GetProcessHeap(), 0, nameW); 
    return (res);
}

/******************************************************************************
 *              AddMonitorW        [WINSPOOL.@]
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
BOOL WINAPI AddMonitorW(LPWSTR pName, DWORD Level, LPBYTE pMonitors)
{
    monitor_t * pm = NULL;
    LPMONITOR_INFO_2W mi2w;
    HKEY    hroot = NULL;
    HKEY    hentry = NULL;
    DWORD   disposition;
    BOOL    res = FALSE;

    mi2w = (LPMONITOR_INFO_2W) pMonitors;
    TRACE("(%s, %d, %p) :  %s %s %s\n", debugstr_w(pName), Level, pMonitors, 
            mi2w ? debugstr_w(mi2w->pName) : NULL,
            mi2w ? debugstr_w(mi2w->pEnvironment) : NULL,
            mi2w ? debugstr_w(mi2w->pDLLName) : NULL);

    if (Level != 2) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    /* XP: unchanged, win9x: ERROR_INVALID_ENVIRONMENT */
    if (mi2w == NULL) {
        return FALSE;
    }

    if (pName && (pName[0])) {
        FIXME("for server %s not implemented\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }


    if (!mi2w->pName || (! mi2w->pName[0])) {
        WARN("pName not valid : %s\n", debugstr_w(mi2w->pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!mi2w->pEnvironment || lstrcmpW(mi2w->pEnvironment, envname_x86W)) {
        WARN("Environment %s requested (we support only %s)\n", 
                debugstr_w(mi2w->pEnvironment), debugstr_w(envname_x86W));
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return FALSE;
    }

    if (!mi2w->pDLLName || (! mi2w->pDLLName[0])) {
        WARN("pDLLName not valid : %s\n", debugstr_w(mi2w->pDLLName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Load and initialize the monitor. SetLastError() is called on failure */
    if ((pm = monitor_load(mi2w->pName, mi2w->pDLLName)) == NULL) {
        return FALSE;
    }
    monitor_unload(pm);

    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, MonitorsW, &hroot) != ERROR_SUCCESS) {
        ERR("unable to create key %s\n", debugstr_w(MonitorsW));
        return FALSE;
    }

    if(RegCreateKeyExW( hroot, mi2w->pName, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_QUERY_VALUE, NULL, &hentry,
                        &disposition) == ERROR_SUCCESS) {

        /* Some installers set options for the port before calling AddMonitor.
           We query the "Driver" entry to verify that the monitor is installed,
           before we return an error.
           When a user installs two print monitors at the same time with the
           same name but with a different driver DLL and a task switch comes
           between RegQueryValueExW and RegSetValueExW, a race condition
           is possible but silently ignored. */

        DWORD   namesize = 0;

        if ((disposition == REG_OPENED_EXISTING_KEY) &&
            (RegQueryValueExW(hentry, DriverW, NULL, NULL, NULL,
                              &namesize) == ERROR_SUCCESS)) {
            TRACE("monitor %s already exists\n", debugstr_w(mi2w->pName));
            /* NT: ERROR_PRINT_MONITOR_ALREADY_INSTALLED (3006)
               9x: ERROR_ALREADY_EXISTS (183) */
            SetLastError(ERROR_PRINT_MONITOR_ALREADY_INSTALLED);
        }
        else
        {
               INT len;
               len = (lstrlenW(mi2w->pDLLName) +1) * sizeof(WCHAR);
               res = (RegSetValueExW(hentry, DriverW, 0,
                      REG_SZ, (LPBYTE) mi2w->pDLLName, len) == ERROR_SUCCESS);
        }
        RegCloseKey(hentry);
    }

    RegCloseKey(hroot);
    return (res);
}

/******************************************************************
 *              DeletePrinterDriverA        [WINSPOOL.@]
 *
 */
BOOL WINAPI DeletePrinterDriverA (LPSTR pName, LPSTR pEnvironment, LPSTR pDriverName)
{
    return DeletePrinterDriverExA(pName, pEnvironment, pDriverName, 0, 0);
}

/******************************************************************
 *              DeletePrinterDriverW        [WINSPOOL.@]
 *
 */
BOOL WINAPI DeletePrinterDriverW (LPWSTR pName, LPWSTR pEnvironment, LPWSTR pDriverName)
{
    return DeletePrinterDriverExW(pName, pEnvironment, pDriverName, 0, 0);
}

/******************************************************************
 *              DeleteMonitorA        [WINSPOOL.@]
 *
 * See DeleteMonitorW.
 *
 */
BOOL WINAPI DeleteMonitorA (LPSTR pName, LPSTR pEnvironment, LPSTR pMonitorName)
{
    LPWSTR  nameW = NULL;
    LPWSTR  EnvironmentW = NULL;
    LPWSTR  MonitorNameW = NULL;
    BOOL    res;
    INT     len;

    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    if (pEnvironment) {
        len = MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, NULL, 0);
        EnvironmentW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, EnvironmentW, len);
    }
    if (pMonitorName) {
        len = MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, NULL, 0);
        MonitorNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, MonitorNameW, len);
    }

    res = DeleteMonitorW(nameW, EnvironmentW, MonitorNameW);

    HeapFree(GetProcessHeap(), 0, MonitorNameW); 
    HeapFree(GetProcessHeap(), 0, EnvironmentW);
    HeapFree(GetProcessHeap(), 0, nameW); 
    return (res);
}

/******************************************************************
 *              DeleteMonitorW        [WINSPOOL.@]
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

BOOL WINAPI DeleteMonitorW (LPWSTR pName, LPWSTR pEnvironment, LPWSTR pMonitorName)
{
    HKEY    hroot = NULL;

    TRACE("(%s, %s, %s)\n",debugstr_w(pName),debugstr_w(pEnvironment),
           debugstr_w(pMonitorName));

    if (pName && (pName[0])) {
        FIXME("for server %s not implemented\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    /*  pEnvironment is ignored in Windows for the local Computer */

    if (!pMonitorName || !pMonitorName[0]) {
        WARN("pMonitorName %s is invalid\n", debugstr_w(pMonitorName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, MonitorsW, &hroot) != ERROR_SUCCESS) {
        ERR("unable to create key %s\n", debugstr_w(MonitorsW));
        return FALSE;
    }

    /* change this, when advapi32.dll/RegDeleteTree is implemented */
    if(WINSPOOL_SHDeleteKeyW(hroot, pMonitorName) == ERROR_SUCCESS) {
        TRACE("monitor %s deleted\n", debugstr_w(pMonitorName));
        RegCloseKey(hroot);
        return TRUE;
    }

    WARN("monitor %s does not exist\n", debugstr_w(pMonitorName));
    RegCloseKey(hroot);

    /* NT: ERROR_UNKNOWN_PRINT_MONITOR (3000), 9x: ERROR_INVALID_PARAMETER (87) */
    SetLastError(ERROR_UNKNOWN_PRINT_MONITOR);
    return (FALSE);
}

/******************************************************************
 *              DeletePortA        [WINSPOOL.@]
 *
 * See DeletePortW.
 *
 */
BOOL WINAPI DeletePortA (LPSTR pName, HWND hWnd, LPSTR pPortName)
{
    LPWSTR  nameW = NULL;
    LPWSTR  portW = NULL;
    INT     len;
    DWORD   res;

    TRACE("(%s, %p, %s)\n", debugstr_a(pName), hWnd, debugstr_a(pPortName));

    /* convert servername to unicode */
    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    /* convert portname to unicode */
    if (pPortName) {
        len = MultiByteToWideChar(CP_ACP, 0, pPortName, -1, NULL, 0);
        portW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pPortName, -1, portW, len);
    }

    res = DeletePortW(nameW, hWnd, portW);
    HeapFree(GetProcessHeap(), 0, nameW);
    HeapFree(GetProcessHeap(), 0, portW);
    return res;
}

/******************************************************************
 *              DeletePortW        [WINSPOOL.@]
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
BOOL WINAPI DeletePortW (LPWSTR pName, HWND hWnd, LPWSTR pPortName)
{
    monitor_t * pm;
    monitor_t * pui;
    DWORD       res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    if (pName && pName[0]) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pPortName) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    /* an empty Portname is Invalid */
    if (!pPortName[0]) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pm = monitor_load_by_port(pPortName);
    if (pm && pm->monitor && pm->monitor->pfnDeletePort) {
        TRACE("Using %s for %s (%p: %s)\n", debugstr_w(pm->name), debugstr_w(pPortName), pm, debugstr_w(pm->dllname));
        res = pm->monitor->pfnDeletePort(pName, hWnd, pPortName);
        TRACE("got %d with %u\n", res, GetLastError());
    }
    else
    {
        pui = monitor_loadui(pm);
        if (pui && pui->monitorUI && pui->monitorUI->pfnDeletePortUI) {
            TRACE("use %s for %s (%p: %s)\n", debugstr_w(pui->name), debugstr_w(pPortName), pui, debugstr_w(pui->dllname));
            res = pui->monitorUI->pfnDeletePortUI(pName, hWnd, pPortName);
            TRACE("got %d with %u\n", res, GetLastError());
        }
        else
        {
            FIXME("not implemented for %s (%p: %s => %p: %s)\n", debugstr_w(pPortName),
                pm, pm ? debugstr_w(pm->dllname) : NULL, pui, pui ? debugstr_w(pui->dllname) : NULL);

            /* XP: ERROR_NOT_SUPPORTED, NT351,9x: ERROR_INVALID_PARAMETER */
            SetLastError(ERROR_NOT_SUPPORTED);
            res = FALSE;
        }
        monitor_unload(pui);
    }
    monitor_unload(pm);

    TRACE("returning %d with %u\n", res, GetLastError());
    return res;
}

/******************************************************************************
 *    SetPrinterW  [WINSPOOL.@]
 */
BOOL WINAPI SetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD Command)
{
    FIXME("(%p, %d, %p, %d): stub\n", hPrinter, Level, pPrinter, Command);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *    WritePrinter  [WINSPOOL.@]
 */
BOOL WINAPI WritePrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf, LPDWORD pcWritten)
{
    opened_printer_t *printer;
    BOOL ret = FALSE;

    TRACE("(%p, %p, %d, %p)\n", hPrinter, pBuf, cbBuf, pcWritten);

    EnterCriticalSection(&printer_handles_cs);
    printer = get_opened_printer(hPrinter);
    if(!printer)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        goto end;
    }

    if(!printer->doc)
    {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        goto end;
    }

    ret = WriteFile(printer->doc->hf, pBuf, cbBuf, pcWritten, NULL);
end:
    LeaveCriticalSection(&printer_handles_cs);
    return ret;
}

/*****************************************************************************
 *          AddFormA  [WINSPOOL.@]
 */
BOOL WINAPI AddFormA(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%p,%d,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddFormW  [WINSPOOL.@]
 */
BOOL WINAPI AddFormW(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%p,%d,%p): stub\n", hPrinter, Level, pForm);
    return 1;
}

/*****************************************************************************
 *          AddJobA  [WINSPOOL.@]
 */
BOOL WINAPI AddJobA(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf, LPDWORD pcbNeeded)
{
    BOOL ret;
    BYTE buf[MAX_PATH * sizeof(WCHAR) + sizeof(ADDJOB_INFO_1W)];
    DWORD needed;

    if(Level != 1) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    ret = AddJobW(hPrinter, Level, buf, sizeof(buf), &needed);

    if(ret) {
        ADDJOB_INFO_1W *addjobW = (ADDJOB_INFO_1W*)buf;
        DWORD len = WideCharToMultiByte(CP_ACP, 0, addjobW->Path, -1, NULL, 0, NULL, NULL);
        *pcbNeeded = len + sizeof(ADDJOB_INFO_1A);
        if(*pcbNeeded > cbBuf) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            ret = FALSE;
        } else {
            ADDJOB_INFO_1A *addjobA = (ADDJOB_INFO_1A*)pData;
            addjobA->JobId = addjobW->JobId;
            addjobA->Path = (char *)(addjobA + 1);
            WideCharToMultiByte(CP_ACP, 0, addjobW->Path, -1, addjobA->Path, len, NULL, NULL);
        }
    }
    return ret;
}

/*****************************************************************************
 *          AddJobW  [WINSPOOL.@]
 */
BOOL WINAPI AddJobW(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf, LPDWORD pcbNeeded)
{
    opened_printer_t *printer;
    job_t *job;
    BOOL ret = FALSE;
    static const WCHAR spool_path[] = {'s','p','o','o','l','\\','P','R','I','N','T','E','R','S','\\',0};
    static const WCHAR fmtW[] = {'%','s','%','0','5','d','.','S','P','L',0};
    WCHAR path[MAX_PATH], filename[MAX_PATH];
    DWORD len;
    ADDJOB_INFO_1W *addjob;

    TRACE("(%p,%d,%p,%d,%p)\n", hPrinter, Level, pData, cbBuf, pcbNeeded);
    
    EnterCriticalSection(&printer_handles_cs);

    printer = get_opened_printer(hPrinter);

    if(!printer) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto end;
    }

    if(Level != 1) {
        SetLastError(ERROR_INVALID_LEVEL);
        goto end;
    }

    job = HeapAlloc(GetProcessHeap(), 0, sizeof(*job));
    if(!job)
        goto end;

    job->job_id = InterlockedIncrement(&next_job_id);

    len = GetSystemDirectoryW(path, sizeof(path) / sizeof(WCHAR));
    if(path[len - 1] != '\\')
        path[len++] = '\\';
    memcpy(path + len, spool_path, sizeof(spool_path));    
    sprintfW(filename, fmtW, path, job->job_id);

    len = strlenW(filename);
    job->filename = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    memcpy(job->filename, filename, (len + 1) * sizeof(WCHAR));
    job->document_title = strdupW(default_doc_title);
    list_add_tail(&printer->queue->jobs, &job->entry);

    *pcbNeeded = (len + 1) * sizeof(WCHAR) + sizeof(*addjob);
    if(*pcbNeeded <= cbBuf) {
        addjob = (ADDJOB_INFO_1W*)pData;
        addjob->JobId = job->job_id;
        addjob->Path = (WCHAR *)(addjob + 1);
        memcpy(addjob->Path, filename, (len + 1) * sizeof(WCHAR));
        ret = TRUE;
    } else 
        SetLastError(ERROR_INSUFFICIENT_BUFFER);

end:
    LeaveCriticalSection(&printer_handles_cs);
    return ret;
}

/*****************************************************************************
 *          GetPrintProcessorDirectoryA  [WINSPOOL.@]
 *
 * Return the PATH for the Print-Processors
 *
 * See GetPrintProcessorDirectoryW.
 *
 *
 */
BOOL WINAPI GetPrintProcessorDirectoryA(LPSTR server, LPSTR env,
                                        DWORD level,  LPBYTE Info,
                                        DWORD cbBuf,  LPDWORD pcbNeeded)
{
    LPWSTR  serverW = NULL;
    LPWSTR  envW = NULL;
    BOOL    ret;
    INT     len;

    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_a(server), 
          debugstr_a(env), level, Info, cbBuf, pcbNeeded);
 

    if (server) {
        len = MultiByteToWideChar(CP_ACP, 0, server, -1, NULL, 0);
        serverW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, server, -1, serverW, len);
    }

    if (env) {
        len = MultiByteToWideChar(CP_ACP, 0, env, -1, NULL, 0);
        envW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, env, -1, envW, len);
    }

    /* NT requires the buffersize from GetPrintProcessorDirectoryW also
       for GetPrintProcessorDirectoryA and WC2MB is done in-place.
     */
    ret = GetPrintProcessorDirectoryW(serverW, envW, level, Info, 
                                      cbBuf, pcbNeeded);

    if (ret) ret = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)Info, -1, (LPSTR)Info,
                                       cbBuf, NULL, NULL) > 0;


    TRACE(" required: 0x%x/%d\n", pcbNeeded ? *pcbNeeded : 0, pcbNeeded ? *pcbNeeded : 0);
    HeapFree(GetProcessHeap(), 0, envW);
    HeapFree(GetProcessHeap(), 0, serverW);
    return ret;
}

/*****************************************************************************
 *          GetPrintProcessorDirectoryW  [WINSPOOL.@]
 *
 * Return the PATH for the Print-Processors
 *
 * PARAMS
 *   server     [I] Servername (NT only) or NULL (local Computer)
 *   env        [I] Printing-Environment (see below) or NULL (Default)
 *   level      [I] Structure-Level (must be 1)
 *   Info       [O] PTR to Buffer that receives the Result
 *   cbBuf      [I] Size of Buffer at "Info"
 *   pcbNeeded  [O] PTR to DWORD that receives the size in Bytes used / 
 *                  required for the Buffer at "Info"
 *
 * RETURNS
 *   Success: TRUE  and in pcbNeeded the Bytes used in Info
 *   Failure: FALSE and in pcbNeeded the Bytes required for Info,
 *   if cbBuf is too small
 * 
 *   Native Values returned in Info on Success:
 *|  NT(Windows NT x86):  "%winsysdir%\\spool\\PRTPROCS\\w32x86" 
 *|  NT(Windows 4.0):     "%winsysdir%\\spool\\PRTPROCS\\win40" 
 *|  win9x(Windows 4.0):  "%winsysdir%" 
 *
 *   "%winsysdir%" is the Value from GetSystemDirectoryW()
 *
 * BUGS
 *  Only NULL or "" is supported for server
 *
 */
BOOL WINAPI GetPrintProcessorDirectoryW(LPWSTR server, LPWSTR env,
                                        DWORD level,  LPBYTE Info,
                                        DWORD cbBuf,  LPDWORD pcbNeeded)
{
    DWORD needed;
    const printenv_t * env_t;

    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_w(server),
            debugstr_w(env), level, Info, cbBuf, pcbNeeded);

    if(server != NULL && server[0]) {
        FIXME("server not supported: %s\n", debugstr_w(server));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    env_t = validate_envW(env);
    if(!env_t) return FALSE;  /* environment invalid or unsupported */

    if(level != 1) {
        WARN("(Level: %d) is ignored in win9x\n", level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    /* GetSystemDirectoryW returns number of WCHAR including the '\0' */
    needed = GetSystemDirectoryW(NULL, 0);
    /* add the Size for the Subdirectories */
    needed += lstrlenW(spoolprtprocsW);
    needed += lstrlenW(env_t->subdir);
    needed *= sizeof(WCHAR);  /* return-value is size in Bytes */

    if(pcbNeeded) *pcbNeeded = needed;
    TRACE ("required: 0x%x/%d\n", needed, needed);
    if (needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    if(pcbNeeded == NULL) {
        /* NT: RPC_X_NULL_REF_POINTER, 9x: ignored */
        WARN("(pcbNeeded == NULL) is ignored in win9x\n");
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }
    if(Info == NULL) {
        /* NT: RPC_X_NULL_REF_POINTER, 9x: ERROR_INVALID_PARAMETER */
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }
    
    GetSystemDirectoryW((LPWSTR) Info, cbBuf/sizeof(WCHAR));
    /* add the Subdirectories */
    lstrcatW((LPWSTR) Info, spoolprtprocsW);
    lstrcatW((LPWSTR) Info, env_t->subdir);
    TRACE(" => %s\n", debugstr_w((LPWSTR) Info));
    return TRUE;
}

/*****************************************************************************
 *          WINSPOOL_OpenDriverReg [internal]
 *
 * opens the registry for the printer drivers depending on the given input
 * variable pEnvironment
 *
 * RETURNS:
 *    the opened hkey on success
 *    NULL on error
 */
static HKEY WINSPOOL_OpenDriverReg( LPCVOID pEnvironment, BOOL unicode)
{   
    HKEY  retval = NULL;
    LPWSTR buffer;
    const printenv_t * env;

    TRACE("(%s, %d)\n",
	  (unicode) ? debugstr_w(pEnvironment) : debugstr_a(pEnvironment), unicode);

    if (!pEnvironment || unicode) {
        /* pEnvironment was NULL or an Unicode-String: use it direct */
        env = validate_envW(pEnvironment);
    }
    else
    {
        /* pEnvironment was an ANSI-String: convert to unicode first */
        LPWSTR  buffer;
        INT len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pEnvironment, -1, NULL, 0);
        buffer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (buffer) MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pEnvironment, -1, buffer, len);
        env = validate_envW(buffer);
        HeapFree(GetProcessHeap(), 0, buffer);
    }
    if (!env) return NULL;

    buffer = HeapAlloc( GetProcessHeap(), 0,
                (strlenW(DriversW) + strlenW(env->envname) + 
                 strlenW(env->versionregpath) + 1) * sizeof(WCHAR));
    if(buffer) {
        wsprintfW(buffer, DriversW, env->envname, env->versionregpath);
        RegCreateKeyW(HKEY_LOCAL_MACHINE, buffer, &retval);
        HeapFree(GetProcessHeap(), 0, buffer);
    }
    return retval;
}

/*****************************************************************************
 *          AddPrinterW  [WINSPOOL.@]
 */
HANDLE WINAPI AddPrinterW(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    PRINTER_INFO_2W *pi = (PRINTER_INFO_2W *) pPrinter;
    LPDEVMODEA dmA;
    LPDEVMODEW dmW;
    HANDLE retval;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDriver, hkeyDrivers;
    LONG size;
    static const WCHAR attributesW[]      = {'A','t','t','r','i','b','u','t','e','s',0},
                       default_devmodeW[] = {'D','e','f','a','u','l','t',' ','D','e','v','M','o','d','e',0},
                       priorityW[]        = {'P','r','i','o','r','i','t','y',0},
                       start_timeW[]      = {'S','t','a','r','t','T','i','m','e',0},
                       statusW[]          = {'S','t','a','t','u','s',0},
                       until_timeW[]      = {'U','n','t','i','l','T','i','m','e',0};

    TRACE("(%s,%d,%p)\n", debugstr_w(pName), Level, pPrinter);

    if(pName != NULL) {
        ERR("pName = %s - unsupported\n", debugstr_w(pName));
	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    if(Level != 2) {
        ERR("Level = %d, unsupported!\n", Level);
	SetLastError(ERROR_INVALID_LEVEL);
	return 0;
    }
    if(!pPrinter) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return 0;
    }
    if(!RegOpenKeyW(hkeyPrinters, pi->pPrinterName, &hkeyPrinter)) {
	if (!RegQueryValueW(hkeyPrinter, attributesW, NULL, NULL)) {
	    SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
	    RegCloseKey(hkeyPrinter);
	    RegCloseKey(hkeyPrinters);
	    return 0;
	}
	RegCloseKey(hkeyPrinter);
    }
    hkeyDrivers = WINSPOOL_OpenDriverReg( NULL, TRUE);
    if(!hkeyDrivers) {
        ERR("Can't create Drivers key\n");
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    if(RegOpenKeyW(hkeyDrivers, pi->pDriverName, &hkeyDriver) !=
       ERROR_SUCCESS) {
        WARN("Can't find driver %s\n", debugstr_w(pi->pDriverName));
	RegCloseKey(hkeyPrinters);
	RegCloseKey(hkeyDrivers);
	SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
	return 0;
    }
    RegCloseKey(hkeyDriver);
    RegCloseKey(hkeyDrivers);

    if(lstrcmpiW(pi->pPrintProcessor, WinPrintW)) {  /* FIXME */
        FIXME("Can't find processor %s\n", debugstr_w(pi->pPrintProcessor));
	SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
	RegCloseKey(hkeyPrinters);
	return 0;
    }

    if(RegCreateKeyW(hkeyPrinters, pi->pPrinterName, &hkeyPrinter) !=
       ERROR_SUCCESS) {
        FIXME("Can't create printer %s\n", debugstr_w(pi->pPrinterName));
	SetLastError(ERROR_INVALID_PRINTER_NAME);
	RegCloseKey(hkeyPrinters);
	return 0;
    }
    RegSetValueExW(hkeyPrinter, attributesW, 0, REG_DWORD,
		   (LPBYTE)&pi->Attributes, sizeof(DWORD));
    set_reg_szW(hkeyPrinter, DatatypeW, pi->pDatatype);

    /* See if we can load the driver.  We may need the devmode structure anyway
     *
     * FIXME:
     * Note that DocumentPropertiesW will briefly try to open the printer we
     * just create to find a DEVMODEA struct (it will use the WINEPS default
     * one in case it is not there, so we are ok).
     */
    size = DocumentPropertiesW(0, 0, pi->pPrinterName, NULL, NULL, 0);

    if(size < 0) {
        FIXME("DocumentPropertiesW on printer %s fails\n", debugstr_w(pi->pPrinterName));
	size = sizeof(DEVMODEW);
    }
    if(pi->pDevMode)
        dmW = pi->pDevMode;
    else
    {
        dmW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        dmW->dmSize = size;
        if (0>DocumentPropertiesW(0,0,pi->pPrinterName,dmW,NULL,DM_OUT_BUFFER))
        {
            WARN("DocumentPropertiesW on printer %s failed!\n", debugstr_w(pi->pPrinterName));
            HeapFree(GetProcessHeap(),0,dmW);
            dmW=NULL;
        }
        else
        {
            /* set devmode to printer name */
            lstrcpynW(dmW->dmDeviceName, pi->pPrinterName, CCHDEVICENAME);
        }
    }

    /* Write DEVMODEA not DEVMODEW into reg.  This is what win9x does
       and we support these drivers.  NT writes DEVMODEW so somehow
       we'll need to distinguish between these when we support NT
       drivers */
    if (dmW)
    {
        dmA = DEVMODEdupWtoA(GetProcessHeap(), dmW);
        RegSetValueExW(hkeyPrinter, default_devmodeW, 0, REG_BINARY,
                       (LPBYTE)dmA, dmA->dmSize + dmA->dmDriverExtra);
        HeapFree(GetProcessHeap(), 0, dmA);
        if(!pi->pDevMode)
            HeapFree(GetProcessHeap(), 0, dmW);
    }
    set_reg_szW(hkeyPrinter, DescriptionW, pi->pComment);
    set_reg_szW(hkeyPrinter, LocationW, pi->pLocation);
    set_reg_szW(hkeyPrinter, NameW, pi->pPrinterName);
    set_reg_szW(hkeyPrinter, ParametersW, pi->pParameters);

    set_reg_szW(hkeyPrinter, PortW, pi->pPortName);
    set_reg_szW(hkeyPrinter, Print_ProcessorW, pi->pPrintProcessor);
    set_reg_szW(hkeyPrinter, Printer_DriverW, pi->pDriverName);
    RegSetValueExW(hkeyPrinter, priorityW, 0, REG_DWORD,
                   (LPBYTE)&pi->Priority, sizeof(DWORD));
    set_reg_szW(hkeyPrinter, Separator_FileW, pi->pSepFile);
    set_reg_szW(hkeyPrinter, Share_NameW, pi->pShareName);
    RegSetValueExW(hkeyPrinter, start_timeW, 0, REG_DWORD,
                   (LPBYTE)&pi->StartTime, sizeof(DWORD));
    RegSetValueExW(hkeyPrinter, statusW, 0, REG_DWORD,
                   (LPBYTE)&pi->Status, sizeof(DWORD));
    RegSetValueExW(hkeyPrinter, until_timeW, 0, REG_DWORD,
                   (LPBYTE)&pi->UntilTime, sizeof(DWORD));

    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(!OpenPrinterW(pi->pPrinterName, &retval, NULL)) {
        ERR("OpenPrinter failing\n");
	return 0;
    }
    return retval;
}

/*****************************************************************************
 *          AddPrinterA  [WINSPOOL.@]
 */
HANDLE WINAPI AddPrinterA(LPSTR pName, DWORD Level, LPBYTE pPrinter)
{
    UNICODE_STRING pNameW;
    PWSTR pwstrNameW;
    PRINTER_INFO_2W *piW;
    PRINTER_INFO_2A *piA = (PRINTER_INFO_2A*)pPrinter;
    HANDLE ret;

    TRACE("(%s,%d,%p): stub\n", debugstr_a(pName), Level, pPrinter);
    if(Level != 2) {
        ERR("Level = %d, unsupported!\n", Level);
	SetLastError(ERROR_INVALID_LEVEL);
	return 0;
    }
    pwstrNameW = asciitounicode(&pNameW,pName);
    piW = PRINTER_INFO_2AtoW(GetProcessHeap(), piA);

    ret = AddPrinterW(pwstrNameW, Level, (LPBYTE)piW);

    FREE_PRINTER_INFO_2W(GetProcessHeap(), piW);
    RtlFreeUnicodeString(&pNameW);
    return ret;
}


/*****************************************************************************
 *          ClosePrinter  [WINSPOOL.@]
 */
BOOL WINAPI ClosePrinter(HANDLE hPrinter)
{
    UINT_PTR i = (UINT_PTR)hPrinter;
    opened_printer_t *printer = NULL;
    BOOL ret = FALSE;

    TRACE("(%p)\n", hPrinter);

    EnterCriticalSection(&printer_handles_cs);

    if ((i > 0) && (i <= nb_printer_handles))
        printer = printer_handles[i - 1];


    if(printer)
    {
        struct list *cursor, *cursor2;

        TRACE("%p: %s (hXcv: %p) for %s (doc: %p)\n", printer->pm,
                debugstr_w(printer->pm ? printer->pm->dllname : NULL),
                printer->hXcv, debugstr_w(printer->name), printer->doc );

        if(printer->doc)
            EndDocPrinter(hPrinter);

        if(InterlockedDecrement(&printer->queue->ref) == 0)
        {
            LIST_FOR_EACH_SAFE(cursor, cursor2, &printer->queue->jobs)
            {
                job_t *job = LIST_ENTRY(cursor, job_t, entry);
                ScheduleJob(hPrinter, job->job_id);
            }
            HeapFree(GetProcessHeap(), 0, printer->queue);
        }
        if (printer->hXcv) printer->pm->monitor->pfnXcvClosePort(printer->hXcv);
        monitor_unload(printer->pm);
        HeapFree(GetProcessHeap(), 0, printer->printername);
        HeapFree(GetProcessHeap(), 0, printer->name);
        HeapFree(GetProcessHeap(), 0, printer);
        printer_handles[i - 1] = NULL;
        ret = TRUE;
    }
    LeaveCriticalSection(&printer_handles_cs);
    return ret;
}

/*****************************************************************************
 *          DeleteFormA  [WINSPOOL.@]
 */
BOOL WINAPI DeleteFormA(HANDLE hPrinter, LPSTR pFormName)
{
    FIXME("(%p,%s): stub\n", hPrinter, pFormName);
    return 1;
}

/*****************************************************************************
 *          DeleteFormW  [WINSPOOL.@]
 */
BOOL WINAPI DeleteFormW(HANDLE hPrinter, LPWSTR pFormName)
{
    FIXME("(%p,%s): stub\n", hPrinter, debugstr_w(pFormName));
    return 1;
}

/*****************************************************************************
 *          DeletePrinter  [WINSPOOL.@]
 */
BOOL WINAPI DeletePrinter(HANDLE hPrinter)
{
    LPCWSTR lpNameW = get_opened_printer_name(hPrinter);
    HKEY hkeyPrinters, hkey;

    if(!lpNameW) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if(RegOpenKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) == ERROR_SUCCESS) {
        WINSPOOL_SHDeleteKeyW(hkeyPrinters, lpNameW);
        RegCloseKey(hkeyPrinters);
    }
    WriteProfileStringW(devicesW, lpNameW, NULL);
    if(RegCreateKeyW(HKEY_CURRENT_USER, user_printers_reg_key, &hkey) == ERROR_SUCCESS) {
        RegDeleteValueW(hkey, lpNameW);
        RegCloseKey(hkey);
    }
    return TRUE;
}

/*****************************************************************************
 *          SetPrinterA  [WINSPOOL.@]
 */
BOOL WINAPI SetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                           DWORD Command)
{
    FIXME("(%p,%d,%p,%d): stub\n",hPrinter,Level,pPrinter,Command);
    return FALSE;
}

/*****************************************************************************
 *          SetJobA  [WINSPOOL.@]
 */
BOOL WINAPI SetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level,
                    LPBYTE pJob, DWORD Command)
{
    BOOL ret;
    LPBYTE JobW;
    UNICODE_STRING usBuffer;

    TRACE("(%p, %d, %d, %p, %d)\n",hPrinter, JobId, Level, pJob, Command);

    /* JobId, pPrinterName, pMachineName, pDriverName, Size, Submitted, Time and TotalPages
       are all ignored by SetJob, so we don't bother copying them */
    switch(Level)
    {
    case 0:
        JobW = NULL;
        break;
    case 1:
      {
        JOB_INFO_1W *info1W = HeapAlloc(GetProcessHeap(), 0, sizeof(*info1W));
        JOB_INFO_1A *info1A = (JOB_INFO_1A*)pJob;

        JobW = (LPBYTE)info1W;
        info1W->pUserName = asciitounicode(&usBuffer, info1A->pUserName);
        info1W->pDocument = asciitounicode(&usBuffer, info1A->pDocument);
        info1W->pDatatype = asciitounicode(&usBuffer, info1A->pDatatype);
        info1W->pStatus = asciitounicode(&usBuffer, info1A->pStatus);
        info1W->Status = info1A->Status;
        info1W->Priority = info1A->Priority;
        info1W->Position = info1A->Position;
        info1W->PagesPrinted = info1A->PagesPrinted;
        break;
      }
    case 2:
      {
        JOB_INFO_2W *info2W = HeapAlloc(GetProcessHeap(), 0, sizeof(*info2W));
        JOB_INFO_2A *info2A = (JOB_INFO_2A*)pJob;

        JobW = (LPBYTE)info2W;
        info2W->pUserName = asciitounicode(&usBuffer, info2A->pUserName);
        info2W->pDocument = asciitounicode(&usBuffer, info2A->pDocument);
        info2W->pNotifyName = asciitounicode(&usBuffer, info2A->pNotifyName);
        info2W->pDatatype = asciitounicode(&usBuffer, info2A->pDatatype);
        info2W->pPrintProcessor = asciitounicode(&usBuffer, info2A->pPrintProcessor);
        info2W->pParameters = asciitounicode(&usBuffer, info2A->pParameters);
        info2W->pDevMode = info2A->pDevMode ? GdiConvertToDevmodeW(info2A->pDevMode) : NULL;
        info2W->pStatus = asciitounicode(&usBuffer, info2A->pStatus);
        info2W->pSecurityDescriptor = info2A->pSecurityDescriptor;
        info2W->Status = info2A->Status;
        info2W->Priority = info2A->Priority;
        info2W->Position = info2A->Position;
        info2W->StartTime = info2A->StartTime;
        info2W->UntilTime = info2A->UntilTime;
        info2W->PagesPrinted = info2A->PagesPrinted;
        break;
      }
    case 3:
        JobW = HeapAlloc(GetProcessHeap(), 0, sizeof(JOB_INFO_3));
        memcpy(JobW, pJob, sizeof(JOB_INFO_3));
        break;
    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    ret = SetJobW(hPrinter, JobId, Level, JobW, Command);

    switch(Level)
    {
    case 1:
      {
        JOB_INFO_1W *info1W = (JOB_INFO_1W*)JobW;
        HeapFree(GetProcessHeap(), 0, info1W->pUserName);
        HeapFree(GetProcessHeap(), 0, info1W->pDocument); 
        HeapFree(GetProcessHeap(), 0, info1W->pDatatype);
        HeapFree(GetProcessHeap(), 0, info1W->pStatus);
        break;
      }
    case 2:
      {
        JOB_INFO_2W *info2W = (JOB_INFO_2W*)JobW;
        HeapFree(GetProcessHeap(), 0, info2W->pUserName);
        HeapFree(GetProcessHeap(), 0, info2W->pDocument); 
        HeapFree(GetProcessHeap(), 0, info2W->pNotifyName);
        HeapFree(GetProcessHeap(), 0, info2W->pDatatype);
        HeapFree(GetProcessHeap(), 0, info2W->pPrintProcessor);
        HeapFree(GetProcessHeap(), 0, info2W->pParameters);
        HeapFree(GetProcessHeap(), 0, info2W->pDevMode);
        HeapFree(GetProcessHeap(), 0, info2W->pStatus);
        break;
      }
    }
    HeapFree(GetProcessHeap(), 0, JobW);

    return ret;
}

/*****************************************************************************
 *          SetJobW  [WINSPOOL.@]
 */
BOOL WINAPI SetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level,
                    LPBYTE pJob, DWORD Command)
{
    BOOL ret = FALSE;
    job_t *job;

    TRACE("(%p, %d, %d, %p, %d)\n", hPrinter, JobId, Level, pJob, Command);
    FIXME("Ignoring everything other than document title\n");

    EnterCriticalSection(&printer_handles_cs);
    job = get_job(hPrinter, JobId);
    if(!job)
        goto end;

    switch(Level)
    {
    case 0:
        break;
    case 1:
      {
        JOB_INFO_1W *info1 = (JOB_INFO_1W*)pJob;
        HeapFree(GetProcessHeap(), 0, job->document_title);
        job->document_title = strdupW(info1->pDocument);
        break;
      }
    case 2:
      {
        JOB_INFO_2W *info2 = (JOB_INFO_2W*)pJob;
        HeapFree(GetProcessHeap(), 0, job->document_title);
        job->document_title = strdupW(info2->pDocument);
        break;
      }
    case 3:
        break;
    default:
        SetLastError(ERROR_INVALID_LEVEL);
        goto end;
    }
    ret = TRUE;
end:
    LeaveCriticalSection(&printer_handles_cs);
    return ret;
}

/*****************************************************************************
 *          EndDocPrinter  [WINSPOOL.@]
 */
BOOL WINAPI EndDocPrinter(HANDLE hPrinter)
{
    opened_printer_t *printer;
    BOOL ret = FALSE;
    TRACE("(%p)\n", hPrinter);

    EnterCriticalSection(&printer_handles_cs);

    printer = get_opened_printer(hPrinter);
    if(!printer)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        goto end;
    }

    if(!printer->doc)    
    {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        goto end;
    }

    CloseHandle(printer->doc->hf);
    ScheduleJob(hPrinter, printer->doc->job_id);
    HeapFree(GetProcessHeap(), 0, printer->doc);
    printer->doc = NULL;
    ret = TRUE;
end:
    LeaveCriticalSection(&printer_handles_cs);
    return ret;
}

/*****************************************************************************
 *          EndPagePrinter  [WINSPOOL.@]
 */
BOOL WINAPI EndPagePrinter(HANDLE hPrinter)
{
    FIXME("(%p): stub\n", hPrinter);
    return TRUE;
}

/*****************************************************************************
 *          StartDocPrinterA  [WINSPOOL.@]
 */
DWORD WINAPI StartDocPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo)
{
    UNICODE_STRING usBuffer;
    DOC_INFO_2W doc2W;
    DOC_INFO_2A *doc2 = (DOC_INFO_2A*)pDocInfo;
    DWORD ret;

    /* DOC_INFO_1, 2 and 3 all have the strings in the same place with either two (DOC_INFO_2)
       or one (DOC_INFO_3) extra DWORDs */

    switch(Level) {
    case 2:
        doc2W.JobId = doc2->JobId;
        /* fall through */
    case 3:
        doc2W.dwMode = doc2->dwMode;
        /* fall through */
    case 1:
        doc2W.pDocName = asciitounicode(&usBuffer, doc2->pDocName);
        doc2W.pOutputFile = asciitounicode(&usBuffer, doc2->pOutputFile);
        doc2W.pDatatype = asciitounicode(&usBuffer, doc2->pDatatype);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    ret = StartDocPrinterW(hPrinter, Level, (LPBYTE)&doc2W);

    HeapFree(GetProcessHeap(), 0, doc2W.pDatatype);
    HeapFree(GetProcessHeap(), 0, doc2W.pOutputFile);
    HeapFree(GetProcessHeap(), 0, doc2W.pDocName);

    return ret;
}

/*****************************************************************************
 *          StartDocPrinterW  [WINSPOOL.@]
 */
DWORD WINAPI StartDocPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo)
{
    DOC_INFO_2W *doc = (DOC_INFO_2W *)pDocInfo;
    opened_printer_t *printer;
    BYTE addjob_buf[MAX_PATH * sizeof(WCHAR) + sizeof(ADDJOB_INFO_1W)];
    ADDJOB_INFO_1W *addjob = (ADDJOB_INFO_1W*) addjob_buf;
    JOB_INFO_1W job_info;
    DWORD needed, ret = 0;
    HANDLE hf;
    WCHAR *filename;

    TRACE("(hPrinter = %p, Level = %d, pDocInfo = %p {pDocName = %s, pOutputFile = %s, pDatatype = %s}):\n",
          hPrinter, Level, doc, debugstr_w(doc->pDocName), debugstr_w(doc->pOutputFile),
          debugstr_w(doc->pDatatype));

    if(Level < 1 || Level > 3)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return 0;
    }

    EnterCriticalSection(&printer_handles_cs);
    printer = get_opened_printer(hPrinter);
    if(!printer)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        goto end;
    }

    if(printer->doc)
    {
        SetLastError(ERROR_INVALID_PRINTER_STATE);
        goto end;
    }

    /* Even if we're printing to a file we still add a print job, we'll
       just ignore the spool file name */

    if(!AddJobW(hPrinter, 1, addjob_buf, sizeof(addjob_buf), &needed))
    {
        ERR("AddJob failed gle %u\n", GetLastError());
        goto end;
    }

    if(doc->pOutputFile)
        filename = doc->pOutputFile;
    else
        filename = addjob->Path;

    hf = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hf == INVALID_HANDLE_VALUE)
        goto end;

    memset(&job_info, 0, sizeof(job_info));
    job_info.pDocument = doc->pDocName;
    SetJobW(hPrinter, addjob->JobId, 1, (LPBYTE)&job_info, 0);

    printer->doc = HeapAlloc(GetProcessHeap(), 0, sizeof(*printer->doc));
    printer->doc->hf = hf;
    ret = printer->doc->job_id = addjob->JobId;
end:
    LeaveCriticalSection(&printer_handles_cs);

    return ret;
}

/*****************************************************************************
 *          StartPagePrinter  [WINSPOOL.@]
 */
BOOL WINAPI StartPagePrinter(HANDLE hPrinter)
{
    FIXME("(%p): stub\n", hPrinter);
    return TRUE;
}

/*****************************************************************************
 *          GetFormA  [WINSPOOL.@]
 */
BOOL WINAPI GetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%p,%s,%d,%p,%d,%p): stub\n",hPrinter,pFormName,
         Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          GetFormW  [WINSPOOL.@]
 */
BOOL WINAPI GetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                 LPBYTE pForm, DWORD cbBuf, LPDWORD pcbNeeded)
{
    FIXME("(%p,%s,%d,%p,%d,%p): stub\n",hPrinter,
	  debugstr_w(pFormName),Level,pForm,cbBuf,pcbNeeded);
    return FALSE;
}

/*****************************************************************************
 *          SetFormA  [WINSPOOL.@]
 */
BOOL WINAPI SetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%p,%s,%d,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          SetFormW  [WINSPOOL.@]
 */
BOOL WINAPI SetFormW(HANDLE hPrinter, LPWSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%p,%p,%d,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          ReadPrinter  [WINSPOOL.@]
 */
BOOL WINAPI ReadPrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                           LPDWORD pNoBytesRead)
{
    FIXME("(%p,%p,%d,%p): stub\n",hPrinter,pBuf,cbBuf,pNoBytesRead);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinterA  [WINSPOOL.@]
 */
BOOL WINAPI ResetPrinterA(HANDLE hPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    FIXME("(%p, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *          ResetPrinterW  [WINSPOOL.@]
 */
BOOL WINAPI ResetPrinterW(HANDLE hPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    FIXME("(%p, %p): stub\n", hPrinter, pDefault);
    return FALSE;
}

/*****************************************************************************
 *    WINSPOOL_GetDWORDFromReg
 *
 * Return DWORD associated with ValueName from hkey.
 */
static DWORD WINSPOOL_GetDWORDFromReg(HKEY hkey, LPCSTR ValueName)
{
    DWORD sz = sizeof(DWORD), type, value = 0;
    LONG ret;

    ret = RegQueryValueExA(hkey, ValueName, 0, &type, (LPBYTE)&value, &sz);

    if(ret != ERROR_SUCCESS) {
        WARN("Got ret = %d on name %s\n", ret, ValueName);
	return 0;
    }
    if(type != REG_DWORD) {
        ERR("Got type %d\n", type);
	return 0;
    }
    return value;
}

/*****************************************************************************
 *    WINSPOOL_GetStringFromReg
 *
 * Get ValueName from hkey storing result in ptr.  buflen is space left in ptr
 * String is stored either as unicode or ascii.
 * Bit of a hack here to get the ValueName if we want ascii.
 */
static BOOL WINSPOOL_GetStringFromReg(HKEY hkey, LPCWSTR ValueName, LPBYTE ptr,
				      DWORD buflen, DWORD *needed,
				      BOOL unicode)
{
    DWORD sz = buflen, type;
    LONG ret;

    if(unicode)
        ret = RegQueryValueExW(hkey, ValueName, 0, &type, ptr, &sz);
    else {
        LPSTR ValueNameA = strdupWtoA(ValueName);
        ret = RegQueryValueExA(hkey, ValueNameA, 0, &type, ptr, &sz);
	HeapFree(GetProcessHeap(),0,ValueNameA);
    }
    if(ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
        WARN("Got ret = %d\n", ret);
	*needed = 0;
	return FALSE;
    }
    /* add space for terminating '\0' */
    sz += unicode ? sizeof(WCHAR) : 1;
    *needed = sz;

    if (ptr)
        TRACE("%s: %s\n", debugstr_w(ValueName), unicode ? debugstr_w((LPCWSTR)ptr) : debugstr_a((LPCSTR)ptr));

    return TRUE;
}

/*****************************************************************************
 *    WINSPOOL_GetDefaultDevMode
 *
 * Get a default DevMode values for wineps.
 * FIXME - use ppd.
 */

static void WINSPOOL_GetDefaultDevMode(
	LPBYTE ptr,
	DWORD buflen, DWORD *needed,
	BOOL unicode)
{
    DEVMODEA	dm;
    static const char szwps[] = "wineps.drv";

	/* fill default DEVMODE - should be read from ppd... */
	ZeroMemory( &dm, sizeof(dm) );
	memcpy(dm.dmDeviceName,szwps,sizeof szwps);
	dm.dmSpecVersion = DM_SPECVERSION;
	dm.dmDriverVersion = 1;
	dm.dmSize = sizeof(DEVMODEA);
	dm.dmDriverExtra = 0;
	dm.dmFields =
		DM_ORIENTATION | DM_PAPERSIZE |
		DM_PAPERLENGTH | DM_PAPERWIDTH |
		DM_SCALE |
		DM_COPIES |
		DM_DEFAULTSOURCE | DM_PRINTQUALITY |
		DM_YRESOLUTION | DM_TTOPTION;

	dm.u1.s1.dmOrientation = DMORIENT_PORTRAIT;
	dm.u1.s1.dmPaperSize = DMPAPER_A4;
	dm.u1.s1.dmPaperLength = 2970;
	dm.u1.s1.dmPaperWidth = 2100;

	dm.dmScale = 100;
	dm.dmCopies = 1;
	dm.dmDefaultSource = DMBIN_AUTO;
	dm.dmPrintQuality = DMRES_MEDIUM;
	/* dm.dmColor */
	/* dm.dmDuplex */
	dm.dmYResolution = 300; /* 300dpi */
	dm.dmTTOption = DMTT_BITMAP;
	/* dm.dmCollate */
	/* dm.dmFormName */
	/* dm.dmLogPixels */
	/* dm.dmBitsPerPel */
	/* dm.dmPelsWidth */
	/* dm.dmPelsHeight */
	/* dm.dmDisplayFlags */
	/* dm.dmDisplayFrequency */
	/* dm.dmICMMethod */
	/* dm.dmICMIntent */
	/* dm.dmMediaType */
	/* dm.dmDitherType */
	/* dm.dmReserved1 */
	/* dm.dmReserved2 */
	/* dm.dmPanningWidth */
	/* dm.dmPanningHeight */

    if(unicode) {
	if(buflen >= sizeof(DEVMODEW)) {
	    DEVMODEW *pdmW = GdiConvertToDevmodeW(&dm);
	    memcpy(ptr, pdmW, sizeof(DEVMODEW));
	    HeapFree(GetProcessHeap(),0,pdmW);
	}
	*needed = sizeof(DEVMODEW);
    }
    else
    {
	if(buflen >= sizeof(DEVMODEA)) {
	    memcpy(ptr, &dm, sizeof(DEVMODEA));
	}
	*needed = sizeof(DEVMODEA);
    }
}

/*****************************************************************************
 *    WINSPOOL_GetDevModeFromReg
 *
 * Get ValueName from hkey storing result in ptr.  buflen is space left in ptr
 * DevMode is stored either as unicode or ascii.
 */
static BOOL WINSPOOL_GetDevModeFromReg(HKEY hkey, LPCWSTR ValueName,
				       LPBYTE ptr,
				       DWORD buflen, DWORD *needed,
				       BOOL unicode)
{
    DWORD sz = buflen, type;
    LONG ret;

    if (ptr && buflen>=sizeof(DEVMODEA)) memset(ptr, 0, sizeof(DEVMODEA));
    ret = RegQueryValueExW(hkey, ValueName, 0, &type, ptr, &sz);
    if ((ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA)) sz = 0;
    if (sz < sizeof(DEVMODEA))
    {
        TRACE("corrupted registry for %s ( size %d)\n",debugstr_w(ValueName),sz);
	return FALSE;
    }
    /* ensures that dmSize is not erratically bogus if registry is invalid */
    if (ptr && ((DEVMODEA*)ptr)->dmSize < sizeof(DEVMODEA))
        ((DEVMODEA*)ptr)->dmSize = sizeof(DEVMODEA);
    if(unicode) {
	sz += (CCHDEVICENAME + CCHFORMNAME);
	if(buflen >= sz) {
	    DEVMODEW *dmW = GdiConvertToDevmodeW((DEVMODEA*)ptr);
	    memcpy(ptr, dmW, sz);
	    HeapFree(GetProcessHeap(),0,dmW);
	}
    }
    *needed = sz;
    return TRUE;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_1
 *
 * Fills out a PRINTER_INFO_1A|W struct storing the strings in buf.
 * The strings are either stored as unicode or ascii.
 */
static BOOL WINSPOOL_GetPrinter_1(HKEY hkeyPrinter, PRINTER_INFO_1W *pi1,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi1->pName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }

    /* FIXME: pDescription should be something like "Name,Driver_Name,". */
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi1->pDescription = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DescriptionW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi1->pComment = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }

    if(pi1) pi1->Flags = PRINTER_ENUM_ICON8; /* We're a printer */

    if(!space && pi1) /* zero out pi1 if we can't completely fill buf */
        memset(pi1, 0, sizeof(*pi1));

    return space;
}
/*********************************************************************
 *    WINSPOOL_GetPrinter_2
 *
 * Fills out a PRINTER_INFO_2A|W struct storing the strings in buf.
 * The strings are either stored as unicode or ascii.
 */
static BOOL WINSPOOL_GetPrinter_2(HKEY hkeyPrinter, PRINTER_INFO_2W *pi2,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Share_NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pShareName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, PortW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Printer_DriverW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pDriverName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DescriptionW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pComment = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, LocationW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi2->pLocation = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetDevModeFromReg(hkeyPrinter, Default_DevModeW, ptr, left,
				  &size, unicode)) {
        if(space && size <= left) {
	    pi2->pDevMode = (LPDEVMODEW)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    else
    {
	WINSPOOL_GetDefaultDevMode(ptr, left, &size, unicode);
        if(space && size <= left) {
	    pi2->pDevMode = (LPDEVMODEW)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Separator_FileW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
            pi2->pSepFile = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Print_ProcessorW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pPrintProcessor = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DatatypeW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pDatatype = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, ParametersW, ptr, left,
				 &size, unicode)) {
        if(space && size <= left) {
	    pi2->pParameters = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi2) {
        pi2->Attributes = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Attributes");
        pi2->Priority = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Priority");
        pi2->DefaultPriority = WINSPOOL_GetDWORDFromReg(hkeyPrinter,
							"Default Priority");
        pi2->StartTime = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "StartTime");
        pi2->UntilTime = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "UntilTime");
    }

    if(!space && pi2) /* zero out pi2 if we can't completely fill buf */
        memset(pi2, 0, sizeof(*pi2));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_4
 *
 * Fills out a PRINTER_INFO_4 struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_4(HKEY hkeyPrinter, PRINTER_INFO_4W *pi4,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi4->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi4) {
        pi4->Attributes = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Attributes");
    }

    if(!space && pi4) /* zero out pi4 if we can't completely fill buf */
        memset(pi4, 0, sizeof(*pi4));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_5
 *
 * Fills out a PRINTER_INFO_5 struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_5(HKEY hkeyPrinter, PRINTER_INFO_5W *pi5,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded,
				  BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi5->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, PortW, ptr, left, &size,
				 unicode)) {
        if(space && size <= left) {
	    pi5->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi5) {
        pi5->Attributes = WINSPOOL_GetDWORDFromReg(hkeyPrinter, "Attributes");
        pi5->DeviceNotSelectedTimeout = WINSPOOL_GetDWORDFromReg(hkeyPrinter,
								"dnsTimeout");
        pi5->TransmissionRetryTimeout = WINSPOOL_GetDWORDFromReg(hkeyPrinter,
								 "txTimeout");
    }

    if(!space && pi5) /* zero out pi5 if we can't completely fill buf */
        memset(pi5, 0, sizeof(*pi5));

    return space;
}

/*****************************************************************************
 *          WINSPOOL_GetPrinter
 *
 *    Implementation of GetPrinterA|W.  Relies on PRINTER_INFO_*W being
 *    essentially the same as PRINTER_INFO_*A. i.e. the structure itself is
 *    just a collection of pointers to strings.
 */
static BOOL WINSPOOL_GetPrinter(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
				DWORD cbBuf, LPDWORD pcbNeeded, BOOL unicode)
{
    LPCWSTR name;
    DWORD size, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters;
    BOOL ret;

    TRACE("(%p,%d,%p,%d,%p)\n",hPrinter,Level,pPrinter,cbBuf, pcbNeeded);

    if (!(name = get_opened_printer_name(hPrinter))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
    if(RegOpenKeyW(hkeyPrinters, name, &hkeyPrinter) != ERROR_SUCCESS)
    {
        ERR("Can't find opened printer %s in registry\n", debugstr_w(name));
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }

    switch(Level) {
    case 2:
      {
        PRINTER_INFO_2W *pi2 = (PRINTER_INFO_2W *)pPrinter;

        size = sizeof(PRINTER_INFO_2W);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi2 = NULL;
	    cbBuf = 0;
	}
	ret = WINSPOOL_GetPrinter_2(hkeyPrinter, pi2, ptr, cbBuf, &needed,
				    unicode);
	needed += size;
	break;
      }

    case 4:
      {
	PRINTER_INFO_4W *pi4 = (PRINTER_INFO_4W *)pPrinter;

        size = sizeof(PRINTER_INFO_4W);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi4 = NULL;
	    cbBuf = 0;
	}
	ret = WINSPOOL_GetPrinter_4(hkeyPrinter, pi4, ptr, cbBuf, &needed,
				    unicode);
	needed += size;
	break;
      }


    case 5:
      {
        PRINTER_INFO_5W *pi5 = (PRINTER_INFO_5W *)pPrinter;

        size = sizeof(PRINTER_INFO_5W);
	if(size <= cbBuf) {
	    ptr = pPrinter + size;
	    cbBuf -= size;
	    memset(pPrinter, 0, size);
	} else {
	    pi5 = NULL;
	    cbBuf = 0;
	}

	ret = WINSPOOL_GetPrinter_5(hkeyPrinter, pi5, ptr, cbBuf, &needed,
				    unicode);
	needed += size;
	break;
      }

    default:
        FIXME("Unimplemented level %d\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
	RegCloseKey(hkeyPrinters);
	RegCloseKey(hkeyPrinter);
	return FALSE;
    }

    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);

    TRACE("returning %d needed = %d\n", ret, needed);
    if(pcbNeeded) *pcbNeeded = needed;
    if(!ret)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return ret;
}

/*****************************************************************************
 *          GetPrinterW  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
			DWORD cbBuf, LPDWORD pcbNeeded)
{
    return WINSPOOL_GetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded,
			       TRUE);
}

/*****************************************************************************
 *          GetPrinterA  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    return WINSPOOL_GetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded,
			       FALSE);
}

/*****************************************************************************
 *          WINSPOOL_EnumPrinters
 *
 *    Implementation of EnumPrintersA|W
 */
static BOOL WINSPOOL_EnumPrinters(DWORD dwType, LPWSTR lpszName,
				  DWORD dwLevel, LPBYTE lpbPrinters,
				  DWORD cbBuf, LPDWORD lpdwNeeded,
				  LPDWORD lpdwReturned, BOOL unicode)

{
    HKEY hkeyPrinters, hkeyPrinter;
    WCHAR PrinterName[255];
    DWORD needed = 0, number = 0;
    DWORD used, i, left;
    PBYTE pi, buf;

    if(lpbPrinters)
        memset(lpbPrinters, 0, cbBuf);
    if(lpdwReturned)
        *lpdwReturned = 0;
    if(lpdwNeeded)
        *lpdwNeeded = 0;

    /* PRINTER_ENUM_DEFAULT is only supported under win9x, we behave like NT */
    if(dwType == PRINTER_ENUM_DEFAULT)
	return TRUE;

    if (dwType & PRINTER_ENUM_CONNECTIONS) {
        TRACE("ignoring PRINTER_ENUM_CONNECTIONS\n");
        dwType &= ~PRINTER_ENUM_CONNECTIONS; /* we don't handle that */
        if (!dwType) {
            FIXME("We don't handle PRINTER_ENUM_CONNECTIONS\n");
            *lpdwNeeded = 0;
            *lpdwReturned = 0;
            return TRUE;
        }

    }

    if (!((dwType & PRINTER_ENUM_LOCAL) || (dwType & PRINTER_ENUM_NAME))) {
        FIXME("dwType = %08x\n", dwType);
	SetLastError(ERROR_INVALID_FLAGS);
	return FALSE;
    }

    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }

    if(RegQueryInfoKeyA(hkeyPrinters, NULL, NULL, NULL, &number, NULL, NULL,
			NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hkeyPrinters);
	ERR("Can't query Printers key\n");
	return FALSE;
    }
    TRACE("Found %d printers\n", number);

    switch(dwLevel) {
    case 1:
        used = number * sizeof(PRINTER_INFO_1W);
        break;
    case 2:
        used = number * sizeof(PRINTER_INFO_2W);
	break;
    case 4:
        used = number * sizeof(PRINTER_INFO_4W);
	break;
    case 5:
        used = number * sizeof(PRINTER_INFO_5W);
	break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
	RegCloseKey(hkeyPrinters);
	return FALSE;
    }
    pi = (used <= cbBuf) ? lpbPrinters : NULL;

    for(i = 0; i < number; i++) {
        if(RegEnumKeyW(hkeyPrinters, i, PrinterName, sizeof(PrinterName)) !=
	   ERROR_SUCCESS) {
	    ERR("Can't enum key number %d\n", i);
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}
	TRACE("Printer %d is %s\n", i, debugstr_w(PrinterName));
	if(RegOpenKeyW(hkeyPrinters, PrinterName, &hkeyPrinter) !=
	   ERROR_SUCCESS) {
	    ERR("Can't open key %s\n", debugstr_w(PrinterName));
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}

	if(cbBuf > used) {
	    buf = lpbPrinters + used;
	    left = cbBuf - used;
	} else {
	    buf = NULL;
	    left = 0;
	}

	switch(dwLevel) {
	case 1:
	    WINSPOOL_GetPrinter_1(hkeyPrinter, (PRINTER_INFO_1W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_1W);
	    break;
	case 2:
	    WINSPOOL_GetPrinter_2(hkeyPrinter, (PRINTER_INFO_2W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_2W);
	    break;
	case 4:
	    WINSPOOL_GetPrinter_4(hkeyPrinter, (PRINTER_INFO_4W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_4W);
	    break;
	case 5:
	    WINSPOOL_GetPrinter_5(hkeyPrinter, (PRINTER_INFO_5W *)pi, buf,
				  left, &needed, unicode);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_5W);
	    break;
	default:
	    ERR("Shouldn't be here!\n");
	    RegCloseKey(hkeyPrinter);
	    RegCloseKey(hkeyPrinters);
	    return FALSE;
	}
	RegCloseKey(hkeyPrinter);
    }
    RegCloseKey(hkeyPrinters);

    if(lpdwNeeded)
        *lpdwNeeded = used;

    if(used > cbBuf) {
        if(lpbPrinters)
	    memset(lpbPrinters, 0, cbBuf);
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }
    if(lpdwReturned)
        *lpdwReturned = number;
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


/******************************************************************
 *              EnumPrintersW        [WINSPOOL.@]
 *
 *    Enumerates the available printers, print servers and print
 *    providers, depending on the specified flags, name and level.
 *
 * RETURNS:
 *
 *    If level is set to 1:
 *      Returns an array of PRINTER_INFO_1 data structures in the
 *      lpbPrinters buffer.
 *
 *    If level is set to 2:
 *		Possible flags: PRINTER_ENUM_CONNECTIONS, PRINTER_ENUM_LOCAL.
 *      Returns an array of PRINTER_INFO_2 data structures in the
 *      lpbPrinters buffer. Note that according to MSDN also an
 *      OpenPrinter should be performed on every remote printer.
 *
 *    If level is set to 4 (officially WinNT only):
 *		Possible flags: PRINTER_ENUM_CONNECTIONS, PRINTER_ENUM_LOCAL.
 *      Fast: Only the registry is queried to retrieve printer names,
 *      no connection to the driver is made.
 *      Returns an array of PRINTER_INFO_4 data structures in the
 *      lpbPrinters buffer.
 *
 *    If level is set to 5 (officially WinNT4/Win9x only):
 *      Fast: Only the registry is queried to retrieve printer names,
 *      no connection to the driver is made.
 *      Returns an array of PRINTER_INFO_5 data structures in the
 *      lpbPrinters buffer.
 *
 *    If level set to 3 or 6+:
 *	    returns zero (failure!)
 *
 *    Returns nonzero (TRUE) on success, or zero on failure, use GetLastError
 *    for information.
 *
 * BUGS:
 *    - Only PRINTER_ENUM_LOCAL and PRINTER_ENUM_NAME are implemented.
 *    - Only levels 2, 4 and 5 are implemented at the moment.
 *    - 16-bit printer drivers are not enumerated.
 *    - Returned amount of bytes used/needed does not match the real Windoze
 *      implementation (as in this implementation, all strings are part
 *      of the buffer, whereas Win32 keeps them somewhere else)
 *    - At level 2, EnumPrinters should also call OpenPrinter for remote printers.
 *
 * NOTE:
 *    - In a regular Wine installation, no registry settings for printers
 *      exist, which makes this function return an empty list.
 */
BOOL  WINAPI EnumPrintersW(
		DWORD dwType,        /* [in] Types of print objects to enumerate */
                LPWSTR lpszName,     /* [in] name of objects to enumerate */
	        DWORD dwLevel,       /* [in] type of printer info structure */
                LPBYTE lpbPrinters,  /* [out] buffer which receives info */
		DWORD cbBuf,         /* [in] max size of buffer in bytes */
		LPDWORD lpdwNeeded,  /* [out] pointer to var: # bytes used/needed */
		LPDWORD lpdwReturned /* [out] number of entries returned */
		)
{
    return WINSPOOL_EnumPrinters(dwType, lpszName, dwLevel, lpbPrinters, cbBuf,
				 lpdwNeeded, lpdwReturned, TRUE);
}

/******************************************************************
 *              EnumPrintersA        [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumPrintersA(DWORD dwType, LPSTR lpszName,
			  DWORD dwLevel, LPBYTE lpbPrinters,
			  DWORD cbBuf, LPDWORD lpdwNeeded,
			  LPDWORD lpdwReturned)
{
    BOOL ret, unicode = FALSE;
    UNICODE_STRING lpszNameW;
    PWSTR pwstrNameW;

    pwstrNameW = asciitounicode(&lpszNameW,lpszName);
    if(!cbBuf) unicode = TRUE; /* return a buffer that's big enough for the unicode version */
    ret = WINSPOOL_EnumPrinters(dwType, pwstrNameW, dwLevel, lpbPrinters, cbBuf,
				lpdwNeeded, lpdwReturned, unicode);
    RtlFreeUnicodeString(&lpszNameW);
    return ret;
}

/*****************************************************************************
 *          WINSPOOL_GetDriverInfoFromReg [internal]
 *
 *    Enters the information from the registry into the DRIVER_INFO struct
 *
 * RETURNS
 *    zero if the printer driver does not exist in the registry
 *    (only if Level > 1) otherwise nonzero
 */
static BOOL WINSPOOL_GetDriverInfoFromReg(
                            HKEY    hkeyDrivers,
                            LPWSTR  DriverName,
                            LPCWSTR pEnvironment,
                            DWORD   Level,
                            LPBYTE  ptr,            /* DRIVER_INFO */
                            LPBYTE  pDriverStrings, /* strings buffer */
                            DWORD   cbBuf,          /* size of string buffer */
                            LPDWORD pcbNeeded,      /* space needed for str. */
                            BOOL    unicode)        /* type of strings */
{
    DWORD  size, tmp;
    HKEY   hkeyDriver;
    LPBYTE strPtr = pDriverStrings;

    TRACE("%s,%s,%d,%p,%p,%d,%d\n",
          debugstr_w(DriverName), debugstr_w(pEnvironment),
          Level, ptr, pDriverStrings, cbBuf, unicode);

    if(unicode) {
        *pcbNeeded = (lstrlenW(DriverName) + 1) * sizeof(WCHAR);
            if (*pcbNeeded <= cbBuf)
               strcpyW((LPWSTR)strPtr, DriverName);
    } else {
        *pcbNeeded = WideCharToMultiByte(CP_ACP, 0, DriverName, -1, NULL, 0,
                                          NULL, NULL);
        if(*pcbNeeded <= cbBuf)
            WideCharToMultiByte(CP_ACP, 0, DriverName, -1,
                                (LPSTR)strPtr, *pcbNeeded, NULL, NULL);
    }
    if(Level == 1) {
       if(ptr)
          ((PDRIVER_INFO_1W) ptr)->pName = (LPWSTR) strPtr;
       return TRUE;
    } else {
       if(ptr)
          ((PDRIVER_INFO_2W) ptr)->pName = (LPWSTR) strPtr;
       strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    if(!DriverName[0] || RegOpenKeyW(hkeyDrivers, DriverName, &hkeyDriver) != ERROR_SUCCESS) {
        ERR("Can't find driver %s in registry\n", debugstr_w(DriverName));
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER); /* ? */
        return FALSE;
    }

    if(ptr)
        ((PDRIVER_INFO_2A) ptr)->cVersion = (GetVersion() & 0x80000000) ? 0 : 3; /* FIXME: add 1, 2 */

    if(!pEnvironment)
        pEnvironment = DefaultEnvironmentW;
    if(unicode)
        size = (lstrlenW(pEnvironment) + 1) * sizeof(WCHAR);
    else
        size = WideCharToMultiByte(CP_ACP, 0, pEnvironment, -1, NULL, 0,
			           NULL, NULL);
    *pcbNeeded += size;
    if(*pcbNeeded <= cbBuf) {
        if(unicode)
            strcpyW((LPWSTR)strPtr, pEnvironment);
        else
            WideCharToMultiByte(CP_ACP, 0, pEnvironment, -1,
                                (LPSTR)strPtr, size, NULL, NULL);
        if(ptr)
            ((PDRIVER_INFO_2W) ptr)->pEnvironment = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, DriverW, strPtr, 0, &size,
			         unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, DriverW, strPtr, size, &tmp,
                                      unicode);
        if(ptr)
            ((PDRIVER_INFO_2W) ptr)->pDriverPath = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, Data_FileW, strPtr, 0, &size,
			         unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, Data_FileW, strPtr, size,
                                      &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_2W) ptr)->pDataFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if(WINSPOOL_GetStringFromReg(hkeyDriver, Configuration_FileW, strPtr,
                                 0, &size, unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, Configuration_FileW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_2W) ptr)->pConfigFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if(Level == 2 ) {
        RegCloseKey(hkeyDriver);
        TRACE("buffer space %d required %d\n", cbBuf, *pcbNeeded);
        return TRUE;
    }

    if (Level != 5 && WINSPOOL_GetStringFromReg(hkeyDriver, Help_FileW, strPtr, 0, &size,
                                 unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, Help_FileW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pHelpFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level != 5 && WINSPOOL_GetStringFromReg(hkeyDriver, Dependent_FilesW, strPtr, 0,
			     &size, unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, Dependent_FilesW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pDependentFiles = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level != 5 && WINSPOOL_GetStringFromReg(hkeyDriver, MonitorW, strPtr, 0, &size,
                                 unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, MonitorW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pMonitorName = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level != 5 && WINSPOOL_GetStringFromReg(hkeyDriver, DatatypeW, strPtr, 0, &size,
                                 unicode)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, DatatypeW, strPtr,
                                      size, &tmp, unicode);
        if(ptr)
            ((PDRIVER_INFO_3W) ptr)->pDefaultDataType = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    TRACE("buffer space %d required %d\n", cbBuf, *pcbNeeded);
    RegCloseKey(hkeyDriver);
    return TRUE;
}

/*****************************************************************************
 *          WINSPOOL_GetPrinterDriver
 */
static BOOL WINSPOOL_GetPrinterDriver(HANDLE hPrinter, LPCWSTR pEnvironment,
				      DWORD Level, LPBYTE pDriverInfo,
				      DWORD cbBuf, LPDWORD pcbNeeded,
				      BOOL unicode)
{
    LPCWSTR name;
    WCHAR DriverName[100];
    DWORD ret, type, size, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDrivers;

    TRACE("(%p,%s,%d,%p,%d,%p)\n",hPrinter,debugstr_w(pEnvironment),
	  Level,pDriverInfo,cbBuf, pcbNeeded);

    ZeroMemory(pDriverInfo, cbBuf);

    if (!(name = get_opened_printer_name(hPrinter))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if(Level < 1 || Level > 6) {
        SetLastError(ERROR_INVALID_LEVEL);
	return FALSE;
    }
    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }
    if(RegOpenKeyW(hkeyPrinters, name, &hkeyPrinter)
       != ERROR_SUCCESS) {
        ERR("Can't find opened printer %s in registry\n", debugstr_w(name));
	RegCloseKey(hkeyPrinters);
        SetLastError(ERROR_INVALID_PRINTER_NAME); /* ? */
	return FALSE;
    }
    size = sizeof(DriverName);
    DriverName[0] = 0;
    ret = RegQueryValueExW(hkeyPrinter, Printer_DriverW, 0, &type,
			   (LPBYTE)DriverName, &size);
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);
    if(ret != ERROR_SUCCESS) {
        ERR("Can't get DriverName for printer %s\n", debugstr_w(name));
	return FALSE;
    }

    hkeyDrivers = WINSPOOL_OpenDriverReg( pEnvironment, TRUE);
    if(!hkeyDrivers) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }

    switch(Level) {
    case 1:
        size = sizeof(DRIVER_INFO_1W);
	break;
    case 2:
        size = sizeof(DRIVER_INFO_2W);
	break;
    case 3:
        size = sizeof(DRIVER_INFO_3W);
	break;
    case 4:
        size = sizeof(DRIVER_INFO_4W);
	break;
    case 5:
        size = sizeof(DRIVER_INFO_5W);
	break;
    case 6:
        size = sizeof(DRIVER_INFO_6W);
	break;
    default:
        ERR("Invalid level\n");
	return FALSE;
    }

    if(size <= cbBuf)
        ptr = pDriverInfo + size;

    if(!WINSPOOL_GetDriverInfoFromReg(hkeyDrivers, DriverName,
                         pEnvironment, Level, pDriverInfo,
                         (cbBuf < size) ? NULL : ptr,
                         (cbBuf < size) ? 0 : cbBuf - size,
                         &needed, unicode)) {
            RegCloseKey(hkeyDrivers);
            return FALSE;
    }

    RegCloseKey(hkeyDrivers);

    if(pcbNeeded) *pcbNeeded = size + needed;
    TRACE("buffer space %d required %d\n", cbBuf, size + needed);
    if(cbBuf >= needed) return TRUE;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
}

/*****************************************************************************
 *          GetPrinterDriverA  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment,
			      DWORD Level, LPBYTE pDriverInfo,
			      DWORD cbBuf, LPDWORD pcbNeeded)
{
    BOOL ret;
    UNICODE_STRING pEnvW;
    PWSTR pwstrEnvW;
    
    pwstrEnvW = asciitounicode(&pEnvW, pEnvironment);
    ret = WINSPOOL_GetPrinterDriver(hPrinter, pwstrEnvW, Level, pDriverInfo,
				    cbBuf, pcbNeeded, FALSE);
    RtlFreeUnicodeString(&pEnvW);
    return ret;
}
/*****************************************************************************
 *          GetPrinterDriverW  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment,
                                  DWORD Level, LPBYTE pDriverInfo,
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    return WINSPOOL_GetPrinterDriver(hPrinter, pEnvironment, Level,
				     pDriverInfo, cbBuf, pcbNeeded, TRUE);
}

/*****************************************************************************
 *       GetPrinterDriverDirectoryW  [WINSPOOL.@]
 *
 * Return the PATH for the Printer-Drivers (UNICODE)
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
 *   if cbBuf is too small
 * 
 *   Native Values returned in pDriverDirectory on Success:
 *|  NT(Windows NT x86):  "%winsysdir%\\spool\\DRIVERS\\w32x86" 
 *|  NT(Windows 4.0):     "%winsysdir%\\spool\\DRIVERS\\win40" 
 *|  win9x(Windows 4.0):  "%winsysdir%" 
 *
 *   "%winsysdir%" is the Value from GetSystemDirectoryW()
 *
 * FIXME
 *-  Only NULL or "" is supported for pName
 *
 */
BOOL WINAPI GetPrinterDriverDirectoryW(LPWSTR pName, LPWSTR pEnvironment,
				       DWORD Level, LPBYTE pDriverDirectory,
				       DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD needed;
    const printenv_t * env;

    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_w(pName), 
          debugstr_w(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);
    if(pName != NULL && pName[0]) {
        FIXME("pName unsupported: %s\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    env = validate_envW(pEnvironment);
    if(!env) return FALSE;  /* pEnvironment invalid or unsupported */

    if(Level != 1) {
        WARN("(Level: %d) is ignored in win9x\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    /* GetSystemDirectoryW returns number of WCHAR including the '\0' */
    needed = GetSystemDirectoryW(NULL, 0);
    /* add the Size for the Subdirectories */
    needed += lstrlenW(spooldriversW);
    needed += lstrlenW(env->subdir);
    needed *= sizeof(WCHAR);  /* return-value is size in Bytes */

    if(pcbNeeded)
        *pcbNeeded = needed;
    TRACE("required: 0x%x/%d\n", needed, needed);
    if(needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    if(pcbNeeded == NULL) {
        WARN("(pcbNeeded == NULL) is ignored in win9x\n");
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }
    if(pDriverDirectory == NULL) {
        /* ERROR_INVALID_USER_BUFFER is NT, ERROR_INVALID_PARAMETER is win9x */
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }
    
    GetSystemDirectoryW((LPWSTR) pDriverDirectory, cbBuf/sizeof(WCHAR));
    /* add the Subdirectories */
    lstrcatW((LPWSTR) pDriverDirectory, spooldriversW);
    lstrcatW((LPWSTR) pDriverDirectory, env->subdir);
    TRACE(" => %s\n", debugstr_w((LPWSTR) pDriverDirectory));
    return TRUE;
}


/*****************************************************************************
 *       GetPrinterDriverDirectoryA  [WINSPOOL.@]
 *
 * Return the PATH for the Printer-Drivers (ANSI)
 *
 * See GetPrinterDriverDirectoryW.
 *
 * NOTES
 * On NT, pDriverDirectory need the same Size as the Unicode-Version
 *
 */
BOOL WINAPI GetPrinterDriverDirectoryA(LPSTR pName, LPSTR pEnvironment,
				       DWORD Level, LPBYTE pDriverDirectory,
				       DWORD cbBuf, LPDWORD pcbNeeded)
{
    UNICODE_STRING nameW, environmentW;
    BOOL ret;
    DWORD pcbNeededW;
    INT len = cbBuf * sizeof(WCHAR)/sizeof(CHAR);
    WCHAR *driverDirectoryW = NULL;

    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_a(pName), 
          debugstr_a(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);
 
    if (len) driverDirectoryW = HeapAlloc( GetProcessHeap(), 0, len );

    if(pName) RtlCreateUnicodeStringFromAsciiz(&nameW, pName);
    else nameW.Buffer = NULL;
    if(pEnvironment) RtlCreateUnicodeStringFromAsciiz(&environmentW, pEnvironment);
    else environmentW.Buffer = NULL;

    ret = GetPrinterDriverDirectoryW( nameW.Buffer, environmentW.Buffer, Level,
				      (LPBYTE)driverDirectoryW, len, &pcbNeededW );
    if (ret) {
        DWORD needed;
        needed =  WideCharToMultiByte( CP_ACP, 0, driverDirectoryW, -1, 
                                   (LPSTR)pDriverDirectory, cbBuf, NULL, NULL);
        if(pcbNeeded)
            *pcbNeeded = needed;
        ret = (needed <= cbBuf) ? TRUE : FALSE;
    } else 
        if(pcbNeeded) *pcbNeeded = pcbNeededW * sizeof(CHAR)/sizeof(WCHAR);

    TRACE("required: 0x%x/%d\n", pcbNeeded ? *pcbNeeded : 0, pcbNeeded ? *pcbNeeded : 0);

    HeapFree( GetProcessHeap(), 0, driverDirectoryW );
    RtlFreeUnicodeString(&environmentW);
    RtlFreeUnicodeString(&nameW);

    return ret;
}

/*****************************************************************************
 *          AddPrinterDriverA  [WINSPOOL.@]
 */
BOOL WINAPI AddPrinterDriverA(LPSTR pName, DWORD level, LPBYTE pDriverInfo)
{
    DRIVER_INFO_3A di3;
    HKEY hkeyDrivers, hkeyName;
    static CHAR empty[]    = "",
                nullnull[] = "\0";

    TRACE("(%s,%d,%p)\n",debugstr_a(pName),level,pDriverInfo);

    if(level != 2 && level != 3) {
        SetLastError(ERROR_INVALID_LEVEL);
	return FALSE;
    }
    if ((pName) && (pName[0])) {
        FIXME("pName= %s - unsupported\n", debugstr_a(pName));
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    if(!pDriverInfo) {
        WARN("pDriverInfo == NULL\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    if(level == 3)
        di3 = *(DRIVER_INFO_3A *)pDriverInfo;
    else {
        memset(&di3, 0, sizeof(di3));
	memcpy(&di3, pDriverInfo, sizeof(DRIVER_INFO_2A));
    }

    if(!di3.pName || !di3.pDriverPath || !di3.pConfigFile ||
       !di3.pDataFile) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    if(!di3.pDefaultDataType) di3.pDefaultDataType = empty;
    if(!di3.pDependentFiles) di3.pDependentFiles = nullnull;
    if(!di3.pHelpFile) di3.pHelpFile = empty;
    if(!di3.pMonitorName) di3.pMonitorName = empty;

    hkeyDrivers = WINSPOOL_OpenDriverReg(di3.pEnvironment, FALSE);

    if(!hkeyDrivers) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }

    if(level == 2) { /* apparently can't overwrite with level2 */
        if(RegOpenKeyA(hkeyDrivers, di3.pName, &hkeyName) == ERROR_SUCCESS) {
	    RegCloseKey(hkeyName);
	    RegCloseKey(hkeyDrivers);
	    WARN("Trying to create existing printer driver %s\n", debugstr_a(di3.pName));
	    SetLastError(ERROR_PRINTER_DRIVER_ALREADY_INSTALLED);
	    return FALSE;
	}
    }
    if(RegCreateKeyA(hkeyDrivers, di3.pName, &hkeyName) != ERROR_SUCCESS) {
	RegCloseKey(hkeyDrivers);
	ERR("Can't create Name key\n");
	return FALSE;
    }
    RegSetValueExA(hkeyName, "Configuration File", 0, REG_SZ, (LPBYTE) di3.pConfigFile,
		   lstrlenA(di3.pConfigFile) + 1);
    RegSetValueExA(hkeyName, "Data File", 0, REG_SZ, (LPBYTE) di3.pDataFile, lstrlenA(di3.pDataFile) + 1);
    RegSetValueExA(hkeyName, "Driver", 0, REG_SZ, (LPBYTE) di3.pDriverPath, lstrlenA(di3.pDriverPath) + 1);
    RegSetValueExA(hkeyName, "Version", 0, REG_DWORD, (LPBYTE) &di3.cVersion,
		   sizeof(DWORD));
    RegSetValueExA(hkeyName, "Datatype", 0, REG_SZ, (LPBYTE) di3.pDefaultDataType, lstrlenA(di3.pDefaultDataType));
    RegSetValueExA(hkeyName, "Dependent Files", 0, REG_MULTI_SZ,
                   (LPBYTE) di3.pDependentFiles, multi_sz_lenA(di3.pDependentFiles));
    RegSetValueExA(hkeyName, "Help File", 0, REG_SZ, (LPBYTE) di3.pHelpFile, lstrlenA(di3.pHelpFile) + 1);
    RegSetValueExA(hkeyName, "Monitor", 0, REG_SZ, (LPBYTE) di3.pMonitorName, lstrlenA(di3.pMonitorName) + 1);
    RegCloseKey(hkeyName);
    RegCloseKey(hkeyDrivers);

    return TRUE;
}

/*****************************************************************************
 *          AddPrinterDriverW  [WINSPOOL.@]
 */
BOOL WINAPI AddPrinterDriverW(LPWSTR printerName,DWORD level,
				   LPBYTE pDriverInfo)
{
    FIXME("(%s,%d,%p): stub\n",debugstr_w(printerName),
	  level,pDriverInfo);
    return FALSE;
}

/*****************************************************************************
 *          AddPrintProcessorA  [WINSPOOL.@]
 */
BOOL WINAPI AddPrintProcessorA(LPSTR pName, LPSTR pEnvironment, LPSTR pPathName,
                               LPSTR pPrintProcessorName)
{
    FIXME("(%s,%s,%s,%s): stub\n", debugstr_a(pName), debugstr_a(pEnvironment),
          debugstr_a(pPathName), debugstr_a(pPrintProcessorName));
    return FALSE;
}

/*****************************************************************************
 *          AddPrintProcessorW  [WINSPOOL.@]
 */
BOOL WINAPI AddPrintProcessorW(LPWSTR pName, LPWSTR pEnvironment, LPWSTR pPathName,
                               LPWSTR pPrintProcessorName)
{
    FIXME("(%s,%s,%s,%s): stub\n", debugstr_w(pName), debugstr_w(pEnvironment),
          debugstr_w(pPathName), debugstr_w(pPrintProcessorName));
    return FALSE;
}

/*****************************************************************************
 *          AddPrintProvidorA  [WINSPOOL.@]
 */
BOOL WINAPI AddPrintProvidorA(LPSTR pName, DWORD Level, LPBYTE pProviderInfo)
{
    FIXME("(%s,0x%08x,%p): stub\n", debugstr_a(pName), Level, pProviderInfo);
    return FALSE;
}

/*****************************************************************************
 *          AddPrintProvidorW  [WINSPOOL.@]
 */
BOOL WINAPI AddPrintProvidorW(LPWSTR pName, DWORD Level, LPBYTE pProviderInfo)
{
    FIXME("(%s,0x%08x,%p): stub\n", debugstr_w(pName), Level, pProviderInfo);
    return FALSE;
}

/*****************************************************************************
 *          AdvancedDocumentPropertiesA  [WINSPOOL.@]
 */
LONG WINAPI AdvancedDocumentPropertiesA(HWND hWnd, HANDLE hPrinter, LPSTR pDeviceName,
                                        PDEVMODEA pDevModeOutput, PDEVMODEA pDevModeInput)
{
    FIXME("(%p,%p,%s,%p,%p): stub\n", hWnd, hPrinter, debugstr_a(pDeviceName),
          pDevModeOutput, pDevModeInput);
    return 0;
}

/*****************************************************************************
 *          AdvancedDocumentPropertiesW  [WINSPOOL.@]
 */
LONG WINAPI AdvancedDocumentPropertiesW(HWND hWnd, HANDLE hPrinter, LPWSTR pDeviceName,
                                        PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput)
{
    FIXME("(%p,%p,%s,%p,%p): stub\n", hWnd, hPrinter, debugstr_w(pDeviceName),
          pDevModeOutput, pDevModeInput);
    return 0;
}

/*****************************************************************************
 *          PrinterProperties  [WINSPOOL.@]
 *
 *     Displays a dialog to set the properties of the printer.
 *
 * RETURNS
 *     nonzero on success or zero on failure
 *
 * BUGS
 *	   implemented as stub only
 */
BOOL WINAPI PrinterProperties(HWND hWnd,      /* [in] handle to parent window */
                              HANDLE hPrinter /* [in] handle to printer object */
){
    FIXME("(%p,%p): stub\n", hWnd, hPrinter);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *          EnumJobsA [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumJobsA(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs,
		      DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded,
		      LPDWORD pcReturned)
{
    FIXME("(%p,first=%d,no=%d,level=%d,job=%p,cb=%d,%p,%p), stub!\n",
	hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned
    );
    if(pcbNeeded) *pcbNeeded = 0;
    if(pcReturned) *pcReturned = 0;
    return FALSE;
}


/*****************************************************************************
 *          EnumJobsW [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumJobsW(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs,
		      DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded,
		      LPDWORD pcReturned)
{
    FIXME("(%p,first=%d,no=%d,level=%d,job=%p,cb=%d,%p,%p), stub!\n",
	hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf, pcbNeeded, pcReturned
    );
    if(pcbNeeded) *pcbNeeded = 0;
    if(pcReturned) *pcReturned = 0;
    return FALSE;
}

/*****************************************************************************
 *          WINSPOOL_EnumPrinterDrivers [internal]
 *
 *    Delivers information about all printer drivers installed on the
 *    localhost or a given server
 *
 * RETURNS
 *    nonzero on success or zero on failure. If the buffer for the returned
 *    information is too small the function will return an error
 *
 * BUGS
 *    - only implemented for localhost, foreign hosts will return an error
 */
static BOOL WINSPOOL_EnumPrinterDrivers(LPWSTR pName, LPCWSTR pEnvironment,
                                        DWORD Level, LPBYTE pDriverInfo,
                                        DWORD cbBuf, LPDWORD pcbNeeded,
                                        LPDWORD pcReturned, BOOL unicode)

{   HKEY  hkeyDrivers;
    DWORD i, needed, number = 0, size = 0;
    WCHAR DriverNameW[255];
    PBYTE ptr;

    TRACE("%s,%s,%d,%p,%d,%d\n",
          debugstr_w(pName), debugstr_w(pEnvironment),
          Level, pDriverInfo, cbBuf, unicode);

    /* check for local drivers */
    if((pName) && (pName[0])) {
        FIXME("remote drivers (%s) not supported!\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    /* check input parameter */
    if((Level < 1) || (Level > 3)) {
        ERR("unsupported level %d\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    /* initialize return values */
    if(pDriverInfo)
        memset( pDriverInfo, 0, cbBuf);
    *pcbNeeded  = 0;
    *pcReturned = 0;

    hkeyDrivers = WINSPOOL_OpenDriverReg(pEnvironment, TRUE);
    if(!hkeyDrivers) {
        ERR("Can't open Drivers key\n");
        return FALSE;
    }

    if(RegQueryInfoKeyA(hkeyDrivers, NULL, NULL, NULL, &number, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hkeyDrivers);
        ERR("Can't query Drivers key\n");
        return FALSE;
    }
    TRACE("Found %d Drivers\n", number);

    /* get size of single struct
     * unicode and ascii structure have the same size
     */
    switch (Level) {
        case 1:
            size = sizeof(DRIVER_INFO_1A);
            break;
        case 2:
            size = sizeof(DRIVER_INFO_2A);
            break;
        case 3:
            size = sizeof(DRIVER_INFO_3A);
            break;
    }

    /* calculate required buffer size */
    *pcbNeeded = size * number;

    for( i = 0,  ptr = (pDriverInfo && (cbBuf >= size)) ? pDriverInfo : NULL ;
         i < number;
         i++, ptr = (ptr && (cbBuf >= size * i)) ? ptr + size : NULL) {
        if(RegEnumKeyW(hkeyDrivers, i, DriverNameW, sizeof(DriverNameW))
                       != ERROR_SUCCESS) {
            ERR("Can't enum key number %d\n", i);
            RegCloseKey(hkeyDrivers);
            return FALSE;
        }
        if(!WINSPOOL_GetDriverInfoFromReg(hkeyDrivers, DriverNameW,
                         pEnvironment, Level, ptr,
                         (cbBuf < *pcbNeeded) ? NULL : pDriverInfo + *pcbNeeded,
                         (cbBuf < *pcbNeeded) ? 0 : cbBuf - *pcbNeeded,
                         &needed, unicode)) {
            RegCloseKey(hkeyDrivers);
            return FALSE;
        }
	(*pcbNeeded) += needed;
    }

    RegCloseKey(hkeyDrivers);

    if(cbBuf < *pcbNeeded){
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    *pcReturned = number;
    return TRUE;
}

/*****************************************************************************
 *          EnumPrinterDriversW  [WINSPOOL.@]
 *
 *    see function EnumPrinterDrivers for RETURNS, BUGS
 */
BOOL WINAPI EnumPrinterDriversW(LPWSTR pName, LPWSTR pEnvironment, DWORD Level,
                                LPBYTE pDriverInfo, DWORD cbBuf,
                                LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    return WINSPOOL_EnumPrinterDrivers(pName, pEnvironment, Level, pDriverInfo,
                                       cbBuf, pcbNeeded, pcReturned, TRUE);
}

/*****************************************************************************
 *          EnumPrinterDriversA  [WINSPOOL.@]
 *
 *    see function EnumPrinterDrivers for RETURNS, BUGS
 */
BOOL WINAPI EnumPrinterDriversA(LPSTR pName, LPSTR pEnvironment, DWORD Level,
                                LPBYTE pDriverInfo, DWORD cbBuf,
                                LPDWORD pcbNeeded, LPDWORD pcReturned)
{   BOOL ret;
    UNICODE_STRING pNameW, pEnvironmentW;
    PWSTR pwstrNameW, pwstrEnvironmentW;

    pwstrNameW = asciitounicode(&pNameW, pName);
    pwstrEnvironmentW = asciitounicode(&pEnvironmentW, pEnvironment);

    ret = WINSPOOL_EnumPrinterDrivers(pwstrNameW, pwstrEnvironmentW,
                                      Level, pDriverInfo, cbBuf, pcbNeeded,
                                      pcReturned, FALSE);
    RtlFreeUnicodeString(&pNameW);
    RtlFreeUnicodeString(&pEnvironmentW);

    return ret;
}

/******************************************************************************
 *		EnumPortsA   (WINSPOOL.@)
 *
 * See EnumPortsW.
 *
 */
BOOL WINAPI EnumPortsA( LPSTR pName, DWORD Level, LPBYTE pPorts, DWORD cbBuf,
                        LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    BOOL    res;
    LPBYTE  bufferW = NULL;
    LPWSTR  nameW = NULL;
    DWORD   needed = 0;
    DWORD   numentries = 0;
    INT     len;

    TRACE("(%s, %d, %p, %d, %p, %p)\n", debugstr_a(pName), Level, pPorts,
          cbBuf, pcbNeeded, pcReturned);

    /* convert servername to unicode */
    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }
    /* alloc (userbuffersize*sizeof(WCHAR) and try to enum the Ports */
    needed = cbBuf * sizeof(WCHAR);    
    if (needed) bufferW = HeapAlloc(GetProcessHeap(), 0, needed);
    res = EnumPortsW(nameW, Level, bufferW, needed, pcbNeeded, pcReturned);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (pcbNeeded) needed = *pcbNeeded;
        /* HeapReAlloc return NULL, when bufferW was NULL */
        bufferW = (bufferW) ? HeapReAlloc(GetProcessHeap(), 0, bufferW, needed) :
                              HeapAlloc(GetProcessHeap(), 0, needed);

        /* Try again with the large Buffer */
        res = EnumPortsW(nameW, Level, bufferW, needed, pcbNeeded, pcReturned);
    }
    needed = pcbNeeded ? *pcbNeeded : 0;
    numentries = pcReturned ? *pcReturned : 0;

    /*
       W2k require the buffersize from EnumPortsW also for EnumPortsA.
       We use the smaller Ansi-Size to avoid conflicts with fixed Buffers of old Apps.
     */
    if (res) {
        /* EnumPortsW collected all Data. Parse them to calculate ANSI-Size */
        DWORD   entrysize = 0;
        DWORD   index;
        LPSTR   ptr;
        LPPORT_INFO_2W pi2w;
        LPPORT_INFO_2A pi2a;

        needed = 0;
        entrysize = (Level == 1) ? sizeof(PORT_INFO_1A) : sizeof(PORT_INFO_2A);

        /* First pass: calculate the size for all Entries */
        pi2w = (LPPORT_INFO_2W) bufferW;
        pi2a = (LPPORT_INFO_2A) pPorts;
        index = 0;
        while (index < numentries) {
            index++;
            needed += entrysize;    /* PORT_INFO_?A */
            TRACE("%p: parsing #%d (%s)\n", pi2w, index, debugstr_w(pi2w->pPortName));

            needed += WideCharToMultiByte(CP_ACP, 0, pi2w->pPortName, -1,
                                            NULL, 0, NULL, NULL);
            if (Level > 1) {
                needed += WideCharToMultiByte(CP_ACP, 0, pi2w->pMonitorName, -1,
                                                NULL, 0, NULL, NULL);
                needed += WideCharToMultiByte(CP_ACP, 0, pi2w->pDescription, -1,
                                                NULL, 0, NULL, NULL);
            }
            /* use LPBYTE with entrysize to avoid double code (PORT_INFO_1 + PORT_INFO_2) */
            pi2w = (LPPORT_INFO_2W) (((LPBYTE)pi2w) + entrysize);
            pi2a = (LPPORT_INFO_2A) (((LPBYTE)pi2a) + entrysize);
        }

        /* check for errors and quit on failure */
        if (cbBuf < needed) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            res = FALSE;
            goto cleanup;
        }
        len = entrysize * numentries;       /* room for all PORT_INFO_?A */
        ptr = (LPSTR) &pPorts[len];         /* room for strings */
        cbBuf -= len ;                      /* free Bytes in the user-Buffer */
        pi2w = (LPPORT_INFO_2W) bufferW;
        pi2a = (LPPORT_INFO_2A) pPorts;
        index = 0;
        /* Second Pass: Fill the User Buffer (if we have one) */
        while ((index < numentries) && pPorts) {
            index++;
            TRACE("%p: writing PORT_INFO_%dA #%d\n", pi2a, Level, index);
            pi2a->pPortName = ptr;
            len = WideCharToMultiByte(CP_ACP, 0, pi2w->pPortName, -1,
                                            ptr, cbBuf , NULL, NULL);
            ptr += len;
            cbBuf -= len;
            if (Level > 1) {
                pi2a->pMonitorName = ptr;
                len = WideCharToMultiByte(CP_ACP, 0, pi2w->pMonitorName, -1,
                                            ptr, cbBuf, NULL, NULL);
                ptr += len;
                cbBuf -= len;

                pi2a->pDescription = ptr;
                len = WideCharToMultiByte(CP_ACP, 0, pi2w->pDescription, -1,
                                            ptr, cbBuf, NULL, NULL);
                ptr += len;
                cbBuf -= len;

                pi2a->fPortType = pi2w->fPortType;
                pi2a->Reserved = 0;              /* documented: "must be zero" */
                
            }
            /* use LPBYTE with entrysize to avoid double code (PORT_INFO_1 + PORT_INFO_2) */
            pi2w = (LPPORT_INFO_2W) (((LPBYTE)pi2w) + entrysize);
            pi2a = (LPPORT_INFO_2A) (((LPBYTE)pi2a) + entrysize);
        }
    }

cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = (res) ? numentries : 0;

    HeapFree(GetProcessHeap(), 0, nameW);
    HeapFree(GetProcessHeap(), 0, bufferW);

    TRACE("returning %d with %d (%d byte for %d of %d entries)\n", 
            (res), GetLastError(), needed, (res)? numentries : 0, numentries);

    return (res);

}

/******************************************************************************
 *      EnumPortsW   (WINSPOOL.@)
 *
 * Enumerate available Ports
 *
 * PARAMS
 *  name        [I] Servername or NULL (local Computer)
 *  level       [I] Structure-Level (1 or 2)
 *  buffer      [O] PTR to Buffer that receives the Result
 *  bufsize     [I] Size of Buffer at buffer
 *  bufneeded   [O] PTR to DWORD that receives the size in Bytes used / required for buffer
 *  bufreturned [O] PTR to DWORD that receives the number of Ports in buffer
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in bufneeded the Bytes required for buffer, if bufsize is too small
 *
 */

BOOL WINAPI EnumPortsW(LPWSTR pName, DWORD Level, LPBYTE pPorts, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD   needed = 0;
    DWORD   numentries = 0;
    BOOL    res = FALSE;

    TRACE("(%s, %d, %p, %d, %p, %p)\n", debugstr_w(pName), Level, pPorts,
          cbBuf, pcbNeeded, pcReturned);

    if (pName && (pName[0])) {
        FIXME("not implemented for Server %s\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        goto emP_cleanup;
    }

    /* Level is not checked in win9x */
    if (!Level || (Level > 2)) {
        WARN("level (%d) is ignored in win9x\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        goto emP_cleanup;
    }
    if (!pcbNeeded) {
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

    TRACE("returning %d with %d (%d byte for %d of %d entries)\n", 
            (res), GetLastError(), needed, (res)? numentries : 0, numentries);

    return (res);
}

/******************************************************************************
 *		GetDefaultPrinterW   (WINSPOOL.@)
 *
 * FIXME
 *	This function must read the value from data 'device' of key
 *	HCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows
 */
BOOL WINAPI GetDefaultPrinterW(LPWSTR name, LPDWORD namesize)
{
    BOOL  retval = TRUE;
    DWORD insize, len;
    WCHAR *buffer, *ptr;

    if (!namesize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* make the buffer big enough for the stuff from the profile/registry,
     * the content must fit into the local buffer to compute the correct
     * size even if the extern buffer is too small or not given.
     * (20 for ,driver,port) */
    insize = *namesize;
    len = max(100, (insize + 20));
    buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR));

    if (!GetProfileStringW(windowsW, deviceW, emptyStringW, buffer, len))
    {
        SetLastError (ERROR_FILE_NOT_FOUND);
        retval = FALSE;
        goto end;
    }
    TRACE("%s\n", debugstr_w(buffer));

    if ((ptr = strchrW(buffer, ',')) == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        retval = FALSE;
        goto end;
    }

    *ptr = 0;
    *namesize = strlenW(buffer) + 1;
    if(!name || (*namesize > insize))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        retval = FALSE;
        goto end;
    }
    strcpyW(name, buffer);

end:
    HeapFree( GetProcessHeap(), 0, buffer);
    return retval;
}


/******************************************************************************
 *		GetDefaultPrinterA   (WINSPOOL.@)
 */
BOOL WINAPI GetDefaultPrinterA(LPSTR name, LPDWORD namesize)
{
    BOOL  retval = TRUE;
    DWORD insize = 0;
    WCHAR *bufferW = NULL;

    if (!namesize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(name && *namesize) {
	insize = *namesize;
	bufferW = HeapAlloc( GetProcessHeap(), 0, insize * sizeof(WCHAR));
    }

    if(!GetDefaultPrinterW( bufferW, namesize)) {
	retval = FALSE;
	goto end;
    }

    *namesize = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, name, insize,
                                    NULL, NULL);
    if (!*namesize)
    {
        *namesize = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
        retval = FALSE;
    }
    TRACE("0x%08x/0x%08x:%s\n", *namesize, insize, debugstr_w(bufferW));

end:
    HeapFree( GetProcessHeap(), 0, bufferW);
    return retval;
}


/******************************************************************************
 *		SetDefaultPrinterW   (WINSPOOL.204)
 *
 * Set the Name of the Default Printer
 *
 * PARAMS
 *  pszPrinter [I] Name of the Printer or NULL
 *
 * RETURNS
 *  Success:    True
 *  Failure:    FALSE
 *
 * NOTES
 *  When the Parameter is NULL or points to an Empty String and
 *  a Default Printer was already present, then this Function changes nothing.
 *  Without a Default Printer and NULL (or an Empty String) as Parameter,
 *  the First enumerated local Printer is used.
 *
 */
BOOL WINAPI SetDefaultPrinterW(LPCWSTR pszPrinter)
{

    TRACE("(%s)\n", debugstr_w(pszPrinter));

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *		SetDefaultPrinterA   (WINSPOOL.202)
 *
 * See SetDefaultPrinterW.
 *
 */
BOOL WINAPI SetDefaultPrinterA(LPCSTR pszPrinter)
{

    TRACE("(%s)\n", debugstr_a(pszPrinter));

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************************
 *		SetPrinterDataExA   (WINSPOOL.@)
 */
DWORD WINAPI SetPrinterDataExA(HANDLE hPrinter, LPCSTR pKeyName,
			       LPCSTR pValueName, DWORD Type,
			       LPBYTE pData, DWORD cbData)
{
    HKEY hkeyPrinter, hkeySubkey;
    DWORD ret;

    TRACE("(%p, %s, %s %08x, %p, %08x)\n", hPrinter, debugstr_a(pKeyName),
	  debugstr_a(pValueName), Type, pData, cbData);

    if((ret = WINSPOOL_GetOpenedPrinterRegKey(hPrinter, &hkeyPrinter))
       != ERROR_SUCCESS)
        return ret;

    if((ret = RegCreateKeyA(hkeyPrinter, pKeyName, &hkeySubkey))
       != ERROR_SUCCESS) {
        ERR("Can't create subkey %s\n", debugstr_a(pKeyName));
	RegCloseKey(hkeyPrinter);
	return ret;
    }
    ret = RegSetValueExA(hkeySubkey, pValueName, 0, Type, pData, cbData);
    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    return ret;
}

/******************************************************************************
 *		SetPrinterDataExW   (WINSPOOL.@)
 */
DWORD WINAPI SetPrinterDataExW(HANDLE hPrinter, LPCWSTR pKeyName,
			       LPCWSTR pValueName, DWORD Type,
			       LPBYTE pData, DWORD cbData)
{
    HKEY hkeyPrinter, hkeySubkey;
    DWORD ret;

    TRACE("(%p, %s, %s %08x, %p, %08x)\n", hPrinter, debugstr_w(pKeyName),
	  debugstr_w(pValueName), Type, pData, cbData);

    if((ret = WINSPOOL_GetOpenedPrinterRegKey(hPrinter, &hkeyPrinter))
       != ERROR_SUCCESS)
        return ret;

    if((ret = RegCreateKeyW(hkeyPrinter, pKeyName, &hkeySubkey))
       != ERROR_SUCCESS) {
        ERR("Can't create subkey %s\n", debugstr_w(pKeyName));
	RegCloseKey(hkeyPrinter);
	return ret;
    }
    ret = RegSetValueExW(hkeySubkey, pValueName, 0, Type, pData, cbData);
    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    return ret;
}

/******************************************************************************
 *		SetPrinterDataA   (WINSPOOL.@)
 */
DWORD WINAPI SetPrinterDataA(HANDLE hPrinter, LPSTR pValueName, DWORD Type,
			       LPBYTE pData, DWORD cbData)
{
    return SetPrinterDataExA(hPrinter, "PrinterDriverData", pValueName, Type,
			     pData, cbData);
}

/******************************************************************************
 *		SetPrinterDataW   (WINSPOOL.@)
 */
DWORD WINAPI SetPrinterDataW(HANDLE hPrinter, LPWSTR pValueName, DWORD Type,
			     LPBYTE pData, DWORD cbData)
{
    return SetPrinterDataExW(hPrinter, PrinterDriverDataW, pValueName, Type,
			     pData, cbData);
}

/******************************************************************************
 *		GetPrinterDataExA   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataExA(HANDLE hPrinter, LPCSTR pKeyName,
			       LPCSTR pValueName, LPDWORD pType,
			       LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    HKEY hkeyPrinter, hkeySubkey;
    DWORD ret;

    TRACE("(%p, %s, %s %p, %p, %08x, %p)\n", hPrinter,
	  debugstr_a(pKeyName), debugstr_a(pValueName), pType, pData, nSize,
	  pcbNeeded);

    if((ret = WINSPOOL_GetOpenedPrinterRegKey(hPrinter, &hkeyPrinter))
       != ERROR_SUCCESS)
        return ret;

    if((ret = RegOpenKeyA(hkeyPrinter, pKeyName, &hkeySubkey))
       != ERROR_SUCCESS) {
        WARN("Can't open subkey %s\n", debugstr_a(pKeyName));
	RegCloseKey(hkeyPrinter);
	return ret;
    }
    *pcbNeeded = nSize;
    ret = RegQueryValueExA(hkeySubkey, pValueName, 0, pType, pData, pcbNeeded);
    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    return ret;
}

/******************************************************************************
 *		GetPrinterDataExW   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataExW(HANDLE hPrinter, LPCWSTR pKeyName,
			       LPCWSTR pValueName, LPDWORD pType,
			       LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    HKEY hkeyPrinter, hkeySubkey;
    DWORD ret;

    TRACE("(%p, %s, %s %p, %p, %08x, %p)\n", hPrinter,
	  debugstr_w(pKeyName), debugstr_w(pValueName), pType, pData, nSize,
	  pcbNeeded);

    if((ret = WINSPOOL_GetOpenedPrinterRegKey(hPrinter, &hkeyPrinter))
       != ERROR_SUCCESS)
        return ret;

    if((ret = RegOpenKeyW(hkeyPrinter, pKeyName, &hkeySubkey))
       != ERROR_SUCCESS) {
        WARN("Can't open subkey %s\n", debugstr_w(pKeyName));
	RegCloseKey(hkeyPrinter);
	return ret;
    }
    *pcbNeeded = nSize;
    ret = RegQueryValueExW(hkeySubkey, pValueName, 0, pType, pData, pcbNeeded);
    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    return ret;
}

/******************************************************************************
 *		GetPrinterDataA   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataA(HANDLE hPrinter, LPSTR pValueName, LPDWORD pType,
			     LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    return GetPrinterDataExA(hPrinter, "PrinterDriverData", pValueName, pType,
			     pData, nSize, pcbNeeded);
}

/******************************************************************************
 *		GetPrinterDataW   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataW(HANDLE hPrinter, LPWSTR pValueName, LPDWORD pType,
			     LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    return GetPrinterDataExW(hPrinter, PrinterDriverDataW, pValueName, pType,
			     pData, nSize, pcbNeeded);
}

/*******************************************************************************
 *		EnumPrinterDataExW	[WINSPOOL.@]
 */
DWORD WINAPI EnumPrinterDataExW(HANDLE hPrinter, LPCWSTR pKeyName,
				LPBYTE pEnumValues, DWORD cbEnumValues,
				LPDWORD pcbEnumValues, LPDWORD pnEnumValues)
{
    HKEY		    hkPrinter, hkSubKey;
    DWORD		    r, ret, dwIndex, cValues, cbMaxValueNameLen,
			    cbValueNameLen, cbMaxValueLen, cbValueLen,
			    cbBufSize, dwType;
    LPWSTR		    lpValueName;
    HANDLE		    hHeap;
    PBYTE		    lpValue;
    PPRINTER_ENUM_VALUESW   ppev;

    TRACE ("%p %s\n", hPrinter, debugstr_w (pKeyName));

    if (pKeyName == NULL || *pKeyName == 0)
	return ERROR_INVALID_PARAMETER;

    ret = WINSPOOL_GetOpenedPrinterRegKey (hPrinter, &hkPrinter);
    if (ret != ERROR_SUCCESS)
    {
	TRACE ("WINSPOOL_GetOpenedPrinterRegKey (%p) returned %i\n",
		hPrinter, ret);
	return ret;
    }

    ret = RegOpenKeyExW (hkPrinter, pKeyName, 0, KEY_READ, &hkSubKey);
    if (ret != ERROR_SUCCESS)
    {
	r = RegCloseKey (hkPrinter);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	TRACE ("RegOpenKeyExW (%p, %s) returned %i\n", hPrinter,
		debugstr_w (pKeyName), ret);
	return ret;
    }

    ret = RegCloseKey (hkPrinter);
    if (ret != ERROR_SUCCESS)
    {
	ERR ("RegCloseKey returned %i\n", ret);
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	return ret;
    }

    ret = RegQueryInfoKeyW (hkSubKey, NULL, NULL, NULL, NULL, NULL, NULL,
	    &cValues, &cbMaxValueNameLen, &cbMaxValueLen, NULL, NULL);
    if (ret != ERROR_SUCCESS)
    {
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	TRACE ("RegQueryInfoKeyW (%p) returned %i\n", hkSubKey, ret);
	return ret;
    }

    TRACE ("RegQueryInfoKeyW returned cValues = %i, cbMaxValueNameLen = %i, "
	    "cbMaxValueLen = %i\n", cValues, cbMaxValueNameLen, cbMaxValueLen);

    if (cValues == 0)			/* empty key */
    {
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	*pcbEnumValues = *pnEnumValues = 0;
	return ERROR_SUCCESS;
    }

    ++cbMaxValueNameLen;			/* allow for trailing '\0' */

    hHeap = GetProcessHeap ();
    if (hHeap == NULL)
    {
	ERR ("GetProcessHeap failed\n");
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	return ERROR_OUTOFMEMORY;
    }

    lpValueName = HeapAlloc (hHeap, 0, cbMaxValueNameLen * sizeof (WCHAR));
    if (lpValueName == NULL)
    {
	ERR ("Failed to allocate %i WCHARs from process heap\n", cbMaxValueNameLen);
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	return ERROR_OUTOFMEMORY;
    }

    lpValue = HeapAlloc (hHeap, 0, cbMaxValueLen);
    if (lpValue == NULL)
    {
	ERR ("Failed to allocate %i bytes from process heap\n", cbMaxValueLen);
	if (HeapFree (hHeap, 0, lpValueName) == 0)
	    WARN ("HeapFree failed with code %i\n", GetLastError ());
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	return ERROR_OUTOFMEMORY;
    }

    TRACE ("pass 1: calculating buffer required for all names and values\n");

    cbBufSize = cValues * sizeof (PRINTER_ENUM_VALUESW);

    TRACE ("%i bytes required for %i headers\n", cbBufSize, cValues);

    for (dwIndex = 0; dwIndex < cValues; ++dwIndex)
    {
	cbValueNameLen = cbMaxValueNameLen; cbValueLen = cbMaxValueLen;
	ret = RegEnumValueW (hkSubKey, dwIndex, lpValueName, &cbValueNameLen,
		NULL, NULL, lpValue, &cbValueLen);
	if (ret != ERROR_SUCCESS)
	{
	    if (HeapFree (hHeap, 0, lpValue) == 0)
		WARN ("HeapFree failed with code %i\n", GetLastError ());
	    if (HeapFree (hHeap, 0, lpValueName) == 0)
		WARN ("HeapFree failed with code %i\n", GetLastError ());
	    r = RegCloseKey (hkSubKey);
	    if (r != ERROR_SUCCESS)
		WARN ("RegCloseKey returned %i\n", r);
	    TRACE ("RegEnumValueW (%i) returned %i\n", dwIndex, ret);
	    return ret;
	}

	TRACE ("%s [%i]: name needs %i WCHARs, data needs %i bytes\n",
		debugstr_w (lpValueName), dwIndex,
		cbValueNameLen + 1, cbValueLen);

	cbBufSize += (cbValueNameLen + 1) * sizeof (WCHAR);
	cbBufSize += cbValueLen;
    }

    TRACE ("%i bytes required for all %i values\n", cbBufSize, cValues);

    *pcbEnumValues = cbBufSize;
    *pnEnumValues = cValues;

    if (cbEnumValues < cbBufSize)	/* buffer too small */
    {
	if (HeapFree (hHeap, 0, lpValue) == 0)
	    WARN ("HeapFree failed with code %i\n", GetLastError ());
	if (HeapFree (hHeap, 0, lpValueName) == 0)
	    WARN ("HeapFree failed with code %i\n", GetLastError ());
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	TRACE ("%i byte buffer is not large enough\n", cbEnumValues);
	return ERROR_MORE_DATA;
    }

    TRACE ("pass 2: copying all names and values to buffer\n");

    ppev = (PPRINTER_ENUM_VALUESW) pEnumValues;		/* array of structs */
    pEnumValues += cValues * sizeof (PRINTER_ENUM_VALUESW);

    for (dwIndex = 0; dwIndex < cValues; ++dwIndex)
    {
	cbValueNameLen = cbMaxValueNameLen; cbValueLen = cbMaxValueLen;
	ret = RegEnumValueW (hkSubKey, dwIndex, lpValueName, &cbValueNameLen,
		NULL, &dwType, lpValue, &cbValueLen);
	if (ret != ERROR_SUCCESS)
	{
	    if (HeapFree (hHeap, 0, lpValue) == 0)
		WARN ("HeapFree failed with code %i\n", GetLastError ());
	    if (HeapFree (hHeap, 0, lpValueName) == 0)
		WARN ("HeapFree failed with code %i\n", GetLastError ());
	    r = RegCloseKey (hkSubKey);
	    if (r != ERROR_SUCCESS)
		WARN ("RegCloseKey returned %i\n", r);
	    TRACE ("RegEnumValueW (%i) returned %i\n", dwIndex, ret);
	    return ret;
	}

	cbValueNameLen = (cbValueNameLen + 1) * sizeof (WCHAR);
	memcpy (pEnumValues, lpValueName, cbValueNameLen);
	ppev[dwIndex].pValueName = (LPWSTR) pEnumValues;
	pEnumValues += cbValueNameLen;

	/* return # of *bytes* (including trailing \0), not # of chars */
	ppev[dwIndex].cbValueName = cbValueNameLen;

	ppev[dwIndex].dwType = dwType;

	memcpy (pEnumValues, lpValue, cbValueLen);
	ppev[dwIndex].pData = pEnumValues;
	pEnumValues += cbValueLen;

	ppev[dwIndex].cbData = cbValueLen;

	TRACE ("%s [%i]: copied name (%i bytes) and data (%i bytes)\n",
		debugstr_w (lpValueName), dwIndex, cbValueNameLen, cbValueLen);
    }

    if (HeapFree (hHeap, 0, lpValue) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %i\n", ret);
	if (HeapFree (hHeap, 0, lpValueName) == 0)
	    WARN ("HeapFree failed with code %i\n", GetLastError ());
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	return ret;
    }

    if (HeapFree (hHeap, 0, lpValueName) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %i\n", ret);
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %i\n", r);
	return ret;
    }

    ret = RegCloseKey (hkSubKey);
    if (ret != ERROR_SUCCESS)
    {
	ERR ("RegCloseKey returned %i\n", ret);
	return ret;
    }

    return ERROR_SUCCESS;
}

/*******************************************************************************
 *		EnumPrinterDataExA	[WINSPOOL.@]
 *
 * This functions returns value names and REG_SZ, REG_EXPAND_SZ, and
 * REG_MULTI_SZ values as ASCII strings in Unicode-sized buffers.  This is
 * what Windows 2000 SP1 does.
 *
 */
DWORD WINAPI EnumPrinterDataExA(HANDLE hPrinter, LPCSTR pKeyName,
				LPBYTE pEnumValues, DWORD cbEnumValues,
				LPDWORD pcbEnumValues, LPDWORD pnEnumValues)
{
    INT	    len;
    LPWSTR  pKeyNameW;
    DWORD   ret, dwIndex, dwBufSize;
    HANDLE  hHeap;
    LPSTR   pBuffer;

    TRACE ("%p %s\n", hPrinter, pKeyName);

    if (pKeyName == NULL || *pKeyName == 0)
	return ERROR_INVALID_PARAMETER;

    len = MultiByteToWideChar (CP_ACP, 0, pKeyName, -1, NULL, 0);
    if (len == 0)
    {
	ret = GetLastError ();
	ERR ("MultiByteToWideChar failed with code %i\n", ret);
	return ret;
    }

    hHeap = GetProcessHeap ();
    if (hHeap == NULL)
    {
	ERR ("GetProcessHeap failed\n");
	return ERROR_OUTOFMEMORY;
    }

    pKeyNameW = HeapAlloc (hHeap, 0, len * sizeof (WCHAR));
    if (pKeyNameW == NULL)
    {
	ERR ("Failed to allocate %i bytes from process heap\n",
             (LONG)(len * sizeof (WCHAR)));
	return ERROR_OUTOFMEMORY;
    }

    if (MultiByteToWideChar (CP_ACP, 0, pKeyName, -1, pKeyNameW, len) == 0)
    {
	ret = GetLastError ();
	ERR ("MultiByteToWideChar failed with code %i\n", ret);
	if (HeapFree (hHeap, 0, pKeyNameW) == 0)
	    WARN ("HeapFree failed with code %i\n", GetLastError ());
	return ret;
    }

    ret = EnumPrinterDataExW (hPrinter, pKeyNameW, pEnumValues, cbEnumValues,
	    pcbEnumValues, pnEnumValues);
    if (ret != ERROR_SUCCESS)
    {
	if (HeapFree (hHeap, 0, pKeyNameW) == 0)
	    WARN ("HeapFree failed with code %i\n", GetLastError ());
	TRACE ("EnumPrinterDataExW returned %i\n", ret);
	return ret;
    }

    if (HeapFree (hHeap, 0, pKeyNameW) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %i\n", ret);
	return ret;
    }

    if (*pnEnumValues == 0)	/* empty key */
	return ERROR_SUCCESS;

    dwBufSize = 0;
    for (dwIndex = 0; dwIndex < *pnEnumValues; ++dwIndex)
    {
	PPRINTER_ENUM_VALUESW ppev =
		&((PPRINTER_ENUM_VALUESW) pEnumValues)[dwIndex];

	if (dwBufSize < ppev->cbValueName)
	    dwBufSize = ppev->cbValueName;

	if (dwBufSize < ppev->cbData && (ppev->dwType == REG_SZ ||
		ppev->dwType == REG_EXPAND_SZ || ppev->dwType == REG_MULTI_SZ))
	    dwBufSize = ppev->cbData;
    }

    TRACE ("Largest Unicode name or value is %i bytes\n", dwBufSize);

    pBuffer = HeapAlloc (hHeap, 0, dwBufSize);
    if (pBuffer == NULL)
    {
	ERR ("Failed to allocate %i bytes from process heap\n", dwBufSize);
	return ERROR_OUTOFMEMORY;
    }

    for (dwIndex = 0; dwIndex < *pnEnumValues; ++dwIndex)
    {
	PPRINTER_ENUM_VALUESW ppev =
		&((PPRINTER_ENUM_VALUESW) pEnumValues)[dwIndex];

	len = WideCharToMultiByte (CP_ACP, 0, ppev->pValueName,
		ppev->cbValueName / sizeof (WCHAR), pBuffer, dwBufSize, NULL,
		NULL);
	if (len == 0)
	{
	    ret = GetLastError ();
	    ERR ("WideCharToMultiByte failed with code %i\n", ret);
	    if (HeapFree (hHeap, 0, pBuffer) == 0)
		WARN ("HeapFree failed with code %i\n", GetLastError ());
	    return ret;
	}

	memcpy (ppev->pValueName, pBuffer, len);

	TRACE ("Converted '%s' from Unicode to ASCII\n", pBuffer);

	if (ppev->dwType != REG_SZ && ppev->dwType != REG_EXPAND_SZ &&
		ppev->dwType != REG_MULTI_SZ)
	    continue;

	len = WideCharToMultiByte (CP_ACP, 0, (LPWSTR) ppev->pData,
		ppev->cbData / sizeof (WCHAR), pBuffer, dwBufSize, NULL, NULL);
	if (len == 0)
	{
	    ret = GetLastError ();
	    ERR ("WideCharToMultiByte failed with code %i\n", ret);
	    if (HeapFree (hHeap, 0, pBuffer) == 0)
		WARN ("HeapFree failed with code %i\n", GetLastError ());
	    return ret;
	}

	memcpy (ppev->pData, pBuffer, len);

	TRACE ("Converted '%s' from Unicode to ASCII\n", pBuffer);
	TRACE ("  (only first string of REG_MULTI_SZ printed)\n");
    }

    if (HeapFree (hHeap, 0, pBuffer) == 0)
    {
	ret = GetLastError ();
	ERR ("HeapFree failed with code %i\n", ret);
	return ret;
    }

    return ERROR_SUCCESS;
}

/******************************************************************************
 *      AbortPrinter (WINSPOOL.@)
 */
BOOL WINAPI AbortPrinter( HANDLE hPrinter )
{
    FIXME("(%p), stub!\n", hPrinter);
    return TRUE;
}

/******************************************************************************
 *		AddPortA (WINSPOOL.@)
 *
 * See AddPortW.
 *
 */
BOOL WINAPI AddPortA(LPSTR pName, HWND hWnd, LPSTR pMonitorName)
{
    LPWSTR  nameW = NULL;
    LPWSTR  monitorW = NULL;
    DWORD   len;
    BOOL    res;

    TRACE("(%s, %p, %s)\n",debugstr_a(pName), hWnd, debugstr_a(pMonitorName));

    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    if (pMonitorName) {
        len = MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, NULL, 0);
        monitorW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, monitorW, len);
    }
    res = AddPortW(nameW, hWnd, monitorW);
    HeapFree(GetProcessHeap(), 0, nameW);
    HeapFree(GetProcessHeap(), 0, monitorW);
    return res;
}

/******************************************************************************
 *      AddPortW (WINSPOOL.@)
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
BOOL WINAPI AddPortW(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName)
{
    monitor_t * pm;
    monitor_t * pui;
    DWORD       res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pMonitorName));

    if (pName && pName[0]) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pMonitorName) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    /* an empty Monitorname is Invalid */
    if (!pMonitorName[0]) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pm = monitor_load(pMonitorName, NULL);
    if (pm && pm->monitor && pm->monitor->pfnAddPort) {
        res = pm->monitor->pfnAddPort(pName, hWnd, pMonitorName);
        TRACE("got %d with %u\n", res, GetLastError());
        res = TRUE;
    }
    else
    {
        pui = monitor_loadui(pm);
        if (pui && pui->monitorUI && pui->monitorUI->pfnAddPortUI) {
            TRACE("use %p: %s\n", pui, debugstr_w(pui->dllname));
            res = pui->monitorUI->pfnAddPortUI(pName, hWnd, pMonitorName, NULL);
            TRACE("got %d with %u\n", res, GetLastError());
            res = TRUE;
        }
        else
        {
            FIXME("not implemented for %s (%p: %s => %p: %s)\n", debugstr_w(pMonitorName),
                pm, pm ? debugstr_w(pm->dllname) : NULL, pui, pui ? debugstr_w(pui->dllname) : NULL);

            /* XP: ERROR_NOT_SUPPORTED, NT351,9x: ERROR_INVALID_PARAMETER */
            SetLastError(ERROR_NOT_SUPPORTED);
            res = FALSE;
        }
        monitor_unload(pui);
    }
    monitor_unload(pm);
    TRACE("returning %d with %u\n", res, GetLastError());
    return res;
}

/******************************************************************************
 *             AddPortExA (WINSPOOL.@)
 *
 * See AddPortExW.
 *
 */
BOOL WINAPI AddPortExA(HANDLE hMonitor, LPSTR pName, DWORD Level, LPBYTE lpBuffer, LPSTR lpMonitorName)
{
    FIXME("(%p, %s, %d, %p, %s), stub!\n",hMonitor, debugstr_a(pName), Level,
          lpBuffer, debugstr_a(lpMonitorName));
    return FALSE;
}

/******************************************************************************
 *             AddPortExW (WINSPOOL.@)
 *
 * Add a Port for a specific Monitor, without presenting a user interface
 *
 * PARAMS
 *  hMonitor      [I] Handle from InitializePrintMonitor2()
 *  pName         [I] Servername or NULL (local Computer)
 *  Level         [I] Structure-Level (1 or 2) for lpBuffer
 *  lpBuffer      [I] PTR to: PORT_INFO_1 or PORT_INFO_2
 *  lpMonitorName [I] Name of the Monitor that manage the Port or NULL
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * BUGS
 *  only a Stub
 *
 */
BOOL WINAPI AddPortExW(HANDLE hMonitor, LPWSTR pName, DWORD Level, LPBYTE lpBuffer, LPWSTR lpMonitorName)
{
    FIXME("(%p, %s, %d, %p, %s), stub!\n", hMonitor, debugstr_w(pName), Level,
          lpBuffer, debugstr_w(lpMonitorName));
    return FALSE;
}

/******************************************************************************
 *      AddPrinterConnectionA (WINSPOOL.@)
 */
BOOL WINAPI AddPrinterConnectionA( LPSTR pName )
{
    FIXME("%s\n", debugstr_a(pName));
    return FALSE;
}

/******************************************************************************
 *      AddPrinterConnectionW (WINSPOOL.@)
 */
BOOL WINAPI AddPrinterConnectionW( LPWSTR pName )
{
    FIXME("%s\n", debugstr_w(pName));
    return FALSE;
}

/******************************************************************************
 *		AddPrinterDriverExW (WINSPOOL.@)
 */
BOOL WINAPI AddPrinterDriverExW( LPWSTR pName, DWORD Level,
    LPBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    FIXME("%s %d %p %d\n", debugstr_w(pName),
           Level, pDriverInfo, dwFileCopyFlags);
    SetLastError(ERROR_PRINTER_DRIVER_BLOCKED);
    return FALSE;
}

/******************************************************************************
 *		AddPrinterDriverExA (WINSPOOL.@)
 */
BOOL WINAPI AddPrinterDriverExA( LPSTR pName, DWORD Level,
    LPBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    FIXME("%s %d %p %d\n", debugstr_a(pName),
           Level, pDriverInfo, dwFileCopyFlags);
    SetLastError(ERROR_PRINTER_DRIVER_BLOCKED);
    return FALSE;
}

/******************************************************************************
 *      ConfigurePortA (WINSPOOL.@)
 *
 * See ConfigurePortW.
 *
 */
BOOL WINAPI ConfigurePortA(LPSTR pName, HWND hWnd, LPSTR pPortName)
{
    LPWSTR  nameW = NULL;
    LPWSTR  portW = NULL;
    INT     len;
    DWORD   res;

    TRACE("(%s, %p, %s)\n", debugstr_a(pName), hWnd, debugstr_a(pPortName));

    /* convert servername to unicode */
    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    /* convert portname to unicode */
    if (pPortName) {
        len = MultiByteToWideChar(CP_ACP, 0, pPortName, -1, NULL, 0);
        portW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pPortName, -1, portW, len);
    }

    res = ConfigurePortW(nameW, hWnd, portW);
    HeapFree(GetProcessHeap(), 0, nameW);
    HeapFree(GetProcessHeap(), 0, portW);
    return res;
}

/******************************************************************************
 *      ConfigurePortW (WINSPOOL.@)
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
BOOL WINAPI ConfigurePortW(LPWSTR pName, HWND hWnd, LPWSTR pPortName)
{
    monitor_t * pm;
    monitor_t * pui;
    DWORD       res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    if (pName && pName[0]) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pPortName) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    /* an empty Portname is Invalid, but can popup a Dialog */
    if (!pPortName[0]) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pm = monitor_load_by_port(pPortName);
    if (pm && pm->monitor && pm->monitor->pfnConfigurePort) {
        TRACE("Using %s for %s (%p: %s)\n", debugstr_w(pm->name), debugstr_w(pPortName), pm, debugstr_w(pm->dllname));
        res = pm->monitor->pfnConfigurePort(pName, hWnd, pPortName);
        TRACE("got %d with %u\n", res, GetLastError());
    }
    else
    {
        pui = monitor_loadui(pm);
        if (pui && pui->monitorUI && pui->monitorUI->pfnConfigurePortUI) {
            TRACE("Use %s for %s (%p: %s)\n", debugstr_w(pui->name), debugstr_w(pPortName), pui, debugstr_w(pui->dllname));
            res = pui->monitorUI->pfnConfigurePortUI(pName, hWnd, pPortName);
            TRACE("got %d with %u\n", res, GetLastError());
        }
        else
        {
            FIXME("not implemented for %s (%p: %s => %p: %s)\n", debugstr_w(pPortName),
                pm, pm ? debugstr_w(pm->dllname) : NULL, pui, pui ? debugstr_w(pui->dllname) : NULL);

            /* XP: ERROR_NOT_SUPPORTED, NT351,9x: ERROR_INVALID_PARAMETER */
            SetLastError(ERROR_NOT_SUPPORTED);
            res = FALSE;
        }
        monitor_unload(pui);
    }
    monitor_unload(pm);

    TRACE("returning %d with %u\n", res, GetLastError());
    return res;
}

/******************************************************************************
 *      ConnectToPrinterDlg (WINSPOOL.@)
 */
HANDLE WINAPI ConnectToPrinterDlg( HWND hWnd, DWORD Flags )
{
    FIXME("%p %x\n", hWnd, Flags);
    return NULL;
}

/******************************************************************************
 *      DeletePrinterConnectionA (WINSPOOL.@)
 */
BOOL WINAPI DeletePrinterConnectionA( LPSTR pName )
{
    FIXME("%s\n", debugstr_a(pName));
    return TRUE;
}

/******************************************************************************
 *      DeletePrinterConnectionW (WINSPOOL.@)
 */
BOOL WINAPI DeletePrinterConnectionW( LPWSTR pName )
{
    FIXME("%s\n", debugstr_w(pName));
    return TRUE;
}

/******************************************************************************
 *		DeletePrinterDriverExW (WINSPOOL.@)
 */
BOOL WINAPI DeletePrinterDriverExW( LPWSTR pName, LPWSTR pEnvironment,
    LPWSTR pDriverName, DWORD dwDeleteFlag, DWORD dwVersionFlag)
{
    HKEY hkey_drivers;
    BOOL ret = FALSE;

    TRACE("%s %s %s %x %x\n", debugstr_w(pName), debugstr_w(pEnvironment),
          debugstr_w(pDriverName), dwDeleteFlag, dwVersionFlag);

    if(pName && pName[0])
    {
        FIXME("pName = %s - unsupported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(dwDeleteFlag)
    {
        FIXME("dwDeleteFlag = %x - unsupported\n", dwDeleteFlag);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hkey_drivers = WINSPOOL_OpenDriverReg(pEnvironment, TRUE);

    if(!hkey_drivers)
    {
        ERR("Can't open drivers key\n");
        return FALSE;
    }

    if(WINSPOOL_SHDeleteKeyW(hkey_drivers, pDriverName) == ERROR_SUCCESS)
        ret = TRUE;

    RegCloseKey(hkey_drivers);

    return ret;
}

/******************************************************************************
 *		DeletePrinterDriverExA (WINSPOOL.@)
 */
BOOL WINAPI DeletePrinterDriverExA( LPSTR pName, LPSTR pEnvironment,
    LPSTR pDriverName, DWORD dwDeleteFlag, DWORD dwVersionFlag)
{
    UNICODE_STRING NameW, EnvW, DriverW;
    BOOL ret;

    asciitounicode(&NameW, pName);
    asciitounicode(&EnvW, pEnvironment);
    asciitounicode(&DriverW, pDriverName);

    ret = DeletePrinterDriverExW(NameW.Buffer, EnvW.Buffer, DriverW.Buffer, dwDeleteFlag, dwVersionFlag);

    RtlFreeUnicodeString(&DriverW);
    RtlFreeUnicodeString(&EnvW);
    RtlFreeUnicodeString(&NameW);

    return ret;
}

/******************************************************************************
 *		DeletePrinterDataExW (WINSPOOL.@)
 */
DWORD WINAPI DeletePrinterDataExW( HANDLE hPrinter, LPCWSTR pKeyName,
                                  LPCWSTR pValueName)
{
    FIXME("%p %s %s\n", hPrinter, 
          debugstr_w(pKeyName), debugstr_w(pValueName));
    return ERROR_INVALID_PARAMETER;
}

/******************************************************************************
 *		DeletePrinterDataExA (WINSPOOL.@)
 */
DWORD WINAPI DeletePrinterDataExA( HANDLE hPrinter, LPCSTR pKeyName,
                                  LPCSTR pValueName)
{
    FIXME("%p %s %s\n", hPrinter, 
          debugstr_a(pKeyName), debugstr_a(pValueName));
    return ERROR_INVALID_PARAMETER;
}

/******************************************************************************
 *      DeletePrintProcessorA (WINSPOOL.@)
 */
BOOL WINAPI DeletePrintProcessorA(LPSTR pName, LPSTR pEnvironment, LPSTR pPrintProcessorName)
{
    FIXME("%s %s %s\n", debugstr_a(pName), debugstr_a(pEnvironment),
          debugstr_a(pPrintProcessorName));
    return TRUE;
}

/******************************************************************************
 *      DeletePrintProcessorW (WINSPOOL.@)
 */
BOOL WINAPI DeletePrintProcessorW(LPWSTR pName, LPWSTR pEnvironment, LPWSTR pPrintProcessorName)
{
    FIXME("%s %s %s\n", debugstr_w(pName), debugstr_w(pEnvironment),
          debugstr_w(pPrintProcessorName));
    return TRUE;
}

/******************************************************************************
 *      DeletePrintProvidorA (WINSPOOL.@)
 */
BOOL WINAPI DeletePrintProvidorA(LPSTR pName, LPSTR pEnvironment, LPSTR pPrintProviderName)
{
    FIXME("%s %s %s\n", debugstr_a(pName), debugstr_a(pEnvironment),
          debugstr_a(pPrintProviderName));
    return TRUE;
}

/******************************************************************************
 *      DeletePrintProvidorW (WINSPOOL.@)
 */
BOOL WINAPI DeletePrintProvidorW(LPWSTR pName, LPWSTR pEnvironment, LPWSTR pPrintProviderName)
{
    FIXME("%s %s %s\n", debugstr_w(pName), debugstr_w(pEnvironment),
          debugstr_w(pPrintProviderName));
    return TRUE;
}

/******************************************************************************
 *      EnumFormsA (WINSPOOL.@)
 */
BOOL WINAPI EnumFormsA( HANDLE hPrinter, DWORD Level, LPBYTE pForm,
    DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned )
{
    FIXME("%p %x %p %x %p %p\n", hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *      EnumFormsW (WINSPOOL.@)
 */
BOOL WINAPI EnumFormsW( HANDLE hPrinter, DWORD Level, LPBYTE pForm,
    DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned )
{
    FIXME("%p %x %p %x %p %p\n", hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *          EnumMonitorsA [WINSPOOL.@]
 *
 * See EnumMonitorsW.
 *
 */
BOOL WINAPI EnumMonitorsA(LPSTR pName, DWORD Level, LPBYTE pMonitors,
                          DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    BOOL    res;
    LPBYTE  bufferW = NULL;
    LPWSTR  nameW = NULL;
    DWORD   needed = 0;
    DWORD   numentries = 0;
    INT     len;

    TRACE("(%s, %d, %p, %d, %p, %p)\n", debugstr_a(pName), Level, pMonitors,
          cbBuf, pcbNeeded, pcReturned);

    /* convert servername to unicode */
    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }
    /* alloc (userbuffersize*sizeof(WCHAR) and try to enum the monitors */
    needed = cbBuf * sizeof(WCHAR);    
    if (needed) bufferW = HeapAlloc(GetProcessHeap(), 0, needed);
    res = EnumMonitorsW(nameW, Level, bufferW, needed, pcbNeeded, pcReturned);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (pcbNeeded) needed = *pcbNeeded;
        /* HeapReAlloc return NULL, when bufferW was NULL */
        bufferW = (bufferW) ? HeapReAlloc(GetProcessHeap(), 0, bufferW, needed) :
                              HeapAlloc(GetProcessHeap(), 0, needed);

        /* Try again with the large Buffer */
        res = EnumMonitorsW(nameW, Level, bufferW, needed, pcbNeeded, pcReturned);
    }
    numentries = pcReturned ? *pcReturned : 0;
    needed = 0;
    /*
       W2k require the buffersize from EnumMonitorsW also for EnumMonitorsA.
       We use the smaller Ansi-Size to avoid conflicts with fixed Buffers of old Apps.
     */
    if (res) {
        /* EnumMonitorsW collected all Data. Parse them to calculate ANSI-Size */
        DWORD   entrysize = 0;
        DWORD   index;
        LPSTR   ptr;
        LPMONITOR_INFO_2W mi2w;
        LPMONITOR_INFO_2A mi2a;

        /* MONITOR_INFO_*W and MONITOR_INFO_*A have the same size */
        entrysize = (Level == 1) ? sizeof(MONITOR_INFO_1A) : sizeof(MONITOR_INFO_2A);

        /* First pass: calculate the size for all Entries */
        mi2w = (LPMONITOR_INFO_2W) bufferW;
        mi2a = (LPMONITOR_INFO_2A) pMonitors;
        index = 0;
        while (index < numentries) {
            index++;
            needed += entrysize;    /* MONITOR_INFO_?A */
            TRACE("%p: parsing #%d (%s)\n", mi2w, index, debugstr_w(mi2w->pName));

            needed += WideCharToMultiByte(CP_ACP, 0, mi2w->pName, -1,
                                            NULL, 0, NULL, NULL);
            if (Level > 1) {
                needed += WideCharToMultiByte(CP_ACP, 0, mi2w->pEnvironment, -1,
                                                NULL, 0, NULL, NULL);
                needed += WideCharToMultiByte(CP_ACP, 0, mi2w->pDLLName, -1,
                                                NULL, 0, NULL, NULL);
            }
            /* use LPBYTE with entrysize to avoid double code (MONITOR_INFO_1 + MONITOR_INFO_2) */
            mi2w = (LPMONITOR_INFO_2W) (((LPBYTE)mi2w) + entrysize);
            mi2a = (LPMONITOR_INFO_2A) (((LPBYTE)mi2a) + entrysize);
        }

        /* check for errors and quit on failure */
        if (cbBuf < needed) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            res = FALSE;
            goto emA_cleanup;
        }
        len = entrysize * numentries;       /* room for all MONITOR_INFO_?A */
        ptr = (LPSTR) &pMonitors[len];      /* room for strings */
        cbBuf -= len ;                      /* free Bytes in the user-Buffer */
        mi2w = (LPMONITOR_INFO_2W) bufferW;
        mi2a = (LPMONITOR_INFO_2A) pMonitors;
        index = 0;
        /* Second Pass: Fill the User Buffer (if we have one) */
        while ((index < numentries) && pMonitors) {
            index++;
            TRACE("%p: writing MONITOR_INFO_%dA #%d\n", mi2a, Level, index);
            mi2a->pName = ptr;
            len = WideCharToMultiByte(CP_ACP, 0, mi2w->pName, -1,
                                            ptr, cbBuf , NULL, NULL);
            ptr += len;
            cbBuf -= len;
            if (Level > 1) {
                mi2a->pEnvironment = ptr;
                len = WideCharToMultiByte(CP_ACP, 0, mi2w->pEnvironment, -1,
                                            ptr, cbBuf, NULL, NULL);
                ptr += len;
                cbBuf -= len;

                mi2a->pDLLName = ptr;
                len = WideCharToMultiByte(CP_ACP, 0, mi2w->pDLLName, -1,
                                            ptr, cbBuf, NULL, NULL);
                ptr += len;
                cbBuf -= len;
            }
            /* use LPBYTE with entrysize to avoid double code (MONITOR_INFO_1 + MONITOR_INFO_2) */
            mi2w = (LPMONITOR_INFO_2W) (((LPBYTE)mi2w) + entrysize);
            mi2a = (LPMONITOR_INFO_2A) (((LPBYTE)mi2a) + entrysize);
        }
    }
emA_cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = (res) ? numentries : 0;

    HeapFree(GetProcessHeap(), 0, nameW);
    HeapFree(GetProcessHeap(), 0, bufferW);

    TRACE("returning %d with %d (%d byte for %d entries)\n", 
            (res), GetLastError(), needed, numentries);

    return (res);

}

/*****************************************************************************
 *          EnumMonitorsW [WINSPOOL.@]
 *
 * Enumerate available Port-Monitors
 *
 * PARAMS
 *  pName       [I] Servername or NULL (local Computer)
 *  Level       [I] Structure-Level (1:Win9x+NT or 2:NT only)
 *  pMonitors   [O] PTR to Buffer that receives the Result
 *  cbBuf       [I] Size of Buffer at pMonitors
 *  pcbNeeded   [O] PTR to DWORD that receives the size in Bytes used / required for pMonitors
 *  pcReturned  [O] PTR to DWORD that receives the number of Monitors in pMonitors
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for buffer, if cbBuf is too small
 *
 * NOTES
 *  Windows reads the Registry once and cache the Results.
 *
 *|  Language-Monitors are also installed in the same Registry-Location but 
 *|  they are filtered in Windows (not returned by EnumMonitors).
 *|  We do no filtering to simplify our Code.
 *
 */
BOOL WINAPI EnumMonitorsW(LPWSTR pName, DWORD Level, LPBYTE pMonitors,
                          DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD   needed = 0;
    DWORD   numentries = 0;
    BOOL    res = FALSE;

    TRACE("(%s, %d, %p, %d, %p, %p)\n", debugstr_w(pName), Level, pMonitors,
          cbBuf, pcbNeeded, pcReturned);

    if (pName && (lstrlenW(pName))) {
        FIXME("for Server %s not implemented\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        goto emW_cleanup;
    }

    /* Level is not checked in win9x */
    if (!Level || (Level > 2)) {
        WARN("level (%d) is ignored in win9x\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        goto emW_cleanup;
    }
    if (!pcbNeeded) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        goto emW_cleanup;
    }

    /* Scan all Monitor-Keys */
    numentries = 0;
    needed = get_local_monitors(Level, NULL, 0, &numentries);

    /* we calculated the needed buffersize. now do the error-checks */
    if (cbBuf < needed) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto emW_cleanup;
    }
    else if (!pMonitors || !pcReturned) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        goto emW_cleanup;
    }

    /* fill the Buffer with the Monitor-Keys */
    needed = get_local_monitors(Level, pMonitors, cbBuf, &numentries);
    res = TRUE;

emW_cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = numentries;

    TRACE("returning %d with %d (%d byte for %d entries)\n", 
            res, GetLastError(), needed, numentries);

    return (res);
}

/******************************************************************************
 *		XcvDataW (WINSPOOL.@)
 *
 * Execute commands in the Printmonitor DLL
 *
 * PARAMS
 *  hXcv            [i] Handle from OpenPrinter (with XcvMonitor or XcvPort)
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
BOOL WINAPI XcvDataW( HANDLE hXcv, LPCWSTR pszDataName, PBYTE pInputData,
    DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData,
    PDWORD pcbOutputNeeded, PDWORD pdwStatus)
{
    opened_printer_t *printer;

    TRACE("(%p, %s, %p, %d, %p, %d, %p, %p)\n", hXcv, debugstr_w(pszDataName),
          pInputData, cbInputData, pOutputData,
          cbOutputData, pcbOutputNeeded, pdwStatus);

    printer = get_opened_printer(hXcv);
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

    *pdwStatus = printer->pm->monitor->pfnXcvDataPort(printer->hXcv, pszDataName,
            pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded);

    return TRUE;
}

/*****************************************************************************
 *          EnumPrinterDataA [WINSPOOL.@]
 *
 */
DWORD WINAPI EnumPrinterDataA( HANDLE hPrinter, DWORD dwIndex, LPSTR pValueName,
    DWORD cbValueName, LPDWORD pcbValueName, LPDWORD pType, LPBYTE pData,
    DWORD cbData, LPDWORD pcbData )
{
    FIXME("%p %x %p %x %p %p %p %x %p\n", hPrinter, dwIndex, pValueName,
          cbValueName, pcbValueName, pType, pData, cbData, pcbData);
    return ERROR_NO_MORE_ITEMS;
}

/*****************************************************************************
 *          EnumPrinterDataW [WINSPOOL.@]
 *
 */
DWORD WINAPI EnumPrinterDataW( HANDLE hPrinter, DWORD dwIndex, LPWSTR pValueName,
    DWORD cbValueName, LPDWORD pcbValueName, LPDWORD pType, LPBYTE pData,
    DWORD cbData, LPDWORD pcbData )
{
    FIXME("%p %x %p %x %p %p %p %x %p\n", hPrinter, dwIndex, pValueName,
          cbValueName, pcbValueName, pType, pData, cbData, pcbData);
    return ERROR_NO_MORE_ITEMS;
}

/*****************************************************************************
 *          EnumPrintProcessorDatatypesA [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumPrintProcessorDatatypesA(LPSTR pName, LPSTR pPrintProcessorName,
                                         DWORD Level, LPBYTE pDatatypes, DWORD cbBuf,
                                         LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    FIXME("Stub: %s %s %d %p %d %p %p\n", debugstr_a(pName),
          debugstr_a(pPrintProcessorName), Level, pDatatypes, cbBuf,
          pcbNeeded, pcReturned);
    return FALSE;
}

/*****************************************************************************
 *          EnumPrintProcessorDatatypesW [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumPrintProcessorDatatypesW(LPWSTR pName, LPWSTR pPrintProcessorName,
                                         DWORD Level, LPBYTE pDatatypes, DWORD cbBuf,
                                         LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    FIXME("Stub: %s %s %d %p %d %p %p\n", debugstr_w(pName),
          debugstr_w(pPrintProcessorName), Level, pDatatypes, cbBuf,
          pcbNeeded, pcReturned);
    return FALSE;
}

/*****************************************************************************
 *          EnumPrintProcessorsA [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumPrintProcessorsA(LPSTR pName, LPSTR pEnvironment, DWORD Level, 
    LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcbReturned)
{
    FIXME("Stub: %s %s %d %p %d %p %p\n", pName, pEnvironment, Level,
        pPrintProcessorInfo, cbBuf, pcbNeeded, pcbReturned);
    return FALSE;
}

/*****************************************************************************
 *          EnumPrintProcessorsW [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumPrintProcessorsW(LPWSTR pName, LPWSTR pEnvironment, DWORD Level,
    LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcbReturned)
{
    FIXME("Stub: %s %s %d %p %d %p %p\n", debugstr_w(pName),
        debugstr_w(pEnvironment), Level, pPrintProcessorInfo,
        cbBuf, pcbNeeded, pcbReturned);
    return FALSE;
}

/*****************************************************************************
 *          ExtDeviceMode [WINSPOOL.@]
 *
 */
LONG WINAPI ExtDeviceMode( HWND hWnd, HANDLE hInst, LPDEVMODEA pDevModeOutput,
    LPSTR pDeviceName, LPSTR pPort, LPDEVMODEA pDevModeInput, LPSTR pProfile,
    DWORD fMode)
{
    FIXME("Stub: %p %p %p %s %s %p %s %x\n", hWnd, hInst, pDevModeOutput,
          debugstr_a(pDeviceName), debugstr_a(pPort), pDevModeInput,
          debugstr_a(pProfile), fMode);
    return -1;
}

/*****************************************************************************
 *          FindClosePrinterChangeNotification [WINSPOOL.@]
 *
 */
BOOL WINAPI FindClosePrinterChangeNotification( HANDLE hChange )
{
    FIXME("Stub: %p\n", hChange);
    return TRUE;
}

/*****************************************************************************
 *          FindFirstPrinterChangeNotification [WINSPOOL.@]
 *
 */
HANDLE WINAPI FindFirstPrinterChangeNotification( HANDLE hPrinter,
    DWORD fdwFlags, DWORD fdwOptions, LPVOID pPrinterNotifyOptions )
{
    FIXME("Stub: %p %x %x %p\n",
          hPrinter, fdwFlags, fdwOptions, pPrinterNotifyOptions);
    return INVALID_HANDLE_VALUE;
}

/*****************************************************************************
 *          FindNextPrinterChangeNotification [WINSPOOL.@]
 *
 */
BOOL WINAPI FindNextPrinterChangeNotification( HANDLE hChange, PDWORD pdwChange,
    LPVOID pPrinterNotifyOptions, LPVOID *ppPrinterNotifyInfo )
{
    FIXME("Stub: %p %p %p %p\n",
          hChange, pdwChange, pPrinterNotifyOptions, ppPrinterNotifyInfo);
    return FALSE;
}

/*****************************************************************************
 *          FreePrinterNotifyInfo [WINSPOOL.@]
 *
 */
BOOL WINAPI FreePrinterNotifyInfo( PPRINTER_NOTIFY_INFO pPrinterNotifyInfo )
{
    FIXME("Stub: %p\n", pPrinterNotifyInfo);
    return TRUE;
}

/*****************************************************************************
 *          string_to_buf
 *
 * Copies a unicode string into a buffer.  The buffer will either contain unicode or
 * ansi depending on the unicode parameter.
 */
static BOOL string_to_buf(LPCWSTR str, LPBYTE ptr, DWORD cb, DWORD *size, BOOL unicode)
{
    if(!str)
    {
        *size = 0;
        return TRUE;
    }

    if(unicode)
    {
        *size = (strlenW(str) + 1) * sizeof(WCHAR);
        if(*size <= cb)
        {
            memcpy(ptr, str, *size);
            return TRUE;
        }
        return FALSE;
    }
    else
    {
        *size = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
        if(*size <= cb)
        {
            WideCharToMultiByte(CP_ACP, 0, str, -1, (LPSTR)ptr, *size, NULL, NULL);
            return TRUE;
        }
        return FALSE;
    }
}

/*****************************************************************************
 *          get_job_info_1
 */
static BOOL get_job_info_1(job_t *job, JOB_INFO_1W *ji1, LPBYTE buf, DWORD cbBuf,
                           LPDWORD pcbNeeded, BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(space)
    {
        ji1->JobId = job->job_id;
    }

    string_to_buf(job->document_title, ptr, left, &size, unicode);
    if(space && size <= left)
    {
        ji1->pDocument = (LPWSTR)ptr;
        ptr += size;
        left -= size;
    }
    else
        space = FALSE;
    *pcbNeeded += size;

    return space;
}

/*****************************************************************************
 *          get_job_info_2
 */
static BOOL get_job_info_2(job_t *job, JOB_INFO_2W *ji2, LPBYTE buf, DWORD cbBuf,
                           LPDWORD pcbNeeded, BOOL unicode)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(space)
    {
        ji2->JobId = job->job_id;
    }

    string_to_buf(job->document_title, ptr, left, &size, unicode);
    if(space && size <= left)
    {
        ji2->pDocument = (LPWSTR)ptr;
        ptr += size;
        left -= size;
    }
    else
        space = FALSE;
    *pcbNeeded += size;

    return space;
}

/*****************************************************************************
 *          get_job_info
 */
static BOOL get_job_info(HANDLE hPrinter, DWORD JobId, DWORD Level, LPBYTE pJob,
                         DWORD cbBuf, LPDWORD pcbNeeded, BOOL unicode)
{
    BOOL ret = FALSE;
    DWORD needed = 0, size;
    job_t *job;
    LPBYTE ptr = pJob;

    TRACE("%p %d %d %p %d %p\n", hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);

    EnterCriticalSection(&printer_handles_cs);
    job = get_job(hPrinter, JobId);
    if(!job)
        goto end;

    switch(Level)
    {
    case 1:
        size = sizeof(JOB_INFO_1W);
        if(cbBuf >= size)
        {
            cbBuf -= size;
            ptr += size;
            memset(pJob, 0, size);
        }
        else
            cbBuf = 0;
        ret = get_job_info_1(job, (JOB_INFO_1W *)pJob, ptr, cbBuf, &needed, unicode);
        needed += size;
        break;

    case 2:
        size = sizeof(JOB_INFO_2W);
        if(cbBuf >= size)
        {
            cbBuf -= size;
            ptr += size;
            memset(pJob, 0, size);
        }
        else
            cbBuf = 0;
        ret = get_job_info_2(job, (JOB_INFO_2W *)pJob, ptr, cbBuf, &needed, unicode);
        needed += size;
        break;

    case 3:
        size = sizeof(JOB_INFO_3);
        if(cbBuf >= size)
        {
            cbBuf -= size;
            memset(pJob, 0, size);
            ret = TRUE;
        }
        else
            cbBuf = 0;
        needed = size;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        goto end;
    }
    if(pcbNeeded)
        *pcbNeeded = needed;
end:
    LeaveCriticalSection(&printer_handles_cs);
    return ret;
}

/*****************************************************************************
 *          GetJobA [WINSPOOL.@]
 *
 */
BOOL WINAPI GetJobA(HANDLE hPrinter, DWORD JobId, DWORD Level, LPBYTE pJob,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    return get_job_info(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded, FALSE);
}

/*****************************************************************************
 *          GetJobW [WINSPOOL.@]
 *
 */
BOOL WINAPI GetJobW(HANDLE hPrinter, DWORD JobId, DWORD Level, LPBYTE pJob,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    return get_job_info(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded, TRUE);
}

/*****************************************************************************
 *          schedule_lpr
 */
static BOOL schedule_lpr(LPCWSTR printer_name, LPCWSTR filename)
{
    char *unixname, *queue, *cmd;
    char fmt[] = "lpr -P%s %s";
    DWORD len;

    if(!(unixname = wine_get_unix_file_name(filename)))
        return FALSE;

    len = WideCharToMultiByte(CP_ACP, 0, printer_name, -1, NULL, 0, NULL, NULL);
    queue = HeapAlloc(GetProcessHeap(), 0, len);
    WideCharToMultiByte(CP_ACP, 0, printer_name, -1, queue, len, NULL, NULL);

    cmd = HeapAlloc(GetProcessHeap(), 0, strlen(unixname) + len + sizeof(fmt) - 5);
    sprintf(cmd, fmt, queue, unixname);

    TRACE("printing with: %s\n", cmd);
    system(cmd);

    HeapFree(GetProcessHeap(), 0, cmd);
    HeapFree(GetProcessHeap(), 0, queue);
    HeapFree(GetProcessHeap(), 0, unixname);
    return TRUE;
}

/*****************************************************************************
 *          schedule_cups
 */
static BOOL schedule_cups(LPCWSTR printer_name, LPCWSTR filename, LPCWSTR document_title)
{
#if HAVE_CUPS_CUPS_H
    if(pcupsPrintFile)
    {
        char *unixname, *queue, *doc_titleA;
        DWORD len;
        BOOL ret;

        if(!(unixname = wine_get_unix_file_name(filename)))
            return FALSE;

        len = WideCharToMultiByte(CP_ACP, 0, printer_name, -1, NULL, 0, NULL, NULL);
        queue = HeapAlloc(GetProcessHeap(), 0, len);
        WideCharToMultiByte(CP_ACP, 0, printer_name, -1, queue, len, NULL, NULL);

        len = WideCharToMultiByte(CP_ACP, 0, document_title, -1, NULL, 0, NULL, NULL);
        doc_titleA = HeapAlloc(GetProcessHeap(), 0, len);
        WideCharToMultiByte(CP_ACP, 0, document_title, -1, doc_titleA, len, NULL, NULL);

        TRACE("printing via cups\n");
        ret = pcupsPrintFile(queue, unixname, doc_titleA, 0, NULL);
        HeapFree(GetProcessHeap(), 0, doc_titleA);
        HeapFree(GetProcessHeap(), 0, queue);
        HeapFree(GetProcessHeap(), 0, unixname);
        return ret;
    }
    else
#endif
    {
        return schedule_lpr(printer_name, filename);
    }
}

INT_PTR CALLBACK file_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LPWSTR filename;

    switch(msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtrW(hwnd, DWLP_USER, lparam);
        return TRUE;

    case WM_COMMAND:
        if(HIWORD(wparam) == BN_CLICKED)
        {
            if(LOWORD(wparam) == IDOK)
            {
                HANDLE hf;
                DWORD len = SendDlgItemMessageW(hwnd, EDITBOX, WM_GETTEXTLENGTH, 0, 0);
                LPWSTR *output;

                filename = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
                GetDlgItemTextW(hwnd, EDITBOX, filename, len + 1);

                if(GetFileAttributesW(filename) != INVALID_FILE_ATTRIBUTES)
                {
                    WCHAR caption[200], message[200];
                    int mb_ret;

                    LoadStringW(WINSPOOL_hInstance, IDS_CAPTION, caption, sizeof(caption) / sizeof(WCHAR));
                    LoadStringW(WINSPOOL_hInstance, IDS_FILE_EXISTS, message, sizeof(message) / sizeof(WCHAR));
                    mb_ret = MessageBoxW(hwnd, message, caption, MB_OKCANCEL | MB_ICONEXCLAMATION);
                    if(mb_ret == IDCANCEL)
                    {
                        HeapFree(GetProcessHeap(), 0, filename);
                        return TRUE;
                    }
                }
                hf = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if(hf == INVALID_HANDLE_VALUE)
                {
                    WCHAR caption[200], message[200];

                    LoadStringW(WINSPOOL_hInstance, IDS_CAPTION, caption, sizeof(caption) / sizeof(WCHAR));
                    LoadStringW(WINSPOOL_hInstance, IDS_CANNOT_OPEN, message, sizeof(message) / sizeof(WCHAR));
                    MessageBoxW(hwnd, message, caption, MB_OK | MB_ICONEXCLAMATION);
                    HeapFree(GetProcessHeap(), 0, filename);
                    return TRUE;
                }
                CloseHandle(hf);
                DeleteFileW(filename);
                output = (LPWSTR *)GetWindowLongPtrW(hwnd, DWLP_USER);
                *output = filename;
                EndDialog(hwnd, IDOK);
                return TRUE;
            }
            if(LOWORD(wparam) == IDCANCEL)
            {
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
        }
        return FALSE;
    }
    return FALSE;
}

/*****************************************************************************
 *          get_filename
 */
static BOOL get_filename(LPWSTR *filename)
{
    return DialogBoxParamW(WINSPOOL_hInstance, MAKEINTRESOURCEW(FILENAME_DIALOG), GetForegroundWindow(),
                           file_dlg_proc, (LPARAM)filename) == IDOK;
}

/*****************************************************************************
 *          schedule_file
 */
static BOOL schedule_file(LPCWSTR filename)
{
    LPWSTR output = NULL;

    if(get_filename(&output))
    {
        TRACE("copy to %s\n", debugstr_w(output));
        CopyFileW(filename, output, FALSE);
        HeapFree(GetProcessHeap(), 0, output);
        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *          schedule_pipe
 */
static BOOL schedule_pipe(LPCWSTR cmd, LPCWSTR filename)
{
#ifdef HAVE_FORK
    char *unixname, *cmdA;
    DWORD len;
    int fds[2] = {-1, -1}, file_fd = -1, no_read;
    BOOL ret = FALSE;
    char buf[1024];

    if(!(unixname = wine_get_unix_file_name(filename)))
        return FALSE;

    len = WideCharToMultiByte(CP_ACP, 0, cmd, -1, NULL, 0, NULL, NULL);
    cmdA = HeapAlloc(GetProcessHeap(), 0, len);
    WideCharToMultiByte(CP_ACP, 0, cmd, -1, cmdA, len, NULL, NULL);

    TRACE("printing with: %s\n", cmdA);

    if((file_fd = open(unixname, O_RDONLY)) == -1)
        goto end;

    if (pipe(fds))
    {
        ERR("pipe() failed!\n"); 
        goto end;
    }

    if (fork() == 0)
    {
        close(0);
        dup2(fds[0], 0);
        close(fds[1]);

        /* reset signals that we previously set to SIG_IGN */
        signal(SIGPIPE, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        execl("/bin/sh", "/bin/sh", "-c", cmdA, (char*)0);
        _exit(1);
    }

    while((no_read = read(file_fd, buf, sizeof(buf))) > 0)
        write(fds[1], buf, no_read);

    ret = TRUE;

end:
    if(file_fd != -1) close(file_fd);
    if(fds[0] != -1) close(fds[0]);
    if(fds[1] != -1) close(fds[1]);

    HeapFree(GetProcessHeap(), 0, cmdA);
    HeapFree(GetProcessHeap(), 0, unixname);
    return ret;
#else
    return FALSE;
#endif
}

/*****************************************************************************
 *          schedule_unixfile
 */
static BOOL schedule_unixfile(LPCWSTR output, LPCWSTR filename)
{
    int in_fd, out_fd, no_read;
    char buf[1024];
    BOOL ret = FALSE;
    char *unixname, *outputA;
    DWORD len;

    if(!(unixname = wine_get_unix_file_name(filename)))
        return FALSE;

    len = WideCharToMultiByte(CP_ACP, 0, output, -1, NULL, 0, NULL, NULL);
    outputA = HeapAlloc(GetProcessHeap(), 0, len);
    WideCharToMultiByte(CP_ACP, 0, output, -1, outputA, len, NULL, NULL);
    
    out_fd = open(outputA, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    in_fd = open(unixname, O_RDONLY);
    if(out_fd == -1 || in_fd == -1)
        goto end;

    while((no_read = read(in_fd, buf, sizeof(buf))) > 0)
        write(out_fd, buf, no_read);

    ret = TRUE;
end:
    if(in_fd != -1) close(in_fd);
    if(out_fd != -1) close(out_fd);
    HeapFree(GetProcessHeap(), 0, outputA);
    HeapFree(GetProcessHeap(), 0, unixname);
    return ret;
}

/*****************************************************************************
 *          ScheduleJob [WINSPOOL.@]
 *
 */
BOOL WINAPI ScheduleJob( HANDLE hPrinter, DWORD dwJobID )
{
    opened_printer_t *printer;
    BOOL ret = FALSE;
    struct list *cursor, *cursor2;

    TRACE("(%p, %x)\n", hPrinter, dwJobID);
    EnterCriticalSection(&printer_handles_cs);
    printer = get_opened_printer(hPrinter);
    if(!printer)
        goto end;

    LIST_FOR_EACH_SAFE(cursor, cursor2, &printer->queue->jobs)
    {
        job_t *job = LIST_ENTRY(cursor, job_t, entry);
        HANDLE hf;

        if(job->job_id != dwJobID) continue;

        hf = CreateFileW(job->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if(hf != INVALID_HANDLE_VALUE)
        {
            PRINTER_INFO_5W *pi5;
            DWORD needed;
            HKEY hkey;
            WCHAR output[1024];
            static const WCHAR spooler_key[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                                'P','r','i','n','t','i','n','g','\\','S','p','o','o','l','e','r',0};

            GetPrinterW(hPrinter, 5, NULL, 0, &needed);
            pi5 = HeapAlloc(GetProcessHeap(), 0, needed);
            GetPrinterW(hPrinter, 5, (LPBYTE)pi5, needed, &needed);
            TRACE("need to schedule job %d filename %s to port %s\n", job->job_id, debugstr_w(job->filename),
                  debugstr_w(pi5->pPortName));
            
            output[0] = 0;

            /* @@ Wine registry key: HKCU\Software\Wine\Printing\Spooler */
            if(RegOpenKeyW(HKEY_CURRENT_USER, spooler_key, &hkey) == ERROR_SUCCESS)
            {
                DWORD type, count = sizeof(output);
                RegQueryValueExW(hkey, pi5->pPortName, NULL, &type, (LPBYTE)output, &count);
                RegCloseKey(hkey);
            }
            if(output[0] == '|')
            {
                schedule_pipe(output + 1, job->filename);
            }
            else if(output[0])
            {
                schedule_unixfile(output, job->filename);
            }
            else if(!strncmpW(pi5->pPortName, LPR_Port, strlenW(LPR_Port)))
            {
                schedule_lpr(pi5->pPortName + strlenW(LPR_Port), job->filename);
            }
            else if(!strncmpW(pi5->pPortName, CUPS_Port, strlenW(CUPS_Port)))
            {
                schedule_cups(pi5->pPortName + strlenW(CUPS_Port), job->filename, job->document_title);
            }
            else if(!strncmpW(pi5->pPortName, FILE_Port, strlenW(FILE_Port)))
            {
                schedule_file(job->filename);
            }
            else
            {
                FIXME("can't schedule to port %s\n", debugstr_w(pi5->pPortName));
            }
            HeapFree(GetProcessHeap(), 0, pi5);
            CloseHandle(hf);
            DeleteFileW(job->filename);
        }
        list_remove(cursor);
        HeapFree(GetProcessHeap(), 0, job->document_title);
        HeapFree(GetProcessHeap(), 0, job->filename);
        HeapFree(GetProcessHeap(), 0, job);
        ret = TRUE;
        break;
    }
end:
    LeaveCriticalSection(&printer_handles_cs);
    return ret;
}

/*****************************************************************************
 *          StartDocDlgA [WINSPOOL.@]
 */
LPSTR WINAPI StartDocDlgA( HANDLE hPrinter, DOCINFOA *doc )
{
    UNICODE_STRING usBuffer;
    DOCINFOW docW;
    LPWSTR retW;
    LPWSTR docnameW = NULL, outputW = NULL, datatypeW = NULL;
    LPSTR ret = NULL;

    docW.cbSize = sizeof(docW);
    if (doc->lpszDocName)
    {
        docnameW = asciitounicode(&usBuffer, doc->lpszDocName);
        if (!(docW.lpszDocName = docnameW)) return NULL;
    }
    if (doc->lpszOutput)
    {
        outputW = asciitounicode(&usBuffer, doc->lpszOutput);
        if (!(docW.lpszOutput = outputW)) return NULL;
    }
    if (doc->lpszDatatype)
    {
        datatypeW = asciitounicode(&usBuffer, doc->lpszDatatype);
        if (!(docW.lpszDatatype = datatypeW)) return NULL;
    }
    docW.fwType = doc->fwType;

    retW = StartDocDlgW(hPrinter, &docW);

    if(retW)
    {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, retW, -1, NULL, 0, NULL, NULL);
        ret = HeapAlloc(GetProcessHeap(), 0, len);
        WideCharToMultiByte(CP_ACP, 0, retW, -1, ret, len, NULL, NULL);
        HeapFree(GetProcessHeap(), 0, retW);
    }

    HeapFree(GetProcessHeap(), 0, datatypeW);
    HeapFree(GetProcessHeap(), 0, outputW);
    HeapFree(GetProcessHeap(), 0, docnameW);

    return ret;
}

/*****************************************************************************
 *          StartDocDlgW [WINSPOOL.@]
 *
 * Undocumented: Apparently used by gdi32:StartDocW() to popup the file dialog
 * when lpszOutput is "FILE:" or if lpszOutput is NULL and the default printer
 * port is "FILE:". Also returns the full path if passed a relative path.
 *
 * The caller should free the returned string from the process heap.
 */
LPWSTR WINAPI StartDocDlgW( HANDLE hPrinter, DOCINFOW *doc )
{
    LPWSTR ret = NULL;
    DWORD len, attr;

    if(doc->lpszOutput == NULL) /* Check whether default port is FILE: */
    {
        PRINTER_INFO_5W *pi5;
        GetPrinterW(hPrinter, 5, NULL, 0, &len);
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return NULL;
        pi5 = HeapAlloc(GetProcessHeap(), 0, len);
        GetPrinterW(hPrinter, 5, (LPBYTE)pi5, len, &len);
        if(!pi5->pPortName || strcmpW(pi5->pPortName, FILE_Port))
        {
            HeapFree(GetProcessHeap(), 0, pi5);
            return NULL;
        }
        HeapFree(GetProcessHeap(), 0, pi5);
    }

    if(doc->lpszOutput == NULL || !strcmpW(doc->lpszOutput, FILE_Port))
    {
        LPWSTR name;

        if (get_filename(&name))
        {
            if(!(len = GetFullPathNameW(name, 0, NULL, NULL)))
            {
                HeapFree(GetProcessHeap(), 0, name);
                return NULL;
            }
            ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            GetFullPathNameW(name, len, ret, NULL);
            HeapFree(GetProcessHeap(), 0, name);
        }
        return ret;
    }

    if(!(len = GetFullPathNameW(doc->lpszOutput, 0, NULL, NULL)))
        return NULL;

    ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    GetFullPathNameW(doc->lpszOutput, len, ret, NULL);
        
    attr = GetFileAttributesW(ret);
    if(attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        HeapFree(GetProcessHeap(), 0, ret);
        ret = NULL;
    }
    return ret;
}
