/*
 * WINSPOOL functions
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
 * Copyright 1999, 2000 Huw D M Davies
 * Copyright 2001 Marcus Meissner
 * Copyright 2005-2010 Detlef Riekenberg
 * Copyright 2010 Vitaly Perov
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
#include <errno.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <signal.h>
#ifdef HAVE_CUPS_CUPS_H
# include <cups/cups.h>
#endif

#ifdef HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H
#define GetCurrentProcess GetCurrentProcess_Mac
#define GetCurrentThread GetCurrentThread_Mac
#define LoadResource LoadResource_Mac
#define AnimatePalette AnimatePalette_Mac
#define EqualRgn EqualRgn_Mac
#define FillRgn FillRgn_Mac
#define FrameRgn FrameRgn_Mac
#define GetPixel GetPixel_Mac
#define InvertRgn InvertRgn_Mac
#define LineTo LineTo_Mac
#define OffsetRgn OffsetRgn_Mac
#define PaintRgn PaintRgn_Mac
#define Polygon Polygon_Mac
#define ResizePalette ResizePalette_Mac
#define SetRectRgn SetRectRgn_Mac
#define EqualRect EqualRect_Mac
#define FillRect FillRect_Mac
#define FrameRect FrameRect_Mac
#define GetCursor GetCursor_Mac
#define InvertRect InvertRect_Mac
#define OffsetRect OffsetRect_Mac
#define PtInRect PtInRect_Mac
#define SetCursor SetCursor_Mac
#define SetRect SetRect_Mac
#define ShowCursor ShowCursor_Mac
#define UnionRect UnionRect_Mac
#include <ApplicationServices/ApplicationServices.h>
#undef GetCurrentProcess
#undef GetCurrentThread
#undef LoadResource
#undef AnimatePalette
#undef EqualRgn
#undef FillRgn
#undef FrameRgn
#undef GetPixel
#undef InvertRgn
#undef LineTo
#undef OffsetRgn
#undef PaintRgn
#undef Polygon
#undef ResizePalette
#undef SetRectRgn
#undef EqualRect
#undef FillRect
#undef FrameRect
#undef GetCursor
#undef InvertRect
#undef OffsetRect
#undef PtInRect
#undef SetCursor
#undef SetRect
#undef ShowCursor
#undef UnionRect
#undef DPRINTF
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
    HANDLE backend_printer;
    jobqueue_t *queue;
    started_doc_t *doc;
    DEVMODEW *devmode;
} opened_printer_t;

typedef struct {
    struct list entry;
    DWORD job_id;
    WCHAR *filename;
    WCHAR *portname;
    WCHAR *document_title;
    WCHAR *printer_name;
    LPDEVMODEW devmode;
} job_t;


typedef struct {
    LPCWSTR  envname;
    LPCWSTR  subdir;
    DWORD    driverversion;
    LPCWSTR  versionregpath;
    LPCWSTR  versionsubdir;
} printenv_t;

/* ############################### */

static opened_printer_t **printer_handles;
static UINT nb_printer_handles;
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

static const WCHAR WinNT_CV_PrinterPortsW[] = { 'S','o','f','t','w','a','r','e','\\',
                                                'M','i','c','r','o','s','o','f','t','\\',
                                                'W','i','n','d','o','w','s',' ','N','T','\\',
                                                'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                                'P','r','i','n','t','e','r','P','o','r','t','s',0};

static const WCHAR DefaultEnvironmentW[] = {'W','i','n','e',0};
static       WCHAR envname_win40W[] = {'W','i','n','d','o','w','s',' ','4','.','0',0};
static const WCHAR envname_x64W[] =   {'W','i','n','d','o','w','s',' ','x','6','4',0};
static       WCHAR envname_x86W[] =   {'W','i','n','d','o','w','s',' ','N','T',' ','x','8','6',0};
static const WCHAR subdir_win40W[] = {'w','i','n','4','0',0};
static const WCHAR subdir_x64W[] =   {'x','6','4',0};
static const WCHAR subdir_x86W[] =   {'w','3','2','x','8','6',0};
static const WCHAR Version0_RegPathW[] = {'\\','V','e','r','s','i','o','n','-','0',0};
static const WCHAR Version0_SubdirW[] = {'\\','0',0};
static const WCHAR Version3_RegPathW[] = {'\\','V','e','r','s','i','o','n','-','3',0};
static const WCHAR Version3_SubdirW[] = {'\\','3',0};

static const WCHAR spooldriversW[] = {'\\','s','p','o','o','l','\\','d','r','i','v','e','r','s','\\',0};

static const WCHAR AttributesW[] = {'A','t','t','r','i','b','u','t','e','s',0};
static const WCHAR backslashW[] = {'\\',0};
static const WCHAR Configuration_FileW[] = {'C','o','n','f','i','g','u','r','a','t',
				      'i','o','n',' ','F','i','l','e',0};
static const WCHAR DatatypeW[] = {'D','a','t','a','t','y','p','e',0};
static const WCHAR Data_FileW[] = {'D','a','t','a',' ','F','i','l','e',0};
static const WCHAR Default_DevModeW[] = {'D','e','f','a','u','l','t',' ','D','e','v','M','o','d','e',0};
static const WCHAR Default_PriorityW[] = {'D','e','f','a','u','l','t',' ','P','r','i','o','r','i','t','y',0};
static const WCHAR Dependent_FilesW[] = {'D','e','p','e','n','d','e','n','t',' ','F','i','l','e','s',0};
static const WCHAR DescriptionW[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR dnsTimeoutW[] = {'d','n','s','T','i','m','e','o','u','t',0};
static const WCHAR DriverW[] = {'D','r','i','v','e','r',0};
static const WCHAR HardwareIDW[] = {'H','a','r','d','w','a','r','e','I','D',0};
static const WCHAR Help_FileW[] = {'H','e','l','p',' ','F','i','l','e',0};
static const WCHAR LocationW[] = {'L','o','c','a','t','i','o','n',0};
static const WCHAR ManufacturerW[] = {'M','a','n','u','f','a','c','t','u','r','e','r',0};
static const WCHAR MonitorW[] = {'M','o','n','i','t','o','r',0};
static const WCHAR NameW[] = {'N','a','m','e',0};
static const WCHAR ObjectGUIDW[] = {'O','b','j','e','c','t','G','U','I','D',0};
static const WCHAR OEM_UrlW[] = {'O','E','M',' ','U','r','l',0};
static const WCHAR ParametersW[] = {'P','a','r','a','m','e','t','e','r','s',0};
static const WCHAR PortW[] = {'P','o','r','t',0};
static const WCHAR bs_Ports_bsW[] = {'\\','P','o','r','t','s','\\',0};
static const WCHAR Previous_NamesW[] = {'P','r','e','v','i','o','u','s',' ','N','a','m','e','s',0};
static const WCHAR Print_ProcessorW[] = {'P','r','i','n','t',' ','P','r','o','c','e','s','s','o','r',0};
static const WCHAR Printer_DriverW[] = {'P','r','i','n','t','e','r',' ','D','r','i','v','e','r',0};
static const WCHAR PrinterDriverDataW[] = {'P','r','i','n','t','e','r','D','r','i','v','e','r','D','a','t','a',0};
static const WCHAR PrinterPortsW[] = {'P','r','i','n','t','e','r','P','o','r','t','s',0};
static const WCHAR PriorityW[] = {'P','r','i','o','r','i','t','y',0};
static const WCHAR ProviderW[] = {'P','r','o','v','i','d','e','r',0};
static const WCHAR Separator_FileW[] = {'S','e','p','a','r','a','t','o','r',' ','F','i','l','e',0};
static const WCHAR Share_NameW[] = {'S','h','a','r','e',' ','N','a','m','e',0};
static const WCHAR StartTimeW[] = {'S','t','a','r','t','T','i','m','e',0};
static const WCHAR StatusW[] = {'S','t','a','t','u','s',0};
static const WCHAR txTimeoutW[] = {'t','x','T','i','m','e','o','u','t',0};
static const WCHAR UntilTimeW[] = {'U','n','t','i','l','T','i','m','e',0};
static const WCHAR VersionW[] = {'V','e','r','s','i','o','n',0};
static       WCHAR WinPrintW[] = {'W','i','n','P','r','i','n','t',0};
static const WCHAR deviceW[]  = {'d','e','v','i','c','e',0};
static const WCHAR devicesW[] = {'d','e','v','i','c','e','s',0};
static const WCHAR windowsW[] = {'w','i','n','d','o','w','s',0};
static       WCHAR rawW[] = {'R','A','W',0};
static       WCHAR driver_9x[] = {'w','i','n','e','p','s','1','6','.','d','r','v',0};
static       WCHAR driver_nt[] = {'w','i','n','e','p','s','.','d','r','v',0};
static const WCHAR timeout_15_45[] = {',','1','5',',','4','5',0};
static const WCHAR commaW[] = {',',0};
static       WCHAR emptyStringW[] = {0};

static const WCHAR May_Delete_Value[] = {'W','i','n','e','M','a','y','D','e','l','e','t','e','M','e',0};

static const WCHAR CUPS_Port[] = {'C','U','P','S',':',0};
static const WCHAR FILE_Port[] = {'F','I','L','E',':',0};
static const WCHAR LPR_Port[] = {'L','P','R',':',0};

static const WCHAR default_doc_title[] = {'L','o','c','a','l',' ','D','o','w','n','l','e','v','e','l',' ',
                                          'D','o','c','u','m','e','n','t',0};

static const WCHAR PPD_Overrides[] = {'P','P','D',' ','O','v','e','r','r','i','d','e','s',0};
static const WCHAR DefaultPageSize[] = {'D','e','f','a','u','l','t','P','a','g','e','S','i','z','e',0};

static const DWORD di_sizeof[] = {0, sizeof(DRIVER_INFO_1W), sizeof(DRIVER_INFO_2W),
                                     sizeof(DRIVER_INFO_3W), sizeof(DRIVER_INFO_4W),
                                     sizeof(DRIVER_INFO_5W), sizeof(DRIVER_INFO_6W),
                                  0, sizeof(DRIVER_INFO_8W)};


static const DWORD pi_sizeof[] = {0, sizeof(PRINTER_INFO_1W), sizeof(PRINTER_INFO_2W),
                                     sizeof(PRINTER_INFO_3),  sizeof(PRINTER_INFO_4W),
                                     sizeof(PRINTER_INFO_5W), sizeof(PRINTER_INFO_6),
                                     sizeof(PRINTER_INFO_7W), sizeof(PRINTER_INFO_8W),
                                     sizeof(PRINTER_INFO_9W)};

static const printenv_t env_x64 = {envname_x64W, subdir_x64W, 3, Version3_RegPathW, Version3_SubdirW};
static const printenv_t env_x86 = {envname_x86W, subdir_x86W, 3, Version3_RegPathW, Version3_SubdirW};
static const printenv_t env_win40 = {envname_win40W, subdir_win40W, 0, Version0_RegPathW, Version0_SubdirW};

static const printenv_t * const all_printenv[] = {&env_x86, &env_x64, &env_win40};

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

static DEVMODEW *dup_devmode( const DEVMODEW *dm )
{
    DEVMODEW *ret;

    if (!dm) return NULL;
    ret = HeapAlloc( GetProcessHeap(), 0, dm->dmSize + dm->dmDriverExtra );
    if (ret) memcpy( ret, dm, dm->dmSize + dm->dmDriverExtra );
    return ret;
}

/***********************************************************
 * DEVMODEdupWtoA
 * Creates an ansi copy of supplied devmode
 */
static DEVMODEA *DEVMODEdupWtoA( const DEVMODEW *dmW )
{
    LPDEVMODEA dmA;
    DWORD size;

    if (!dmW) return NULL;
    size = dmW->dmSize - CCHDEVICENAME -
                        ((dmW->dmSize > FIELD_OFFSET( DEVMODEW, dmFormName )) ? CCHFORMNAME : 0);

    dmA = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size + dmW->dmDriverExtra );
    if (!dmA) return NULL;

    WideCharToMultiByte( CP_ACP, 0, dmW->dmDeviceName, -1,
                         (LPSTR)dmA->dmDeviceName, CCHDEVICENAME, NULL, NULL );

    if (FIELD_OFFSET( DEVMODEW, dmFormName ) >= dmW->dmSize)
    {
        memcpy( &dmA->dmSpecVersion, &dmW->dmSpecVersion,
                dmW->dmSize - FIELD_OFFSET( DEVMODEW, dmSpecVersion ) );
    }
    else
    {
        memcpy( &dmA->dmSpecVersion, &dmW->dmSpecVersion,
                FIELD_OFFSET( DEVMODEW, dmFormName ) - FIELD_OFFSET( DEVMODEW, dmSpecVersion ) );
        WideCharToMultiByte( CP_ACP, 0, dmW->dmFormName, -1,
                             (LPSTR)dmA->dmFormName, CCHFORMNAME, NULL, NULL );

        memcpy( &dmA->dmLogPixels, &dmW->dmLogPixels, dmW->dmSize - FIELD_OFFSET( DEVMODEW, dmLogPixels ) );
    }

    dmA->dmSize = size;
    memcpy( (char *)dmA + dmA->dmSize, (const char *)dmW + dmW->dmSize, dmW->dmDriverExtra );
    return dmA;
}


/******************************************************************
 * verify, that the filename is a local file
 *
 */
static inline BOOL is_local_file(LPWSTR name)
{
    return (name[0] && (name[1] == ':') && (name[2] == '\\'));
}

/* ################################ */

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

/*****************************************************************************
 *    get_dword_from_reg
 *
 * Return DWORD associated with name from hkey.
 */
static DWORD get_dword_from_reg( HKEY hkey, const WCHAR *name )
{
    DWORD sz = sizeof(DWORD), type, value = 0;
    LONG ret;

    ret = RegQueryValueExW( hkey, name, 0, &type, (LPBYTE)&value, &sz );

    if (ret != ERROR_SUCCESS)
    {
        WARN( "Got ret = %d on name %s\n", ret, debugstr_w(name) );
        return 0;
    }
    if (type != REG_DWORD)
    {
        ERR( "Got type %d\n", type );
        return 0;
    }
    return value;
}

static inline DWORD set_reg_DWORD( HKEY hkey, const WCHAR *keyname, const DWORD value )
{
    return RegSetValueExW( hkey, keyname, 0, REG_DWORD, (const BYTE*)&value, sizeof(value) );
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

    if ((idx > 0) && (idx <= nb_printer_handles)) {
        ret = printer_handles[idx - 1];
    }
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

static DWORD open_printer_reg_key( const WCHAR *name, HKEY *key )
{
    HKEY printers;
    DWORD err;

    *key = NULL;
    err = RegCreateKeyW( HKEY_LOCAL_MACHINE, PrintersW, &printers );
    if (err) return err;

    err = RegOpenKeyW( printers, name, key );
    if (err) err = ERROR_INVALID_PRINTER_NAME;
    RegCloseKey( printers );
    return err;
}

/******************************************************************
 *  WINSPOOL_GetOpenedPrinterRegKey
 *
 */
static DWORD WINSPOOL_GetOpenedPrinterRegKey(HANDLE hPrinter, HKEY *phkey)
{
    LPCWSTR name = get_opened_printer_name(hPrinter);

    if(!name) return ERROR_INVALID_HANDLE;
    return open_printer_reg_key( name, phkey );
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

static BOOL add_printer_driver(const WCHAR *name, WCHAR *ppd)
{
    DRIVER_INFO_3W di3;

    ZeroMemory(&di3, sizeof(DRIVER_INFO_3W));
    di3.cVersion         = 3;
    di3.pName            = (WCHAR*)name;
    di3.pEnvironment     = envname_x86W;
    di3.pDriverPath      = driver_nt;
    di3.pDataFile        = ppd;
    di3.pConfigFile      = driver_nt;
    di3.pDefaultDataType = rawW;

    if (AddPrinterDriverExW( NULL, 3, (LPBYTE)&di3, APD_COPY_NEW_FILES | APD_COPY_FROM_DIRECTORY ) ||
        (GetLastError() ==  ERROR_PRINTER_DRIVER_ALREADY_INSTALLED ))
    {
        di3.cVersion     = 0;
        di3.pEnvironment = envname_win40W;
        di3.pDriverPath  = driver_9x;
        di3.pConfigFile  = driver_9x;
        if (AddPrinterDriverExW( NULL, 3, (LPBYTE)&di3, APD_COPY_NEW_FILES | APD_COPY_FROM_DIRECTORY ) ||
            (GetLastError() ==  ERROR_PRINTER_DRIVER_ALREADY_INSTALLED ))
        {
            return TRUE;
        }
    }
    ERR("failed with %u for %s (%s)\n", GetLastError(), debugstr_w(di3.pDriverPath), debugstr_w(di3.pEnvironment));
    return FALSE;
}

static inline char *expand_env_string( char *str, DWORD type )
{
    if (type == REG_EXPAND_SZ)
    {
        char *tmp;
        DWORD needed = ExpandEnvironmentStringsA( str, NULL, 0 );
        tmp = HeapAlloc( GetProcessHeap(), 0, needed );
        if (tmp)
        {
            ExpandEnvironmentStringsA( str, tmp, needed );
            HeapFree( GetProcessHeap(), 0, str );
            return tmp;
        }
    }
    return str;
}

static char *get_fallback_ppd_name( const char *printer_name )
{
    static const WCHAR ppds_key[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                     'P','r','i','n','t','i','n','g','\\','P','P','D',' ','F','i','l','e','s',0};
    HKEY hkey;
    DWORD needed, type;
    char *ret = NULL;

    if (RegOpenKeyW( HKEY_CURRENT_USER, ppds_key, &hkey ) == ERROR_SUCCESS )
    {
        const char *value_name = NULL;

        if (RegQueryValueExA( hkey, printer_name, 0, NULL, NULL, &needed ) == ERROR_SUCCESS)
            value_name = printer_name;
        else if (RegQueryValueExA( hkey, "generic", 0, NULL, NULL, &needed ) == ERROR_SUCCESS)
            value_name = "generic";

        if (value_name)
        {
            ret = HeapAlloc( GetProcessHeap(), 0, needed );
            if (!ret) return NULL;
            RegQueryValueExA( hkey, value_name, 0, &type, (BYTE *)ret, &needed );
        }
        RegCloseKey( hkey );
        if (ret) return expand_env_string( ret, type );
    }
    return NULL;
}

static BOOL copy_file( const char *src, const char *dst )
{
    int fds[2] = {-1, -1}, num;
    char buf[1024];
    BOOL ret = FALSE;

    fds[0] = open( src, O_RDONLY );
    fds[1] = open( dst, O_CREAT | O_TRUNC | O_WRONLY, 0666 );
    if (fds[0] == -1 || fds[1] == -1) goto fail;

    while ((num = read( fds[0], buf, sizeof(buf) )) != 0)
    {
        if (num == -1) goto fail;
        if (write( fds[1], buf, num ) != num) goto fail;
    }
    ret = TRUE;

fail:
    if (fds[1] != -1) close( fds[1] );
    if (fds[0] != -1) close( fds[0] );
    return ret;
}

static BOOL get_internal_fallback_ppd( const WCHAR *ppd )
{
    static const WCHAR typeW[] = {'P','P','D','F','I','L','E',0};

    char *ptr, *end;
    DWORD size, written;
    HANDLE file;
    BOOL ret;
    HRSRC res = FindResourceW( WINSPOOL_hInstance, MAKEINTRESOURCEW(1), typeW );

    if (!res || !(ptr = LoadResource( WINSPOOL_hInstance, res ))) return FALSE;
    size = SizeofResource( WINSPOOL_hInstance, res );
    end = memchr( ptr, 0, size );  /* resource file may contain additional nulls */
    if (end) size = end - ptr;
    file = CreateFileW( ppd, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0 );
    if (file == INVALID_HANDLE_VALUE) return FALSE;
    ret = WriteFile( file, ptr, size, &written, NULL ) && written == size;
    CloseHandle( file );
    if (ret) TRACE( "using internal fallback for %s\n", debugstr_w( ppd ));
    else DeleteFileW( ppd );
    return ret;
}

static BOOL get_fallback_ppd( const char *printer_name, const WCHAR *ppd )
{
    char *dst, *src = get_fallback_ppd_name( printer_name );
    BOOL ret = FALSE;

    if (!src) return get_internal_fallback_ppd( ppd );

    TRACE( "(%s %s) found %s\n", debugstr_a(printer_name), debugstr_w(ppd), debugstr_a(src) );

    if (!(dst = wine_get_unix_file_name( ppd ))) goto fail;

    if (symlink( src, dst ) == -1)
        if (errno != ENOSYS || !copy_file( src, dst ))
            goto fail;

    ret = TRUE;
fail:
    HeapFree( GetProcessHeap(), 0, dst );
    HeapFree( GetProcessHeap(), 0, src );
    return ret;
}

static WCHAR *get_ppd_filename( const WCHAR *dir, const WCHAR *file_name )
{
    static const WCHAR dot_ppd[] = {'.','p','p','d',0};
    int len = (strlenW( dir ) + strlenW( file_name )) * sizeof(WCHAR) + sizeof(dot_ppd);
    WCHAR *ppd = HeapAlloc( GetProcessHeap(), 0, len );

    if (!ppd) return NULL;
    strcpyW( ppd, dir );
    strcatW( ppd, file_name );
    strcatW( ppd, dot_ppd );

    return ppd;
}

static WCHAR *get_ppd_dir( void )
{
    static const WCHAR wine_ppds[] = {'w','i','n','e','_','p','p','d','s','\\',0};
    DWORD len;
    WCHAR *dir, tmp_path[MAX_PATH];
    BOOL res;

    len = GetTempPathW( sizeof(tmp_path) / sizeof(tmp_path[0]), tmp_path );
    if (!len) return NULL;
    dir = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) + sizeof(wine_ppds) ) ;
    if (!dir) return NULL;

    memcpy( dir, tmp_path, len * sizeof(WCHAR) );
    memcpy( dir + len, wine_ppds, sizeof(wine_ppds) );
    res = CreateDirectoryW( dir, NULL );
    if (!res && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        HeapFree( GetProcessHeap(), 0, dir );
        dir = NULL;
    }
    TRACE( "ppd temporary dir: %s\n", debugstr_w(dir) );
    return dir;
}

static void unlink_ppd( const WCHAR *ppd )
{
    char *unix_name = wine_get_unix_file_name( ppd );
    unlink( unix_name );
    HeapFree( GetProcessHeap(), 0, unix_name );
}

#ifdef SONAME_LIBCUPS

static void *cupshandle;

#define CUPS_FUNCS \
    DO_FUNC(cupsAddOption); \
    DO_FUNC(cupsFreeDests); \
    DO_FUNC(cupsFreeOptions); \
    DO_FUNC(cupsGetDests); \
    DO_FUNC(cupsGetOption); \
    DO_FUNC(cupsGetPPD); \
    DO_FUNC(cupsParseOptions); \
    DO_FUNC(cupsPrintFile)
#define CUPS_OPT_FUNCS \
    DO_FUNC(cupsGetNamedDest); \
    DO_FUNC(cupsGetPPD3)

#define DO_FUNC(f) static typeof(f) *p##f
CUPS_FUNCS;
#undef DO_FUNC
static cups_dest_t * (*pcupsGetNamedDest)(http_t *, const char *, const char *);
static http_status_t (*pcupsGetPPD3)(http_t *, const char *, time_t *, char *, size_t);

static http_status_t cupsGetPPD3_wrapper( http_t *http, const char *name,
                                          time_t *modtime, char *buffer,
                                          size_t bufsize )
{
    const char *ppd;

    if (pcupsGetPPD3) return pcupsGetPPD3( http, name, modtime, buffer, bufsize );

    if (!pcupsGetPPD) return HTTP_NOT_FOUND;

    TRACE( "No cupsGetPPD3 implementation, so calling cupsGetPPD\n" );

    *modtime = 0;
    ppd = pcupsGetPPD( name );

    TRACE( "cupsGetPPD returns %s\n", debugstr_a(ppd) );

    if (!ppd) return HTTP_NOT_FOUND;

    if (rename( ppd, buffer ) == -1)
    {
        BOOL res = copy_file( ppd, buffer );
        unlink( ppd );
        if (!res) return HTTP_NOT_FOUND;
    }
    return HTTP_OK;
}

static BOOL get_cups_ppd( const char *printer_name, const WCHAR *ppd )
{
    time_t modtime = 0;
    http_status_t http_status;
    char *unix_name = wine_get_unix_file_name( ppd );

    TRACE( "(%s, %s)\n", debugstr_a(printer_name), debugstr_w(ppd) );

    if (!unix_name) return FALSE;

    http_status = cupsGetPPD3_wrapper( 0, printer_name, &modtime,
                                       unix_name, strlen( unix_name ) + 1 );

    if (http_status != HTTP_OK) unlink( unix_name );
    HeapFree( GetProcessHeap(), 0, unix_name );

    if (http_status == HTTP_OK) return TRUE;

    TRACE( "failed to get ppd for printer %s from cups (status %d), calling fallback\n",
           debugstr_a(printer_name), http_status );
    return get_fallback_ppd( printer_name, ppd );
}

static WCHAR *get_cups_option( const char *name, int num_options, cups_option_t *options )
{
    const char *value;
    WCHAR *ret;
    int len;

    value = pcupsGetOption( name, num_options, options );
    if (!value) return NULL;

    len = MultiByteToWideChar( CP_UNIXCP, 0, value, -1, NULL, 0 );
    ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (ret) MultiByteToWideChar( CP_UNIXCP, 0, value, -1, ret, len );

    return ret;
}

static cups_ptype_t get_cups_printer_type( const cups_dest_t *dest )
{
    WCHAR *type = get_cups_option( "printer-type", dest->num_options, dest->options ), *end;
    cups_ptype_t ret = 0;

    if (type && *type)
    {
        ret = (cups_ptype_t)strtoulW( type, &end, 10 );
        if (*end) ret = 0;
    }
    HeapFree( GetProcessHeap(), 0, type );
    return ret;
}

static void load_cups(void)
{
    cupshandle = wine_dlopen( SONAME_LIBCUPS, RTLD_NOW, NULL, 0 );
    if (!cupshandle) return;

    TRACE("%p: %s loaded\n", cupshandle, SONAME_LIBCUPS);

#define DO_FUNC(x) \
    p##x = wine_dlsym( cupshandle, #x, NULL, 0 ); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        cupshandle = NULL; \
        return; \
    }
    CUPS_FUNCS;
#undef DO_FUNC
#define DO_FUNC(x) p##x = wine_dlsym( cupshandle, #x, NULL, 0 )
    CUPS_OPT_FUNCS;
#undef DO_FUNC
}

static BOOL CUPS_LoadPrinters(void)
{
    int	                  i, nrofdests;
    BOOL                  hadprinter = FALSE, haddefault = FALSE;
    cups_dest_t          *dests;
    PRINTER_INFO_2W       pi2;
    WCHAR *port, *ppd_dir = NULL, *ppd;
    HKEY hkeyPrinter, hkeyPrinters;
    WCHAR   nameW[MAX_PATH];
    HANDLE  added_printer;
    cups_ptype_t printer_type;

    if (!cupshandle) return FALSE;

    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	return FALSE;
    }

    nrofdests = pcupsGetDests(&dests);
    TRACE("Found %d CUPS %s:\n", nrofdests, (nrofdests == 1) ? "printer" : "printers");
    for (i=0;i<nrofdests;i++) {
        MultiByteToWideChar(CP_UNIXCP, 0, dests[i].name, -1, nameW, sizeof(nameW) / sizeof(WCHAR));
        printer_type = get_cups_printer_type( dests + i );

        TRACE( "Printer %d: %s. printer_type %x\n", i, debugstr_w(nameW), printer_type );

        if (printer_type & 0x2000000 /* CUPS_PRINTER_SCANNER */)
        {
            TRACE( "skipping scanner-only device\n" );
            continue;
        }

        port = HeapAlloc(GetProcessHeap(), 0, sizeof(CUPS_Port) + lstrlenW(nameW) * sizeof(WCHAR));
        lstrcpyW(port, CUPS_Port);
        lstrcatW(port, nameW);

        if(RegOpenKeyW(hkeyPrinters, nameW, &hkeyPrinter) == ERROR_SUCCESS) {
            DWORD status = get_dword_from_reg( hkeyPrinter, StatusW );
            /* Printer already in registry, delete the tag added in WINSPOOL_LoadSystemPrinters
               and continue */
            TRACE("Printer already exists\n");
            /* overwrite old LPR:* port */
            RegSetValueExW(hkeyPrinter, PortW, 0, REG_SZ, (LPBYTE)port, (lstrlenW(port) + 1) * sizeof(WCHAR));
            RegDeleteValueW(hkeyPrinter, May_Delete_Value);
            /* flag that the PPD file should be checked for an update */
            set_reg_DWORD( hkeyPrinter, StatusW, status | PRINTER_STATUS_DRIVER_UPDATE_NEEDED );
            RegCloseKey(hkeyPrinter);
        } else {
            BOOL added_driver = FALSE;

            if (!ppd_dir) ppd_dir = get_ppd_dir();
            ppd = get_ppd_filename( ppd_dir, nameW );
            if (get_cups_ppd( dests[i].name, ppd ))
            {
                added_driver = add_printer_driver( nameW, ppd );
                unlink_ppd( ppd );
            }
            HeapFree( GetProcessHeap(), 0, ppd );
            if (!added_driver)
            {
                HeapFree( GetProcessHeap(), 0, port );
                continue;
            }

            memset(&pi2, 0, sizeof(PRINTER_INFO_2W));
            pi2.pPrinterName    = nameW;
            pi2.pDatatype       = rawW;
            pi2.pPrintProcessor = WinPrintW;
            pi2.pDriverName     = nameW;
            pi2.pComment        = get_cups_option( "printer-info", dests[i].num_options, dests[i].options );
            pi2.pLocation       = get_cups_option( "printer-location", dests[i].num_options, dests[i].options );
            pi2.pPortName       = port;
            pi2.pParameters     = emptyStringW;
            pi2.pShareName      = emptyStringW;
            pi2.pSepFile        = emptyStringW;

            added_printer = AddPrinterW( NULL, 2, (LPBYTE)&pi2 );
            if (added_printer) ClosePrinter( added_printer );
            else if (GetLastError() != ERROR_PRINTER_ALREADY_EXISTS)
                ERR( "printer '%s' not added by AddPrinter (error %d)\n", debugstr_w(nameW), GetLastError() );

            HeapFree( GetProcessHeap(), 0, pi2.pComment );
            HeapFree( GetProcessHeap(), 0, pi2.pLocation );
        }
        HeapFree( GetProcessHeap(), 0, port );

        hadprinter = TRUE;
        if (dests[i].is_default) {
            SetDefaultPrinterW(nameW);
            haddefault = TRUE;
        }
    }

    if (ppd_dir)
    {
        RemoveDirectoryW( ppd_dir );
        HeapFree( GetProcessHeap(), 0, ppd_dir );
    }

    if (hadprinter && !haddefault) {
        MultiByteToWideChar(CP_UNIXCP, 0, dests[0].name, -1, nameW, sizeof(nameW) / sizeof(WCHAR));
        SetDefaultPrinterW(nameW);
    }
    pcupsFreeDests(nrofdests, dests);
    RegCloseKey(hkeyPrinters);
    return TRUE;
}

#endif

static char *get_queue_name( HANDLE printer, BOOL *cups )
{
    WCHAR *port, *name = NULL;
    DWORD err, needed, type;
    char *ret = NULL;
    HKEY key;

    *cups = FALSE;

    err = WINSPOOL_GetOpenedPrinterRegKey( printer, &key );
    if (err) return NULL;
    err = RegQueryValueExW( key, PortW, 0, &type, NULL, &needed );
    if (err) goto end;
    port = HeapAlloc( GetProcessHeap(), 0, needed );
    if (!port) goto end;
    RegQueryValueExW( key, PortW, 0, &type, (BYTE*)port, &needed );

    if (!strncmpW( port, CUPS_Port, sizeof(CUPS_Port) / sizeof(WCHAR) -1 ))
    {
        name = port + sizeof(CUPS_Port) / sizeof(WCHAR) - 1;
        *cups = TRUE;
    }
    else if (!strncmpW( port, LPR_Port, sizeof(LPR_Port) / sizeof(WCHAR) -1 ))
        name = port + sizeof(LPR_Port) / sizeof(WCHAR) - 1;
    if (name)
    {
        needed = WideCharToMultiByte( CP_UNIXCP, 0, name, -1, NULL, 0, NULL, NULL );
        ret = HeapAlloc( GetProcessHeap(), 0, needed );
        if(ret) WideCharToMultiByte( CP_UNIXCP, 0, name, -1, ret, needed, NULL, NULL );
    }
    HeapFree( GetProcessHeap(), 0, port );
end:
    RegCloseKey( key );
    return ret;
}


static void set_ppd_overrides( HANDLE printer )
{
    WCHAR *wstr = NULL;
    int size = 0;
#ifdef HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H
    OSStatus status;
    PMPrintSession session = NULL;
    PMPageFormat format = NULL;
    PMPaper paper;
    CFStringRef paper_name;
    CFRange range;

    status = PMCreateSession( &session );
    if (status) goto end;

    status = PMCreatePageFormat( &format );
    if (status) goto end;

    status = PMSessionDefaultPageFormat( session, format );
    if (status) goto end;

    status = PMGetPageFormatPaper( format, &paper );
    if (status) goto end;

    status = PMPaperGetPPDPaperName( paper, &paper_name );
    if (status) goto end;

    range.location = 0;
    range.length = CFStringGetLength( paper_name );
    size = (range.length + 1) * sizeof(WCHAR);

    wstr = HeapAlloc( GetProcessHeap(), 0, size );
    CFStringGetCharacters( paper_name, range, (UniChar*)wstr );
    wstr[range.length] = 0;

end:
    if (format) PMRelease( format );
    if (session) PMRelease( session );
#endif

    SetPrinterDataExW( printer, PPD_Overrides, DefaultPageSize, REG_SZ, (BYTE*)wstr, size );
    HeapFree( GetProcessHeap(), 0, wstr );
}

static BOOL update_driver( HANDLE printer )
{
    BOOL ret, is_cups;
    const WCHAR *name = get_opened_printer_name( printer );
    WCHAR *ppd_dir, *ppd;
    char *queue_name;

    if (!name) return FALSE;
    queue_name = get_queue_name( printer, &is_cups );
    if (!queue_name) return FALSE;

    ppd_dir = get_ppd_dir();
    ppd = get_ppd_filename( ppd_dir, name );

#ifdef SONAME_LIBCUPS
    if (is_cups)
        ret = get_cups_ppd( queue_name, ppd );
    else
#endif
        ret = get_fallback_ppd( queue_name, ppd );

    if (ret)
    {
        TRACE( "updating driver %s\n", debugstr_w( name ) );
        ret = add_printer_driver( name, ppd );
        unlink_ppd( ppd );
    }
    HeapFree( GetProcessHeap(), 0, ppd_dir );
    HeapFree( GetProcessHeap(), 0, ppd );
    HeapFree( GetProcessHeap(), 0, queue_name );

    set_ppd_overrides( printer );

    /* call into the driver to update the devmode */
    DocumentPropertiesW( 0, printer, NULL, NULL, NULL, 0 );

    return ret;
}

static BOOL PRINTCAP_ParseEntry( const char *pent, BOOL isfirst )
{
    PRINTER_INFO_2A	pinfo2a;
    const char	*r;
    size_t		name_len;
    char		*e,*s,*name,*prettyname,*devname;
    BOOL		ret = FALSE, set_default = FALSE;
    char *port = NULL, *env_default;
    HKEY hkeyPrinter, hkeyPrinters = NULL;
    WCHAR devnameW[MAX_PATH], *ppd_dir = NULL, *ppd;
    HANDLE added_printer;

    while (isspace(*pent)) pent++;
    r = strchr(pent,':');
    if (r)
        name_len = r - pent;
    else
        name_len = strlen(pent);
    name = HeapAlloc(GetProcessHeap(), 0, name_len + 1);
    memcpy(name, pent, name_len);
    name[name_len] = '\0';
    if (r)
        pent = r;
    else
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

    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) !=
       ERROR_SUCCESS) {
        ERR("Can't create Printers key\n");
	ret = FALSE;
        goto end;
    }

    MultiByteToWideChar(CP_ACP, 0, devname, -1, devnameW, sizeof(devnameW) / sizeof(WCHAR));

    if(RegOpenKeyA(hkeyPrinters, devname, &hkeyPrinter) == ERROR_SUCCESS) {
        DWORD status = get_dword_from_reg( hkeyPrinter, StatusW );
        /* Printer already in registry, delete the tag added in WINSPOOL_LoadSystemPrinters
           and continue */
        TRACE("Printer already exists\n");
        RegDeleteValueW(hkeyPrinter, May_Delete_Value);
        /* flag that the PPD file should be checked for an update */
        set_reg_DWORD( hkeyPrinter, StatusW, status | PRINTER_STATUS_DRIVER_UPDATE_NEEDED );
        RegCloseKey(hkeyPrinter);
    } else {
        static CHAR data_type[]   = "RAW",
                    print_proc[]  = "WinPrint",
                    comment[]     = "WINEPS Printer using LPR",
                    params[]      = "<parameters?>",
                    share_name[]  = "<share name?>",
                    sep_file[]    = "<sep file?>";
        BOOL added_driver = FALSE;

        if (!ppd_dir) ppd_dir = get_ppd_dir();
        ppd = get_ppd_filename( ppd_dir, devnameW );
        if (get_fallback_ppd( devname, ppd ))
        {
            added_driver = add_printer_driver( devnameW, ppd );
            unlink_ppd( ppd );
        }
        HeapFree( GetProcessHeap(), 0, ppd );
        if (!added_driver) goto end;

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

        added_printer = AddPrinterA( NULL, 2, (LPBYTE)&pinfo2a );
        if (added_printer) ClosePrinter( added_printer );
        else if (GetLastError() != ERROR_PRINTER_ALREADY_EXISTS)
            ERR( "printer '%s' not added by AddPrinter (error %d)\n", debugstr_a(name), GetLastError() );
    }

    if (isfirst || set_default)
        WINSPOOL_SetDefaultPrinter(devname,name,TRUE);

 end:
    if (hkeyPrinters) RegCloseKey( hkeyPrinters );
    if (ppd_dir)
    {
        RemoveDirectoryW( ppd_dir );
        HeapFree( GetProcessHeap(), 0, ppd_dir );
    }
    HeapFree(GetProcessHeap(), 0, port);
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

static inline DWORD set_reg_devmode( HKEY key, const WCHAR *name, const DEVMODEW *dm )
{
    DEVMODEA *dmA = DEVMODEdupWtoA( dm );
    DWORD ret = ERROR_FILE_NOT_FOUND;

    /* FIXME: Write DEVMODEA not DEVMODEW into reg.  This is what win9x does
       and we support these drivers.  NT writes DEVMODEW so somehow
       we'll need to distinguish between these when we support NT
       drivers */

    if (dmA)
    {
        ret = RegSetValueExW( key, name, 0, REG_BINARY,
                              (LPBYTE)dmA, dmA->dmSize + dmA->dmDriverExtra );
        HeapFree( GetProcessHeap(), 0, dmA );
    }

    return ret;
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

static void free_printer_entry( opened_printer_t *printer )
{
    /* the queue is shared, so don't free that here */
    HeapFree( GetProcessHeap(), 0, printer->printername );
    HeapFree( GetProcessHeap(), 0, printer->name );
    HeapFree( GetProcessHeap(), 0, printer->devmode );
    HeapFree( GetProcessHeap(), 0, printer );
}

/******************************************************************
 *  get_opened_printer_entry
 *  Get the first place empty in the opened printer table
 *
 * ToDo:
 *  - pDefault is ignored
 */
static HANDLE get_opened_printer_entry(LPWSTR name, LPPRINTER_DEFAULTSW pDefault)
{
    UINT_PTR handle = nb_printer_handles, i;
    jobqueue_t *queue = NULL;
    opened_printer_t *printer = NULL;
    LPWSTR  servername;
    LPCWSTR printername;

    if ((backend == NULL)  && !load_backend()) return NULL;

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

    /* get a printer handle from the backend */
    if (! backend->fpOpenPrinter(name, &printer->backend_printer, pDefault)) {
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

    if (pDefault && pDefault->pDevMode)
        printer->devmode = dup_devmode( pDefault->pDevMode );

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
        if (!queue) HeapFree(GetProcessHeap(), 0, printer->queue);
        free_printer_entry( printer );
    }

    return (HANDLE)handle;
}

static void old_printer_check( BOOL delete_phase )
{
    PRINTER_INFO_5W* pi;
    DWORD needed, type, num, delete, i, size;
    const DWORD one = 1;
    HKEY key;
    HANDLE hprn;

    EnumPrintersW( PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &needed, &num );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;

    pi = HeapAlloc( GetProcessHeap(), 0, needed );
    EnumPrintersW( PRINTER_ENUM_LOCAL, NULL, 5, (LPBYTE)pi, needed, &needed, &num );
    for (i = 0; i < num; i++)
    {
        if (strncmpW( pi[i].pPortName, CUPS_Port, strlenW(CUPS_Port) ) &&
            strncmpW( pi[i].pPortName, LPR_Port, strlenW(LPR_Port) ))
            continue;

        if (open_printer_reg_key( pi[i].pPrinterName, &key )) continue;

        if (!delete_phase)
        {
            RegSetValueExW( key, May_Delete_Value, 0, REG_DWORD, (LPBYTE)&one, sizeof(one) );
            RegCloseKey( key );
        }
        else
        {
            delete = 0;
            size = sizeof( delete );
            RegQueryValueExW( key, May_Delete_Value, NULL, &type, (LPBYTE)&delete, &size );
            RegCloseKey( key );
            if (delete)
            {
                TRACE( "Deleting old printer %s\n", debugstr_w(pi[i].pPrinterName) );
                if (OpenPrinterW( pi[i].pPrinterName, &hprn, NULL ))
                {
                    DeletePrinter( hprn );
                    ClosePrinter( hprn );
                }
                DeletePrinterDriverExW( NULL, NULL, pi[i].pPrinterName, 0, 0 );
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, pi);
}

static const WCHAR winspool_mutex_name[] = {'_','_','W','I','N','E','_','W','I','N','S','P','O','O','L','_',
                                            'M','U','T','E','X','_','_','\0'};
static HANDLE init_mutex;

void WINSPOOL_LoadSystemPrinters(void)
{
    HKEY                hkey, hkeyPrinters;
    DWORD               needed, num, i;
    WCHAR               PrinterName[256];
    BOOL                done = FALSE;

#ifdef SONAME_LIBCUPS
    load_cups();
#endif

    /* FIXME: The init code should be moved to spoolsv.exe */
    init_mutex = CreateMutexW( NULL, TRUE, winspool_mutex_name );
    if (!init_mutex)
    {
        ERR( "Failed to create mutex\n" );
        return;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        WaitForSingleObject( init_mutex, INFINITE );
        ReleaseMutex( init_mutex );
        TRACE( "Init already done\n" );
        return;
    }

    /* This ensures that all printer entries have a valid Name value.  If causes
       problems later if they don't.  If one is found to be missed we create one
       and set it equal to the name of the key */
    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) == ERROR_SUCCESS) {
        if(RegQueryInfoKeyW(hkeyPrinters, NULL, NULL, NULL, &num, NULL, NULL,
                            NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            for(i = 0; i < num; i++) {
                if(RegEnumKeyW(hkeyPrinters, i, PrinterName, sizeof(PrinterName)/sizeof(PrinterName[0])) == ERROR_SUCCESS) {
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

    old_printer_check( FALSE );

#ifdef SONAME_LIBCUPS
    done = CUPS_LoadPrinters();
#endif

    if(!done) /* If we have any CUPS based printers, skip looking for printcap printers */
        PRINTCAP_LoadPrinters();

    old_printer_check( TRUE );

    ReleaseMutex( init_mutex );
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

/******************************************************************
 * convert_printerinfo_W_to_A [internal]
 *
 */
static void convert_printerinfo_W_to_A(LPBYTE out, LPBYTE pPrintersW,
                                       DWORD level, DWORD outlen, DWORD numentries)
{
    DWORD id = 0;
    LPSTR ptr;
    INT len;

    TRACE("(%p, %p, %d, %u, %u)\n", out, pPrintersW, level, outlen, numentries);

    len = pi_sizeof[level] * numentries;
    ptr = (LPSTR) out + len;
    outlen -= len;

    /* copy the numbers of all PRINTER_INFO_* first */
    memcpy(out, pPrintersW, len);

    while (id < numentries) {
        switch (level) {
            case 1:
                {
                    PRINTER_INFO_1W * piW = (PRINTER_INFO_1W *) pPrintersW;
                    PRINTER_INFO_1A * piA = (PRINTER_INFO_1A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(piW->pName));
                    if (piW->pDescription) {
                        piA->pDescription = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pDescription, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pName) {
                        piA->pName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pComment) {
                        piA->pComment = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pComment, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    break;
                }

            case 2:
                {
                    PRINTER_INFO_2W * piW = (PRINTER_INFO_2W *) pPrintersW;
                    PRINTER_INFO_2A * piA = (PRINTER_INFO_2A *) out;
                    LPDEVMODEA dmA;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(piW->pPrinterName));
                    if (piW->pServerName) {
                        piA->pServerName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pServerName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pPrinterName) {
                        piA->pPrinterName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pPrinterName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pShareName) {
                        piA->pShareName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pShareName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pPortName) {
                        piA->pPortName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pPortName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pDriverName) {
                        piA->pDriverName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pDriverName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pComment) {
                        piA->pComment = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pComment, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pLocation) {
                        piA->pLocation = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pLocation, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }

                    dmA = DEVMODEdupWtoA(piW->pDevMode);
                    if (dmA) {
                        /* align DEVMODEA to a DWORD boundary */
                        len = (4 - ( (DWORD_PTR) ptr & 3)) & 3;
                        ptr += len;
                        outlen -= len;

                        piA->pDevMode = (LPDEVMODEA) ptr;
                        len = dmA->dmSize + dmA->dmDriverExtra;
                        memcpy(ptr, dmA, len);
                        HeapFree(GetProcessHeap(), 0, dmA);

                        ptr += len;
                        outlen -= len;
                    }

                    if (piW->pSepFile) {
                        piA->pSepFile = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pSepFile, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pPrintProcessor) {
                        piA->pPrintProcessor = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pPrintProcessor, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pDatatype) {
                        piA->pDatatype = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pDatatype, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pParameters) {
                        piA->pParameters = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pParameters, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pSecurityDescriptor) {
                        piA->pSecurityDescriptor = NULL;
                        FIXME("pSecurityDescriptor ignored: %s\n", debugstr_w(piW->pPrinterName));
                    }
                    break;
                }

            case 4:
                {
                    PRINTER_INFO_4W * piW = (PRINTER_INFO_4W *) pPrintersW;
                    PRINTER_INFO_4A * piA = (PRINTER_INFO_4A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(piW->pPrinterName));

                    if (piW->pPrinterName) {
                        piA->pPrinterName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pPrinterName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pServerName) {
                        piA->pServerName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pServerName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    break;
                }

            case 5:
                {
                    PRINTER_INFO_5W * piW = (PRINTER_INFO_5W *) pPrintersW;
                    PRINTER_INFO_5A * piA = (PRINTER_INFO_5A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(piW->pPrinterName));

                    if (piW->pPrinterName) {
                        piA->pPrinterName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pPrinterName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    if (piW->pPortName) {
                        piA->pPortName = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pPortName, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    break;
                }

            case 6:  /* 6A and 6W are the same structure */
                break;

            case 7:
                {
                    PRINTER_INFO_7W * piW = (PRINTER_INFO_7W *) pPrintersW;
                    PRINTER_INFO_7A * piA = (PRINTER_INFO_7A *) out;

                    TRACE("(%u) #%u\n", level, id);
                    if (piW->pszObjectGUID) {
                        piA->pszObjectGUID = ptr;
                        len = WideCharToMultiByte(CP_ACP, 0, piW->pszObjectGUID, -1,
                                                  ptr, outlen, NULL, NULL);
                        ptr += len;
                        outlen -= len;
                    }
                    break;
                }

            case 8:
            case 9:
                {
                    PRINTER_INFO_9W * piW = (PRINTER_INFO_9W *) pPrintersW;
                    PRINTER_INFO_9A * piA = (PRINTER_INFO_9A *) out;
                    LPDEVMODEA dmA;

                    TRACE("(%u) #%u\n", level, id);
                    dmA = DEVMODEdupWtoA(piW->pDevMode);
                    if (dmA) {
                        /* align DEVMODEA to a DWORD boundary */
                        len = (4 - ( (DWORD_PTR) ptr & 3)) & 3;
                        ptr += len;
                        outlen -= len;

                        piA->pDevMode = (LPDEVMODEA) ptr;
                        len = dmA->dmSize + dmA->dmDriverExtra;
                        memcpy(ptr, dmA, len);
                        HeapFree(GetProcessHeap(), 0, dmA);

                        ptr += len;
                        outlen -= len;
                    }

                    break;
                }

            default:
                FIXME("for level %u\n", level);
        }
        pPrintersW += pi_sizeof[level];
        out += pi_sizeof[level];
        id++;
    }
}

/******************************************************************
 * convert_driverinfo_W_to_A [internal]
 *
 */
static void convert_driverinfo_W_to_A(LPBYTE out, LPBYTE pDriversW,
                                       DWORD level, DWORD outlen, DWORD numentries)
{
    DWORD id = 0;
    LPSTR ptr;
    INT len;

    TRACE("(%p, %p, %d, %u, %u)\n", out, pDriversW, level, outlen, numentries);

    len = di_sizeof[level] * numentries;
    ptr = (LPSTR) out + len;
    outlen -= len;

    /* copy the numbers of all PRINTER_INFO_* first */
    memcpy(out, pDriversW, len);

#define COPY_STRING(fld) \
                    { if (diW->fld){ \
                        diA->fld = ptr; \
                        len = WideCharToMultiByte(CP_ACP, 0, diW->fld, -1, ptr, outlen, NULL, NULL);\
                        ptr += len; outlen -= len;\
                    }}
#define COPY_MULTIZ_STRING(fld) \
                    { LPWSTR p = diW->fld; if (p){ \
                        diA->fld = ptr; \
                        do {\
                        len = WideCharToMultiByte(CP_ACP, 0, p, -1, ptr, outlen, NULL, NULL);\
                        ptr += len; outlen -= len; p += len;\
                        }\
                        while(len > 1 && outlen > 0); \
                    }}

    while (id < numentries)
    {
        switch (level)
        {
            case 1:
                {
                    DRIVER_INFO_1W * diW = (DRIVER_INFO_1W *) pDriversW;
                    DRIVER_INFO_1A * diA = (DRIVER_INFO_1A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(diW->pName));

                    COPY_STRING(pName);
                    break;
                }
            case 2:
                {
                    DRIVER_INFO_2W * diW = (DRIVER_INFO_2W *) pDriversW;
                    DRIVER_INFO_2A * diA = (DRIVER_INFO_2A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(diW->pName));

                    COPY_STRING(pName);
                    COPY_STRING(pEnvironment);
                    COPY_STRING(pDriverPath);
                    COPY_STRING(pDataFile);
                    COPY_STRING(pConfigFile);
                    break;
                }
            case 3:
                {
                    DRIVER_INFO_3W * diW = (DRIVER_INFO_3W *) pDriversW;
                    DRIVER_INFO_3A * diA = (DRIVER_INFO_3A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(diW->pName));

                    COPY_STRING(pName);
                    COPY_STRING(pEnvironment);
                    COPY_STRING(pDriverPath);
                    COPY_STRING(pDataFile);
                    COPY_STRING(pConfigFile);
                    COPY_STRING(pHelpFile);
                    COPY_MULTIZ_STRING(pDependentFiles);
                    COPY_STRING(pMonitorName);
                    COPY_STRING(pDefaultDataType);
                    break;
                }
            case 4:
                {
                    DRIVER_INFO_4W * diW = (DRIVER_INFO_4W *) pDriversW;
                    DRIVER_INFO_4A * diA = (DRIVER_INFO_4A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(diW->pName));

                    COPY_STRING(pName);
                    COPY_STRING(pEnvironment);
                    COPY_STRING(pDriverPath);
                    COPY_STRING(pDataFile);
                    COPY_STRING(pConfigFile);
                    COPY_STRING(pHelpFile);
                    COPY_MULTIZ_STRING(pDependentFiles);
                    COPY_STRING(pMonitorName);
                    COPY_STRING(pDefaultDataType);
                    COPY_MULTIZ_STRING(pszzPreviousNames);
                    break;
                }
            case 5:
                {
                    DRIVER_INFO_5W * diW = (DRIVER_INFO_5W *) pDriversW;
                    DRIVER_INFO_5A * diA = (DRIVER_INFO_5A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(diW->pName));

                    COPY_STRING(pName);
                    COPY_STRING(pEnvironment);
                    COPY_STRING(pDriverPath);
                    COPY_STRING(pDataFile);
                    COPY_STRING(pConfigFile);
                    break;
                }
            case 6:
                {
                    DRIVER_INFO_6W * diW = (DRIVER_INFO_6W *) pDriversW;
                    DRIVER_INFO_6A * diA = (DRIVER_INFO_6A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(diW->pName));

                    COPY_STRING(pName);
                    COPY_STRING(pEnvironment);
                    COPY_STRING(pDriverPath);
                    COPY_STRING(pDataFile);
                    COPY_STRING(pConfigFile);
                    COPY_STRING(pHelpFile);
                    COPY_MULTIZ_STRING(pDependentFiles);
                    COPY_STRING(pMonitorName);
                    COPY_STRING(pDefaultDataType);
                    COPY_MULTIZ_STRING(pszzPreviousNames);
                    COPY_STRING(pszMfgName);
                    COPY_STRING(pszOEMUrl);
                    COPY_STRING(pszHardwareID);
                    COPY_STRING(pszProvider);
                    break;
                }
            case 8:
                {
                    DRIVER_INFO_8W * diW = (DRIVER_INFO_8W *) pDriversW;
                    DRIVER_INFO_8A * diA = (DRIVER_INFO_8A *) out;

                    TRACE("(%u) #%u: %s\n", level, id, debugstr_w(diW->pName));

                    COPY_STRING(pName);
                    COPY_STRING(pEnvironment);
                    COPY_STRING(pDriverPath);
                    COPY_STRING(pDataFile);
                    COPY_STRING(pConfigFile);
                    COPY_STRING(pHelpFile);
                    COPY_MULTIZ_STRING(pDependentFiles);
                    COPY_STRING(pMonitorName);
                    COPY_STRING(pDefaultDataType);
                    COPY_MULTIZ_STRING(pszzPreviousNames);
                    COPY_STRING(pszMfgName);
                    COPY_STRING(pszOEMUrl);
                    COPY_STRING(pszHardwareID);
                    COPY_STRING(pszProvider);
                    COPY_STRING(pszPrintProcessor);
                    COPY_STRING(pszVendorSetup);
                    COPY_MULTIZ_STRING(pszzColorProfiles);
                    COPY_STRING(pszInfPath);
                    COPY_MULTIZ_STRING(pszzCoreDriverDependencies);
                    break;
                }


            default:
                FIXME("for level %u\n", level);
        }

        pDriversW += di_sizeof[level];
        out += di_sizeof[level];
        id++;

    }
#undef COPY_STRING
#undef COPY_MULTIZ_STRING
}


/***********************************************************
 *             printer_info_AtoW
 */
static void *printer_info_AtoW( const void *data, DWORD level )
{
    void *ret;
    UNICODE_STRING usBuffer;

    if (!data) return NULL;

    if (level < 1 || level > 9) return NULL;

    ret = HeapAlloc( GetProcessHeap(), 0, pi_sizeof[level] );
    if (!ret) return NULL;

    memcpy( ret, data, pi_sizeof[level] ); /* copy everything first */

    switch (level)
    {
    case 2:
    {
        const PRINTER_INFO_2A *piA = (const PRINTER_INFO_2A *)data;
        PRINTER_INFO_2W *piW = (PRINTER_INFO_2W *)ret;

        piW->pServerName = asciitounicode( &usBuffer, piA->pServerName );
        piW->pPrinterName = asciitounicode( &usBuffer, piA->pPrinterName );
        piW->pShareName = asciitounicode( &usBuffer, piA->pShareName );
        piW->pPortName = asciitounicode( &usBuffer, piA->pPortName );
        piW->pDriverName = asciitounicode( &usBuffer, piA->pDriverName );
        piW->pComment = asciitounicode( &usBuffer, piA->pComment );
        piW->pLocation = asciitounicode( &usBuffer, piA->pLocation );
        piW->pDevMode = piA->pDevMode ? GdiConvertToDevmodeW( piA->pDevMode ) : NULL;
        piW->pSepFile = asciitounicode( &usBuffer, piA->pSepFile );
        piW->pPrintProcessor = asciitounicode( &usBuffer, piA->pPrintProcessor );
        piW->pDatatype = asciitounicode( &usBuffer, piA->pDatatype );
        piW->pParameters = asciitounicode( &usBuffer, piA->pParameters );
        break;
    }

    case 8:
    case 9:
    {
        const PRINTER_INFO_9A *piA = (const PRINTER_INFO_9A *)data;
        PRINTER_INFO_9W *piW = (PRINTER_INFO_9W *)ret;

        piW->pDevMode = piA->pDevMode ? GdiConvertToDevmodeW( piA->pDevMode ) : NULL;
        break;
    }

    default:
        FIXME( "Unhandled level %d\n", level );
        HeapFree( GetProcessHeap(), 0, ret );
        return NULL;
    }

    return ret;
}

/***********************************************************
 *       free_printer_info
 */
static void free_printer_info( void *data, DWORD level )
{
    if (!data) return;

    switch (level)
    {
    case 2:
    {
        PRINTER_INFO_2W *piW = (PRINTER_INFO_2W *)data;

        HeapFree( GetProcessHeap(), 0, piW->pServerName );
        HeapFree( GetProcessHeap(), 0, piW->pPrinterName );
        HeapFree( GetProcessHeap(), 0, piW->pShareName );
        HeapFree( GetProcessHeap(), 0, piW->pPortName );
        HeapFree( GetProcessHeap(), 0, piW->pDriverName );
        HeapFree( GetProcessHeap(), 0, piW->pComment );
        HeapFree( GetProcessHeap(), 0, piW->pLocation );
        HeapFree( GetProcessHeap(), 0, piW->pDevMode );
        HeapFree( GetProcessHeap(), 0, piW->pSepFile );
        HeapFree( GetProcessHeap(), 0, piW->pPrintProcessor );
        HeapFree( GetProcessHeap(), 0, piW->pDatatype );
        HeapFree( GetProcessHeap(), 0, piW->pParameters );
        break;
    }

    case 8:
    case 9:
    {
        PRINTER_INFO_9W *piW = (PRINTER_INFO_9W *)data;

        HeapFree( GetProcessHeap(), 0, piW->pDevMode );
        break;
    }

    default:
        FIXME( "Unhandled level %d\n", level );
    }

    HeapFree( GetProcessHeap(), 0, data );
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

    TRACE("%s,%s,%u,%p,%p\n", debugstr_a(pDevice), debugstr_a(pPort), cap, pOutput, lpdm);

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
    LPDEVMODEA dmA = DEVMODEdupWtoA(pDevMode);
    LPSTR pDeviceA = strdupWtoA(pDevice);
    LPSTR pPortA = strdupWtoA(pPort);
    INT ret;

    TRACE("%s,%s,%u,%p,%p\n", debugstr_w(pDevice), debugstr_w(pPort), fwCapability, pOutput, pDevMode);

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

    if(!pDeviceName || !*pDeviceName) {
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
    LPDEVMODEA pDevModeInputA;
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
    pDevModeInputA = (fMode & DM_IN_BUFFER) ? DEVMODEdupWtoA(pDevModeInput) : NULL;
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

/*****************************************************************************
 *          IsValidDevmodeA            [WINSPOOL.@]
 *
 * Validate a DEVMODE structure and fix errors if possible.
 *
 */
BOOL WINAPI IsValidDevmodeA(PDEVMODEA *pDevMode, SIZE_T size)
{
    FIXME("(%p,%ld): stub\n", pDevMode, size);

    if(!pDevMode)
        return FALSE;

    return TRUE;
}

/*****************************************************************************
 *          IsValidDevmodeW            [WINSPOOL.@]
 *
 * Validate a DEVMODE structure and fix errors if possible.
 *
 */
BOOL WINAPI IsValidDevmodeW(PDEVMODEW *pDevMode, SIZE_T size)
{
    FIXME("(%p,%ld): stub\n", pDevMode, size);

    if(!pDevMode)
        return FALSE;

    return TRUE;
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

    TRACE("%s,%p,%p\n", debugstr_a(lpPrinterName), phPrinter, pDefault);

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
    HKEY key;

    TRACE("(%s, %p, %p)\n", debugstr_w(lpPrinterName), phPrinter, pDefault);

    if(!phPrinter) {
        /* NT: FALSE with ERROR_INVALID_PARAMETER, 9x: TRUE */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Get the unique handle of the printer or Printserver */
    *phPrinter = get_opened_printer_entry(lpPrinterName, pDefault);

    if (*phPrinter && WINSPOOL_GetOpenedPrinterRegKey( *phPrinter, &key ) == ERROR_SUCCESS)
    {
        DWORD deleting = 0, size = sizeof( deleting ), type;
        DWORD status;
        RegQueryValueExW( key, May_Delete_Value, NULL, &type, (LPBYTE)&deleting, &size );
        WaitForSingleObject( init_mutex, INFINITE );
        status = get_dword_from_reg( key, StatusW );
        set_reg_DWORD( key, StatusW, status & ~PRINTER_STATUS_DRIVER_UPDATE_NEEDED );
        ReleaseMutex( init_mutex );
        if (!deleting && (status & PRINTER_STATUS_DRIVER_UPDATE_NEEDED))
            update_driver( *phPrinter );
        RegCloseKey( key );
    }

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
          debugstr_a(mi2a ? mi2a->pName : NULL),
          debugstr_a(mi2a ? mi2a->pEnvironment : NULL),
          debugstr_a(mi2a ? mi2a->pDLLName : NULL));

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
    LPMONITOR_INFO_2W mi2w;

    mi2w = (LPMONITOR_INFO_2W) pMonitors;
    TRACE("(%s, %d, %p) :  %s %s %s\n", debugstr_w(pName), Level, pMonitors,
          debugstr_w(mi2w ? mi2w->pName : NULL),
          debugstr_w(mi2w ? mi2w->pEnvironment : NULL),
          debugstr_w(mi2w ? mi2w->pDLLName : NULL));

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if (Level != 2) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    /* XP: unchanged, win9x: ERROR_INVALID_ENVIRONMENT */
    if (mi2w == NULL) {
        return FALSE;
    }

    return backend->fpAddMonitor(pName, Level, pMonitors);
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
BOOL WINAPI DeleteMonitorW(LPWSTR pName, LPWSTR pEnvironment, LPWSTR pMonitorName)
{

    TRACE("(%s, %s, %s)\n",debugstr_w(pName),debugstr_w(pEnvironment),
           debugstr_w(pMonitorName));

    if ((backend == NULL)  && !load_backend()) return FALSE;

    return backend->fpDeleteMonitor(pName, pEnvironment, pMonitorName);
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
    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if (!pPortName) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    return backend->fpDeletePort(pName, hWnd, pPortName);
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
    return TRUE;
}

/*****************************************************************************
 *          AddFormW  [WINSPOOL.@]
 */
BOOL WINAPI AddFormW(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%p,%d,%p): stub\n", hPrinter, Level, pForm);
    return TRUE;
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
    job->portname = NULL;
    job->document_title = strdupW(default_doc_title);
    job->printer_name = strdupW(printer->name);
    job->devmode = dup_devmode( printer->devmode );
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

    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_w(server), debugstr_w(env), level,
                                        Info, cbBuf, pcbNeeded);

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if (level != 1) {
        /* (Level != 1) is ignored in win9x */
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (pcbNeeded == NULL) {
        /* (pcbNeeded == NULL) is ignored in win9x */
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    return backend->fpGetPrintProcessorDirectory(server, env, level, Info, cbBuf, pcbNeeded);
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
static HKEY WINSPOOL_OpenDriverReg( LPCVOID pEnvironment)
{   
    HKEY  retval = NULL;
    LPWSTR buffer;
    const printenv_t * env;

    TRACE("(%s)\n", debugstr_w(pEnvironment));

    env = validate_envW(pEnvironment);
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
 * set_devices_and_printerports [internal]
 *
 * set the [Devices] and [PrinterPorts] entries for a printer.
 *
 */
static void set_devices_and_printerports(PRINTER_INFO_2W *pi)
{
    DWORD portlen = lstrlenW(pi->pPortName) * sizeof(WCHAR);
    WCHAR *devline;
    HKEY  hkey;

    TRACE("(%p) %s\n", pi, debugstr_w(pi->pPrinterName));

    /* FIXME: the driver must change to "winspool" */
    devline = HeapAlloc(GetProcessHeap(), 0, sizeof(driver_nt) + portlen + sizeof(timeout_15_45));
    if (devline) {
        lstrcpyW(devline, driver_nt);
        lstrcatW(devline, commaW);
        lstrcatW(devline, pi->pPortName);

        TRACE("using %s\n", debugstr_w(devline));
        WriteProfileStringW(devicesW, pi->pPrinterName, devline);
        if (!RegCreateKeyW(HKEY_CURRENT_USER, user_printers_reg_key, &hkey)) {
            RegSetValueExW(hkey, pi->pPrinterName, 0, REG_SZ, (LPBYTE)devline,
                            (lstrlenW(devline) + 1) * sizeof(WCHAR));
            RegCloseKey(hkey);
        }

        lstrcatW(devline, timeout_15_45);
        WriteProfileStringW(PrinterPortsW, pi->pPrinterName, devline);
        if (!RegCreateKeyW(HKEY_CURRENT_USER, WinNT_CV_PrinterPortsW, &hkey)) {
            RegSetValueExW(hkey, pi->pPrinterName, 0, REG_SZ, (LPBYTE)devline,
                            (lstrlenW(devline) + 1) * sizeof(WCHAR));
            RegCloseKey(hkey);
        }
        HeapFree(GetProcessHeap(), 0, devline);
    }
}

/*****************************************************************************
 *          AddPrinterW  [WINSPOOL.@]
 */
HANDLE WINAPI AddPrinterW(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    PRINTER_INFO_2W *pi = (PRINTER_INFO_2W *) pPrinter;
    LPDEVMODEW dm;
    HANDLE retval;
    HKEY hkeyPrinter, hkeyPrinters, hkeyDriver, hkeyDrivers;
    LONG size;

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
	if (!RegQueryValueW(hkeyPrinter, AttributesW, NULL, NULL)) {
	    SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
	    RegCloseKey(hkeyPrinter);
	    RegCloseKey(hkeyPrinters);
	    return 0;
	}
	RegCloseKey(hkeyPrinter);
    }
    hkeyDrivers = WINSPOOL_OpenDriverReg(NULL);
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

    set_devices_and_printerports(pi);

    set_reg_DWORD(hkeyPrinter, AttributesW, pi->Attributes);
    set_reg_szW(hkeyPrinter, DatatypeW, pi->pDatatype);
    set_reg_DWORD(hkeyPrinter, Default_PriorityW, pi->DefaultPriority);
    set_reg_szW(hkeyPrinter, DescriptionW, pi->pComment);
    set_reg_DWORD(hkeyPrinter, dnsTimeoutW, 0);
    set_reg_szW(hkeyPrinter, LocationW, pi->pLocation);
    set_reg_szW(hkeyPrinter, NameW, pi->pPrinterName);
    set_reg_szW(hkeyPrinter, ParametersW, pi->pParameters);
    set_reg_szW(hkeyPrinter, PortW, pi->pPortName);
    set_reg_szW(hkeyPrinter, Print_ProcessorW, pi->pPrintProcessor);
    set_reg_szW(hkeyPrinter, Printer_DriverW, pi->pDriverName);
    set_reg_DWORD(hkeyPrinter, PriorityW, pi->Priority);
    set_reg_szW(hkeyPrinter, Separator_FileW, pi->pSepFile);
    set_reg_szW(hkeyPrinter, Share_NameW, pi->pShareName);
    set_reg_DWORD(hkeyPrinter, StartTimeW, pi->StartTime);
    set_reg_DWORD(hkeyPrinter, StatusW, pi->Status);
    set_reg_DWORD(hkeyPrinter, txTimeoutW, 0);
    set_reg_DWORD(hkeyPrinter, UntilTimeW, pi->UntilTime);

    size = DocumentPropertiesW(0, 0, pi->pPrinterName, NULL, NULL, 0);

    if (size < 0)
    {
        FIXME("DocumentPropertiesW on printer %s fails\n", debugstr_w(pi->pPrinterName));
        size = sizeof(DEVMODEW);
    }
    if(pi->pDevMode)
        dm = pi->pDevMode;
    else
    {
        dm = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
        dm->dmSize = size;
        if (DocumentPropertiesW(0, 0, pi->pPrinterName, dm, NULL, DM_OUT_BUFFER) < 0)
        {
            WARN("DocumentPropertiesW on printer %s failed!\n", debugstr_w(pi->pPrinterName));
            HeapFree( GetProcessHeap(), 0, dm );
            dm = NULL;
        }
        else
        {
            /* set devmode to printer name */
            lstrcpynW( dm->dmDeviceName, pi->pPrinterName, CCHDEVICENAME );
        }
    }

    set_reg_devmode( hkeyPrinter, Default_DevModeW, dm );
    if (!pi->pDevMode) HeapFree( GetProcessHeap(), 0, dm );

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

    TRACE("(%s, %d, %p)\n", debugstr_a(pName), Level, pPrinter);
    if(Level != 2) {
        ERR("Level = %d, unsupported!\n", Level);
	SetLastError(ERROR_INVALID_LEVEL);
	return 0;
    }
    pwstrNameW = asciitounicode(&pNameW,pName);
    piW = printer_info_AtoW( piA, Level );

    ret = AddPrinterW(pwstrNameW, Level, (LPBYTE)piW);

    free_printer_info( piW, Level );
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

        TRACE("closing %s (doc: %p)\n", debugstr_w(printer->name), printer->doc);

        if (printer->backend_printer) {
            backend->fpClosePrinter(printer->backend_printer);
        }

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

        free_printer_entry( printer );
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
    return TRUE;
}

/*****************************************************************************
 *          DeleteFormW  [WINSPOOL.@]
 */
BOOL WINAPI DeleteFormW(HANDLE hPrinter, LPWSTR pFormName)
{
    FIXME("(%p,%s): stub\n", hPrinter, debugstr_w(pFormName));
    return TRUE;
}

/*****************************************************************************
 *          DeletePrinter  [WINSPOOL.@]
 */
BOOL WINAPI DeletePrinter(HANDLE hPrinter)
{
    LPCWSTR lpNameW = get_opened_printer_name(hPrinter);
    HKEY hkeyPrinters, hkey;
    WCHAR def[MAX_PATH];
    DWORD size = sizeof( def ) / sizeof( def[0] );

    if(!lpNameW) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if(RegOpenKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters) == ERROR_SUCCESS) {
        RegDeleteTreeW(hkeyPrinters, lpNameW);
        RegCloseKey(hkeyPrinters);
    }
    WriteProfileStringW(devicesW, lpNameW, NULL);
    WriteProfileStringW(PrinterPortsW, lpNameW, NULL);

    if(RegCreateKeyW(HKEY_CURRENT_USER, user_printers_reg_key, &hkey) == ERROR_SUCCESS) {
        RegDeleteValueW(hkey, lpNameW);
        RegCloseKey(hkey);
    }

    if(RegCreateKeyW(HKEY_CURRENT_USER, WinNT_CV_PrinterPortsW, &hkey) == ERROR_SUCCESS) {
        RegDeleteValueW(hkey, lpNameW);
        RegCloseKey(hkey);
    }

    if (GetDefaultPrinterW( def, &size ) && !strcmpW( def, lpNameW ))
    {
        WriteProfileStringW( windowsW, deviceW, NULL );
        if (!RegCreateKeyW( HKEY_CURRENT_USER, user_default_reg_key, &hkey ))
        {
            RegDeleteValueW( hkey, deviceW );
            RegCloseKey( hkey );
        }
        SetDefaultPrinterW( NULL );
    }

    return TRUE;
}

/*****************************************************************************
 *          SetPrinterA  [WINSPOOL.@]
 */
BOOL WINAPI SetPrinterA( HANDLE printer, DWORD level, LPBYTE data, DWORD command )
{
    BYTE *dataW = data;
    BOOL ret;

    if (level != 0)
    {
        dataW = printer_info_AtoW( data, level );
        if (!dataW) return FALSE;
    }

    ret = SetPrinterW( printer, level, dataW, command );

    if (dataW != data) free_printer_info( dataW, level );

    return ret;
}

static void set_printer_2( HKEY key, const PRINTER_INFO_2W *pi )
{
    set_reg_szW( key, NameW, pi->pPrinterName );
    set_reg_szW( key, Share_NameW, pi->pShareName );
    set_reg_szW( key, PortW, pi->pPortName );
    set_reg_szW( key, Printer_DriverW, pi->pDriverName );
    set_reg_szW( key, DescriptionW, pi->pComment );
    set_reg_szW( key, LocationW, pi->pLocation );

    if (pi->pDevMode)
        set_reg_devmode( key, Default_DevModeW, pi->pDevMode );

    set_reg_szW( key, Separator_FileW, pi->pSepFile );
    set_reg_szW( key, Print_ProcessorW, pi->pPrintProcessor );
    set_reg_szW( key, DatatypeW, pi->pDatatype );
    set_reg_szW( key, ParametersW, pi->pParameters );

    set_reg_DWORD( key, AttributesW, pi->Attributes );
    set_reg_DWORD( key, PriorityW, pi->Priority );
    set_reg_DWORD( key, Default_PriorityW, pi->DefaultPriority );
    set_reg_DWORD( key, StartTimeW, pi->StartTime );
    set_reg_DWORD( key, UntilTimeW, pi->UntilTime );
}

static BOOL set_printer_9( HKEY key, const PRINTER_INFO_9W *pi )
{
    if (!pi->pDevMode) return FALSE;

    set_reg_devmode( key, Default_DevModeW, pi->pDevMode );
    return TRUE;
}

/******************************************************************************
 *    SetPrinterW  [WINSPOOL.@]
 */
BOOL WINAPI SetPrinterW( HANDLE printer, DWORD level, LPBYTE data, DWORD command )
{
    HKEY key;
    BOOL ret = FALSE;

    TRACE( "(%p, %d, %p, %d)\n", printer, level, data, command );

    if (command != 0) FIXME( "Ignoring command %d\n", command );

    if (WINSPOOL_GetOpenedPrinterRegKey( printer, &key ))
        return FALSE;

    switch (level)
    {
    case 2:
    {
        PRINTER_INFO_2W *pi2 = (PRINTER_INFO_2W *)data;
        set_printer_2( key, pi2 );
        ret = TRUE;
        break;
    }

    case 9:
    {
        PRINTER_INFO_9W *pi = (PRINTER_INFO_9W *)data;
        ret = set_printer_9( key, pi );
        break;
    }

    default:
        FIXME( "Unimplemented level %d\n", level );
        SetLastError( ERROR_INVALID_LEVEL );
    }

    RegCloseKey( key );
    return ret;
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
        HeapFree(GetProcessHeap(), 0, job->devmode);
        job->devmode = dup_devmode( info2->pDevMode );
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
    job_t *job;

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

    /* use pOutputFile only, when it is a real filename */
    if ((doc->pOutputFile) && is_local_file(doc->pOutputFile))
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
    job = get_job(hPrinter, ret);
    job->portname = strdupW(doc->pOutputFile);

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
 * get_filename_from_reg [internal]
 *
 * Get ValueName from hkey storing result in out
 * when the Value in the registry has only a filename, use driverdir as prefix
 * outlen is space left in out
 * String is stored either as unicode or ascii
 *
 */

static BOOL get_filename_from_reg(HKEY hkey, LPCWSTR driverdir, DWORD dirlen, LPCWSTR ValueName,
                                  LPBYTE out, DWORD outlen, LPDWORD needed)
{
    WCHAR   filename[MAX_PATH];
    DWORD   size;
    DWORD   type;
    LONG    ret;
    LPWSTR  buffer = filename;
    LPWSTR  ptr;

    *needed = 0;
    size = sizeof(filename);
    buffer[0] = '\0';
    ret = RegQueryValueExW(hkey, ValueName, NULL, &type, (LPBYTE) buffer, &size);
    if (ret == ERROR_MORE_DATA) {
        TRACE("need dynamic buffer: %u\n", size);
        buffer = HeapAlloc(GetProcessHeap(), 0, size);
        if (!buffer) {
            /* No Memory is bad */
            return FALSE;
        }
        buffer[0] = '\0';
        ret = RegQueryValueExW(hkey, ValueName, NULL, &type, (LPBYTE) buffer, &size);
    }

    if ((ret != ERROR_SUCCESS) || (!buffer[0])) {
        if (buffer != filename) HeapFree(GetProcessHeap(), 0, buffer);
        return FALSE;
    }

    ptr = buffer;
    while (ptr) {
        /* do we have a full path ? */
        ret = (((buffer[0] == '\\') && (buffer[1] == '\\')) ||
                (buffer[0] && (buffer[1] == ':') && (buffer[2] == '\\')) );

        if (!ret) {
            /* we must build the full Path */
            *needed += dirlen;
            if ((out) && (outlen > dirlen)) {
                lstrcpyW((LPWSTR)out, driverdir);
                out += dirlen;
                outlen -= dirlen;
            }
            else
                out = NULL;
        }

        /* write the filename */
        size = (lstrlenW(ptr) + 1) * sizeof(WCHAR);
        if ((out) && (outlen >= size)) {
            lstrcpyW((LPWSTR)out, ptr);
            out += size;
            outlen -= size;
        }
        else
            out = NULL;
        *needed += size;
        ptr +=  lstrlenW(ptr)+1;
        if ((type != REG_MULTI_SZ) || (!ptr[0]))  ptr = NULL;
    }

    if (buffer != filename) HeapFree(GetProcessHeap(), 0, buffer);

    /* write the multisz-termination */
    if (type == REG_MULTI_SZ) {
        size = sizeof(WCHAR);

        *needed += size;
        if (out && (outlen >= size)) {
            memset (out, 0, size);
        }
    }
    return TRUE;
}

/*****************************************************************************
 *    WINSPOOL_GetStringFromReg
 *
 * Get ValueName from hkey storing result in ptr.  buflen is space left in ptr
 * String is stored as unicode.
 */
static BOOL WINSPOOL_GetStringFromReg(HKEY hkey, LPCWSTR ValueName, LPBYTE ptr,
				      DWORD buflen, DWORD *needed)
{
    DWORD sz = buflen, type;
    LONG ret;

    ret = RegQueryValueExW(hkey, ValueName, 0, &type, ptr, &sz);
    if(ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
        WARN("Got ret = %d\n", ret);
	*needed = 0;
	return FALSE;
    }
    /* add space for terminating '\0' */
    sz += sizeof(WCHAR);
    *needed = sz;

    if (ptr)
        TRACE("%s: %s\n", debugstr_w(ValueName), debugstr_w((LPCWSTR)ptr));

    return TRUE;
}

/*****************************************************************************
 *    WINSPOOL_GetDefaultDevMode
 *
 * Get a default DevMode values for wineps.
 */
static void WINSPOOL_GetDefaultDevMode(LPBYTE ptr, DWORD buflen, DWORD *needed)
{
    static const WCHAR winepsW[] = { 'w','i','n','e','p','s','.','d','r','v',0 };

    if (buflen >= sizeof(DEVMODEW))
    {
        DEVMODEW *dm = (DEVMODEW *)ptr;

        /* the driver will update registry with real values */
        memset(dm, 0, sizeof(*dm));
        dm->dmSize = sizeof(*dm);
        lstrcpyW(dm->dmDeviceName, winepsW);
    }
    *needed = sizeof(DEVMODEW);
}

/*****************************************************************************
 *    WINSPOOL_GetDevModeFromReg
 *
 * Get ValueName from hkey storing result in ptr.  buflen is space left in ptr
 * DevMode is stored either as unicode or ascii.
 */
static BOOL WINSPOOL_GetDevModeFromReg(HKEY hkey, LPCWSTR ValueName,
				       LPBYTE ptr,
				       DWORD buflen, DWORD *needed)
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
    sz += (CCHDEVICENAME + CCHFORMNAME);
    if (ptr && (buflen >= sz)) {
        DEVMODEW *dmW = GdiConvertToDevmodeW((DEVMODEA*)ptr);
        memcpy(ptr, dmW, sz);
        HeapFree(GetProcessHeap(),0,dmW);
    }
    *needed = sz;
    return TRUE;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_1
 *
 * Fills out a PRINTER_INFO_1W struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_1(HKEY hkeyPrinter, PRINTER_INFO_1W *pi1,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi1->pName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }

    /* FIXME: pDescription should be something like "Name,Driver_Name,". */
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi1->pDescription = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DescriptionW, ptr, left, &size)) {
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
 * Fills out a PRINTER_INFO_2W struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_2(HKEY hkeyPrinter, PRINTER_INFO_2W *pi2,
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi2->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Share_NameW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi2->pShareName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, PortW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi2->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Printer_DriverW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi2->pDriverName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DescriptionW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi2->pComment = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, LocationW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi2->pLocation = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetDevModeFromReg(hkeyPrinter, Default_DevModeW, ptr, left, &size)) {
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
	WINSPOOL_GetDefaultDevMode(ptr, left, &size);
        if(space && size <= left) {
	    pi2->pDevMode = (LPDEVMODEW)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Separator_FileW, ptr, left, &size)) {
        if(space && size <= left) {
            pi2->pSepFile = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, Print_ProcessorW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi2->pPrintProcessor = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, DatatypeW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi2->pDatatype = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, ParametersW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi2->pParameters = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi2) {
        pi2->Attributes = get_dword_from_reg( hkeyPrinter, AttributesW );
        pi2->Priority = get_dword_from_reg( hkeyPrinter, PriorityW );
        pi2->DefaultPriority = get_dword_from_reg( hkeyPrinter, Default_PriorityW );
        pi2->StartTime = get_dword_from_reg( hkeyPrinter, StartTimeW );
        pi2->UntilTime = get_dword_from_reg( hkeyPrinter, UntilTimeW );
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
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi4->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi4) {
        pi4->Attributes = get_dword_from_reg( hkeyPrinter, AttributesW );
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
				  LPBYTE buf, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if(WINSPOOL_GetStringFromReg(hkeyPrinter, NameW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi5->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(WINSPOOL_GetStringFromReg(hkeyPrinter, PortW, ptr, left, &size)) {
        if(space && size <= left) {
	    pi5->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi5) {
        pi5->Attributes = get_dword_from_reg( hkeyPrinter, AttributesW );
        pi5->DeviceNotSelectedTimeout = get_dword_from_reg( hkeyPrinter, dnsTimeoutW );
        pi5->TransmissionRetryTimeout = get_dword_from_reg( hkeyPrinter, txTimeoutW );
    }

    if(!space && pi5) /* zero out pi5 if we can't completely fill buf */
        memset(pi5, 0, sizeof(*pi5));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_7
 *
 * Fills out a PRINTER_INFO_7 struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_7(HKEY hkeyPrinter, PRINTER_INFO_7W *pi7, LPBYTE buf,
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size, left = cbBuf;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;

    *pcbNeeded = 0;

    if (! WINSPOOL_GetStringFromReg(hkeyPrinter, ObjectGUIDW, ptr, left, &size))
    {
        ptr = NULL;
        size = sizeof(pi7->pszObjectGUID);
    }
    if (space && size <= left) {
        pi7->pszObjectGUID = (LPWSTR)ptr;
        ptr += size;
        left -= size;
    } else
        space = FALSE;
    *pcbNeeded += size;
    if (pi7) {
        /* We do not have a Directory Service */
        pi7->dwAction = DSPRINT_UNPUBLISH;
    }

    if (!space && pi7) /* zero out pi7 if we can't completely fill buf */
        memset(pi7, 0, sizeof(*pi7));

    return space;
}

/*********************************************************************
 *    WINSPOOL_GetPrinter_9
 *
 * Fills out a PRINTER_INFO_9AW struct storing the strings in buf.
 */
static BOOL WINSPOOL_GetPrinter_9(HKEY hkeyPrinter, PRINTER_INFO_9W *pi9, LPBYTE buf,
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size;
    BOOL space = (cbBuf > 0);

    *pcbNeeded = 0;

    if(WINSPOOL_GetDevModeFromReg(hkeyPrinter, Default_DevModeW, buf, cbBuf, &size)) {
        if(space && size <= cbBuf) {
            pi9->pDevMode = (LPDEVMODEW)buf;
        } else
            space = FALSE;
        *pcbNeeded += size;
    }
    else
    {
        WINSPOOL_GetDefaultDevMode(buf, cbBuf, &size);
        if(space && size <= cbBuf) {
            pi9->pDevMode = (LPDEVMODEW)buf;
        } else
            space = FALSE;
        *pcbNeeded += size;
    }

    if(!space && pi9) /* zero out pi9 if we can't completely fill buf */
        memset(pi9, 0, sizeof(*pi9));

    return space;
}

/*****************************************************************************
 *          GetPrinterW  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
			DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD size, needed = 0, err;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter;
    BOOL ret;

    TRACE("(%p,%d,%p,%d,%p)\n",hPrinter,Level,pPrinter,cbBuf, pcbNeeded);

    err = WINSPOOL_GetOpenedPrinterRegKey( hPrinter, &hkeyPrinter );
    if (err)
    {
        SetLastError( err );
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
	ret = WINSPOOL_GetPrinter_2(hkeyPrinter, pi2, ptr, cbBuf, &needed);
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
	ret = WINSPOOL_GetPrinter_4(hkeyPrinter, pi4, ptr, cbBuf, &needed);
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

	ret = WINSPOOL_GetPrinter_5(hkeyPrinter, pi5, ptr, cbBuf, &needed);
	needed += size;
	break;
      }


    case 6:
      {
        PRINTER_INFO_6 *pi6 = (PRINTER_INFO_6 *) pPrinter;

        size = sizeof(PRINTER_INFO_6);
        if (size <= cbBuf) {
            /* FIXME: We do not update the status yet */
            pi6->dwStatus = get_dword_from_reg( hkeyPrinter, StatusW );
            ret = TRUE;
        } else {
            ret = FALSE;
        }

        needed += size;
        break;
      }

    case 7:
      {
        PRINTER_INFO_7W *pi7 = (PRINTER_INFO_7W *) pPrinter;

        size = sizeof(PRINTER_INFO_7W);
        if (size <= cbBuf) {
            ptr = pPrinter + size;
            cbBuf -= size;
            memset(pPrinter, 0, size);
        } else {
            pi7 = NULL;
            cbBuf = 0;
        }

        ret = WINSPOOL_GetPrinter_7(hkeyPrinter, pi7, ptr, cbBuf, &needed);
        needed += size;
        break;
      }


    case 8:
        /* 8 is the global default printer info and 9 already gets it instead of the per-user one */
        /* still, PRINTER_INFO_8W is the same as PRINTER_INFO_9W */
        /* fall through */
    case 9:
      {
        PRINTER_INFO_9W *pi9 = (PRINTER_INFO_9W *)pPrinter;

        size = sizeof(PRINTER_INFO_9W);
        if(size <= cbBuf) {
            ptr = pPrinter + size;
            cbBuf -= size;
            memset(pPrinter, 0, size);
        } else {
            pi9 = NULL;
            cbBuf = 0;
        }

        ret = WINSPOOL_GetPrinter_9(hkeyPrinter, pi9, ptr, cbBuf, &needed);
        needed += size;
        break;
      }


    default:
        FIXME("Unimplemented level %d\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
	RegCloseKey(hkeyPrinter);
	return FALSE;
    }

    RegCloseKey(hkeyPrinter);

    TRACE("returning %d needed = %d\n", ret, needed);
    if(pcbNeeded) *pcbNeeded = needed;
    if(!ret)
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return ret;
}

/*****************************************************************************
 *          GetPrinterA  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter,
                    DWORD cbBuf, LPDWORD pcbNeeded)
{
    BOOL ret;
    LPBYTE buf = NULL;

    if (cbBuf)
        buf = HeapAlloc(GetProcessHeap(), 0, cbBuf);

    ret = GetPrinterW(hPrinter, Level, buf, cbBuf, pcbNeeded);
    if (ret)
        convert_printerinfo_W_to_A(pPrinter, buf, Level, cbBuf, 1);
    HeapFree(GetProcessHeap(), 0, buf);

    return ret;
}

/*****************************************************************************
 *          WINSPOOL_EnumPrintersW
 *
 *    Implementation of EnumPrintersW
 */
static BOOL WINSPOOL_EnumPrintersW(DWORD dwType, LPWSTR lpszName,
				  DWORD dwLevel, LPBYTE lpbPrinters,
				  DWORD cbBuf, LPDWORD lpdwNeeded,
				  LPDWORD lpdwReturned)

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
        if(RegEnumKeyW(hkeyPrinters, i, PrinterName, sizeof(PrinterName)/sizeof(PrinterName[0])) !=
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
				  left, &needed);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_1W);
	    break;
	case 2:
	    WINSPOOL_GetPrinter_2(hkeyPrinter, (PRINTER_INFO_2W *)pi, buf,
				  left, &needed);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_2W);
	    break;
	case 4:
	    WINSPOOL_GetPrinter_4(hkeyPrinter, (PRINTER_INFO_4W *)pi, buf,
				  left, &needed);
	    used += needed;
	    if(pi) pi += sizeof(PRINTER_INFO_4W);
	    break;
	case 5:
	    WINSPOOL_GetPrinter_5(hkeyPrinter, (PRINTER_INFO_5W *)pi, buf,
				  left, &needed);
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
    return WINSPOOL_EnumPrintersW(dwType, lpszName, dwLevel, lpbPrinters, cbBuf,
				 lpdwNeeded, lpdwReturned);
}

/******************************************************************
 * EnumPrintersA    [WINSPOOL.@]
 *
 * See EnumPrintersW
 *
 */
BOOL WINAPI EnumPrintersA(DWORD flags, LPSTR pName, DWORD level, LPBYTE pPrinters,
                          DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    BOOL ret;
    UNICODE_STRING pNameU;
    LPWSTR pNameW;
    LPBYTE pPrintersW;

    TRACE("(0x%x, %s, %u, %p, %d, %p, %p)\n", flags, debugstr_a(pName), level,
                                              pPrinters, cbBuf, pcbNeeded, pcReturned);

    pNameW = asciitounicode(&pNameU, pName);

    /* Request a buffer with a size, that is big enough for EnumPrintersW.
       MS Office need this */
    pPrintersW = (pPrinters && cbBuf) ? HeapAlloc(GetProcessHeap(), 0, cbBuf) : NULL;

    ret = EnumPrintersW(flags, pNameW, level, pPrintersW, cbBuf, pcbNeeded, pcReturned);

    RtlFreeUnicodeString(&pNameU);
    if (ret) {
        convert_printerinfo_W_to_A(pPrinters, pPrintersW, level, *pcbNeeded, *pcReturned);
    }
    HeapFree(GetProcessHeap(), 0, pPrintersW);
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
                            const printenv_t * env,
                            DWORD   Level,
                            LPBYTE  ptr,            /* DRIVER_INFO */
                            LPBYTE  pDriverStrings, /* strings buffer */
                            DWORD   cbBuf,          /* size of string buffer */
                            LPDWORD pcbNeeded)      /* space needed for str. */
{
    DWORD  size, tmp;
    HKEY   hkeyDriver;
    WCHAR  driverdir[MAX_PATH];
    DWORD  dirlen;
    LPBYTE strPtr = pDriverStrings;
    LPDRIVER_INFO_8W di = (LPDRIVER_INFO_8W) ptr;

    TRACE("(%p, %s, %p, %d, %p, %p, %d)\n", hkeyDrivers,
          debugstr_w(DriverName), env,
          Level, di, pDriverStrings, cbBuf);

    if (di) ZeroMemory(di, di_sizeof[Level]);

    *pcbNeeded = (lstrlenW(DriverName) + 1) * sizeof(WCHAR);
    if (*pcbNeeded <= cbBuf)
       strcpyW((LPWSTR)strPtr, DriverName);

    /* pName for level 1 has a different offset! */
    if (Level == 1) {
       if (di) ((LPDRIVER_INFO_1W) di)->pName = (LPWSTR) strPtr;
       return TRUE;
    }

    /* .cVersion and .pName for level > 1 */
    if (di) {
        di->cVersion = env->driverversion;
        di->pName = (LPWSTR) strPtr;
        strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    /* Reserve Space for the largest subdir and a Backslash*/
    size = sizeof(driverdir) - sizeof(Version3_SubdirW) - sizeof(WCHAR);
    if (!GetPrinterDriverDirectoryW(NULL, (LPWSTR) env->envname, 1, (LPBYTE) driverdir, size, &size)) {
        /* Should never Fail */
        return FALSE;
    }
    lstrcatW(driverdir, env->versionsubdir);
    lstrcatW(driverdir, backslashW);

    /* dirlen must not include the terminating zero */
    dirlen = lstrlenW(driverdir) * sizeof(WCHAR);

    if (!DriverName[0] || RegOpenKeyW(hkeyDrivers, DriverName, &hkeyDriver) != ERROR_SUCCESS) {
        ERR("Can't find driver %s in registry\n", debugstr_w(DriverName));
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER); /* ? */
        return FALSE;
    }

    /* pEnvironment */
    size = (lstrlenW(env->envname) + 1) * sizeof(WCHAR);

    *pcbNeeded += size;
    if (*pcbNeeded <= cbBuf) {
        lstrcpyW((LPWSTR)strPtr, env->envname);
        if (di) di->pEnvironment = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    /* .pDriverPath is the Graphics rendering engine.
        The full Path is required to avoid a crash in some apps */
    if (get_filename_from_reg(hkeyDriver, driverdir, dirlen, DriverW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg(hkeyDriver, driverdir, dirlen, DriverW, strPtr, size, &tmp);

        if (di) di->pDriverPath = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    /* .pDataFile: For postscript-drivers, this is the ppd-file */
    if (get_filename_from_reg(hkeyDriver, driverdir, dirlen, Data_FileW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg(hkeyDriver, driverdir, dirlen, Data_FileW, strPtr, size, &size);

        if (di) di->pDataFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pConfigFile is the Driver user Interface */
    if (get_filename_from_reg(hkeyDriver, driverdir, dirlen, Configuration_FileW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg(hkeyDriver, driverdir, dirlen, Configuration_FileW, strPtr, size, &size);

        if (di) di->pConfigFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level == 2 ) {
        RegCloseKey(hkeyDriver);
        TRACE("buffer space %d required %d\n", cbBuf, *pcbNeeded);
        return TRUE;
    }

    if (Level == 5 ) {
        RegCloseKey(hkeyDriver);
        FIXME("level 5: incomplete\n");
        return TRUE;
    }

    /* .pHelpFile */
    if (get_filename_from_reg(hkeyDriver, driverdir, dirlen, Help_FileW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg(hkeyDriver, driverdir, dirlen, Help_FileW, strPtr, size, &size);

        if (di) di->pHelpFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pDependentFiles */
    if (get_filename_from_reg(hkeyDriver, driverdir, dirlen, Dependent_FilesW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg(hkeyDriver, driverdir, dirlen, Dependent_FilesW, strPtr, size, &size);

        if (di) di->pDependentFiles = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }
    else if (GetVersion() & 0x80000000) {
        /* PowerPoint XP expects that pDependentFiles is always valid on win9x */
        size = 2 * sizeof(WCHAR);
        *pcbNeeded += size;
        if ((*pcbNeeded <= cbBuf) && strPtr) ZeroMemory(strPtr, size);

        if (di) di->pDependentFiles = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pMonitorName is the optional Language Monitor */
    if (WINSPOOL_GetStringFromReg(hkeyDriver, MonitorW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, MonitorW, strPtr, size, &size);

        if (di) di->pMonitorName = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pDefaultDataType */
    if (WINSPOOL_GetStringFromReg(hkeyDriver, DatatypeW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, DatatypeW, strPtr, size, &size);

        if (di) di->pDefaultDataType = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level == 3 ) {
        RegCloseKey(hkeyDriver);
        TRACE("buffer space %d required %d\n", cbBuf, *pcbNeeded);
        return TRUE;
    }

    /* .pszzPreviousNames */
    if (WINSPOOL_GetStringFromReg(hkeyDriver, Previous_NamesW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, Previous_NamesW, strPtr, size, &size);

        if (di) di->pszzPreviousNames = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level == 4 ) {
        RegCloseKey(hkeyDriver);
        TRACE("buffer space %d required %d\n", cbBuf, *pcbNeeded);
        return TRUE;
    }

    /* support is missing, but not important enough for a FIXME */
    TRACE("%s: DriverDate + DriverVersion not supported\n", debugstr_w(DriverName));

    /* .pszMfgName */
    if (WINSPOOL_GetStringFromReg(hkeyDriver, ManufacturerW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, ManufacturerW, strPtr, size, &size);

        if (di) di->pszMfgName = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pszOEMUrl */
    if (WINSPOOL_GetStringFromReg(hkeyDriver, OEM_UrlW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, OEM_UrlW, strPtr, size, &size);

        if (di) di->pszOEMUrl = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pszHardwareID */
    if (WINSPOOL_GetStringFromReg(hkeyDriver, HardwareIDW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, HardwareIDW, strPtr, size, &size);

        if (di) di->pszHardwareID = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pszProvider */
    if (WINSPOOL_GetStringFromReg(hkeyDriver, ProviderW, strPtr, 0, &size)) {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg(hkeyDriver, ProviderW, strPtr, size, &size);

        if (di) di->pszProvider = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level == 6 ) {
        RegCloseKey(hkeyDriver);
        return TRUE;
    }

    /* support is missing, but not important enough for a FIXME */
    TRACE("level 8: incomplete\n");

    TRACE("buffer space %d required %d\n", cbBuf, *pcbNeeded);
    RegCloseKey(hkeyDriver);
    return TRUE;
}

/*****************************************************************************
 *          GetPrinterDriverW  [WINSPOOL.@]
 */
BOOL WINAPI GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment,
                                  DWORD Level, LPBYTE pDriverInfo,
                                  DWORD cbBuf, LPDWORD pcbNeeded)
{
    LPCWSTR name;
    WCHAR DriverName[100];
    DWORD ret, type, size, needed = 0;
    LPBYTE ptr = NULL;
    HKEY hkeyPrinter, hkeyDrivers;
    const printenv_t * env;

    TRACE("(%p,%s,%d,%p,%d,%p)\n",hPrinter,debugstr_w(pEnvironment),
	  Level,pDriverInfo,cbBuf, pcbNeeded);

    if (cbBuf > 0)
        ZeroMemory(pDriverInfo, cbBuf);

    if (!(name = get_opened_printer_name(hPrinter))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (Level < 1 || Level == 7 || Level > 8) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    env = validate_envW(pEnvironment);
    if (!env) return FALSE;     /* SetLastError() is in validate_envW */

    ret = open_printer_reg_key( name, &hkeyPrinter );
    if (ret)
    {
        ERR( "Can't find opened printer %s in registry\n", debugstr_w(name) );
        SetLastError( ret );
        return FALSE;
    }

    size = sizeof(DriverName);
    DriverName[0] = 0;
    ret = RegQueryValueExW(hkeyPrinter, Printer_DriverW, 0, &type,
			   (LPBYTE)DriverName, &size);
    RegCloseKey(hkeyPrinter);
    if(ret != ERROR_SUCCESS) {
        ERR("Can't get DriverName for printer %s\n", debugstr_w(name));
	return FALSE;
    }

    hkeyDrivers = WINSPOOL_OpenDriverReg(pEnvironment);
    if(!hkeyDrivers) {
        ERR("Can't create Drivers key\n");
	return FALSE;
    }

    size = di_sizeof[Level];
    if ((size <= cbBuf) && pDriverInfo)
        ptr = pDriverInfo + size;

    if(!WINSPOOL_GetDriverInfoFromReg(hkeyDrivers, DriverName,
                         env, Level, pDriverInfo, ptr,
                         (cbBuf < size) ? 0 : cbBuf - size,
                         &needed)) {
            RegCloseKey(hkeyDrivers);
            return FALSE;
    }

    RegCloseKey(hkeyDrivers);

    if(pcbNeeded) *pcbNeeded = size + needed;
    TRACE("buffer space %d required %d\n", cbBuf, size + needed);
    if(cbBuf >= size + needed) return TRUE;
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
    LPBYTE buf = NULL;

    if (cbBuf)
    {
        ZeroMemory(pDriverInfo, cbBuf);
        buf = HeapAlloc(GetProcessHeap(), 0, cbBuf);
    }

    pwstrEnvW = asciitounicode(&pEnvW, pEnvironment);
    ret = GetPrinterDriverW(hPrinter, pwstrEnvW, Level, buf,
				    cbBuf, pcbNeeded);
    if (ret)
        convert_driverinfo_W_to_A(pDriverInfo, buf, Level, cbBuf, 1);

    HeapFree(GetProcessHeap(), 0, buf);

    RtlFreeUnicodeString(&pEnvW);
    return ret;
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
    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_w(pName), 
          debugstr_w(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if (Level != 1) {
        /* (Level != 1) is ignored in win9x */
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }
    if (pcbNeeded == NULL) {
        /* (pcbNeeded == NULL) is ignored in win9x */
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    return backend->fpGetPrinterDriverDirectory(pName, pEnvironment, Level,
                                                pDriverDirectory, cbBuf, pcbNeeded);

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
        ret = needed <= cbBuf;
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
 *
 * See AddPrinterDriverW.
 *
 */
BOOL WINAPI AddPrinterDriverA(LPSTR pName, DWORD level, LPBYTE pDriverInfo)
{
    TRACE("(%s, %d, %p)\n", debugstr_a(pName), level, pDriverInfo);
    return AddPrinterDriverExA(pName, level, pDriverInfo, APD_COPY_NEW_FILES);
}

/******************************************************************************
 *  AddPrinterDriverW (WINSPOOL.@)
 *
 * Install a Printer Driver
 *
 * PARAMS
 *  pName           [I] Servername or NULL (local Computer)
 *  level           [I] Level for the supplied DRIVER_INFO_*W struct
 *  pDriverInfo     [I] PTR to DRIVER_INFO_*W struct with the Driver Parameter
 *
 * RESULTS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
BOOL WINAPI AddPrinterDriverW(LPWSTR pName, DWORD level, LPBYTE pDriverInfo)
{
    TRACE("(%s, %d, %p)\n", debugstr_w(pName), level, pDriverInfo);
    return AddPrinterDriverExW(pName, level, pDriverInfo, APD_COPY_NEW_FILES);
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
    return TRUE;
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
                                        DWORD driver_index,
                                        DWORD cbBuf, LPDWORD pcbNeeded,
                                        LPDWORD pcFound, DWORD data_offset)

{   HKEY  hkeyDrivers;
    DWORD i, size = 0;
    const printenv_t * env;

    TRACE("%s,%s,%d,%p,%d,%d,%d\n",
          debugstr_w(pName), debugstr_w(pEnvironment),
          Level, pDriverInfo, driver_index, cbBuf, data_offset);

    env = validate_envW(pEnvironment);
    if (!env) return FALSE;     /* SetLastError() is in validate_envW */

    *pcFound = 0;

    hkeyDrivers = WINSPOOL_OpenDriverReg(pEnvironment);
    if(!hkeyDrivers) {
        ERR("Can't open Drivers key\n");
        return FALSE;
    }

    if(RegQueryInfoKeyA(hkeyDrivers, NULL, NULL, NULL, pcFound, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hkeyDrivers);
        ERR("Can't query Drivers key\n");
        return FALSE;
    }
    TRACE("Found %d Drivers\n", *pcFound);

    /* get size of single struct
     * unicode and ascii structure have the same size
     */
    size = di_sizeof[Level];

    if (data_offset == 0)
        data_offset = size * (*pcFound);
    *pcbNeeded = data_offset;

    for( i = 0; i < *pcFound; i++) {
        WCHAR DriverNameW[255];
        PBYTE table_ptr = NULL;
        PBYTE data_ptr = NULL;
        DWORD needed = 0;

        if(RegEnumKeyW(hkeyDrivers, i, DriverNameW, sizeof(DriverNameW)/sizeof(DriverNameW[0]))
                       != ERROR_SUCCESS) {
            ERR("Can't enum key number %d\n", i);
            RegCloseKey(hkeyDrivers);
            return FALSE;
        }

        if (pDriverInfo && ((driver_index + i + 1) * size) <= cbBuf)
            table_ptr = pDriverInfo + (driver_index + i) * size;
        if (pDriverInfo && *pcbNeeded <= cbBuf)
            data_ptr = pDriverInfo + *pcbNeeded;

        if(!WINSPOOL_GetDriverInfoFromReg(hkeyDrivers, DriverNameW,
                         env, Level, table_ptr, data_ptr,
                         (cbBuf < *pcbNeeded) ? 0 : cbBuf - *pcbNeeded,
                         &needed)) {
            RegCloseKey(hkeyDrivers);
            return FALSE;
        }

        *pcbNeeded += needed;
    }

    RegCloseKey(hkeyDrivers);

    if(cbBuf < *pcbNeeded){
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

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
    static const WCHAR allW[] = {'a','l','l',0};
    BOOL ret;
    DWORD found;

    if ((pcbNeeded == NULL) || (pcReturned == NULL))
    {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    /* check for local drivers */
    if((pName) && (pName[0])) {
        FIXME("remote drivers (%s) not supported!\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    /* check input parameter */
    if ((Level < 1) || (Level == 7) || (Level > 8)) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if(pDriverInfo && cbBuf > 0)
        memset( pDriverInfo, 0, cbBuf);

    /* Exception:  pull all printers */
    if (pEnvironment && !strcmpW(pEnvironment, allW))
    {
        DWORD i, needed, bufsize = cbBuf;
        DWORD total_found = 0;
        DWORD data_offset;

        /* Precompute the overall total; we need this to know
           where pointers end and data begins (i.e. data_offset) */
        for (i = 0; i < sizeof(all_printenv)/sizeof(all_printenv[0]); i++)
        {
            needed = found = 0;
            ret = WINSPOOL_EnumPrinterDrivers(pName, all_printenv[i]->envname, Level,
                                              NULL, 0, 0, &needed, &found, 0);
            if (!ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER) return FALSE;
            total_found += found;
        }

        data_offset = di_sizeof[Level] * total_found;

        *pcReturned = 0;
        *pcbNeeded = 0;
        total_found = 0;
        for (i = 0; i < sizeof(all_printenv)/sizeof(all_printenv[0]); i++)
        {
            needed = found = 0;
            ret = WINSPOOL_EnumPrinterDrivers(pName, all_printenv[i]->envname, Level,
                                              pDriverInfo, total_found, bufsize, &needed, &found, data_offset);
            if (!ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER) return FALSE;
            else if (ret)
                *pcReturned += found;
            *pcbNeeded = needed;
            data_offset = needed;
            total_found += found;
        }
        return ret;
    }

    /* Normal behavior */
    ret = WINSPOOL_EnumPrinterDrivers(pName, pEnvironment, Level, pDriverInfo,
                                       0, cbBuf, pcbNeeded, &found, 0);
    if (ret)
        *pcReturned = found;

    return ret;
}

/*****************************************************************************
 *          EnumPrinterDriversA  [WINSPOOL.@]
 *
 *    see function EnumPrinterDrivers for RETURNS, BUGS
 */
BOOL WINAPI EnumPrinterDriversA(LPSTR pName, LPSTR pEnvironment, DWORD Level,
                                LPBYTE pDriverInfo, DWORD cbBuf,
                                LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    BOOL ret;
    UNICODE_STRING pNameW, pEnvironmentW;
    PWSTR pwstrNameW, pwstrEnvironmentW;
    LPBYTE buf = NULL;

    if (cbBuf)
        buf = HeapAlloc(GetProcessHeap(), 0, cbBuf);

    pwstrNameW = asciitounicode(&pNameW, pName);
    pwstrEnvironmentW = asciitounicode(&pEnvironmentW, pEnvironment);

    ret = EnumPrinterDriversW(pwstrNameW, pwstrEnvironmentW, Level,
                                buf, cbBuf, pcbNeeded, pcReturned);
    if (ret)
        convert_driverinfo_W_to_A(pDriverInfo, buf, Level, cbBuf, *pcReturned);

    HeapFree(GetProcessHeap(), 0, buf);

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
BOOL WINAPI EnumPortsW(LPWSTR pName, DWORD Level, LPBYTE pPorts, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{

    TRACE("(%s, %d, %p, %d, %p, %p)\n", debugstr_w(pName), Level, pPorts,
          cbBuf, pcbNeeded, pcReturned);

    if ((backend == NULL)  && !load_backend()) return FALSE;

    /* Level is not checked in win9x */
    if (!Level || (Level > 2)) {
        WARN("level (%d) is ignored in win9x\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }
    if (!pcbNeeded || (!pPorts && (cbBuf > 0))) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    return backend->fpEnumPorts(pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);
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
    WCHAR   default_printer[MAX_PATH];
    LPWSTR  buffer = NULL;
    HKEY    hreg;
    DWORD   size;
    DWORD   namelen;
    LONG    lres;

    TRACE("(%s)\n", debugstr_w(pszPrinter));
    if ((pszPrinter == NULL) || (pszPrinter[0] == '\0')) {

        default_printer[0] = '\0';
        size = sizeof(default_printer)/sizeof(WCHAR);

        /* if we have a default Printer, do nothing. */
        if (GetDefaultPrinterW(default_printer, &size))
            return TRUE;

        pszPrinter = NULL;
        /* we have no default Printer: search local Printers and use the first */
        if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, PrintersW, 0, KEY_READ, &hreg)) {

            default_printer[0] = '\0';
            size = sizeof(default_printer)/sizeof(WCHAR);
            if (!RegEnumKeyExW(hreg, 0, default_printer, &size, NULL, NULL, NULL, NULL)) {

                pszPrinter = default_printer;
                TRACE("using %s\n", debugstr_w(pszPrinter));
            }
            RegCloseKey(hreg);
        }

        if (pszPrinter == NULL) {
            TRACE("no local printer found\n");
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }

    /* "pszPrinter" is never empty or NULL here. */
    namelen = lstrlenW(pszPrinter);
    size = namelen + (MAX_PATH * 2) + 3; /* printer,driver,port and a 0 */
    buffer = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (!buffer ||
        (RegOpenKeyExW(HKEY_CURRENT_USER, user_printers_reg_key, 0, KEY_READ, &hreg) != ERROR_SUCCESS)) {
        HeapFree(GetProcessHeap(), 0, buffer);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    /* read the devices entry for the printer (driver,port) to build the string for the
       default device entry (printer,driver,port) */
    memcpy(buffer, pszPrinter, namelen * sizeof(WCHAR));
    buffer[namelen] = ',';
    namelen++; /* move index to the start of the driver */

    size = ((MAX_PATH * 2) + 2) * sizeof(WCHAR); /* driver,port and a 0 */
    lres = RegQueryValueExW(hreg, pszPrinter, NULL, NULL, (LPBYTE) (&buffer[namelen]), &size);
    if (!lres) {
        TRACE("set device to %s\n", debugstr_w(buffer));

        if (!WriteProfileStringW(windowsW, deviceW, buffer)) {
            TRACE("failed to set the device entry: %d\n", GetLastError());
            lres = ERROR_INVALID_PRINTER_NAME;
        }

        /* remove the next section, when INIFileMapping is implemented */
        {
            HKEY hdev;
            if (!RegCreateKeyW(HKEY_CURRENT_USER, user_default_reg_key, &hdev)) {
                RegSetValueExW(hdev, deviceW, 0, REG_SZ, (LPBYTE)buffer, (lstrlenW(buffer) + 1) * sizeof(WCHAR));
                RegCloseKey(hdev);
            }
        }
    }
    else
    {
        if (lres != ERROR_FILE_NOT_FOUND)
            FIXME("RegQueryValueExW failed with %d for %s\n", lres, debugstr_w(pszPrinter));

        SetLastError(ERROR_INVALID_PRINTER_NAME);
    }

    RegCloseKey(hreg);
    HeapFree(GetProcessHeap(), 0, buffer);
    return (lres == ERROR_SUCCESS);
}

/******************************************************************************
 *		SetDefaultPrinterA   (WINSPOOL.202)
 *
 * See SetDefaultPrinterW.
 *
 */
BOOL WINAPI SetDefaultPrinterA(LPCSTR pszPrinter)
{
    LPWSTR  bufferW = NULL;
    BOOL    res;

    TRACE("(%s)\n", debugstr_a(pszPrinter));
    if(pszPrinter) {
        INT len = MultiByteToWideChar(CP_ACP, 0, pszPrinter, -1, NULL, 0);
        bufferW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (bufferW) MultiByteToWideChar(CP_ACP, 0, pszPrinter, -1, bufferW, len);
    }
    res = SetDefaultPrinterW(bufferW);
    HeapFree(GetProcessHeap(), 0, bufferW);
    return res;
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
    opened_printer_t *printer;
    HKEY hkeyPrinters, hkeyPrinter = 0, hkeySubkey = 0;
    DWORD ret;

    TRACE("(%p, %s, %s, %p, %p, %u, %p)\n", hPrinter, debugstr_a(pKeyName),
            debugstr_a(pValueName), pType, pData, nSize, pcbNeeded);

    printer = get_opened_printer(hPrinter);
    if(!printer) return ERROR_INVALID_HANDLE;

    ret = RegOpenKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters);
    if (ret) return ret;

    TRACE("printer->name: %s\n", debugstr_w(printer->name));

    if (printer->name) {

        ret = RegOpenKeyW(hkeyPrinters, printer->name, &hkeyPrinter);
        if (ret) {
            RegCloseKey(hkeyPrinters);
            return ret;
        }
        if((ret = RegOpenKeyA(hkeyPrinter, pKeyName, &hkeySubkey)) != ERROR_SUCCESS) {
            WARN("Can't open subkey %s: %d\n", debugstr_a(pKeyName), ret);
            RegCloseKey(hkeyPrinter);
            RegCloseKey(hkeyPrinters);
            return ret;
        }
    }
    *pcbNeeded = nSize;
    ret = RegQueryValueExA(printer->name ? hkeySubkey : hkeyPrinters, pValueName,
                          0, pType, pData, pcbNeeded);

    if (!ret && !pData) ret = ERROR_MORE_DATA;

    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);

    TRACE("--> %d\n", ret);
    return ret;
}

/******************************************************************************
 *		GetPrinterDataExW   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataExW(HANDLE hPrinter, LPCWSTR pKeyName,
			       LPCWSTR pValueName, LPDWORD pType,
			       LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    opened_printer_t *printer;
    HKEY hkeyPrinters, hkeyPrinter = 0, hkeySubkey = 0;
    DWORD ret;

    TRACE("(%p, %s, %s, %p, %p, %u, %p)\n", hPrinter, debugstr_w(pKeyName),
            debugstr_w(pValueName), pType, pData, nSize, pcbNeeded);

    printer = get_opened_printer(hPrinter);
    if(!printer) return ERROR_INVALID_HANDLE;

    ret = RegOpenKeyW(HKEY_LOCAL_MACHINE, PrintersW, &hkeyPrinters);
    if (ret) return ret;

    TRACE("printer->name: %s\n", debugstr_w(printer->name));

    if (printer->name) {

        ret = RegOpenKeyW(hkeyPrinters, printer->name, &hkeyPrinter);
        if (ret) {
            RegCloseKey(hkeyPrinters);
            return ret;
        }
        if((ret = RegOpenKeyW(hkeyPrinter, pKeyName, &hkeySubkey)) != ERROR_SUCCESS) {
            WARN("Can't open subkey %s: %d\n", debugstr_w(pKeyName), ret);
            RegCloseKey(hkeyPrinter);
            RegCloseKey(hkeyPrinters);
            return ret;
        }
    }
    *pcbNeeded = nSize;
    ret = RegQueryValueExW(printer->name ? hkeySubkey : hkeyPrinters, pValueName,
                          0, pType, pData, pcbNeeded);

    if (!ret && !pData) ret = ERROR_MORE_DATA;

    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    RegCloseKey(hkeyPrinters);

    TRACE("--> %d\n", ret);
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
    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pMonitorName));

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if (!pMonitorName) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    return backend->fpAddPort(pName, hWnd, pMonitorName);
}

/******************************************************************************
 *             AddPortExA (WINSPOOL.@)
 *
 * See AddPortExW.
 *
 */
BOOL WINAPI AddPortExA(LPSTR pName, DWORD level, LPBYTE pBuffer, LPSTR pMonitorName)
{
    PORT_INFO_2W   pi2W;
    PORT_INFO_2A * pi2A;
    LPWSTR  nameW = NULL;
    LPWSTR  monitorW = NULL;
    DWORD   len;
    BOOL    res;

    pi2A = (PORT_INFO_2A *) pBuffer;

    TRACE("(%s, %d, %p, %s): %s\n", debugstr_a(pName), level, pBuffer,
            debugstr_a(pMonitorName), debugstr_a(pi2A ? pi2A->pPortName : NULL));

    if ((level < 1) || (level > 2)) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!pi2A) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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

    ZeroMemory(&pi2W, sizeof(PORT_INFO_2W));

    if (pi2A->pPortName) {
        len = MultiByteToWideChar(CP_ACP, 0, pi2A->pPortName, -1, NULL, 0);
        pi2W.pPortName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pi2A->pPortName, -1, pi2W.pPortName, len);
    }

    if (level > 1) {
        if (pi2A->pMonitorName) {
            len = MultiByteToWideChar(CP_ACP, 0, pi2A->pMonitorName, -1, NULL, 0);
            pi2W.pMonitorName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pi2A->pMonitorName, -1, pi2W.pMonitorName, len);
        }

        if (pi2A->pDescription) {
            len = MultiByteToWideChar(CP_ACP, 0, pi2A->pDescription, -1, NULL, 0);
            pi2W.pDescription = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pi2A->pDescription, -1, pi2W.pDescription, len);
        }
        pi2W.fPortType = pi2A->fPortType;
        pi2W.Reserved = pi2A->Reserved;
    }

    res = AddPortExW(nameW, level, (LPBYTE) &pi2W, monitorW);

    HeapFree(GetProcessHeap(), 0, nameW);
    HeapFree(GetProcessHeap(), 0, monitorW);
    HeapFree(GetProcessHeap(), 0, pi2W.pPortName);
    HeapFree(GetProcessHeap(), 0, pi2W.pMonitorName);
    HeapFree(GetProcessHeap(), 0, pi2W.pDescription);
    return res;

}

/******************************************************************************
 *             AddPortExW (WINSPOOL.@)
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
BOOL WINAPI AddPortExW(LPWSTR pName, DWORD level, LPBYTE pBuffer, LPWSTR pMonitorName)
{
    PORT_INFO_2W * pi2;

    pi2 = (PORT_INFO_2W *) pBuffer;

    TRACE("(%s, %d, %p, %s): %s %s %s\n", debugstr_w(pName), level, pBuffer,
            debugstr_w(pMonitorName), debugstr_w(pi2 ? pi2->pPortName : NULL),
            debugstr_w(((level > 1) && pi2) ? pi2->pMonitorName : NULL),
            debugstr_w(((level > 1) && pi2) ? pi2->pDescription : NULL));

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if ((!pi2) || (!pMonitorName) || (!pMonitorName[0])) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return backend->fpAddPortEx(pName, level, pBuffer, pMonitorName);
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
 *  AddPrinterDriverExW (WINSPOOL.@)
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
BOOL WINAPI AddPrinterDriverExW( LPWSTR pName, DWORD level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    TRACE("(%s, %d, %p, 0x%x)\n", debugstr_w(pName), level, pDriverInfo, dwFileCopyFlags);

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if (level < 2 || level == 5 || level == 7 || level > 8) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!pDriverInfo) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return backend->fpAddPrinterDriverEx(pName, level, pDriverInfo, dwFileCopyFlags);
}

/******************************************************************************
 *  AddPrinterDriverExA (WINSPOOL.@)
 *
 * See AddPrinterDriverExW.
 *
 */
BOOL WINAPI AddPrinterDriverExA(LPSTR pName, DWORD Level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    DRIVER_INFO_8A  *diA;
    DRIVER_INFO_8W   diW;
    LPWSTR  nameW = NULL;
    DWORD   lenA;
    DWORD   len;
    BOOL    res = FALSE;

    TRACE("(%s, %d, %p, 0x%x)\n", debugstr_a(pName), Level, pDriverInfo, dwFileCopyFlags);

    diA = (DRIVER_INFO_8A  *) pDriverInfo;
    ZeroMemory(&diW, sizeof(diW));

    if (Level < 2 || Level == 5 || Level == 7 || Level > 8) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (diA == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* convert servername to unicode */
    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    /* common fields */
    diW.cVersion = diA->cVersion;

    if (diA->pName) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pName, -1, NULL, 0);
        diW.pName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pName, -1, diW.pName, len);
    }

    if (diA->pEnvironment) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pEnvironment, -1, NULL, 0);
        diW.pEnvironment = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pEnvironment, -1, diW.pEnvironment, len);
    }

    if (diA->pDriverPath) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pDriverPath, -1, NULL, 0);
        diW.pDriverPath = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pDriverPath, -1, diW.pDriverPath, len);
    }

    if (diA->pDataFile) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pDataFile, -1, NULL, 0);
        diW.pDataFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pDataFile, -1, diW.pDataFile, len);
    }

    if (diA->pConfigFile) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pConfigFile, -1, NULL, 0);
        diW.pConfigFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pConfigFile, -1, diW.pConfigFile, len);
    }

    if ((Level > 2) && diA->pDependentFiles) {
        lenA = multi_sz_lenA(diA->pDependentFiles);
        len = MultiByteToWideChar(CP_ACP, 0, diA->pDependentFiles, lenA, NULL, 0);
        diW.pDependentFiles = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pDependentFiles, lenA, diW.pDependentFiles, len);
    }

    if ((Level > 2) && diA->pMonitorName) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pMonitorName, -1, NULL, 0);
        diW.pMonitorName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pMonitorName, -1, diW.pMonitorName, len);
    }

    if ((Level > 3) && diA->pDefaultDataType) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pDefaultDataType, -1, NULL, 0);
        diW.pDefaultDataType = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pDefaultDataType, -1, diW.pDefaultDataType, len);
    }

    if ((Level > 3) && diA->pszzPreviousNames) {
        lenA = multi_sz_lenA(diA->pszzPreviousNames);
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszzPreviousNames, lenA, NULL, 0);
        diW.pszzPreviousNames = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszzPreviousNames, lenA, diW.pszzPreviousNames, len);
    }

    if ((Level > 5) && diA->pszMfgName) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszMfgName, -1, NULL, 0);
        diW.pszMfgName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszMfgName, -1, diW.pszMfgName, len);
    }

    if ((Level > 5) && diA->pszOEMUrl) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszOEMUrl, -1, NULL, 0);
        diW.pszOEMUrl = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszOEMUrl, -1, diW.pszOEMUrl, len);
    }

    if ((Level > 5) && diA->pszHardwareID) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszHardwareID, -1, NULL, 0);
        diW.pszHardwareID = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszHardwareID, -1, diW.pszHardwareID, len);
    }

    if ((Level > 5) && diA->pszProvider) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszProvider, -1, NULL, 0);
        diW.pszProvider = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszProvider, -1, diW.pszProvider, len);
    }

    if (Level > 7) {
        FIXME("level %u is incomplete\n", Level);
    }

    res = AddPrinterDriverExW(nameW, Level, (LPBYTE) &diW, dwFileCopyFlags);
    TRACE("got %u with %u\n", res, GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);
    HeapFree(GetProcessHeap(), 0, diW.pName);
    HeapFree(GetProcessHeap(), 0, diW.pEnvironment);
    HeapFree(GetProcessHeap(), 0, diW.pDriverPath);
    HeapFree(GetProcessHeap(), 0, diW.pDataFile);
    HeapFree(GetProcessHeap(), 0, diW.pConfigFile);
    HeapFree(GetProcessHeap(), 0, diW.pDependentFiles);
    HeapFree(GetProcessHeap(), 0, diW.pMonitorName);
    HeapFree(GetProcessHeap(), 0, diW.pDefaultDataType);
    HeapFree(GetProcessHeap(), 0, diW.pszzPreviousNames);
    HeapFree(GetProcessHeap(), 0, diW.pszMfgName);
    HeapFree(GetProcessHeap(), 0, diW.pszOEMUrl);
    HeapFree(GetProcessHeap(), 0, diW.pszHardwareID);
    HeapFree(GetProcessHeap(), 0, diW.pszProvider);

    TRACE("=> %u with %u\n", res, GetLastError());
    return res;
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

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if (!pPortName) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    return backend->fpConfigurePort(pName, hWnd, pPortName);
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

    hkey_drivers = WINSPOOL_OpenDriverReg(pEnvironment);

    if(!hkey_drivers)
    {
        ERR("Can't open drivers key\n");
        return FALSE;
    }

    if(RegDeleteTreeW(hkey_drivers, pDriverName) == ERROR_SUCCESS)
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
 */
BOOL WINAPI EnumMonitorsW(LPWSTR pName, DWORD Level, LPBYTE pMonitors,
                          DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{

    TRACE("(%s, %d, %p, %d, %p, %p)\n", debugstr_w(pName), Level, pMonitors,
          cbBuf, pcbNeeded, pcReturned);

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if (!pcbNeeded || !pcReturned || (!pMonitors && (cbBuf > 0))) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    return backend->fpEnumMonitors(pName, Level, pMonitors, cbBuf, pcbNeeded, pcReturned);
}

/******************************************************************************
 * SpoolerInit (WINSPOOL.@)
 *
 * Initialize the Spooler
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  The function fails on windows, when the spooler service is not running
 *
 */
BOOL WINAPI SpoolerInit(void)
{

    if ((backend == NULL)  && !load_backend()) return FALSE;
    return TRUE;
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

    if ((backend == NULL)  && !load_backend()) return FALSE;

    printer = get_opened_printer(hXcv);
    if (!printer || (!printer->backend_printer)) {
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

    return backend->fpXcvData(printer->backend_printer, pszDataName, pInputData,
                    cbInputData, pOutputData, cbOutputData, pcbOutputNeeded, pdwStatus);

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
 * See EnumPrintProcessorsW.
 *
 */
BOOL WINAPI EnumPrintProcessorsA(LPSTR pName, LPSTR pEnvironment, DWORD Level, 
                            LPBYTE pPPInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    BOOL    res;
    LPBYTE  bufferW = NULL;
    LPWSTR  nameW = NULL;
    LPWSTR  envW = NULL;
    DWORD   needed = 0;
    DWORD   numentries = 0;
    INT     len;

    TRACE("(%s, %s, %d, %p, %d, %p, %p)\n", debugstr_a(pName), debugstr_a(pEnvironment),
                Level, pPPInfo, cbBuf, pcbNeeded, pcReturned);

    /* convert names to unicode */
    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }
    if (pEnvironment) {
        len = MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, NULL, 0);
        envW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, envW, len);
    }

    /* alloc (userbuffersize*sizeof(WCHAR) and try to enum the monitors */
    needed = cbBuf * sizeof(WCHAR);
    if (needed) bufferW = HeapAlloc(GetProcessHeap(), 0, needed);
    res = EnumPrintProcessorsW(nameW, envW, Level, bufferW, needed, pcbNeeded, pcReturned);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (pcbNeeded) needed = *pcbNeeded;
        /* HeapReAlloc return NULL, when bufferW was NULL */
        bufferW = (bufferW) ? HeapReAlloc(GetProcessHeap(), 0, bufferW, needed) :
                              HeapAlloc(GetProcessHeap(), 0, needed);

        /* Try again with the large Buffer */
        res = EnumPrintProcessorsW(nameW, envW, Level, bufferW, needed, pcbNeeded, pcReturned);
    }
    numentries = pcReturned ? *pcReturned : 0;
    needed = 0;

    if (res) {
        /* EnumPrintProcessorsW collected all Data. Parse them to calculate ANSI-Size */
        DWORD   index;
        LPSTR   ptr;
        PPRINTPROCESSOR_INFO_1W ppiw;
        PPRINTPROCESSOR_INFO_1A ppia;

        /* First pass: calculate the size for all Entries */
        ppiw = (PPRINTPROCESSOR_INFO_1W) bufferW;
        ppia = (PPRINTPROCESSOR_INFO_1A) pPPInfo;
        index = 0;
        while (index < numentries) {
            index++;
            needed += sizeof(PRINTPROCESSOR_INFO_1A);
            TRACE("%p: parsing #%d (%s)\n", ppiw, index, debugstr_w(ppiw->pName));

            needed += WideCharToMultiByte(CP_ACP, 0, ppiw->pName, -1,
                                            NULL, 0, NULL, NULL);

            ppiw = (PPRINTPROCESSOR_INFO_1W) (((LPBYTE)ppiw) + sizeof(PRINTPROCESSOR_INFO_1W));
            ppia = (PPRINTPROCESSOR_INFO_1A) (((LPBYTE)ppia) + sizeof(PRINTPROCESSOR_INFO_1A));
        }

        /* check for errors and quit on failure */
        if (cbBuf < needed) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            res = FALSE;
            goto epp_cleanup;
        }

        len = numentries * sizeof(PRINTPROCESSOR_INFO_1A); /* room for structs */
        ptr = (LPSTR) &pPPInfo[len];        /* start of strings */
        cbBuf -= len ;                      /* free Bytes in the user-Buffer */
        ppiw = (PPRINTPROCESSOR_INFO_1W) bufferW;
        ppia = (PPRINTPROCESSOR_INFO_1A) pPPInfo;
        index = 0;
        /* Second Pass: Fill the User Buffer (if we have one) */
        while ((index < numentries) && pPPInfo) {
            index++;
            TRACE("%p: writing PRINTPROCESSOR_INFO_1A #%d\n", ppia, index);
            ppia->pName = ptr;
            len = WideCharToMultiByte(CP_ACP, 0, ppiw->pName, -1,
                                            ptr, cbBuf , NULL, NULL);
            ptr += len;
            cbBuf -= len;

            ppiw = (PPRINTPROCESSOR_INFO_1W) (((LPBYTE)ppiw) + sizeof(PRINTPROCESSOR_INFO_1W));
            ppia = (PPRINTPROCESSOR_INFO_1A) (((LPBYTE)ppia) + sizeof(PRINTPROCESSOR_INFO_1A));

        }
    }
epp_cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = (res) ? numentries : 0;

    HeapFree(GetProcessHeap(), 0, nameW);
    HeapFree(GetProcessHeap(), 0, envW);
    HeapFree(GetProcessHeap(), 0, bufferW);

    TRACE("returning %d with %d (%d byte for %d entries)\n",
            (res), GetLastError(), needed, numentries);

    return (res);
}

/*****************************************************************************
 *          EnumPrintProcessorsW [WINSPOOL.@]
 *
 * Enumerate available Print Processors
 *
 * PARAMS
 *  pName        [I] Servername or NULL (local Computer)
 *  pEnvironment [I] Printing-Environment or NULL (Default)
 *  Level        [I] Structure-Level (Only 1 is allowed)
 *  pPPInfo      [O] PTR to Buffer that receives the Result
 *  cbBuf        [I] Size of Buffer at pPPInfo
 *  pcbNeeded    [O] PTR to DWORD that receives the size in Bytes used / required for pPPInfo
 *  pcReturned   [O] PTR to DWORD that receives the number of Print Processors in pPPInfo
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPPInfo, if cbBuf is too small
 *
 */
BOOL WINAPI EnumPrintProcessorsW(LPWSTR pName, LPWSTR pEnvironment, DWORD Level,
                            LPBYTE pPPInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{

    TRACE("(%s, %s, %d, %p, %d, %p, %p)\n", debugstr_w(pName), debugstr_w(pEnvironment),
                                Level, pPPInfo, cbBuf, pcbNeeded, pcReturned);

    if ((backend == NULL)  && !load_backend()) return FALSE;

    if (!pcbNeeded || !pcReturned) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    if (!pPPInfo && (cbBuf > 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return backend->fpEnumPrintProcessors(pName, pEnvironment, Level, pPPInfo,
                                          cbBuf, pcbNeeded, pcReturned);
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

    if (job->printer_name)
    {
        string_to_buf(job->printer_name, ptr, left, &size, unicode);
        if(space && size <= left)
        {
            ji1->pPrinterName = (LPWSTR)ptr;
            ptr += size;
            left -= size;
        }
        else
            space = FALSE;
        *pcbNeeded += size;
    }

    return space;
}

/*****************************************************************************
 *          get_job_info_2
 */
static BOOL get_job_info_2(job_t *job, JOB_INFO_2W *ji2, LPBYTE buf, DWORD cbBuf,
                           LPDWORD pcbNeeded, BOOL unicode)
{
    DWORD size, left = cbBuf;
    DWORD shift;
    BOOL space = (cbBuf > 0);
    LPBYTE ptr = buf;
    LPDEVMODEA  dmA = NULL;
    LPDEVMODEW  devmode;

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

    if (job->printer_name)
    {
        string_to_buf(job->printer_name, ptr, left, &size, unicode);
        if(space && size <= left)
        {
            ji2->pPrinterName = (LPWSTR)ptr;
            ptr += size;
            left -= size;
        }
        else
            space = FALSE;
        *pcbNeeded += size;
    }

    if (job->devmode)
    {
        if (!unicode)
        {
            dmA = DEVMODEdupWtoA(job->devmode);
            devmode = (LPDEVMODEW) dmA;
            if (dmA) size = dmA->dmSize + dmA->dmDriverExtra;
        }
        else
        {
            devmode = job->devmode;
            size = devmode->dmSize + devmode->dmDriverExtra;
        }

        if (!devmode)
             FIXME("Can't convert DEVMODE W to A\n");
        else
        {
            /* align DEVMODE to a DWORD boundary */
            shift = (4 - (*pcbNeeded & 3)) & 3;
            size += shift;

            if (size <= left)
            {
                ptr += shift;
                memcpy(ptr, devmode, size-shift);
                ji2->pDevMode = (LPDEVMODEW)ptr;
                if (!unicode) HeapFree(GetProcessHeap(), 0, dmA);
                ptr += size-shift;
                left -= size;
            }
            else
                space = FALSE;
            *pcbNeeded +=size;
        }
    }

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
    pid_t pid, wret;
    int status;

    if(!(unixname = wine_get_unix_file_name(filename)))
        return FALSE;

    len = WideCharToMultiByte(CP_UNIXCP, 0, cmd, -1, NULL, 0, NULL, NULL);
    cmdA = HeapAlloc(GetProcessHeap(), 0, len);
    WideCharToMultiByte(CP_UNIXCP, 0, cmd, -1, cmdA, len, NULL, NULL);

    TRACE("printing with: %s\n", cmdA);

    if((file_fd = open(unixname, O_RDONLY)) == -1)
        goto end;

    if (pipe(fds))
    {
        ERR("pipe() failed!\n");
        goto end;
    }

    if ((pid = fork()) == 0)
    {
        close(0);
        dup2(fds[0], 0);
        close(fds[1]);

        /* reset signals that we previously set to SIG_IGN */
        signal(SIGPIPE, SIG_DFL);

        execl("/bin/sh", "/bin/sh", "-c", cmdA, NULL);
        _exit(1);
    }
    else if (pid == -1)
    {
        ERR("fork() failed!\n");
        goto end;
    }

    close(fds[0]);
    fds[0] = -1;
    while((no_read = read(file_fd, buf, sizeof(buf))) > 0)
        write(fds[1], buf, no_read);

    close(fds[1]);
    fds[1] = -1;

    /* reap child */
    do {
        wret = waitpid(pid, &status, 0);
    } while (wret < 0 && errno == EINTR);
    if (wret < 0)
    {
        ERR("waitpid() failed!\n");
        goto end;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status))
    {
        ERR("child process failed! %d\n", status);
        goto end;
    }

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
 *          schedule_lpr
 */
static BOOL schedule_lpr(LPCWSTR printer_name, LPCWSTR filename)
{
    WCHAR *cmd;
    const WCHAR fmtW[] = {'l','p','r',' ','-','P','\'','%','s','\'',0};
    BOOL r;

    cmd = HeapAlloc(GetProcessHeap(), 0, strlenW(printer_name) * sizeof(WCHAR) + sizeof(fmtW));
    sprintfW(cmd, fmtW, printer_name);

    r = schedule_pipe(cmd, filename);

    HeapFree(GetProcessHeap(), 0, cmd);
    return r;
}

#ifdef SONAME_LIBCUPS
/*****************************************************************************
 *          get_cups_jobs_ticket_options
 *
 * Explicitly set CUPS options based on any %cupsJobTicket lines.
 * The CUPS scheduler only looks for these in Print-File requests, and since
 * cupsPrintFile uses Create-Job / Send-Document, the ticket lines don't get
 * parsed.
 */
static int get_cups_job_ticket_options( const char *file, int num_options, cups_option_t **options )
{
    FILE *fp = fopen( file, "r" );
    char buf[257]; /* DSC max of 256 + '\0' */
    const char *ps_adobe = "%!PS-Adobe-";
    const char *cups_job = "%cupsJobTicket:";

    if (!fp) return num_options;
    if (!fgets( buf, sizeof(buf), fp )) goto end;
    if (strncmp( buf, ps_adobe, strlen( ps_adobe ) )) goto end;
    while (fgets( buf, sizeof(buf), fp ))
    {
        if (strncmp( buf, cups_job, strlen( cups_job ) )) break;
        num_options = pcupsParseOptions( buf + strlen( cups_job ), num_options, options );
    }

end:
    fclose( fp );
    return num_options;
}

static int get_cups_default_options( const char *printer, int num_options, cups_option_t **options )
{
    cups_dest_t *dest;
    int i;

    if (!pcupsGetNamedDest) return num_options;

    dest = pcupsGetNamedDest( NULL, printer, NULL );
    if (!dest) return num_options;

    for (i = 0; i < dest->num_options; i++)
    {
        if (!pcupsGetOption( dest->options[i].name, num_options, *options ))
            num_options = pcupsAddOption( dest->options[i].name, dest->options[i].value,
                                          num_options, options );
    }

    pcupsFreeDests( 1, dest );
    return num_options;
}
#endif

/*****************************************************************************
 *          schedule_cups
 */
static BOOL schedule_cups(LPCWSTR printer_name, LPCWSTR filename, LPCWSTR document_title)
{
#ifdef SONAME_LIBCUPS
    if(pcupsPrintFile)
    {
        char *unixname, *queue, *unix_doc_title;
        DWORD len;
        BOOL ret;
        int num_options = 0, i;
        cups_option_t *options = NULL;

        if(!(unixname = wine_get_unix_file_name(filename)))
            return FALSE;

        len = WideCharToMultiByte(CP_UNIXCP, 0, printer_name, -1, NULL, 0, NULL, NULL);
        queue = HeapAlloc(GetProcessHeap(), 0, len);
        WideCharToMultiByte(CP_UNIXCP, 0, printer_name, -1, queue, len, NULL, NULL);

        len = WideCharToMultiByte(CP_UNIXCP, 0, document_title, -1, NULL, 0, NULL, NULL);
        unix_doc_title = HeapAlloc(GetProcessHeap(), 0, len);
        WideCharToMultiByte(CP_UNIXCP, 0, document_title, -1, unix_doc_title, len, NULL, NULL);

        num_options = get_cups_job_ticket_options( unixname, num_options, &options );
        num_options = get_cups_default_options( queue, num_options, &options );

        TRACE( "printing via cups with options:\n" );
        for (i = 0; i < num_options; i++)
            TRACE( "\t%d: %s = %s\n", i, options[i].name, options[i].value );

        ret = pcupsPrintFile( queue, unixname, unix_doc_title, num_options, options );

        pcupsFreeOptions( num_options, options );

        HeapFree(GetProcessHeap(), 0, unix_doc_title);
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

static INT_PTR CALLBACK file_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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
        BOOL r;
        TRACE("copy to %s\n", debugstr_w(output));
        r = CopyFileW(filename, output, FALSE);
        HeapFree(GetProcessHeap(), 0, output);
        return r;
    }
    return FALSE;
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

    len = WideCharToMultiByte(CP_UNIXCP, 0, output, -1, NULL, 0, NULL, NULL);
    outputA = HeapAlloc(GetProcessHeap(), 0, len);
    WideCharToMultiByte(CP_UNIXCP, 0, output, -1, outputA, len, NULL, NULL);
    
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
            PRINTER_INFO_5W *pi5 = NULL;
            LPWSTR portname = job->portname;
            DWORD needed;
            HKEY hkey;
            WCHAR output[1024];
            static const WCHAR spooler_key[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\',
                                                'P','r','i','n','t','i','n','g','\\','S','p','o','o','l','e','r',0};

            if (!portname)
            {
                GetPrinterW(hPrinter, 5, NULL, 0, &needed);
                pi5 = HeapAlloc(GetProcessHeap(), 0, needed);
                GetPrinterW(hPrinter, 5, (LPBYTE)pi5, needed, &needed);
                portname = pi5->pPortName;
            }
            TRACE("need to schedule job %d filename %s to port %s\n", job->job_id, debugstr_w(job->filename),
                  debugstr_w(portname));
            
            output[0] = 0;

            /* @@ Wine registry key: HKCU\Software\Wine\Printing\Spooler */
            if(RegOpenKeyW(HKEY_CURRENT_USER, spooler_key, &hkey) == ERROR_SUCCESS)
            {
                DWORD type, count = sizeof(output);
                RegQueryValueExW(hkey, portname, NULL, &type, (LPBYTE)output, &count);
                RegCloseKey(hkey);
            }
            if(output[0] == '|')
            {
                ret = schedule_pipe(output + 1, job->filename);
            }
            else if(output[0])
            {
                ret = schedule_unixfile(output, job->filename);
            }
            else if(!strncmpW(portname, LPR_Port, strlenW(LPR_Port)))
            {
                ret = schedule_lpr(portname + strlenW(LPR_Port), job->filename);
            }
            else if(!strncmpW(portname, CUPS_Port, strlenW(CUPS_Port)))
            {
                ret = schedule_cups(portname + strlenW(CUPS_Port), job->filename, job->document_title);
            }
            else if(!strncmpW(portname, FILE_Port, strlenW(FILE_Port)))
            {
                ret = schedule_file(job->filename);
            }
            else
            {
                FIXME("can't schedule to port %s\n", debugstr_w(portname));
            }
            HeapFree(GetProcessHeap(), 0, pi5);
            CloseHandle(hf);
            DeleteFileW(job->filename);
        }
        list_remove(cursor);
        HeapFree(GetProcessHeap(), 0, job->document_title);
        HeapFree(GetProcessHeap(), 0, job->printer_name);
        HeapFree(GetProcessHeap(), 0, job->portname);
        HeapFree(GetProcessHeap(), 0, job->filename);
        HeapFree(GetProcessHeap(), 0, job->devmode);
        HeapFree(GetProcessHeap(), 0, job);
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

/*****************************************************************************
 *          UploadPrinterDriverPackageA [WINSPOOL.@]
 */
HRESULT WINAPI UploadPrinterDriverPackageA( LPCSTR server, LPCSTR path, LPCSTR env,
                                            DWORD flags, HWND hwnd, LPSTR dst, PULONG dstlen )
{
    FIXME("%s, %s, %s, %x, %p, %p, %p\n", debugstr_a(server), debugstr_a(path), debugstr_a(env),
          flags, hwnd, dst, dstlen);
    return E_NOTIMPL;
}

/*****************************************************************************
 *          UploadPrinterDriverPackageW [WINSPOOL.@]
 */
HRESULT WINAPI UploadPrinterDriverPackageW( LPCWSTR server, LPCWSTR path, LPCWSTR env,
                                            DWORD flags, HWND hwnd, LPWSTR dst, PULONG dstlen )
{
    FIXME("%s, %s, %s, %x, %p, %p, %p\n", debugstr_w(server), debugstr_w(path), debugstr_w(env),
          flags, hwnd, dst, dstlen);
    return E_NOTIMPL;
}
