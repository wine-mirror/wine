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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "winspool.h"
#include "winternl.h"
#include "winnls.h"
#include "wine/windef16.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wine/rbtree.h"
#include "wine/heap.h"
#include <wine/unixlib.h>

#include "ddk/compstui.h"
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
    LPWSTR name;
    HANDLE backend_printer;
} opened_printer_t;

typedef struct {
    LPCWSTR  envname;
    LPCWSTR  subdir;
    DWORD    driverversion;
    LPCWSTR  versionregpath;
    LPCWSTR  versionsubdir;
} printenv_t;

typedef struct
{
    struct wine_rb_entry entry;
    HMODULE module;
    LONG ref;

    /* entry points */
    DWORD (WINAPI *pDrvDeviceCapabilities)(HANDLE, const WCHAR *, WORD, void *, const DEVMODEW *);
    LONG  (WINAPI *pDrvDocumentPropertySheets)(PROPSHEETUI_INFO*, LPARAM);

    WCHAR name[1];
} config_module_t;

typedef struct {
    WORD cbSize;
    WORD Reserved;
    HANDLE hPrinter;
    LPCWSTR pszPrinterName;
    PDEVMODEW pdmIn;
    PDEVMODEW pdmOut;
    DWORD cbOut;
    DWORD fMode;
} DOCUMENTPROPERTYHEADERW;

typedef struct {
    DOCUMENTPROPERTYHEADERW dph;
    config_module_t *config;
} document_property_t;

/* ############################### */

static opened_printer_t **printer_handles;
static UINT nb_printer_handles;

static const WCHAR * const May_Delete_Value = L"WineMayDeleteMe";

static const DWORD di_sizeof[] = {0, sizeof(DRIVER_INFO_1W), sizeof(DRIVER_INFO_2W),
                                     sizeof(DRIVER_INFO_3W), sizeof(DRIVER_INFO_4W),
                                     sizeof(DRIVER_INFO_5W), sizeof(DRIVER_INFO_6W),
                                  0, sizeof(DRIVER_INFO_8W)};


static const DWORD pi_sizeof[] = {0, sizeof(PRINTER_INFO_1W), sizeof(PRINTER_INFO_2W),
                                     sizeof(PRINTER_INFO_3),  sizeof(PRINTER_INFO_4W),
                                     sizeof(PRINTER_INFO_5W), sizeof(PRINTER_INFO_6),
                                     sizeof(PRINTER_INFO_7W), sizeof(PRINTER_INFO_8W),
                                     sizeof(PRINTER_INFO_9W)};

static const printenv_t env_x64 = { L"Windows x64", L"x64", 3, L"\\Version-3", L"\\3" };
static const printenv_t env_x86 = { L"Windows NT x86", L"w32x86", 3, L"\\Version-3", L"\\3" };
static const printenv_t env_arm = { L"Windows ARM", L"arm", 3, L"\\Version-3", L"\\3" };
static const printenv_t env_arm64 = { L"Windows ARM64", L"arm64", 3, L"\\Version-3", L"\\3" };
static const printenv_t env_win40 = { L"Windows 4.0", L"win40", 0, L"\\Version-0", L"\\0" };

static const printenv_t * const all_printenv[] = { &env_x86, &env_x64, &env_arm, &env_arm64, &env_win40 };

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
 *  SetLastError(ERROR_INVALID_ENVIRONMENT) is called on Failure
 *
 */

static const  printenv_t * validate_envW(LPCWSTR env)
{
    const printenv_t *result = NULL;
    unsigned int i;

    TRACE("testing %s\n", debugstr_w(env));
    if (env && env[0])
    {
        for (i = 0; i < ARRAY_SIZE(all_printenv); i++)
        {
            if (!wcsicmp( env, all_printenv[i]->envname ))
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

/* stringWtoA
 * Converts Unicode string to Ansi (possibly in place).
 */
static void stringWtoA( const WCHAR *strW, char *strA, size_t size )
{
    DWORD ret;
    char *str;

    str = (char *)strW == strA ? malloc( size ) : strA;
    ret = WideCharToMultiByte( CP_ACP, 0, strW, -1, str, size, NULL, NULL );
    if (str != strA)
    {
        memcpy( strA, str, ret );
        free( str );
    }
    memset( strA + ret, 0, size - ret );
}

/***********************************************************
 * DEVMODEWtoA
 * Converts DEVMODEW to DEVMODEA (possibly in place).
 * Allocates DEVMODEA if needed.
 */
static DEVMODEA *DEVMODEWtoA( const DEVMODEW *dmW, DEVMODEA *dmA )
{
    DWORD size, sizeW;

    if (!dmW) return NULL;
    sizeW = dmW->dmSize;
    size = sizeW - CCHDEVICENAME -
                        ((sizeW > FIELD_OFFSET( DEVMODEW, dmFormName )) ? CCHFORMNAME : 0);

    if (!dmA)
        dmA = calloc( 1, size + dmW->dmDriverExtra );
    if (!dmA) return NULL;

    stringWtoA( dmW->dmDeviceName, (char *)dmA->dmDeviceName, CCHDEVICENAME );

    if (FIELD_OFFSET( DEVMODEW, dmFormName ) >= sizeW)
    {
        memmove( &dmA->dmSpecVersion, &dmW->dmSpecVersion,
                sizeW - FIELD_OFFSET( DEVMODEW, dmSpecVersion ) );
    }
    else
    {
        memmove( &dmA->dmSpecVersion, &dmW->dmSpecVersion,
                FIELD_OFFSET( DEVMODEW, dmFormName ) - FIELD_OFFSET( DEVMODEW, dmSpecVersion ) );
        stringWtoA( dmW->dmFormName, (char *)dmA->dmFormName, CCHFORMNAME );
        memmove( &dmA->dmLogPixels, &dmW->dmLogPixels, sizeW - FIELD_OFFSET( DEVMODEW, dmLogPixels ) );
    }

    dmA->dmSize = size;
    memmove( (char *)dmA + dmA->dmSize, (const char *)dmW + sizeW, dmW->dmDriverExtra );
    return dmA;
}

/*********************************************************************
 *                 packed_struct_WtoA
 *
 * Convert a packed struct from W to A overwriting the unicode strings
 * with their ansi equivalents.
 */
static void packed_struct_WtoA( BYTE *data, const DWORD *string_info )
{
    WCHAR *strW;

    string_info++; /* sizeof */
    while (*string_info != ~0u)
    {
        strW = *(WCHAR **)(data + *string_info);
        if (strW) stringWtoA( strW, (char *)strW, (wcslen(strW) + 1) * sizeof(WCHAR) );
        string_info++;
    }
}

static inline const DWORD *form_string_info( DWORD level )
{
    static const DWORD info_1[] =
    {
        sizeof( FORM_INFO_1W ),
        FIELD_OFFSET( FORM_INFO_1W, pName ),
        ~0u
    };
    static const DWORD info_2[] =
    {
        sizeof( FORM_INFO_2W ),
        FIELD_OFFSET( FORM_INFO_2W, pName ),
        FIELD_OFFSET( FORM_INFO_2W, pMuiDll ),
        FIELD_OFFSET( FORM_INFO_2W, pDisplayName ),
        ~0u
    };

    if (level == 1) return info_1;
    if (level == 2) return info_2;

    SetLastError( ERROR_INVALID_LEVEL );
    return NULL;
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
static HKEY WINSPOOL_OpenDriverReg(const void *pEnvironment)
{
    HKEY retval = NULL;
    LPWSTR buffer;
    const printenv_t *env;
    unsigned int len;
    static const WCHAR driver_fmt[] = L"System\\CurrentControlSet\\control\\Print\\Environments\\%s\\Drivers%s";

    TRACE("(%s)\n", debugstr_w(pEnvironment));

    env = validate_envW(pEnvironment);
    if (!env) return NULL;

    len = ARRAY_SIZE( driver_fmt ) + wcslen( env->envname ) + wcslen( env->versionregpath );
    buffer = malloc( len * sizeof(WCHAR) );
    if (buffer)
    {
        swprintf( buffer, len, driver_fmt, env->envname, env->versionregpath );
        RegCreateKeyW( HKEY_LOCAL_MACHINE, buffer, &retval );
        free( buffer );
    }
    return retval;
}

enum printers_key
{
    system_printers_key,
    user_printers_key,
    user_ports_key,
    user_default_key,
};

static DWORD create_printers_reg_key( enum printers_key type, HKEY *key )
{
    switch( type )
    {
    case system_printers_key:
        return RegCreateKeyW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Print\\Printers", key );
    case user_printers_key:
        return RegCreateKeyW( HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices", key );
    case user_ports_key:
        return RegCreateKeyW( HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\PrinterPorts", key );
    case user_default_key:
        return RegCreateKeyW( HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows", key );
    }
    return ERROR_PATH_NOT_FOUND;
}

static CRITICAL_SECTION config_modules_cs;
static CRITICAL_SECTION_DEBUG config_modules_cs_debug =
{
    0, 0, &config_modules_cs,
    { &config_modules_cs_debug.ProcessLocksList, &config_modules_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": config_modules_cs") }
};
static CRITICAL_SECTION config_modules_cs = { &config_modules_cs_debug, -1, 0, 0, 0, 0 };

static int compare_config_modules(const void *key, const struct wine_rb_entry *entry)
{
    config_module_t *module = WINE_RB_ENTRY_VALUE(entry, config_module_t, entry);
    return wcsicmp( key, module->name );
}

static struct wine_rb_tree config_modules = { compare_config_modules };

static void release_config_module(config_module_t *config_module)
{
    if (InterlockedDecrement(&config_module->ref)) return;
    FreeLibrary(config_module->module);
    free(config_module);
}

static config_module_t *get_config_module(const WCHAR *device, BOOL grab)
{
    WCHAR driver_name[100], driver[MAX_PATH];
    DWORD size, len;
    HKEY printers_key, printer_key, drivers_key, driver_key;
    HMODULE driver_module;
    config_module_t *ret = NULL;
    struct wine_rb_entry *entry;
    DWORD r, type;
    LSTATUS res;

    EnterCriticalSection(&config_modules_cs);
    entry = wine_rb_get(&config_modules, device);
    if (entry) {
        ret = WINE_RB_ENTRY_VALUE(entry, config_module_t, entry);
        if (grab) InterlockedIncrement(&ret->ref);
        goto ret;
    }
    if (!grab) goto ret;

    if (create_printers_reg_key(system_printers_key, &printers_key))
    {
        ERR("Can't create Printers key\n");
        goto ret;
    }

    r = RegOpenKeyW(printers_key, device, &printer_key);
    RegCloseKey(printers_key);
    if (r)
    {
        WARN("Device %s key not found\n", debugstr_w(device));
        goto ret;
    }

    size = sizeof(driver_name);
    r = RegQueryValueExW(printer_key, L"Printer Driver", 0, &type, (BYTE *)driver_name, &size);
    RegCloseKey(printer_key);
    if (r || type != REG_SZ)
    {
        WARN("Can't get Printer Driver name\n");
        goto ret;
    }

    if (!(drivers_key = WINSPOOL_OpenDriverReg(NULL))) goto ret;

    res = RegOpenKeyW(drivers_key, driver_name, &driver_key);
    RegCloseKey(drivers_key);
    if (res) {
        WARN("Driver %s key not found\n", debugstr_w(driver_name));
        goto ret;
    }

    size = sizeof(driver);
    if (!GetPrinterDriverDirectoryW(NULL, NULL, 1, (LPBYTE)driver, size, &size)) goto ret;

    len = size / sizeof(WCHAR) - 1;
    driver[len++] = '\\';
    driver[len++] = '3';
    driver[len++] = '\\';
    size = sizeof(driver) - len * sizeof(WCHAR);
    res = RegQueryValueExW( driver_key, L"Configuration File", NULL, &type,
                            (BYTE *)(driver + len), &size );
    RegCloseKey(driver_key);
    if (res || type != REG_SZ) {
        WARN("no configuration file: %lu\n", res);
        goto ret;
    }

    if (!(driver_module = LoadLibraryW(driver))) {
        WARN("Could not load %s\n", debugstr_w(driver));
        goto ret;
    }

    len = wcslen( device );
    if (!(ret = malloc(FIELD_OFFSET(config_module_t, name[len + 1]))))
        goto ret;

    ret->ref = 2; /* one for config_module and one for the caller */
    ret->module = driver_module;
    ret->pDrvDeviceCapabilities = (void *)GetProcAddress(driver_module, "DrvDeviceCapabilities");
    ret->pDrvDocumentPropertySheets = (void *)GetProcAddress(driver_module, "DrvDocumentPropertySheets");
    wcscpy( ret->name, device );

    wine_rb_put(&config_modules, ret->name, &ret->entry);
ret:
    LeaveCriticalSection(&config_modules_cs);
    return ret;
}

/* ################################ */

static int multi_sz_lenA(const char *str)
{
    const char *ptr = str;
    if(!str) return 0;
    do
    {
        ptr += strlen( ptr ) + 1;
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
        WARN( "Got ret = %ld on name %s\n", ret, debugstr_w(name) );
        return 0;
    }
    if (type != REG_DWORD)
    {
        ERR( "Got type %ld\n", type );
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

static HANDLE get_backend_handle( HANDLE hprn )
{
    opened_printer_t *printer = get_opened_printer( hprn );
    if (!printer) return NULL;
    return printer->backend_printer;
}

static DWORD open_printer_reg_key( const WCHAR *name, HKEY *key )
{
    HKEY printers;
    DWORD err;

    *key = NULL;
    err = create_printers_reg_key( system_printers_key, &printers );
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

static BOOL get_internal_fallback_ppd( const WCHAR *ppd )
{
    char *ptr, *end;
    DWORD size, written;
    HANDLE file;
    BOOL ret;
    HRSRC res = FindResourceW( WINSPOOL_hInstance, MAKEINTRESOURCEW(1), L"PPDFILE" );

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

static WCHAR *get_ppd_filename( const WCHAR *dir, const WCHAR *file_name )
{
    static const WCHAR dot_ppd[] = L".ppd";
    static const WCHAR invalid_chars[] = L"*?<>|\"/\\";
    int dir_len = wcslen( dir ), file_len = wcslen( file_name );
    int len = (dir_len + file_len + ARRAY_SIZE( dot_ppd )) * sizeof(WCHAR);
    WCHAR *ppd = malloc( len ), *p;

    if (!ppd) return NULL;
    memcpy( ppd, dir, dir_len * sizeof(WCHAR) );
    memcpy( ppd + dir_len, file_name, file_len * sizeof(WCHAR) );
    memcpy( ppd + dir_len + file_len, dot_ppd, sizeof(dot_ppd) );

    p = ppd + dir_len;
    while ((p = wcspbrk( p, invalid_chars ))) *p++ = '_';

    return ppd;
}

static BOOL add_printer_driver( const WCHAR *name, const WCHAR *ppd_dir )
{
    WCHAR *ppd = get_ppd_filename( ppd_dir, name );
    struct get_ppd_params ppd_params;
    UNICODE_STRING nt_ppd = { .Buffer = NULL };
    DRIVER_INFO_3W di3;
    unsigned int i;
    BOOL res = FALSE;
    WCHAR raw[] = L"RAW", driver_nt[] = L"wineps.drv", driver_9x[] = L"wineps16.drv";

    if (!ppd) return FALSE;
    if (!RtlDosPathNameToNtPathName_U( ppd, &nt_ppd, NULL, NULL )) goto end;

    ppd_params.printer = name;
    ppd_params.ppd = nt_ppd.Buffer;
    res = !UNIX_CALL( get_ppd, &ppd_params ) || get_internal_fallback_ppd( ppd );
    if (!res) goto end;

    AddPrintProcessorW(NULL, NULL, driver_nt, (WCHAR *)L"wineps");

    memset( &di3, 0, sizeof(DRIVER_INFO_3W) );
    di3.cVersion         = 3;
    di3.pName            = (WCHAR *)name;
    di3.pDriverPath      = driver_nt;
    di3.pDataFile        = ppd;
    di3.pConfigFile      = driver_nt;
    di3.pDefaultDataType = raw;

    for (i = 0; i < ARRAY_SIZE(all_printenv); i++)
    {
        di3.pEnvironment = (WCHAR *)all_printenv[i]->envname;
        if (all_printenv[i]->driverversion == 0)
        {
            /* We use wineps16.drv as driver for 16 bit */
            di3.pDriverPath = driver_9x;
            di3.pConfigFile = driver_9x;
        }
        res = AddPrinterDriverExW( NULL, 3, (BYTE *)&di3, APD_COPY_NEW_FILES | APD_COPY_FROM_DIRECTORY );
        TRACE( "got %d and %ld for %s (%s)\n", res, GetLastError(), debugstr_w( name ), debugstr_w( di3.pEnvironment ) );

        if (!res && (GetLastError() != ERROR_PRINTER_DRIVER_ALREADY_INSTALLED))
        {
            ERR( "failed with %lu for %s (%s) %s\n", GetLastError(), debugstr_w( name ),
                 debugstr_w( di3.pEnvironment ), debugstr_w( di3.pDriverPath ) );
            break;
        }
        res = TRUE;
    }
    ppd_params.printer = NULL; /* unlink the ppd */
    UNIX_CALL( get_ppd, &ppd_params );

end:
    RtlFreeUnicodeString( &nt_ppd );
    free( ppd );
    return res;
}

static WCHAR *get_ppd_dir( void )
{
    static const WCHAR wine_ppds[] = L"wine_ppds\\";
    DWORD len;
    WCHAR *dir, tmp_path[MAX_PATH];
    BOOL res;

    len = GetTempPathW( ARRAY_SIZE( tmp_path ), tmp_path );
    if (!len) return NULL;
    dir = malloc( len * sizeof(WCHAR) + sizeof(wine_ppds) ) ;
    if (!dir) return NULL;

    memcpy( dir, tmp_path, len * sizeof(WCHAR) );
    memcpy( dir + len, wine_ppds, sizeof(wine_ppds) );
    res = CreateDirectoryW( dir, NULL );
    if (!res && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        free( dir );
        dir = NULL;
    }
    TRACE( "ppd temporary dir: %s\n", debugstr_w(dir) );
    return dir;
}

static BOOL init_unix_printers( void )
{
    WCHAR *port, *ppd_dir = NULL, *default_printer = NULL;
    unsigned int size, num;
    struct enum_printers_params enum_params = { NULL, &size, &num };
    HKEY printer_key, printers_key;
    HANDLE added_printer;
    PRINTER_INFO_2W pi2;
    NTSTATUS status;
    WCHAR raw[] = L"RAW", wineps[] = L"wineps", empty[] = L"";
    int i;

    if (create_printers_reg_key( system_printers_key, &printers_key ))
    {
        ERR( "Can't create Printers key\n" );
        return FALSE;
    }

    size = 10000;
    do
    {
        size *= 2;
        free( enum_params.printers );
        enum_params.printers = malloc( size );
        status = UNIX_CALL( enum_printers, &enum_params );
    } while (status == STATUS_BUFFER_OVERFLOW);
    if (status) goto end;

    TRACE( "Found %d CUPS %s:\n", num, (num == 1) ? "printer" : "printers" );
    for (i = 0; i < num; i++)
    {
        struct printer_info *printer = enum_params.printers + i;

        if (RegOpenKeyW( printers_key, printer->name, &printer_key ) == ERROR_SUCCESS)
        {
            DWORD status = get_dword_from_reg( printer_key, L"Status" );
            /* Printer already in registry, delete the tag added in WINSPOOL_LoadSystemPrinters
               and continue */
            TRACE("Printer already exists\n");
            RegDeleteValueW( printer_key, May_Delete_Value );
            /* flag that the PPD file should be checked for an update */
            set_reg_DWORD( printer_key, L"Status", status | PRINTER_STATUS_DRIVER_UPDATE_NEEDED );
            RegSetValueExW( printer_key, L"Print Processor", 0, REG_SZ, (const BYTE*)wineps,
                    (wcslen( wineps ) + 1) * sizeof(WCHAR));
            RegCloseKey( printer_key );
        }
        else
        {
            if (!ppd_dir && !(ppd_dir = get_ppd_dir())) break;
            if (!add_printer_driver( printer->name, ppd_dir )) continue;

            port = malloc( sizeof(L"CUPS:") + wcslen( printer->name ) * sizeof(WCHAR) );
            wcscpy( port, L"CUPS:" );
            wcscat( port, printer->name );

            memset( &pi2, 0, sizeof(PRINTER_INFO_2W) );
            pi2.pPrinterName    = printer->name;
            pi2.pDatatype       = raw;
            pi2.pPrintProcessor = wineps;
            pi2.pDriverName     = printer->name;
            pi2.pComment        = printer->comment;
            pi2.pLocation       = printer->location;
            pi2.pPortName       = port;
            pi2.pParameters     = empty;
            pi2.pShareName      = empty;
            pi2.pSepFile        = empty;

            added_printer = AddPrinterW( NULL, 2, (BYTE *)&pi2 );
            if (added_printer) ClosePrinter( added_printer );
            else if (GetLastError() != ERROR_PRINTER_ALREADY_EXISTS)
                ERR( "printer '%s' not added by AddPrinter (error %ld)\n", debugstr_w( printer->name ), GetLastError() );

            free( port );
        }
        if (printer->is_default) default_printer = printer->name;
    }

    if (!default_printer && num) default_printer = enum_params.printers[0].name;
    if (default_printer) SetDefaultPrinterW( default_printer );

    if (ppd_dir)
    {
        RemoveDirectoryW( ppd_dir );
        free( ppd_dir );
    }
end:
    free( enum_params.printers );
    RegCloseKey( printers_key );
    return TRUE;
}

static void set_ppd_overrides( HANDLE printer )
{
    WCHAR buffer[256];
    unsigned int name_size = sizeof(buffer);
    struct get_default_page_size_params params = { .name = buffer, .name_size = &name_size };
    NTSTATUS status;

    while (1)
    {
        status = UNIX_CALL( get_default_page_size, &params );
        if (status != STATUS_BUFFER_OVERFLOW) break;
        if (params.name != buffer) free( params.name );
        params.name = malloc( name_size );
        if (!params.name) break;
    }
    if (!status) SetPrinterDataExW( printer, L"PPD Overrides", L"DefaultPageSize", REG_SZ, (BYTE*)params.name, name_size );
    if (params.name != buffer) free( params.name );
}

static BOOL update_driver( HANDLE printer )
{
    BOOL ret;
    const WCHAR *name = get_opened_printer_name( printer );
    WCHAR *ppd_dir;

    if (!name) return FALSE;
    if (!(ppd_dir = get_ppd_dir())) return FALSE;

    TRACE( "updating driver %s\n", debugstr_w( name ) );
    ret = add_printer_driver( name, ppd_dir );

    free( ppd_dir );

    set_ppd_overrides( printer );

    /* call into the driver to update the devmode */
    DocumentPropertiesW( 0, printer, NULL, NULL, NULL, 0 );

    return ret;
}

static inline DWORD set_reg_szW(HKEY hkey, const WCHAR *keyname, const WCHAR *value)
{
    if (value)
        return RegSetValueExW(hkey, keyname, 0, REG_SZ, (const BYTE*)value,
                              (wcslen( value ) + 1) * sizeof(WCHAR));
    else
        return ERROR_FILE_NOT_FOUND;
}

static inline DWORD set_reg_devmode( HKEY key, const WCHAR *name, const DEVMODEW *dm )
{
    DEVMODEA *dmA = DEVMODEWtoA( dm, NULL );
    DWORD ret = ERROR_FILE_NOT_FOUND;

    /* FIXME: Write DEVMODEA not DEVMODEW into reg.  This is what win9x does
       and we support these drivers.  NT writes DEVMODEW so somehow
       we'll need to distinguish between these when we support NT
       drivers */

    if (dmA)
    {
        ret = RegSetValueExW( key, name, 0, REG_BINARY,
                              (LPBYTE)dmA, dmA->dmSize + dmA->dmDriverExtra );
        free( dmA );
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

    server = wcsdup(&name[2]);     /* skip over both backslashes */
    if (server == NULL) return NULL;

    /* strip '\' and the printername */
    ptr = wcschr( server, '\\' );
    if (ptr) ptr[0] = '\0';

    TRACE("found %s\n", debugstr_w(server));

    len = ARRAY_SIZE(buffer);
    if (GetComputerNameW(buffer, &len))
    {
        if (!wcscmp( buffer, server ))
        {
            /* The requested Servername is our computername */
            free(server);
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
    if ((name[0] == '\\') && (name[1] == '\\'))
    {
        /* skip over the servername and search for the following '\'  */
        name = wcschr( &name[2], '\\' );
        if ((name) && (name[1]))
        {
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
    free( printer->name );
    free( printer );
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
    opened_printer_t *printer = NULL;
    LPWSTR  servername;
    LPCWSTR printername;
    UINT_PTR handle;

    if ((backend == NULL)  && !load_backend()) return NULL;

    servername = get_servername_from_name(name);
    if (servername) {
        FIXME("server %s not supported\n", debugstr_w(servername));
        free(servername);
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

    for (handle = 0; handle < nb_printer_handles; handle++)
    {
        if (!printer_handles[handle])
            break;
    }

    if (handle >= nb_printer_handles)
    {
        opened_printer_t **new_array;
        if (printer_handles)
        {
            new_array = realloc(printer_handles, (nb_printer_handles + 16) * sizeof(*new_array));
            memset(new_array + nb_printer_handles, 0, 16 * sizeof(*new_array));
        }
        else
        {
            new_array = calloc(nb_printer_handles + 16, sizeof(*new_array));
        }

        if (!new_array)
        {
            handle = 0;
            goto end;
        }
        printer_handles = new_array;
        nb_printer_handles += 16;
    }

    if (!(printer = calloc(1, sizeof(*printer))))
    {
        handle = 0;
        goto end;
    }

    /* get a printer handle from the backend */
    if (! backend->fpOpenPrinter(name, &printer->backend_printer, pDefault)) {
        handle = 0;
        goto end;
    }

    /* clone the full name */
    printer->name = wcsdup(name);
    if (name && (!printer->name)) {
        handle = 0;
        goto end;
    }

    printer_handles[handle] = printer;
    handle++;
end:
    LeaveCriticalSection(&printer_handles_cs);
    if (!handle && printer)
        free_printer_entry( printer );

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

    pi = malloc( needed );
    EnumPrintersW( PRINTER_ENUM_LOCAL, NULL, 5, (LPBYTE)pi, needed, &needed, &num );
    for (i = 0; i < num; i++)
    {
        if (!pi[i].pPortName) continue;

        if (wcsncmp( pi[i].pPortName, L"CUPS:", ARRAY_SIZE(L"CUPS:") - 1 ) &&
            wcsncmp( pi[i].pPortName, L"LPR:", ARRAY_SIZE(L"LPR:") - 1 ))
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
    free(pi);
}

static HANDLE init_mutex;

void WINSPOOL_LoadSystemPrinters(void)
{
    HKEY printers_key, printer_key;
    DWORD               needed, num, i;
    WCHAR               PrinterName[256];

    /* FIXME: The init code should be moved to spoolsv.exe */
    init_mutex = CreateMutexW( NULL, TRUE, L"__WINE_WINSPOOL_MUTEX__" );
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
    if (!create_printers_reg_key( system_printers_key, &printers_key ))
    {
        if (!RegQueryInfoKeyW( printers_key, NULL, NULL, NULL, &num, NULL, NULL, NULL, NULL, NULL, NULL, NULL ))
            for (i = 0; i < num; i++)
                if (!RegEnumKeyW( printers_key, i, PrinterName, ARRAY_SIZE(PrinterName) ))
                    if (!RegOpenKeyW( printers_key, PrinterName, &printer_key ))
                    {
                        if (RegQueryValueExW( printer_key, L"Name", 0, 0, 0, &needed ) == ERROR_FILE_NOT_FOUND)
                            set_reg_szW( printer_key, L"Name", PrinterName );
                        RegCloseKey( printer_key );
                    }
        RegCloseKey( printers_key );
    }

    old_printer_check( FALSE );
    init_unix_printers();
    old_printer_check( TRUE );

    ReleaseMutex( init_mutex );
    return;
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

    TRACE("(%p, %p, %ld, %lu, %lu)\n", out, pPrintersW, level, outlen, numentries);

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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(piW->pName));
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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(piW->pPrinterName));
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

                    if (piW->pDevMode)
                    {
                        /* align DEVMODEA to a DWORD boundary */
                        len = (4 - ( (DWORD_PTR) ptr & 3)) & 3;
                        ptr += len;
                        outlen -= len;

                        piA->pDevMode = (LPDEVMODEA) ptr;
                        DEVMODEWtoA(piW->pDevMode, piA->pDevMode);
                        len = piA->pDevMode->dmSize + piA->pDevMode->dmDriverExtra;

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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(piW->pPrinterName));

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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(piW->pPrinterName));

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

                    TRACE("(%lu) #%lu\n", level, id);
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

                    TRACE("(%lu) #%lu\n", level, id);
                    if (piW->pDevMode)
                    {
                        /* align DEVMODEA to a DWORD boundary */
                        len = (4 - ( (DWORD_PTR) ptr & 3)) & 3;
                        ptr += len;
                        outlen -= len;

                        piA->pDevMode = (LPDEVMODEA) ptr;
                        DEVMODEWtoA(piW->pDevMode, piA->pDevMode);
                        len = piA->pDevMode->dmSize + piA->pDevMode->dmDriverExtra;

                        ptr += len;
                        outlen -= len;
                    }
                    break;
                }

            default:
                FIXME("for level %lu\n", level);
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

    TRACE("(%p, %p, %ld, %lu, %lu)\n", out, pDriversW, level, outlen, numentries);

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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(diW->pName));

                    COPY_STRING(pName);
                    break;
                }
            case 2:
                {
                    DRIVER_INFO_2W * diW = (DRIVER_INFO_2W *) pDriversW;
                    DRIVER_INFO_2A * diA = (DRIVER_INFO_2A *) out;

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(diW->pName));

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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(diW->pName));

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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(diW->pName));

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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(diW->pName));

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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(diW->pName));

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

                    TRACE("(%lu) #%lu: %s\n", level, id, debugstr_w(diW->pName));

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
                FIXME("for level %lu\n", level);
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

    ret = malloc( pi_sizeof[level] );
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
        FIXME( "Unhandled level %ld\n", level );
        free( ret );
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

        free( piW->pServerName );
        free( piW->pPrinterName );
        free( piW->pShareName );
        free( piW->pPortName );
        free( piW->pDriverName );
        free( piW->pComment );
        free( piW->pLocation );
        heap_free( piW->pDevMode );
        free( piW->pSepFile );
        free( piW->pPrintProcessor );
        free( piW->pDatatype );
        free( piW->pParameters );
        break;
    }

    case 8:
    case 9:
    {
        PRINTER_INFO_9W *piW = (PRINTER_INFO_9W *)data;

        heap_free( piW->pDevMode );
        break;
    }

    default:
        FIXME( "Unhandled level %ld\n", level );
    }

    free( data );
    return;
}

/******************************************************************
 *              DeviceCapabilities     [WINSPOOL.@]
 *              DeviceCapabilitiesA    [WINSPOOL.@]
 *
 */
INT WINAPI DeviceCapabilitiesA(const char *device, const char *portA, WORD cap,
                               char *output, DEVMODEA *devmodeA)
{
    WCHAR *device_name = NULL, *port = NULL;
    DEVMODEW *devmode = NULL;
    DWORD ret, len;

    len = MultiByteToWideChar(CP_ACP, 0, device, -1, NULL, 0);
    if (len) {
        device_name = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, device, -1, device_name, len);
    }

    len = MultiByteToWideChar(CP_ACP, 0, portA, -1, NULL, 0);
    if (len) {
        port = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, portA, -1, port, len);
    }

    if (devmodeA) devmode = GdiConvertToDevmodeW( devmodeA );

    if (output && (cap == DC_BINNAMES || cap == DC_FILEDEPENDENCIES || cap == DC_PAPERNAMES)) {
        /* These need A -> W translation */
        unsigned int size = 0, i;
        WCHAR *outputW;

        ret = DeviceCapabilitiesW(device_name, port, cap, NULL, devmode);
        if (ret == -1) goto cleanup;

        switch (cap) {
        case DC_BINNAMES:
            size = 24;
            break;
        case DC_PAPERNAMES:
        case DC_FILEDEPENDENCIES:
            size = 64;
            break;
        }
        outputW = malloc(size * ret * sizeof(WCHAR));
        ret = DeviceCapabilitiesW(device_name, port, cap, outputW, devmode);
        for (i = 0; i < ret; i++)
            WideCharToMultiByte(CP_ACP, 0, outputW + (i * size), -1,
                                output + (i * size), size, NULL, NULL);
        free(outputW);
    } else {
        ret = DeviceCapabilitiesW(device_name, port, cap, (WCHAR *)output, devmode);
    }
cleanup:
    free(device_name);
    heap_free(devmode);
    free(port);
    return ret;
}

/*****************************************************************************
 *          DeviceCapabilitiesW        [WINSPOOL.@]
 *
 */
INT WINAPI DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort,
			       WORD fwCapability, LPWSTR pOutput,
			       const DEVMODEW *pDevMode)
{
    config_module_t *config;
    int ret;

    TRACE("%s,%s,%u,%p,%p\n", debugstr_w(pDevice), debugstr_w(pPort), fwCapability,
          pOutput, pDevMode);

    if (!(config = get_config_module(pDevice, TRUE))) {
        WARN("Could not load config module for %s\n", debugstr_w(pDevice));
        return 0;
    }

    ret = config->pDrvDeviceCapabilities(NULL /* FIXME */, pDevice, fwCapability,
                                         pOutput, pDevMode);
    release_config_module(config);
    return ret;
}

/******************************************************************
 *              DocumentPropertiesA   [WINSPOOL.@]
 */
LONG WINAPI DocumentPropertiesA(HWND hwnd, HANDLE printer, char *device_name, DEVMODEA *output,
				DEVMODEA *input, DWORD mode)
{
    DEVMODEW *outputW = NULL, *inputW = NULL;
    WCHAR *device = NULL;
    unsigned int len;
    int ret;

    TRACE("(%p,%p,%s,%p,%p,%ld)\n", hwnd, printer, debugstr_a(device_name), output, input, mode);

    len = MultiByteToWideChar(CP_ACP, 0, device_name, -1, NULL, 0);
    if (len) {
        device = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, device_name, -1, device, len);
    }

    if (output && (mode & (DM_COPY | DM_UPDATE))) {
        ret = DocumentPropertiesW(hwnd, printer, device, NULL, NULL, 0);
        if (ret <= 0) {
            free(device);
            return -1;
        }
        outputW = malloc(ret);
    }

    if (input && (mode & DM_IN_BUFFER)) inputW = GdiConvertToDevmodeW(input);

    ret = DocumentPropertiesW(hwnd, printer, device, outputW, inputW, mode);

    if (ret >= 0 && outputW && (mode & (DM_COPY | DM_UPDATE)))
        DEVMODEWtoA( outputW, output );

    free(device);
    heap_free(inputW);
    free(outputW);

    if (!mode && ret > 0) ret -= CCHDEVICENAME + CCHFORMNAME;
    return ret;
}

static LONG WINAPI document_callback(PROPSHEETUI_INFO *info, LPARAM lparam)
{
    if (info->Reason == PROPSHEETUI_REASON_INIT)
    {
        document_property_t *dp = (document_property_t *)info->lParamInit;

        if (!info->pfnComPropSheet(info->hComPropSheet, CPSFUNC_ADD_PFNPROPSHEETUIW,
                    (LPARAM)dp->config->pDrvDocumentPropertySheets, (LPARAM)&dp->dph))
            return ERR_CPSUI_GETLASTERROR;
    }
    return CPSUI_OK;
}

/*****************************************************************************
 *          DocumentPropertiesW (WINSPOOL.@)
 */
LONG WINAPI DocumentPropertiesW(HWND hWnd, HANDLE hPrinter,
				LPWSTR pDeviceName,
				LPDEVMODEW pDevModeOutput,
				LPDEVMODEW pDevModeInput, DWORD fMode)
{
    document_property_t dp;
    const WCHAR *device;
    LONG ret;

    TRACE("(%p,%p,%s,%p,%p,%ld)\n",
          hWnd, hPrinter, debugstr_w(pDeviceName), pDevModeOutput, pDevModeInput, fMode);

    device = hPrinter ? get_opened_printer_name(hPrinter) : pDeviceName;
    if (!device) {
        ERR("no device name\n");
        return -1;
    }

    dp.dph.cbSize = sizeof(dp.dph);
    dp.dph.Reserved = 0;
    dp.dph.hPrinter = hPrinter;
    dp.dph.pszPrinterName = device;
    dp.dph.pdmIn = pDevModeInput;
    dp.dph.pdmOut = pDevModeOutput;
    dp.dph.cbOut = dp.dph.pdmOut ? dp.dph.pdmOut->dmSize : 0;
    dp.dph.fMode = fMode;
    dp.config = get_config_module(device, TRUE);
    if (!dp.config) {
        ERR("Could not load config module for %s\n", debugstr_w(device));
        return -1;
    }

    if (!(fMode & ~(DM_IN_BUFFER | DM_OUT_BUFFER | DM_OUT_DEFAULT))) {
        ret = dp.config->pDrvDocumentPropertySheets(NULL, (LPARAM)&dp.dph);

        if ((!fMode || !pDevModeOutput) && dp.dph.cbOut != ret)
            FIXME("size mismatch: ret = %ld cbOut = %ld\n", ret, dp.dph.cbOut);
    } else {
        ret = CommonPropertySheetUIW(hWnd, document_callback, (LPARAM)&dp, NULL);
    }

    release_config_module(dp.config);
    return ret;
}

/*****************************************************************************
 *          IsValidDevmodeA            [WINSPOOL.@]
 *
 * Validate a DEVMODE structure and fix errors if possible.
 *
 */
BOOL WINAPI IsValidDevmodeA(PDEVMODEA pDevMode, SIZE_T size)
{
    FIXME("(%p,%Id): stub\n", pDevMode, size);

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
BOOL WINAPI IsValidDevmodeW(PDEVMODEW dm, SIZE_T size)
{
    static const struct
    {
        DWORD flag;
        SIZE_T size;
    } map[] =
    {
#define F_SIZE(field) FIELD_OFFSET(DEVMODEW, field) + sizeof(dm->field)
        { DM_ORIENTATION, F_SIZE(dmOrientation) },
        { DM_PAPERSIZE, F_SIZE(dmPaperSize) },
        { DM_PAPERLENGTH, F_SIZE(dmPaperLength) },
        { DM_PAPERWIDTH, F_SIZE(dmPaperWidth) },
        { DM_SCALE, F_SIZE(dmScale) },
        { DM_COPIES, F_SIZE(dmCopies) },
        { DM_DEFAULTSOURCE, F_SIZE(dmDefaultSource) },
        { DM_PRINTQUALITY, F_SIZE(dmPrintQuality) },
        { DM_POSITION, F_SIZE(dmPosition) },
        { DM_DISPLAYORIENTATION, F_SIZE(dmDisplayOrientation) },
        { DM_DISPLAYFIXEDOUTPUT, F_SIZE(dmDisplayFixedOutput) },
        { DM_COLOR, F_SIZE(dmColor) },
        { DM_DUPLEX, F_SIZE(dmDuplex) },
        { DM_YRESOLUTION, F_SIZE(dmYResolution) },
        { DM_TTOPTION, F_SIZE(dmTTOption) },
        { DM_COLLATE, F_SIZE(dmCollate) },
        { DM_FORMNAME, F_SIZE(dmFormName) },
        { DM_LOGPIXELS, F_SIZE(dmLogPixels) },
        { DM_BITSPERPEL, F_SIZE(dmBitsPerPel) },
        { DM_PELSWIDTH, F_SIZE(dmPelsWidth) },
        { DM_PELSHEIGHT, F_SIZE(dmPelsHeight) },
        { DM_DISPLAYFLAGS, F_SIZE(dmDisplayFlags) },
        { DM_NUP, F_SIZE(dmNup) },
        { DM_DISPLAYFREQUENCY, F_SIZE(dmDisplayFrequency) },
        { DM_ICMMETHOD, F_SIZE(dmICMMethod) },
        { DM_ICMINTENT, F_SIZE(dmICMIntent) },
        { DM_MEDIATYPE, F_SIZE(dmMediaType) },
        { DM_DITHERTYPE, F_SIZE(dmDitherType) },
        { DM_PANNINGWIDTH, F_SIZE(dmPanningWidth) },
        { DM_PANNINGHEIGHT, F_SIZE(dmPanningHeight) }
#undef F_SIZE
    };
    int i;
    const DWORD fields_off = FIELD_OFFSET(DEVMODEW, dmFields) + sizeof(dm->dmFields);

    if (!dm) return FALSE;
    if (size < fields_off) return FALSE;
    if (dm->dmSize < fields_off || size < dm->dmSize + dm->dmDriverExtra) return FALSE;

    for (i = 0; i < ARRAY_SIZE(map); i++)
        if ((dm->dmFields & map[i].flag) && dm->dmSize < map[i].size)
            return FALSE;

    return TRUE;
}

/******************************************************************
 *              OpenPrinterA        [WINSPOOL.@]
 *
 * See OpenPrinterW.
 *
 */
BOOL WINAPI OpenPrinterA(LPSTR name, HANDLE *printer, PRINTER_DEFAULTSA *defaults)
{
    return OpenPrinter2A(name, printer, defaults, NULL);
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
BOOL WINAPI OpenPrinterW(LPWSTR name, HANDLE *printer, PRINTER_DEFAULTSW *defaults)
{
    return OpenPrinter2W(name, printer, defaults, NULL);
}

BOOL WINAPI OpenPrinter2A(LPSTR name, HANDLE *printer,
    PRINTER_DEFAULTSA *defaults, PRINTER_OPTIONSA *options)
{
    UNICODE_STRING nameU;
    UNICODE_STRING datatypeU;
    PRINTER_DEFAULTSW defaultsW, *p_defaultsW = NULL;
    PRINTER_OPTIONSW optionsW, *p_optionsW = NULL;
    WCHAR *nameW;
    BOOL ret;

    TRACE("(%s,%p,%p,%p)\n", debugstr_a(name), printer, defaults, options);

    nameW = asciitounicode(&nameU, name);

    if (options)
    {
        optionsW.cbSize = sizeof(optionsW);
        optionsW.dwFlags = options->dwFlags;
        p_optionsW = &optionsW;
    }

    if (defaults)
    {
        defaultsW.pDatatype = asciitounicode(&datatypeU, defaults->pDatatype);
        defaultsW.pDevMode = defaults->pDevMode ? GdiConvertToDevmodeW(defaults->pDevMode) : NULL;
        defaultsW.DesiredAccess = defaults->DesiredAccess;
        p_defaultsW = &defaultsW;
    }

    ret = OpenPrinter2W(nameW, printer, p_defaultsW, p_optionsW);

    if (p_defaultsW)
    {
        RtlFreeUnicodeString(&datatypeU);
        heap_free(defaultsW.pDevMode);
    }
    RtlFreeUnicodeString(&nameU);

    return ret;
}

BOOL WINAPI OpenPrinter2W(LPWSTR name, HANDLE *printer,
    PRINTER_DEFAULTSW *defaults, PRINTER_OPTIONSW *options)
{
    HKEY key;

    TRACE("(%s,%p,%p,%p)\n", debugstr_w(name), printer, defaults, options);

    if (options)
        FIXME("flags %08lx ignored\n", options->dwFlags);

    if(!printer)
    {
        /* NT: FALSE with ERROR_INVALID_PARAMETER, 9x: TRUE */
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* Get the unique handle of the printer or Printserver */
    *printer = get_opened_printer_entry( name, defaults );

    if (*printer && WINSPOOL_GetOpenedPrinterRegKey( *printer, &key ) == ERROR_SUCCESS)
    {
        DWORD deleting = 0, size = sizeof( deleting ), type;
        DWORD status;
        RegQueryValueExW( key, May_Delete_Value, NULL, &type, (LPBYTE)&deleting, &size );
        WaitForSingleObject( init_mutex, INFINITE );
        status = get_dword_from_reg( key, L"Status" );
        set_reg_DWORD( key, L"Status", status & ~PRINTER_STATUS_DRIVER_UPDATE_NEEDED );
        ReleaseMutex( init_mutex );
        if (!deleting && (status & PRINTER_STATUS_DRIVER_UPDATE_NEEDED))
            update_driver( *printer );
        RegCloseKey( key );
    }

    TRACE("returning %d with %lu and %p\n", *printer != NULL, GetLastError(), *printer);
    return (*printer != NULL);
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
    TRACE("(%s, %ld, %p) :  %s %s %s\n", debugstr_a(pName), Level, pMonitors,
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
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    memset(&mi2w, 0, sizeof(MONITOR_INFO_2W));
    if (mi2a->pName) {
        len = MultiByteToWideChar(CP_ACP, 0, mi2a->pName, -1, NULL, 0);
        mi2w.pName = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, mi2a->pName, -1, mi2w.pName, len);
    }
    if (mi2a->pEnvironment) {
        len = MultiByteToWideChar(CP_ACP, 0, mi2a->pEnvironment, -1, NULL, 0);
        mi2w.pEnvironment = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, mi2a->pEnvironment, -1, mi2w.pEnvironment, len);
    }
    if (mi2a->pDLLName) {
        len = MultiByteToWideChar(CP_ACP, 0, mi2a->pDLLName, -1, NULL, 0);
        mi2w.pDLLName = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, mi2a->pDLLName, -1, mi2w.pDLLName, len);
    }

    res = AddMonitorW(nameW, Level, (LPBYTE) &mi2w);

    free(mi2w.pName);
    free(mi2w.pEnvironment);
    free(mi2w.pDLLName);

    free(nameW);
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
    TRACE("(%s, %ld, %p) :  %s %s %s\n", debugstr_w(pName), Level, pMonitors,
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
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    if (pEnvironment) {
        len = MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, NULL, 0);
        EnvironmentW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, EnvironmentW, len);
    }
    if (pMonitorName) {
        len = MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, NULL, 0);
        MonitorNameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, MonitorNameW, len);
    }

    res = DeleteMonitorW(nameW, EnvironmentW, MonitorNameW);

    free(MonitorNameW);
    free(EnvironmentW);
    free(nameW);
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
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    /* convert portname to unicode */
    if (pPortName) {
        len = MultiByteToWideChar(CP_ACP, 0, pPortName, -1, NULL, 0);
        portW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pPortName, -1, portW, len);
    }

    res = DeletePortW(nameW, hWnd, portW);
    free(nameW);
    free(portW);
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
BOOL WINAPI WritePrinter(HANDLE printer, void *buf, DWORD size, DWORD *written)
{
    HANDLE handle = get_backend_handle(printer);

    TRACE("(%p, %p, %ld, %p)\n", printer, buf, size, written);

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return backend->fpWritePrinter(handle, buf, size, written);
}

/*****************************************************************************
 *          AddFormA  [WINSPOOL.@]
 */
BOOL WINAPI AddFormA(HANDLE hPrinter, DWORD Level, LPBYTE pForm)
{
    FIXME("(%p,%ld,%p): stub\n", hPrinter, Level, pForm);
    return TRUE;
}

/*****************************************************************************
 *          AddFormW  [WINSPOOL.@]
 */
BOOL WINAPI AddFormW( HANDLE printer, DWORD level, BYTE *form )
{
    HANDLE handle = get_backend_handle( printer );

    TRACE( "(%p, %ld, %p)\n", printer, level, form );

    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    return backend->fpAddForm( handle, level, form );
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
BOOL WINAPI AddJobW(HANDLE printer, DWORD level, LPBYTE data, DWORD size, DWORD *needed)
{
    HANDLE handle = get_backend_handle(printer);

    TRACE("(%p, %ld, %p, %ld, %p)\n", printer, level, data, size, needed);

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return backend->fpAddJob(handle, level, data, size, needed);
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

    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", debugstr_a(server),
          debugstr_a(env), level, Info, cbBuf, pcbNeeded);


    if (server) {
        len = MultiByteToWideChar(CP_ACP, 0, server, -1, NULL, 0);
        serverW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, server, -1, serverW, len);
    }

    if (env) {
        len = MultiByteToWideChar(CP_ACP, 0, env, -1, NULL, 0);
        envW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, env, -1, envW, len);
    }

    /* NT requires the buffersize from GetPrintProcessorDirectoryW also
       for GetPrintProcessorDirectoryA and WC2MB is done in-place.
     */
    ret = GetPrintProcessorDirectoryW(serverW, envW, level, Info, 
                                      cbBuf, pcbNeeded);

    if (ret) ret = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)Info, -1, (LPSTR)Info,
                                       cbBuf, NULL, NULL) > 0;


    TRACE(" required: 0x%lx/%ld\n", pcbNeeded ? *pcbNeeded : 0, pcbNeeded ? *pcbNeeded : 0);
    free(envW);
    free(serverW);
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

    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", debugstr_w(server), debugstr_w(env), level,
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
 * set_devices_and_printerports [internal]
 *
 * set the [Devices] and [PrinterPorts] entries for a printer.
 *
 */
static void set_devices_and_printerports(PRINTER_INFO_2W *pi)
{
    DWORD portlen = wcslen( pi->pPortName ) * sizeof(WCHAR);
    WCHAR *devline;
    HKEY key;

    TRACE("(%p) %s\n", pi, debugstr_w(pi->pPrinterName));

    /* FIXME: the driver must change to "winspool" */
    devline = malloc( sizeof(L"wineps.drv") + portlen + sizeof(L",15,45") );
    if (devline)
    {
        wcscpy( devline, L"wineps.drv," );
        wcscat( devline, pi->pPortName );

        TRACE("using %s\n", debugstr_w(devline));
        if (!create_printers_reg_key( user_printers_key, &key ))
        {
            RegSetValueExW( key, pi->pPrinterName, 0, REG_SZ, (BYTE *)devline,
                            (wcslen( devline ) + 1) * sizeof(WCHAR) );
            RegCloseKey( key );
        }

        wcscat( devline, L",15,45" );
        if (!create_printers_reg_key( user_ports_key, &key ))
        {
            RegSetValueExW( key, pi->pPrinterName, 0, REG_SZ, (BYTE *)devline,
                            (wcslen( devline ) + 1) * sizeof(WCHAR) );
            RegCloseKey( key );
        }
        free(devline);
    }
}

static BOOL validate_print_proc(WCHAR *server, const WCHAR *name)
{
    PRINTPROCESSOR_INFO_1W *ppi;
    DWORD size, i, no;

    if (!EnumPrintProcessorsW(server, NULL, 1, NULL, 0, &size, &no)
            && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return FALSE;
    }
    ppi = malloc(size);
    if (!ppi)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    if (!EnumPrintProcessorsW(server, NULL, 1, (BYTE*)ppi, size, &size, &no))
    {
        free(ppi);
        return FALSE;
    }

    for (i = 0; i < no; i++)
    {
        if (!wcsicmp(ppi[i].pName, name))
            break;
    }
    free(ppi);
    return i != no;
}

/*****************************************************************************
 *          AddPrinterW  [WINSPOOL.@]
 */
HANDLE WINAPI AddPrinterW(LPWSTR pName, DWORD Level, LPBYTE pPrinter)
{
    PRINTER_INFO_2W *pi = (PRINTER_INFO_2W *) pPrinter;
    LPDEVMODEW dm;
    HANDLE retval;
    HKEY printer_key, printers_key, hkeyDriver, hkeyDrivers;
    LONG size;

    TRACE("(%s,%ld,%p)\n", debugstr_w(pName), Level, pPrinter);

    if(pName && *pName) {
        ERR("pName = %s - unsupported\n", debugstr_w(pName));
	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    if(Level != 2) {
        ERR("Level = %ld, unsupported!\n", Level);
	SetLastError(ERROR_INVALID_LEVEL);
	return 0;
    }
    if(!pPrinter) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    if (create_printers_reg_key( system_printers_key, &printers_key ))
    {
        ERR("Can't create Printers key\n");
	return 0;
    }
    if (!RegOpenKeyW( printers_key, pi->pPrinterName, &printer_key ))
    {
        if (!RegQueryValueW( printer_key, L"Attributes", NULL, NULL ))
        {
	    SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
	    RegCloseKey( printer_key );
	    RegCloseKey( printers_key );
	    return 0;
	}
	RegCloseKey( printer_key );
    }
    hkeyDrivers = WINSPOOL_OpenDriverReg(NULL);
    if(!hkeyDrivers) {
        ERR("Can't create Drivers key\n");
	RegCloseKey( printers_key );
	return 0;
    }
    if(RegOpenKeyW(hkeyDrivers, pi->pDriverName, &hkeyDriver) !=
       ERROR_SUCCESS) {
        WARN("Can't find driver %s\n", debugstr_w(pi->pDriverName));
	RegCloseKey( printers_key );
	RegCloseKey(hkeyDrivers);
	SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
	return 0;
    }
    RegCloseKey(hkeyDriver);
    RegCloseKey(hkeyDrivers);

    if (!validate_print_proc(pName, pi->pPrintProcessor))
    {
        FIXME("Can't find processor %s\n", debugstr_w(pi->pPrintProcessor));
	SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
	RegCloseKey( printers_key );
	return 0;
    }

    if (RegCreateKeyW( printers_key, pi->pPrinterName, &printer_key ))
    {
        FIXME("Can't create printer %s\n", debugstr_w(pi->pPrinterName));
	SetLastError(ERROR_INVALID_PRINTER_NAME);
	RegCloseKey( printers_key );
	return 0;
    }

    set_devices_and_printerports(pi);

    set_reg_DWORD( printer_key, L"Attributes", pi->Attributes );
    set_reg_szW( printer_key, L"Datatype", pi->pDatatype );
    set_reg_DWORD( printer_key, L"Default Priority", pi->DefaultPriority );
    set_reg_szW( printer_key, L"Description", pi->pComment );
    set_reg_DWORD( printer_key, L"dnsTimeout", 0 );
    set_reg_szW( printer_key, L"Location", pi->pLocation );
    set_reg_szW( printer_key, L"Name", pi->pPrinterName );
    set_reg_szW( printer_key, L"Parameters", pi->pParameters );
    set_reg_szW( printer_key, L"Port", pi->pPortName );
    set_reg_szW( printer_key, L"Print Processor", pi->pPrintProcessor );
    set_reg_szW( printer_key, L"Printer Driver", pi->pDriverName );
    set_reg_DWORD( printer_key, L"Priority", pi->Priority );
    set_reg_szW( printer_key, L"Separator File", pi->pSepFile );
    set_reg_szW( printer_key, L"Share Name", pi->pShareName );
    set_reg_DWORD( printer_key, L"StartTime", pi->StartTime );
    set_reg_DWORD( printer_key, L"Status", pi->Status );
    set_reg_DWORD( printer_key, L"txTimeout", 0 );
    set_reg_DWORD( printer_key, L"UntilTime", pi->UntilTime );

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
        dm = calloc( 1, size );
        dm->dmSize = size;
        if (DocumentPropertiesW(0, 0, pi->pPrinterName, dm, NULL, DM_OUT_BUFFER) < 0)
        {
            WARN("DocumentPropertiesW on printer %s failed!\n", debugstr_w(pi->pPrinterName));
            free( dm );
            dm = NULL;
        }
        else
        {
            unsigned int len = min( ARRAY_SIZE( dm->dmDeviceName ) - 1, wcslen( pi->pPrinterName ) );
            memcpy( dm->dmDeviceName, pi->pPrinterName, len * sizeof(WCHAR) );
            dm->dmDeviceName[len] = '\0';
        }
    }

    set_reg_devmode( printer_key, L"Default DevMode", dm );
    if (!pi->pDevMode) free( dm );

    RegCloseKey( printer_key );
    RegCloseKey( printers_key );
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

    TRACE("(%s, %ld, %p)\n", debugstr_a(pName), Level, pPrinter);
    if(Level != 2) {
        ERR("Level = %ld, unsupported!\n", Level);
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

    TRACE("(%p)\n", hPrinter);

    EnterCriticalSection(&printer_handles_cs);

    if ((i > 0) && (i <= nb_printer_handles))
        printer = printer_handles[i - 1];


    if(printer)
    {
        TRACE("closing %s\n", debugstr_w(printer->name));
        if (printer->backend_printer) {
            backend->fpClosePrinter(printer->backend_printer);
        }

        free_printer_entry( printer );
        printer_handles[i - 1] = NULL;
        LeaveCriticalSection(&printer_handles_cs);
        return TRUE;
    }

    LeaveCriticalSection(&printer_handles_cs);
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
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
BOOL WINAPI DeleteFormW( HANDLE printer, WCHAR *name )
{
    HANDLE handle = get_backend_handle( printer );

    TRACE( "(%p, %s)\n", printer, debugstr_w( name ) );

    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    return backend->fpDeleteForm( handle, name );
}

/*****************************************************************************
 *          DeletePrinter  [WINSPOOL.@]
 */
BOOL WINAPI DeletePrinter(HANDLE hPrinter)
{
    LPCWSTR lpNameW = get_opened_printer_name(hPrinter);
    config_module_t *config_module;
    HKEY key;
    WCHAR def[MAX_PATH];
    DWORD size = ARRAY_SIZE(def);

    if(!lpNameW) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    EnterCriticalSection(&config_modules_cs);
    if ((config_module = get_config_module(lpNameW, FALSE))) {
        wine_rb_remove(&config_modules, &config_module->entry);
        release_config_module(config_module);
    }
    LeaveCriticalSection(&config_modules_cs);

    if (!create_printers_reg_key( system_printers_key, &key ))
    {
        RegDeleteTreeW( key, lpNameW );
        RegCloseKey( key );
    }

    if (!create_printers_reg_key( user_printers_key, &key ))
    {
        RegDeleteValueW( key, lpNameW );
        RegCloseKey( key );
    }

    if (!create_printers_reg_key( user_ports_key, &key ))
    {
        RegDeleteValueW( key, lpNameW );
        RegCloseKey( key );
    }

    if (GetDefaultPrinterW( def, &size ) && !wcscmp( def, lpNameW ))
    {
        if (!create_printers_reg_key( user_default_key, &key ))
        {
            RegDeleteValueW( key, L"device" );
            RegCloseKey( key );
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
    set_reg_szW( key, L"Name", pi->pPrinterName );
    set_reg_szW( key, L"Share Name", pi->pShareName );
    set_reg_szW( key, L"Port", pi->pPortName );
    set_reg_szW( key, L"Printer Driver", pi->pDriverName );
    set_reg_szW( key, L"Description", pi->pComment );
    set_reg_szW( key, L"Location", pi->pLocation );

    if (pi->pDevMode)
        set_reg_devmode( key, L"Default DevMode", pi->pDevMode );

    set_reg_szW( key, L"Separator File", pi->pSepFile );
    set_reg_szW( key, L"Print Processor", pi->pPrintProcessor );
    set_reg_szW( key, L"Datatype", pi->pDatatype );
    set_reg_szW( key, L"Parameters", pi->pParameters );

    set_reg_DWORD( key, L"Attributes", pi->Attributes );
    set_reg_DWORD( key, L"Priority", pi->Priority );
    set_reg_DWORD( key, L"Default Priority", pi->DefaultPriority );
    set_reg_DWORD( key, L"StartTime", pi->StartTime );
    set_reg_DWORD( key, L"UntilTime", pi->UntilTime );
}

static BOOL set_printer_9( HKEY key, const PRINTER_INFO_9W *pi )
{
    if (!pi->pDevMode) return FALSE;

    set_reg_devmode( key, L"Default DevMode", pi->pDevMode );
    return TRUE;
}

/******************************************************************************
 *    SetPrinterW  [WINSPOOL.@]
 */
BOOL WINAPI SetPrinterW( HANDLE printer, DWORD level, LPBYTE data, DWORD command )
{
    HKEY key;
    BOOL ret = FALSE;

    TRACE( "(%p, %ld, %p, %ld)\n", printer, level, data, command );

    if (command != 0) FIXME( "Ignoring command %ld\n", command );

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

    case 8:
        /* 8 is the global default printer info and 9 already sets it instead of the per-user one */
        /* still, PRINTER_INFO_8W is the same as PRINTER_INFO_9W */
        /* fall through */
    case 9:
    {
        PRINTER_INFO_9W *pi = (PRINTER_INFO_9W *)data;
        ret = set_printer_9( key, pi );
        break;
    }

    default:
        FIXME( "Unimplemented level %ld\n", level );
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
    void *JobW;
    UNICODE_STRING usBuffer;

    TRACE("(%p, %ld, %ld, %p, %ld)\n",hPrinter, JobId, Level, pJob, Command);

    /* JobId, pPrinterName, pMachineName, pDriverName, Size, Submitted, Time and TotalPages
       are all ignored by SetJob, so we don't bother copying them */
    switch(Level)
    {
    case 0:
        JobW = NULL;
        break;
    case 1:
      {
        JOB_INFO_1W *info1W = malloc(sizeof(*info1W));
        JOB_INFO_1A *info1A = (JOB_INFO_1A*)pJob;

        JobW = info1W;
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
        JOB_INFO_2W *info2W = malloc(sizeof(*info2W));
        JOB_INFO_2A *info2A = (JOB_INFO_2A*)pJob;

        JobW = info2W;
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
        JobW = malloc(sizeof(JOB_INFO_3));
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
        JOB_INFO_1W *info1W = JobW;
        free(info1W->pUserName);
        free(info1W->pDocument);
        free(info1W->pDatatype);
        free(info1W->pStatus);
        break;
      }
    case 2:
      {
        JOB_INFO_2W *info2W = JobW;
        free(info2W->pUserName);
        free(info2W->pDocument);
        free(info2W->pNotifyName);
        free(info2W->pDatatype);
        free(info2W->pPrintProcessor);
        free(info2W->pParameters);
        heap_free(info2W->pDevMode);
        free(info2W->pStatus);
        break;
      }
    }
    free(JobW);

    return ret;
}

/*****************************************************************************
 *          SetJobW  [WINSPOOL.@]
 */
BOOL WINAPI SetJobW(HANDLE printer, DWORD job_id, DWORD level,
                    LPBYTE data, DWORD command)
{
    HANDLE handle = get_backend_handle(printer);

    TRACE("(%p, %ld, %ld, %p, %ld)\n", printer, job_id, level, data, command);

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return backend->fpSetJob(handle, job_id, level, data, command);
}

/*****************************************************************************
 *          EndDocPrinter  [WINSPOOL.@]
 */
BOOL WINAPI EndDocPrinter(HANDLE printer)
{
    HANDLE handle = get_backend_handle(printer);

    TRACE("(%p)\n", printer);

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return backend->fpEndDocPrinter(handle);
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

    free(doc2W.pDatatype);
    free(doc2W.pOutputFile);
    free(doc2W.pDocName);

    return ret;
}

/*****************************************************************************
 *          StartDocPrinterW  [WINSPOOL.@]
 */
DWORD WINAPI StartDocPrinterW(HANDLE printer, DWORD level, BYTE *doc_info)
{
    HANDLE handle = get_backend_handle(printer);
    DOC_INFO_1W *info = (DOC_INFO_1W *)doc_info;

    TRACE("(%p, %ld, %p {%s, %s, %s})\n", printer, level, doc_info,
            debugstr_w(info->pDocName), debugstr_w(info->pOutputFile),
            debugstr_w(info->pDatatype));

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    return backend->fpStartDocPrinter(handle, level, doc_info);
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
BOOL WINAPI GetFormA( HANDLE printer, char *name, DWORD level, BYTE *form, DWORD size, DWORD *needed )
{
    UNICODE_STRING nameW;
    const DWORD *string_info = form_string_info( level );
    BOOL ret;

    if (!string_info) return FALSE;

    asciitounicode( &nameW, name );

    ret = GetFormW( printer, nameW.Buffer, level, form, size, needed );
    if (ret) packed_struct_WtoA( form, string_info );

    RtlFreeUnicodeString( &nameW );
    return ret;
}

/*****************************************************************************
 *          GetFormW  [WINSPOOL.@]
 */
BOOL WINAPI GetFormW( HANDLE printer, WCHAR *name, DWORD level, BYTE *form, DWORD size, DWORD *needed )
{
    HANDLE handle = get_backend_handle( printer );

    TRACE( "(%p, %s, %ld, %p, %ld, %p)\n", printer, debugstr_w( name ), level, form, size, needed );

    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    return backend->fpGetForm( handle, name, level, form, size, needed );
}

/*****************************************************************************
 *          SetFormA  [WINSPOOL.@]
 */
BOOL WINAPI SetFormA(HANDLE hPrinter, LPSTR pFormName, DWORD Level,
                        LPBYTE pForm)
{
    FIXME("(%p,%s,%ld,%p): stub\n",hPrinter,pFormName,Level,pForm);
    return FALSE;
}

/*****************************************************************************
 *          SetFormW  [WINSPOOL.@]
 */
BOOL WINAPI SetFormW( HANDLE printer, WCHAR *name, DWORD level, BYTE *form )
{
    HANDLE handle = get_backend_handle( printer );

    TRACE( "(%p, %s, %ld, %p)\n", printer, debugstr_w( name ), level, form );

    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    return backend->fpSetForm( handle, name, level, form );
}

/*****************************************************************************
 *          ReadPrinter  [WINSPOOL.@]
 */
BOOL WINAPI ReadPrinter(HANDLE printer, void *buf, DWORD size, DWORD *bytes_read)
{
    HANDLE handle = get_backend_handle(printer);

    TRACE("(%p,%p,%ld,%p)\n", printer, buf, size, bytes_read);

    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    return backend->fpReadPrinter(handle, buf, size, bytes_read);
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
BOOL WINAPI ResetPrinterW(HANDLE printer, PRINTER_DEFAULTSW *def)
{
    HANDLE handle  = get_backend_handle(printer);

    TRACE("(%p, %p)\n", printer, def);

    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    return backend->fpResetPrinter(handle, def);
}

/*****************************************************************************
 * get_filename_from_reg [internal]
 *
 * Get ValueName from hkey storing result in out
 * when the Value in the registry has only a filename, use driverdir as prefix
 * outlen is space left in out
 * String is stored either as unicode or ansi
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
        TRACE("need dynamic buffer: %lu\n", size);
        buffer = malloc(size);
        if (!buffer) {
            /* No Memory is bad */
            return FALSE;
        }
        buffer[0] = '\0';
        ret = RegQueryValueExW(hkey, ValueName, NULL, &type, (LPBYTE) buffer, &size);
    }

    if ((ret != ERROR_SUCCESS) || (!buffer[0])) {
        if (buffer != filename) free(buffer);
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
                wcscpy( (WCHAR *)out, driverdir );
                out += dirlen;
                outlen -= dirlen;
            }
            else
                out = NULL;
        }

        /* write the filename */
        size = (wcslen( ptr ) + 1) * sizeof(WCHAR);
        if ((out) && (outlen >= size)) {
            wcscpy( (WCHAR *)out, ptr );
            out += size;
            outlen -= size;
        }
        else
            out = NULL;
        *needed += size;
        ptr += wcslen( ptr ) + 1;
        if ((type != REG_MULTI_SZ) || (!ptr[0]))  ptr = NULL;
    }

    if (buffer != filename) free(buffer);

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
        WARN("Got ret = %ld\n", ret);
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
    if (buflen >= sizeof(DEVMODEW))
    {
        DEVMODEW *dm = (DEVMODEW *)ptr;

        /* the driver will update registry with real values */
        memset(dm, 0, sizeof(*dm));
        dm->dmSize = sizeof(*dm);
        wcscpy( dm->dmDeviceName, L"wineps.drv" );
    }
    *needed = sizeof(DEVMODEW);
}

/*****************************************************************************
 *    WINSPOOL_GetDevModeFromReg
 *
 * Get ValueName from hkey storing result in ptr.  buflen is space left in ptr
 * DevMode is stored either as unicode or ansi.
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
        TRACE("corrupted registry for %s ( size %ld)\n",debugstr_w(ValueName),sz);
	return FALSE;
    }
    /* ensures that dmSize is not erratically bogus if registry is invalid */
    if (ptr && ((DEVMODEA*)ptr)->dmSize < sizeof(DEVMODEA))
        ((DEVMODEA*)ptr)->dmSize = sizeof(DEVMODEA);
    sz += (CCHDEVICENAME + CCHFORMNAME);
    if (ptr && (buflen >= sz)) {
        DEVMODEW *dmW = GdiConvertToDevmodeW((DEVMODEA*)ptr);
        memcpy(ptr, dmW, sz);
        heap_free(dmW);
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

    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Name", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi1->pName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }

    /* FIXME: pDescription should be something like "Name,Driver_Name,". */
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Name", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi1->pDescription = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }

    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Description", ptr, left, &size ))
    {
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

    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Name", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi2->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Share Name", ptr, left, &size))
    {
        if(space && size <= left) {
	    pi2->pShareName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Port", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi2->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Printer Driver", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi2->pDriverName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Description", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi2->pComment = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Location", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi2->pLocation = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetDevModeFromReg( hkeyPrinter, L"Default DevMode", ptr, left, &size ))
    {
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
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Separator File", ptr, left, &size ))
    {
        if(space && size <= left) {
            pi2->pSepFile = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Print Processor", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi2->pPrintProcessor = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Datatype", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi2->pDatatype = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Parameters", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi2->pParameters = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (pi2)
    {
        pi2->Attributes = get_dword_from_reg( hkeyPrinter, L"Attributes" );
        pi2->Priority = get_dword_from_reg( hkeyPrinter, L"Priority" );
        pi2->DefaultPriority = get_dword_from_reg( hkeyPrinter, L"Default Priority" );
        pi2->StartTime = get_dword_from_reg( hkeyPrinter, L"StartTime" );
        pi2->UntilTime = get_dword_from_reg( hkeyPrinter, L"UntilTime" );
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

    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Name", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi4->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi4) {
        pi4->Attributes = get_dword_from_reg( hkeyPrinter, L"Attributes" );
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

    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Name", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi5->pPrinterName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if (WINSPOOL_GetStringFromReg( hkeyPrinter, L"Port", ptr, left, &size ))
    {
        if(space && size <= left) {
	    pi5->pPortName = (LPWSTR)ptr;
	    ptr += size;
	    left -= size;
	} else
	    space = FALSE;
	*pcbNeeded += size;
    }
    if(pi5) {
        pi5->Attributes = get_dword_from_reg( hkeyPrinter, L"Attributes" );
        pi5->DeviceNotSelectedTimeout = get_dword_from_reg( hkeyPrinter, L"dnsTimeout" );
        pi5->TransmissionRetryTimeout = get_dword_from_reg( hkeyPrinter, L"txTimeout" );
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

    if (!WINSPOOL_GetStringFromReg( hkeyPrinter, L"ObjectGUID", ptr, left, &size ))
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

    if (WINSPOOL_GetDevModeFromReg( hkeyPrinter, L"Default DevMode", buf, cbBuf, &size ))
    {
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

    TRACE("(%p,%ld,%p,%ld,%p)\n",hPrinter,Level,pPrinter,cbBuf, pcbNeeded);

    err = WINSPOOL_GetOpenedPrinterRegKey( hPrinter, &hkeyPrinter );
    if (err)
    {
        SetLastError( err );
        return FALSE;
    }

    switch(Level) {
    case 1:
      {
        PRINTER_INFO_1W *pi1 = (PRINTER_INFO_1W *)pPrinter;

        size = sizeof(PRINTER_INFO_1W);
        if (size <= cbBuf) {
            ptr = pPrinter + size;
            cbBuf -= size;
            memset(pPrinter, 0, size);
        } else {
            pi1 = NULL;
            cbBuf = 0;
        }
        ret = WINSPOOL_GetPrinter_1(hkeyPrinter, pi1, ptr, cbBuf, &needed);
        needed += size;
        break;
      }

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
            pi6->dwStatus = get_dword_from_reg( hkeyPrinter, L"Status" );
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
        FIXME("Unimplemented level %ld\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
	RegCloseKey(hkeyPrinter);
	return FALSE;
    }

    RegCloseKey(hkeyPrinter);

    TRACE("returning %d needed = %ld\n", ret, needed);
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
        buf = malloc(cbBuf);

    ret = GetPrinterW(hPrinter, Level, buf, cbBuf, pcbNeeded);
    if (ret)
        convert_printerinfo_W_to_A(pPrinter, buf, Level, cbBuf, 1);
    free(buf);

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
    HKEY printers_key, hkeyPrinter;
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
        FIXME("dwType = %08lx\n", dwType);
	SetLastError(ERROR_INVALID_FLAGS);
	return FALSE;
    }

    if (create_printers_reg_key( system_printers_key, &printers_key ))
    {
        ERR("Can't create Printers key\n");
	return FALSE;
    }

    if (RegQueryInfoKeyA( printers_key, NULL, NULL, NULL, &number, NULL, NULL, NULL, NULL, NULL, NULL, NULL ))
    {
        RegCloseKey( printers_key );
	ERR("Can't query Printers key\n");
	return FALSE;
    }
    TRACE("Found %ld printers\n", number);

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
	RegCloseKey( printers_key );
	return FALSE;
    }
    pi = (used <= cbBuf) ? lpbPrinters : NULL;

    for(i = 0; i < number; i++) {
        if (RegEnumKeyW( printers_key, i, PrinterName, ARRAY_SIZE(PrinterName) ))
        {
	    ERR("Can't enum key number %ld\n", i);
	    RegCloseKey( printers_key );
	    return FALSE;
	}
	TRACE("Printer %ld is %s\n", i, debugstr_w(PrinterName));
	if (RegOpenKeyW( printers_key, PrinterName, &hkeyPrinter ))
        {
	    ERR("Can't open key %s\n", debugstr_w(PrinterName));
	    RegCloseKey( printers_key );
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
	    RegCloseKey( printers_key );
	    return FALSE;
	}
	RegCloseKey(hkeyPrinter);
    }
    RegCloseKey( printers_key );

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

    TRACE("(0x%lx, %s, %lu, %p, %ld, %p, %p)\n", flags, debugstr_a(pName), level,
                                              pPrinters, cbBuf, pcbNeeded, pcReturned);

    pNameW = asciitounicode(&pNameU, pName);

    /* Request a buffer with a size, that is big enough for EnumPrintersW.
       MS Office need this */
    pPrintersW = (pPrinters && cbBuf) ? malloc(cbBuf) : NULL;

    ret = EnumPrintersW(flags, pNameW, level, pPrintersW, cbBuf, pcbNeeded, pcReturned);

    RtlFreeUnicodeString(&pNameU);
    if (ret) {
        convert_printerinfo_W_to_A(pPrinters, pPrintersW, level, *pcbNeeded, *pcReturned);
    }
    free(pPrintersW);
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

    TRACE("(%p, %s, %p, %ld, %p, %p, %ld)\n", hkeyDrivers,
          debugstr_w(DriverName), env,
          Level, di, pDriverStrings, cbBuf);

    if (di) ZeroMemory(di, di_sizeof[Level]);

    *pcbNeeded = (wcslen( DriverName ) + 1) * sizeof(WCHAR);
    if (*pcbNeeded <= cbBuf)
       wcscpy( (WCHAR *)strPtr, DriverName );

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

    /* Reserve Space for "\\3\\" + \0 */
    size = sizeof(driverdir) - 4 * sizeof(WCHAR);
    if (!GetPrinterDriverDirectoryW(NULL, (LPWSTR) env->envname, 1, (LPBYTE) driverdir, size, &size)) {
        /* Should never Fail */
        return FALSE;
    }
    wcscat( driverdir, env->versionsubdir );
    wcscat( driverdir, L"\\" );

    /* dirlen must not include the terminating zero */
    dirlen = wcslen( driverdir ) * sizeof(WCHAR);

    if (!DriverName[0] || RegOpenKeyW(hkeyDrivers, DriverName, &hkeyDriver) != ERROR_SUCCESS) {
        ERR("Can't find driver %s in registry\n", debugstr_w(DriverName));
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER); /* ? */
        return FALSE;
    }

    /* pEnvironment */
    size = (wcslen( env->envname ) + 1) * sizeof(WCHAR);

    *pcbNeeded += size;
    if (*pcbNeeded <= cbBuf) {
        wcscpy( (WCHAR *)strPtr, env->envname );
        if (di) di->pEnvironment = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    /* .pDriverPath is the Graphics rendering engine.
        The full Path is required to avoid a crash in some apps */
    if (get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Driver", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Driver", strPtr, size, &tmp );

        if (di) di->pDriverPath = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? (pDriverStrings + (*pcbNeeded)) : NULL;
    }

    /* .pDataFile: For postscript-drivers, this is the ppd-file */
    if (get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Data File", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Data File", strPtr, size, &size );

        if (di) di->pDataFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pConfigFile is the Driver user Interface */
    if (get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Configuration File", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Configuration File", strPtr, size, &size );

        if (di) di->pConfigFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level == 2 ) {
        RegCloseKey(hkeyDriver);
        TRACE("buffer space %ld required %ld\n", cbBuf, *pcbNeeded);
        return TRUE;
    }

    if (Level == 5 ) {
        RegCloseKey(hkeyDriver);
        FIXME("level 5: incomplete\n");
        return TRUE;
    }

    /* .pHelpFile */
    if (get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Help File", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Help File", strPtr, size, &size );

        if (di) di->pHelpFile = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pDependentFiles */
    if (get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Dependent Files", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            get_filename_from_reg( hkeyDriver, driverdir, dirlen, L"Dependent Files", strPtr, size, &size );

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
    if (WINSPOOL_GetStringFromReg( hkeyDriver, L"Monitor", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if (*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg( hkeyDriver, L"Monitor", strPtr, size, &size );

        if (di) di->pMonitorName = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pDefaultDataType */
    if (WINSPOOL_GetStringFromReg( hkeyDriver, L"Datatype", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg( hkeyDriver, L"Datatype", strPtr, size, &size );

        if (di) di->pDefaultDataType = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level == 3 ) {
        RegCloseKey(hkeyDriver);
        TRACE("buffer space %ld required %ld\n", cbBuf, *pcbNeeded);
        return TRUE;
    }

    /* .pszzPreviousNames */
    if (WINSPOOL_GetStringFromReg( hkeyDriver, L"Previous Names", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg( hkeyDriver, L"Previous Names", strPtr, size, &size );

        if (di) di->pszzPreviousNames = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level == 4 ) {
        RegCloseKey(hkeyDriver);
        TRACE("buffer space %ld required %ld\n", cbBuf, *pcbNeeded);
        return TRUE;
    }

    /* support is missing, but not important enough for a FIXME */
    TRACE("%s: DriverDate + DriverVersion not supported\n", debugstr_w(DriverName));

    /* .pszMfgName */
    if (WINSPOOL_GetStringFromReg( hkeyDriver, L"Manufacturer", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg( hkeyDriver, L"Manufacturer", strPtr, size, &size );

        if (di) di->pszMfgName = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pszOEMUrl */
    if (WINSPOOL_GetStringFromReg( hkeyDriver, L"OEM Url", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg( hkeyDriver, L"OEM Url", strPtr, size, &size );

        if (di) di->pszOEMUrl = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pszHardwareID */
    if (WINSPOOL_GetStringFromReg( hkeyDriver, L"HardwareID", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg( hkeyDriver, L"HardwareID", strPtr, size, &size );

        if (di) di->pszHardwareID = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    /* .pszProvider */
    if (WINSPOOL_GetStringFromReg( hkeyDriver, L"Provider", strPtr, 0, &size ))
    {
        *pcbNeeded += size;
        if(*pcbNeeded <= cbBuf)
            WINSPOOL_GetStringFromReg( hkeyDriver, L"Provider", strPtr, size, &size );

        if (di) di->pszProvider = (LPWSTR)strPtr;
        strPtr = (pDriverStrings) ? pDriverStrings + (*pcbNeeded) : NULL;
    }

    if (Level == 6 ) {
        RegCloseKey(hkeyDriver);
        return TRUE;
    }

    /* support is missing, but not important enough for a FIXME */
    TRACE("level 8: incomplete\n");

    TRACE("buffer space %ld required %ld\n", cbBuf, *pcbNeeded);
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

    TRACE("(%p,%s,%ld,%p,%ld,%p)\n",hPrinter,debugstr_w(pEnvironment),
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
    ret = RegQueryValueExW( hkeyPrinter, L"Printer Driver", 0, &type, (BYTE *)DriverName, &size );
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
    TRACE("buffer space %ld required %ld\n", cbBuf, size + needed);
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
        buf = malloc(cbBuf);
    }

    pwstrEnvW = asciitounicode(&pEnvW, pEnvironment);
    ret = GetPrinterDriverW(hPrinter, pwstrEnvW, Level, buf,
				    cbBuf, pcbNeeded);
    if (ret)
        convert_driverinfo_W_to_A(pDriverInfo, buf, Level, cbBuf, 1);

    free(buf);

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
    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", debugstr_w(pName),
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

    TRACE("(%s, %s, %ld, %p, %ld, %p)\n", debugstr_a(pName),
          debugstr_a(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);
 
    if (len) driverDirectoryW = malloc( len );

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

    TRACE("required: 0x%lx/%ld\n", pcbNeeded ? *pcbNeeded : 0, pcbNeeded ? *pcbNeeded : 0);

    free( driverDirectoryW );
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
    TRACE("(%s, %ld, %p)\n", debugstr_a(pName), level, pDriverInfo);
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
    TRACE("(%s, %ld, %p)\n", debugstr_w(pName), level, pDriverInfo);
    return AddPrinterDriverExW(pName, level, pDriverInfo, APD_COPY_NEW_FILES);
}

/*****************************************************************************
 *          AddPrintProcessorA  [WINSPOOL.@]
 */
BOOL WINAPI AddPrintProcessorA(LPSTR pName, LPSTR pEnvironment, LPSTR pPathName,
                               LPSTR pPrintProcessorName)
{
    UNICODE_STRING NameW, EnvW, PathW, ProcessorW;
    BOOL ret;

    TRACE("(%s,%s,%s,%s)\n", debugstr_a(pName), debugstr_a(pEnvironment),
          debugstr_a(pPathName), debugstr_a(pPrintProcessorName));

    asciitounicode(&NameW, pName);
    asciitounicode(&EnvW, pEnvironment);
    asciitounicode(&PathW, pPathName);
    asciitounicode(&ProcessorW, pPrintProcessorName);

    ret = AddPrintProcessorW(NameW.Buffer, EnvW.Buffer, PathW.Buffer, ProcessorW.Buffer);

    RtlFreeUnicodeString(&ProcessorW);
    RtlFreeUnicodeString(&PathW);
    RtlFreeUnicodeString(&EnvW);
    RtlFreeUnicodeString(&NameW);

    return ret;
}

/*****************************************************************************
 *          AddPrintProcessorW  [WINSPOOL.@]
 */
BOOL WINAPI AddPrintProcessorW(WCHAR *name, WCHAR *env, WCHAR *path, WCHAR *print_proc)
{
    TRACE("(%s,%s,%s,%s)\n", debugstr_w(name), debugstr_w(env),
            debugstr_w(path), debugstr_w(print_proc));

    if (!path || !print_proc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((backend == NULL)  && !load_backend()) return FALSE;
    return backend->fpAddPrintProcessor(name, env, path, print_proc);
}

/*****************************************************************************
 *          AddPrintProvidorA  [WINSPOOL.@]
 */
BOOL WINAPI AddPrintProvidorA(LPSTR pName, DWORD Level, LPBYTE pProviderInfo)
{
    FIXME("(%s,0x%08lx,%p): stub\n", debugstr_a(pName), Level, pProviderInfo);
    return FALSE;
}

/*****************************************************************************
 *          AddPrintProvidorW  [WINSPOOL.@]
 */
BOOL WINAPI AddPrintProvidorW(LPWSTR pName, DWORD Level, LPBYTE pProviderInfo)
{
    FIXME("(%s,0x%08lx,%p): stub\n", debugstr_w(pName), Level, pProviderInfo);
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
    FIXME("(%p,first=%ld,no=%ld,level=%ld,job=%p,cb=%ld,%p,%p), stub!\n",
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
    FIXME("(%p,first=%ld,no=%ld,level=%ld,job=%p,cb=%ld,%p,%p), stub!\n",
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

    TRACE("%s,%s,%ld,%p,%ld,%ld,%ld\n",
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
    TRACE("Found %ld Drivers\n", *pcFound);

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

        if(RegEnumKeyW(hkeyDrivers, i, DriverNameW, ARRAY_SIZE(DriverNameW)) != ERROR_SUCCESS) {
            ERR("Can't enum key number %ld\n", i);
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
    if (pEnvironment && !wcscmp( pEnvironment, L"all" ))
    {
        DWORD i, needed, bufsize = cbBuf;
        DWORD total_found = 0;
        DWORD data_offset;

        /* Precompute the overall total; we need this to know
           where pointers end and data begins (i.e. data_offset) */
        for (i = 0; i < ARRAY_SIZE(all_printenv); i++)
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
        for (i = 0; i < ARRAY_SIZE(all_printenv); i++)
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
        buf = malloc(cbBuf);

    pwstrNameW = asciitounicode(&pNameW, pName);
    pwstrEnvironmentW = asciitounicode(&pEnvironmentW, pEnvironment);

    ret = EnumPrinterDriversW(pwstrNameW, pwstrEnvironmentW, Level,
                                buf, cbBuf, pcbNeeded, pcReturned);
    if (ret)
        convert_driverinfo_W_to_A(pDriverInfo, buf, Level, cbBuf, *pcReturned);

    free(buf);

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

    TRACE("(%s, %ld, %p, %ld, %p, %p)\n", debugstr_a(pName), Level, pPorts,
          cbBuf, pcbNeeded, pcReturned);

    /* convert servername to unicode */
    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }
    /* alloc (userbuffersize*sizeof(WCHAR) and try to enum the Ports */
    needed = cbBuf * sizeof(WCHAR);    
    if (needed) bufferW = malloc(needed);
    res = EnumPortsW(nameW, Level, bufferW, needed, pcbNeeded, pcReturned);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (pcbNeeded) needed = *pcbNeeded;
        /* HeapReAlloc return NULL, when bufferW was NULL */
        bufferW = (bufferW) ? realloc(bufferW, needed) : malloc(needed);

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
            TRACE("%p: parsing #%ld (%s)\n", pi2w, index, debugstr_w(pi2w->pPortName));

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
            TRACE("%p: writing PORT_INFO_%ldA #%ld\n", pi2a, Level, index);
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

    free(nameW);
    free(bufferW);

    TRACE("returning %d with %ld (%ld byte for %ld of %ld entries)\n",
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

    TRACE("(%s, %ld, %p, %ld, %p, %p)\n", debugstr_w(pName), Level, pPorts,
          cbBuf, pcbNeeded, pcReturned);

    if ((backend == NULL)  && !load_backend()) return FALSE;

    /* Level is not checked in win9x */
    if (!Level || (Level > 2)) {
        WARN("level (%ld) is ignored in win9x\n", Level);
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
    buffer = malloc( len * sizeof(WCHAR));

    if (!GetProfileStringW( L"windows", L"device", L"", buffer, len ))
    {
        SetLastError (ERROR_FILE_NOT_FOUND);
        retval = FALSE;
        goto end;
    }
    TRACE("%s\n", debugstr_w(buffer));

    if ((ptr = wcschr( buffer, ',' )) == NULL)
    {
        SetLastError(ERROR_INVALID_NAME);
        retval = FALSE;
        goto end;
    }

    *ptr = 0;
    *namesize = wcslen( buffer ) + 1;
    if(!name || (*namesize > insize))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        retval = FALSE;
        goto end;
    }
    wcscpy( name, buffer );

end:
    free( buffer);
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
	bufferW = malloc( insize * sizeof(WCHAR));
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
    TRACE("0x%08lx/0x%08lx:%s\n", *namesize, insize, debugstr_w(bufferW));

end:
    free( bufferW);
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
        size = ARRAY_SIZE(default_printer);

        /* if we have a default Printer, do nothing. */
        if (GetDefaultPrinterW(default_printer, &size))
            return TRUE;

        pszPrinter = NULL;
        /* we have no default Printer: search local Printers and use the first */
        if (!create_printers_reg_key( system_printers_key, &hreg ))
        {
            default_printer[0] = '\0';
            size = ARRAY_SIZE(default_printer);
            if (!RegEnumKeyExW( hreg, 0, default_printer, &size, NULL, NULL, NULL, NULL ))
            {
                pszPrinter = default_printer;
                TRACE("using %s\n", debugstr_w(pszPrinter));
            }
            RegCloseKey( hreg );
        }

        if (pszPrinter == NULL) {
            TRACE("no local printer found\n");
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }

    /* "pszPrinter" is never empty or NULL here. */
    namelen = wcslen( pszPrinter );
    size = namelen + (MAX_PATH * 2) + 3; /* printer,driver,port and a 0 */
    buffer = malloc(size * sizeof(WCHAR));
    if (!buffer || create_printers_reg_key( user_printers_key, &hreg ))
    {
        free(buffer);
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
        HKEY hdev;

        if (!create_printers_reg_key( user_default_key, &hdev ))
        {
            RegSetValueExW( hdev, L"device", 0, REG_SZ, (BYTE *)buffer, (wcslen( buffer ) + 1) * sizeof(WCHAR) );
            RegCloseKey(hdev);
        }
    }
    else
    {
        if (lres != ERROR_FILE_NOT_FOUND)
            FIXME("RegQueryValueExW failed with %ld for %s\n", lres, debugstr_w(pszPrinter));

        SetLastError(ERROR_INVALID_PRINTER_NAME);
    }

    RegCloseKey(hreg);
    free(buffer);
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
        bufferW = malloc(len * sizeof(WCHAR));
        if (bufferW) MultiByteToWideChar(CP_ACP, 0, pszPrinter, -1, bufferW, len);
    }
    res = SetDefaultPrinterW(bufferW);
    free(bufferW);
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

    TRACE("(%p, %s, %s %08lx, %p, %08lx)\n", hPrinter, debugstr_a(pKeyName),
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

    TRACE("(%p, %s, %s %08lx, %p, %08lx)\n", hPrinter, debugstr_w(pKeyName),
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
    return SetPrinterDataExW( hPrinter, L"PrinterDriverData", pValueName, Type,
                              pData, cbData );
}

/******************************************************************************
 *		GetPrinterDataExA   (WINSPOOL.@)
 */
DWORD WINAPI GetPrinterDataExA(HANDLE hPrinter, LPCSTR pKeyName,
			       LPCSTR pValueName, LPDWORD pType,
			       LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    opened_printer_t *printer;
    HKEY printers_key, hkeyPrinter = 0, hkeySubkey = 0;
    DWORD ret;

    TRACE("(%p, %s, %s, %p, %p, %lu, %p)\n", hPrinter, debugstr_a(pKeyName),
            debugstr_a(pValueName), pType, pData, nSize, pcbNeeded);

    printer = get_opened_printer(hPrinter);
    if(!printer) return ERROR_INVALID_HANDLE;

    ret = create_printers_reg_key( system_printers_key, &printers_key );
    if (ret) return ret;

    TRACE("printer->name: %s\n", debugstr_w(printer->name));

    if (printer->name) {

        ret = RegOpenKeyW( printers_key, printer->name, &hkeyPrinter );
        if (ret)
        {
            RegCloseKey( printers_key );
            return ret;
        }
        if((ret = RegOpenKeyA(hkeyPrinter, pKeyName, &hkeySubkey)) != ERROR_SUCCESS) {
            WARN("Can't open subkey %s: %ld\n", debugstr_a(pKeyName), ret);
            RegCloseKey(hkeyPrinter);
            RegCloseKey( printers_key );
            return ret;
        }
    }
    *pcbNeeded = nSize;
    ret = RegQueryValueExA( printer->name ? hkeySubkey : printers_key, pValueName,
                            0, pType, pData, pcbNeeded );

    if (!ret && !pData) ret = ERROR_MORE_DATA;

    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    RegCloseKey( printers_key );

    TRACE("--> %ld\n", ret);
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
    HKEY printers_key, hkeyPrinter = 0, hkeySubkey = 0;
    DWORD ret;

    TRACE("(%p, %s, %s, %p, %p, %lu, %p)\n", hPrinter, debugstr_w(pKeyName),
            debugstr_w(pValueName), pType, pData, nSize, pcbNeeded);

    printer = get_opened_printer(hPrinter);
    if(!printer) return ERROR_INVALID_HANDLE;

    ret = create_printers_reg_key( system_printers_key, &printers_key );
    if (ret) return ret;

    TRACE("printer->name: %s\n", debugstr_w(printer->name));

    if (printer->name)
    {
        ret = RegOpenKeyW( printers_key, printer->name, &hkeyPrinter);
        if (ret)
        {
            RegCloseKey( printers_key );
            return ret;
        }
        if ((ret = RegOpenKeyW(hkeyPrinter, pKeyName, &hkeySubkey)) != ERROR_SUCCESS)
        {
            WARN("Can't open subkey %s: %ld\n", debugstr_w(pKeyName), ret);
            RegCloseKey(hkeyPrinter);
            RegCloseKey( printers_key );
            return ret;
        }
    }
    *pcbNeeded = nSize;
    ret = RegQueryValueExW( printer->name ? hkeySubkey : printers_key, pValueName,
                            0, pType, pData, pcbNeeded );

    if (!ret && !pData) ret = ERROR_MORE_DATA;

    RegCloseKey(hkeySubkey);
    RegCloseKey(hkeyPrinter);
    RegCloseKey( printers_key );

    TRACE("--> %ld\n", ret);
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
    return GetPrinterDataExW( hPrinter, L"PrinterDriverData", pValueName, pType,
                              pData, nSize, pcbNeeded );
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
    PBYTE		    lpValue;
    PPRINTER_ENUM_VALUESW   ppev;

    TRACE ("%p %s\n", hPrinter, debugstr_w (pKeyName));

    if (pKeyName == NULL || *pKeyName == 0)
	return ERROR_INVALID_PARAMETER;

    ret = WINSPOOL_GetOpenedPrinterRegKey (hPrinter, &hkPrinter);
    if (ret != ERROR_SUCCESS)
    {
	TRACE ("WINSPOOL_GetOpenedPrinterRegKey (%p) returned %li\n",
		hPrinter, ret);
	return ret;
    }

    ret = RegOpenKeyExW (hkPrinter, pKeyName, 0, KEY_READ, &hkSubKey);
    if (ret != ERROR_SUCCESS)
    {
	r = RegCloseKey (hkPrinter);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	TRACE ("RegOpenKeyExW (%p, %s) returned %li\n", hPrinter,
		debugstr_w (pKeyName), ret);
	return ret;
    }

    ret = RegCloseKey (hkPrinter);
    if (ret != ERROR_SUCCESS)
    {
	ERR ("RegCloseKey returned %li\n", ret);
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	return ret;
    }

    ret = RegQueryInfoKeyW (hkSubKey, NULL, NULL, NULL, NULL, NULL, NULL,
	    &cValues, &cbMaxValueNameLen, &cbMaxValueLen, NULL, NULL);
    if (ret != ERROR_SUCCESS)
    {
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	TRACE ("RegQueryInfoKeyW (%p) returned %li\n", hkSubKey, ret);
	return ret;
    }

    TRACE ("RegQueryInfoKeyW returned cValues = %li, cbMaxValueNameLen = %li, "
	    "cbMaxValueLen = %li\n", cValues, cbMaxValueNameLen, cbMaxValueLen);

    if (cValues == 0)			/* empty key */
    {
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	*pcbEnumValues = *pnEnumValues = 0;
	return ERROR_SUCCESS;
    }

    ++cbMaxValueNameLen;			/* allow for trailing '\0' */

    lpValueName = malloc (cbMaxValueNameLen * sizeof(WCHAR));
    if (lpValueName == NULL)
    {
	ERR ("Failed to allocate %li WCHARs\n", cbMaxValueNameLen);
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	return ERROR_OUTOFMEMORY;
    }

    lpValue = malloc (cbMaxValueLen);
    if (lpValue == NULL)
    {
	ERR ("Failed to allocate %li bytes\n", cbMaxValueLen);
	free (lpValueName);
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	return ERROR_OUTOFMEMORY;
    }

    TRACE ("pass 1: calculating buffer required for all names and values\n");

    cbBufSize = cValues * sizeof (PRINTER_ENUM_VALUESW);

    TRACE ("%li bytes required for %li headers\n", cbBufSize, cValues);

    for (dwIndex = 0; dwIndex < cValues; ++dwIndex)
    {
	cbValueNameLen = cbMaxValueNameLen; cbValueLen = cbMaxValueLen;
	ret = RegEnumValueW (hkSubKey, dwIndex, lpValueName, &cbValueNameLen,
		NULL, NULL, lpValue, &cbValueLen);
	if (ret != ERROR_SUCCESS)
	{
            free (lpValue);
            free (lpValueName);
	    r = RegCloseKey (hkSubKey);
	    if (r != ERROR_SUCCESS)
		WARN ("RegCloseKey returned %li\n", r);
	    TRACE ("RegEnumValueW (%li) returned %li\n", dwIndex, ret);
	    return ret;
	}

	TRACE ("%s [%li]: name needs %li WCHARs, data needs %li bytes\n",
		debugstr_w (lpValueName), dwIndex,
		cbValueNameLen + 1, cbValueLen);

	cbBufSize += (cbValueNameLen + 1) * sizeof (WCHAR);
	cbBufSize += cbValueLen;
    }

    TRACE ("%li bytes required for all %li values\n", cbBufSize, cValues);

    *pcbEnumValues = cbBufSize;
    *pnEnumValues = cValues;

    if (cbEnumValues < cbBufSize)	/* buffer too small */
    {
        free (lpValue);
        free (lpValueName);
	r = RegCloseKey (hkSubKey);
	if (r != ERROR_SUCCESS)
	    WARN ("RegCloseKey returned %li\n", r);
	TRACE ("%li byte buffer is not large enough\n", cbEnumValues);
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
            free (lpValue);
            free (lpValueName);
	    r = RegCloseKey (hkSubKey);
	    if (r != ERROR_SUCCESS)
		WARN ("RegCloseKey returned %li\n", r);
	    TRACE ("RegEnumValueW (%li) returned %li\n", dwIndex, ret);
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

	TRACE ("%s [%li]: copied name (%li bytes) and data (%li bytes)\n",
		debugstr_w (lpValueName), dwIndex, cbValueNameLen, cbValueLen);
    }

    free (lpValue);
    free (lpValueName);

    ret = RegCloseKey (hkSubKey);
    if (ret != ERROR_SUCCESS)
    {
	ERR ("RegCloseKey returned %li\n", ret);
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
    LPSTR   pBuffer;

    TRACE ("%p %s\n", hPrinter, pKeyName);

    if (pKeyName == NULL || *pKeyName == 0)
	return ERROR_INVALID_PARAMETER;

    len = MultiByteToWideChar (CP_ACP, 0, pKeyName, -1, NULL, 0);
    if (len == 0)
    {
	ret = GetLastError ();
	ERR ("MultiByteToWideChar failed with code %li\n", ret);
	return ret;
    }

    pKeyNameW = malloc (len * sizeof(WCHAR));
    if (pKeyNameW == NULL)
    {
	ERR ("Failed to allocate %li bytes\n",
             (LONG)(len * sizeof (WCHAR)));
	return ERROR_OUTOFMEMORY;
    }

    if (MultiByteToWideChar (CP_ACP, 0, pKeyName, -1, pKeyNameW, len) == 0)
    {
	ret = GetLastError ();
	ERR ("MultiByteToWideChar failed with code %li\n", ret);
        free (pKeyNameW);
	return ret;
    }

    ret = EnumPrinterDataExW (hPrinter, pKeyNameW, pEnumValues, cbEnumValues,
	    pcbEnumValues, pnEnumValues);
    free (pKeyNameW);
    if (ret != ERROR_SUCCESS)
    {
	TRACE ("EnumPrinterDataExW returned %li\n", ret);
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

    TRACE ("Largest Unicode name or value is %li bytes\n", dwBufSize);

    pBuffer = malloc(dwBufSize);
    if (pBuffer == NULL)
    {
	ERR ("Failed to allocate %li bytes\n", dwBufSize);
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
	    ERR ("WideCharToMultiByte failed with code %li\n", ret);
            free (pBuffer);
	    return ret;
	}

	memcpy (ppev->pValueName, pBuffer, len);

	TRACE ("Converted '%s' from Unicode to ANSI\n", pBuffer);

	if (ppev->dwType != REG_SZ && ppev->dwType != REG_EXPAND_SZ &&
		ppev->dwType != REG_MULTI_SZ)
	    continue;

	len = WideCharToMultiByte (CP_ACP, 0, (LPWSTR) ppev->pData,
		ppev->cbData / sizeof (WCHAR), pBuffer, dwBufSize, NULL, NULL);
	if (len == 0)
	{
	    ret = GetLastError ();
	    ERR ("WideCharToMultiByte failed with code %li\n", ret);
            free (pBuffer);
	    return ret;
	}

	memcpy (ppev->pData, pBuffer, len);

	TRACE ("Converted '%s' from Unicode to ANSI\n", pBuffer);
	TRACE ("  (only first string of REG_MULTI_SZ printed)\n");
    }

    free(pBuffer);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *      AbortPrinter (WINSPOOL.@)
 */
BOOL WINAPI AbortPrinter(HANDLE printer)
{
    HANDLE handle = get_backend_handle(printer);

    TRACE("(%p)\n", printer);

    return backend->fpAbortPrinter(handle);
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
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    if (pMonitorName) {
        len = MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, NULL, 0);
        monitorW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, monitorW, len);
    }
    res = AddPortW(nameW, hWnd, monitorW);
    free(nameW);
    free(monitorW);
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

    TRACE("(%s, %ld, %p, %s): %s\n", debugstr_a(pName), level, pBuffer,
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
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    if (pMonitorName) {
        len = MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, NULL, 0);
        monitorW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pMonitorName, -1, monitorW, len);
    }

    ZeroMemory(&pi2W, sizeof(PORT_INFO_2W));

    if (pi2A->pPortName) {
        len = MultiByteToWideChar(CP_ACP, 0, pi2A->pPortName, -1, NULL, 0);
        pi2W.pPortName = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pi2A->pPortName, -1, pi2W.pPortName, len);
    }

    if (level > 1) {
        if (pi2A->pMonitorName) {
            len = MultiByteToWideChar(CP_ACP, 0, pi2A->pMonitorName, -1, NULL, 0);
            pi2W.pMonitorName = malloc(len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pi2A->pMonitorName, -1, pi2W.pMonitorName, len);
        }

        if (pi2A->pDescription) {
            len = MultiByteToWideChar(CP_ACP, 0, pi2A->pDescription, -1, NULL, 0);
            pi2W.pDescription = malloc(len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pi2A->pDescription, -1, pi2W.pDescription, len);
        }
        pi2W.fPortType = pi2A->fPortType;
        pi2W.Reserved = pi2A->Reserved;
    }

    res = AddPortExW(nameW, level, (LPBYTE) &pi2W, monitorW);

    free(nameW);
    free(monitorW);
    free(pi2W.pPortName);
    free(pi2W.pMonitorName);
    free(pi2W.pDescription);
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

    TRACE("(%s, %ld, %p, %s): %s %s %s\n", debugstr_w(pName), level, pBuffer,
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
    TRACE("(%s, %ld, %p, 0x%lx)\n", debugstr_w(pName), level, pDriverInfo, dwFileCopyFlags);

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

    TRACE("(%s, %ld, %p, 0x%lx)\n", debugstr_a(pName), Level, pDriverInfo, dwFileCopyFlags);

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
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    /* common fields */
    diW.cVersion = diA->cVersion;

    if (diA->pName) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pName, -1, NULL, 0);
        diW.pName = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pName, -1, diW.pName, len);
    }

    if (diA->pEnvironment) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pEnvironment, -1, NULL, 0);
        diW.pEnvironment = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pEnvironment, -1, diW.pEnvironment, len);
    }

    if (diA->pDriverPath) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pDriverPath, -1, NULL, 0);
        diW.pDriverPath = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pDriverPath, -1, diW.pDriverPath, len);
    }

    if (diA->pDataFile) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pDataFile, -1, NULL, 0);
        diW.pDataFile = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pDataFile, -1, diW.pDataFile, len);
    }

    if (diA->pConfigFile) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pConfigFile, -1, NULL, 0);
        diW.pConfigFile = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pConfigFile, -1, diW.pConfigFile, len);
    }

    if ((Level > 2) && diA->pHelpFile) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pHelpFile, -1, NULL, 0);
        diW.pHelpFile = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pHelpFile, -1, diW.pHelpFile, len);
    }

    if ((Level > 2) && diA->pDependentFiles) {
        lenA = multi_sz_lenA(diA->pDependentFiles);
        len = MultiByteToWideChar(CP_ACP, 0, diA->pDependentFiles, lenA, NULL, 0);
        diW.pDependentFiles = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pDependentFiles, lenA, diW.pDependentFiles, len);
    }

    if ((Level > 2) && diA->pMonitorName) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pMonitorName, -1, NULL, 0);
        diW.pMonitorName = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pMonitorName, -1, diW.pMonitorName, len);
    }

    if ((Level > 2) && diA->pDefaultDataType) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pDefaultDataType, -1, NULL, 0);
        diW.pDefaultDataType = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pDefaultDataType, -1, diW.pDefaultDataType, len);
    }

    if ((Level > 3) && diA->pszzPreviousNames) {
        lenA = multi_sz_lenA(diA->pszzPreviousNames);
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszzPreviousNames, lenA, NULL, 0);
        diW.pszzPreviousNames = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszzPreviousNames, lenA, diW.pszzPreviousNames, len);
    }

    if (Level > 5) {
        diW.ftDriverDate = diA->ftDriverDate;
        diW.dwlDriverVersion = diA->dwlDriverVersion;
    }

    if ((Level > 5) && diA->pszMfgName) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszMfgName, -1, NULL, 0);
        diW.pszMfgName = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszMfgName, -1, diW.pszMfgName, len);
    }

    if ((Level > 5) && diA->pszOEMUrl) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszOEMUrl, -1, NULL, 0);
        diW.pszOEMUrl = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszOEMUrl, -1, diW.pszOEMUrl, len);
    }

    if ((Level > 5) && diA->pszHardwareID) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszHardwareID, -1, NULL, 0);
        diW.pszHardwareID = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszHardwareID, -1, diW.pszHardwareID, len);
    }

    if ((Level > 5) && diA->pszProvider) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszProvider, -1, NULL, 0);
        diW.pszProvider = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszProvider, -1, diW.pszProvider, len);
    }

    if ((Level > 7) && diA->pszPrintProcessor) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszPrintProcessor, -1, NULL, 0);
        diW.pszPrintProcessor = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszPrintProcessor, -1, diW.pszPrintProcessor, len);
    }

    if ((Level > 7) && diA->pszVendorSetup) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszVendorSetup, -1, NULL, 0);
        diW.pszVendorSetup = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszVendorSetup, -1, diW.pszVendorSetup, len);
    }

    if ((Level > 7) && diA->pszzColorProfiles) {
        lenA = multi_sz_lenA(diA->pszzColorProfiles);
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszzColorProfiles, lenA, NULL, 0);
        diW.pszzColorProfiles = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszzColorProfiles, lenA, diW.pszzColorProfiles, len);
    }

    if ((Level > 7) && diA->pszInfPath) {
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszInfPath, -1, NULL, 0);
        diW.pszInfPath = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszInfPath, -1, diW.pszInfPath, len);
    }

    if ((Level > 7) && diA->pszzCoreDriverDependencies) {
        lenA = multi_sz_lenA(diA->pszzCoreDriverDependencies);
        len = MultiByteToWideChar(CP_ACP, 0, diA->pszzCoreDriverDependencies, lenA, NULL, 0);
        diW.pszzCoreDriverDependencies = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, diA->pszzCoreDriverDependencies, lenA, diW.pszzCoreDriverDependencies, len);
    }

    if (Level > 7) {
        diW.dwPrinterDriverAttributes = diA->dwPrinterDriverAttributes;
        diW.ftMinInboxDriverVerDate = diA->ftMinInboxDriverVerDate;
        diW.dwlMinInboxDriverVerVersion = diA->dwlMinInboxDriverVerVersion;
    }

    res = AddPrinterDriverExW(nameW, Level, (LPBYTE) &diW, dwFileCopyFlags);
    TRACE("got %u with %lu\n", res, GetLastError());
    free(nameW);
    free(diW.pName);
    free(diW.pEnvironment);
    free(diW.pDriverPath);
    free(diW.pDataFile);
    free(diW.pConfigFile);
    free(diW.pHelpFile);
    free(diW.pDependentFiles);
    free(diW.pMonitorName);
    free(diW.pDefaultDataType);
    free(diW.pszzPreviousNames);
    free(diW.pszMfgName);
    free(diW.pszOEMUrl);
    free(diW.pszHardwareID);
    free(diW.pszProvider);
    free(diW.pszPrintProcessor);
    free(diW.pszVendorSetup);
    free(diW.pszzColorProfiles);
    free(diW.pszInfPath);
    free(diW.pszzCoreDriverDependencies);

    TRACE("=> %u with %lu\n", res, GetLastError());
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
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    /* convert portname to unicode */
    if (pPortName) {
        len = MultiByteToWideChar(CP_ACP, 0, pPortName, -1, NULL, 0);
        portW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pPortName, -1, portW, len);
    }

    res = ConfigurePortW(nameW, hWnd, portW);
    free(nameW);
    free(portW);
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
    FIXME("%p %lx\n", hWnd, Flags);
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

    TRACE("%s %s %s %lx %lx\n", debugstr_w(pName), debugstr_w(pEnvironment),
          debugstr_w(pDriverName), dwDeleteFlag, dwVersionFlag);

    if(pName && pName[0])
    {
        FIXME("pName = %s - unsupported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(dwDeleteFlag)
    {
        FIXME("dwDeleteFlag = %lx - unsupported\n", dwDeleteFlag);
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
BOOL WINAPI EnumFormsA( HANDLE printer, DWORD level, BYTE *form, DWORD size, DWORD *needed, DWORD *count )
{
    const DWORD *string_info = form_string_info( level );
    BOOL ret;
    DWORD i;

    if (!string_info) return FALSE;

    ret = EnumFormsW( printer, level, form, size, needed, count );
    if (ret)
        for (i = 0; i < *count; i++)
            packed_struct_WtoA( form + i * string_info[0], string_info );

    return ret;
}

/******************************************************************************
 *      EnumFormsW (WINSPOOL.@)
 */
BOOL WINAPI EnumFormsW( HANDLE printer, DWORD level, BYTE *form, DWORD size, DWORD *needed, DWORD *count )
{
    HANDLE handle = get_backend_handle( printer );

    TRACE( "(%p, %ld, %p, %ld, %p, %p)\n", printer, level, form, size, needed, count );

    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!needed || !count)
    {
        SetLastError( RPC_X_NULL_REF_POINTER );
        return FALSE;
    }

    if (!form && size)
    {
        SetLastError( ERROR_INVALID_USER_BUFFER );
        return FALSE;
    }

    return backend->fpEnumForms( handle, level, form, size, needed, count );
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

    TRACE("(%s, %ld, %p, %ld, %p, %p)\n", debugstr_a(pName), Level, pMonitors,
          cbBuf, pcbNeeded, pcReturned);

    /* convert servername to unicode */
    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }
    /* alloc (userbuffersize*sizeof(WCHAR) and try to enum the monitors */
    needed = cbBuf * sizeof(WCHAR);    
    if (needed) bufferW = malloc(needed);
    res = EnumMonitorsW(nameW, Level, bufferW, needed, pcbNeeded, pcReturned);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (pcbNeeded) needed = *pcbNeeded;
        /* HeapReAlloc return NULL, when bufferW was NULL */
        bufferW = (bufferW) ? realloc(bufferW, needed) : malloc(needed);

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
            TRACE("%p: parsing #%ld (%s)\n", mi2w, index, debugstr_w(mi2w->pName));

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
            TRACE("%p: writing MONITOR_INFO_%ldA #%ld\n", mi2a, Level, index);
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

    free(nameW);
    free(bufferW);

    TRACE("returning %d with %ld (%ld byte for %ld entries)\n",
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

    TRACE("(%s, %ld, %p, %ld, %p, %p)\n", debugstr_w(pName), Level, pMonitors,
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

    TRACE("(%p, %s, %p, %ld, %p, %ld, %p, %p)\n", hXcv, debugstr_w(pszDataName),
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
    FIXME("%p %lx %p %lx %p %p %p %lx %p\n", hPrinter, dwIndex, pValueName,
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
    FIXME("%p %lx %p %lx %p %p %p %lx %p\n", hPrinter, dwIndex, pValueName,
          cbValueName, pcbValueName, pType, pData, cbData, pcbData);
    return ERROR_NO_MORE_ITEMS;
}

/*****************************************************************************
 *          EnumPrinterKeyA [WINSPOOL.@]
 *
 */
DWORD WINAPI EnumPrinterKeyA(HANDLE printer, const CHAR *key, CHAR *subkey, DWORD size, DWORD *needed)
{
    FIXME("%p %s %p %lx %p\n", printer, debugstr_a(key), subkey, size, needed);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*****************************************************************************
 *          EnumPrinterKeyW [WINSPOOL.@]
 *
 */
DWORD WINAPI EnumPrinterKeyW(HANDLE printer, const WCHAR *key, WCHAR *subkey, DWORD size, DWORD *needed)
{
    FIXME("%p %s %p %lx %p\n", printer, debugstr_w(key), subkey, size, needed);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*****************************************************************************
 *          EnumPrintProcessorDatatypesA [WINSPOOL.@]
 *
 */
BOOL WINAPI EnumPrintProcessorDatatypesA(LPSTR pName, LPSTR pPrintProcessorName,
                                         DWORD Level, LPBYTE pDatatypes, DWORD cbBuf,
                                         LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    FIXME("Stub: %s %s %ld %p %ld %p %p\n", debugstr_a(pName),
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
    FIXME("Stub: %s %s %ld %p %ld %p %p\n", debugstr_w(pName),
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

    TRACE("(%s, %s, %ld, %p, %ld, %p, %p)\n", debugstr_a(pName), debugstr_a(pEnvironment),
                Level, pPPInfo, cbBuf, pcbNeeded, pcReturned);

    /* convert names to unicode */
    if (pName) {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }
    if (pEnvironment) {
        len = MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, NULL, 0);
        envW = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, envW, len);
    }

    /* alloc (userbuffersize*sizeof(WCHAR) and try to enum the monitors */
    needed = cbBuf * sizeof(WCHAR);
    if (needed) bufferW = malloc(needed);
    res = EnumPrintProcessorsW(nameW, envW, Level, bufferW, needed, pcbNeeded, pcReturned);

    if(!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (pcbNeeded) needed = *pcbNeeded;
        /* HeapReAlloc return NULL, when bufferW was NULL */
        bufferW = (bufferW) ? realloc(bufferW, needed) : malloc(needed);

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
            TRACE("%p: parsing #%ld (%s)\n", ppiw, index, debugstr_w(ppiw->pName));

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
            TRACE("%p: writing PRINTPROCESSOR_INFO_1A #%ld\n", ppia, index);
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

    free(nameW);
    free(envW);
    free(bufferW);

    TRACE("returning %d with %ld (%ld byte for %ld entries)\n",
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

    TRACE("(%s, %s, %ld, %p, %ld, %p, %p)\n", debugstr_w(pName), debugstr_w(pEnvironment),
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
    FIXME("Stub: %p %p %p %s %s %p %s %lx\n", hWnd, hInst, pDevModeOutput,
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
    FIXME("Stub: %p %lx %lx %p\n",
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

static inline const DWORD *job_string_info(DWORD level)
{
    static const DWORD info_1[] =
    {
        sizeof(JOB_INFO_1W),
        FIELD_OFFSET(JOB_INFO_1W, pPrinterName),
        FIELD_OFFSET(JOB_INFO_1W, pMachineName),
        FIELD_OFFSET(JOB_INFO_1W, pUserName),
        FIELD_OFFSET(JOB_INFO_1W, pDocument),
        FIELD_OFFSET(JOB_INFO_1W, pDatatype),
        FIELD_OFFSET(JOB_INFO_1W, pStatus),
        ~0u
    };
    static const DWORD info_2[] =
    {
        sizeof(JOB_INFO_2W),
        FIELD_OFFSET(JOB_INFO_2W, pPrinterName),
        FIELD_OFFSET(JOB_INFO_2W, pMachineName),
        FIELD_OFFSET(JOB_INFO_2W, pUserName),
        FIELD_OFFSET(JOB_INFO_2W, pDocument),
        FIELD_OFFSET(JOB_INFO_2W, pNotifyName),
        FIELD_OFFSET(JOB_INFO_2W, pDatatype),
        FIELD_OFFSET(JOB_INFO_2W, pPrintProcessor),
        FIELD_OFFSET(JOB_INFO_2W, pParameters),
        FIELD_OFFSET(JOB_INFO_2W, pDriverName),
        FIELD_OFFSET(JOB_INFO_2W, pStatus),
        ~0u
    };
    static const DWORD info_3[] =
    {
        sizeof(JOB_INFO_3),
        ~0u
    };

    switch (level)
    {
    case 1: return info_1;
    case 2: return info_2;
    case 3: return info_3;
    }

    SetLastError( ERROR_INVALID_LEVEL );
    return NULL;
}

/*****************************************************************************
 *          GetJobA [WINSPOOL.@]
 *
 */
BOOL WINAPI GetJobA(HANDLE printer, DWORD job_id, DWORD level, BYTE *data,
        DWORD size, DWORD *needed)
{
    const DWORD *string_info = job_string_info(level);

    if (!string_info)
        return FALSE;

    if (!GetJobW(printer, job_id, level, data, size, needed))
        return FALSE;
    packed_struct_WtoA(data, string_info);
    if (level == 2)
        DEVMODEWtoA(((JOB_INFO_2W *)data)->pDevMode, ((JOB_INFO_2A *)data)->pDevMode);
    return TRUE;
}

/*****************************************************************************
 *          GetJobW [WINSPOOL.@]
 *
 */
BOOL WINAPI GetJobW(HANDLE printer, DWORD job_id, DWORD level, BYTE *data,
        DWORD size, DWORD *needed)
{
    HANDLE handle = get_backend_handle(printer);

    TRACE("(%p, %ld, %ld, %p, %ld, %p)\n", printer, job_id, level, data, size, needed);

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return backend->fpGetJob(handle, job_id, level, data, size, needed);
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

                filename = malloc((len + 1) * sizeof(WCHAR));
                GetDlgItemTextW(hwnd, EDITBOX, filename, len + 1);

                if(GetFileAttributesW(filename) != INVALID_FILE_ATTRIBUTES)
                {
                    WCHAR caption[200], message[200];
                    int mb_ret;

                    LoadStringW(WINSPOOL_hInstance, IDS_CAPTION, caption, ARRAY_SIZE(caption));
                    LoadStringW(WINSPOOL_hInstance, IDS_FILE_EXISTS, message, ARRAY_SIZE(message));
                    mb_ret = MessageBoxW(hwnd, message, caption, MB_OKCANCEL | MB_ICONEXCLAMATION);
                    if(mb_ret == IDCANCEL)
                    {
                        free(filename);
                        return TRUE;
                    }
                }
                hf = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if(hf == INVALID_HANDLE_VALUE)
                {
                    WCHAR caption[200], message[200];

                    LoadStringW(WINSPOOL_hInstance, IDS_CAPTION, caption, ARRAY_SIZE(caption));
                    LoadStringW(WINSPOOL_hInstance, IDS_CANNOT_OPEN, message, ARRAY_SIZE(message));
                    MessageBoxW(hwnd, message, caption, MB_OK | MB_ICONEXCLAMATION);
                    free(filename);
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
 *          ScheduleJob [WINSPOOL.@]
 *
 */
BOOL WINAPI ScheduleJob(HANDLE printer, DWORD job_id)
{
    HANDLE handle = get_backend_handle(printer);

    TRACE("(%p, %lx)\n", printer, job_id);

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return backend->fpScheduleJob(handle, job_id);
}

/*****************************************************************************
 *          StartDocDlgA [WINSPOOL.@]
 */
LPSTR WINAPI StartDocDlgA( HANDLE hPrinter, DOCINFOA *doc )
{
    UNICODE_STRING usBuffer;
    DOCINFOW docW = { 0 };
    LPWSTR retW;
    LPWSTR docnameW = NULL, outputW = NULL, datatypeW = NULL;
    LPSTR ret = NULL;

    docW.cbSize = sizeof(docW);
    if (doc->lpszDocName)
    {
        docnameW = asciitounicode(&usBuffer, doc->lpszDocName);
        if (!(docW.lpszDocName = docnameW)) goto failed;
    }
    if (doc->lpszOutput)
    {
        outputW = asciitounicode(&usBuffer, doc->lpszOutput);
        if (!(docW.lpszOutput = outputW)) goto failed;
    }
    if (doc->lpszDatatype)
    {
        datatypeW = asciitounicode(&usBuffer, doc->lpszDatatype);
        if (!(docW.lpszDatatype = datatypeW)) goto failed;
    }
    docW.fwType = doc->fwType;

    retW = StartDocDlgW(hPrinter, &docW);

    if(retW)
    {
        DWORD len = WideCharToMultiByte(CP_ACP, 0, retW, -1, NULL, 0, NULL, NULL);
        ret = heap_alloc(len);
        WideCharToMultiByte(CP_ACP, 0, retW, -1, ret, len, NULL, NULL);
        heap_free(retW);
    }

failed:
    free(datatypeW);
    free(outputW);
    free(docnameW);

    return ret;
}

static BOOL is_port(const WCHAR *port_list, const WCHAR *output)
{
    size_t len;

    if (wcschr(output, ':'))
        return TRUE;

    len = wcslen(output);
    while (port_list && *port_list)
    {
        if (!wcsncmp(output, port_list, len) && (!port_list[len] || port_list[len] == ','))
            return TRUE;

        port_list = wcschr(port_list, ',');
        if (port_list) port_list++;
    }
    return FALSE;
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
    PRINTER_INFO_5W *pi5;
    LPWSTR ret = NULL;
    DWORD len, attr;
    BOOL b;

    GetPrinterW(hPrinter, 5, NULL, 0, &len);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return NULL;
    pi5 = malloc(len);
    GetPrinterW(hPrinter, 5, (LPBYTE)pi5, len, &len);

    /* Check whether default port is FILE: */
    b = !doc->lpszOutput && (!pi5->pPortName || wcscmp( pi5->pPortName, L"FILE:" ));
    if (!b && doc->lpszOutput && wcscmp( doc->lpszOutput, L"FILE:" ))
        b = is_port(pi5->pPortName, doc->lpszOutput);
    free(pi5);
    if (b)
        return NULL;

    if(doc->lpszOutput == NULL || !wcscmp( doc->lpszOutput, L"FILE:" ))
    {
        LPWSTR name;

        if (get_filename(&name))
        {
            if(!(len = GetFullPathNameW(name, 0, NULL, NULL)))
            {
                free(name);
                return NULL;
            }
            ret = heap_alloc(len * sizeof(WCHAR));
            GetFullPathNameW(name, len, ret, NULL);
            free(name);
        }
        return ret;
    }

    if(!(len = GetFullPathNameW(doc->lpszOutput, 0, NULL, NULL)))
        return NULL;

    ret = heap_alloc(len * sizeof(WCHAR));
    GetFullPathNameW(doc->lpszOutput, len, ret, NULL);
        
    attr = GetFileAttributesW(ret);
    if(attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        heap_free(ret);
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
    FIXME("%s, %s, %s, %lx, %p, %p, %p\n", debugstr_a(server), debugstr_a(path), debugstr_a(env),
          flags, hwnd, dst, dstlen);
    return E_NOTIMPL;
}

/*****************************************************************************
 *          UploadPrinterDriverPackageW [WINSPOOL.@]
 */
HRESULT WINAPI UploadPrinterDriverPackageW( LPCWSTR server, LPCWSTR path, LPCWSTR env,
                                            DWORD flags, HWND hwnd, LPWSTR dst, PULONG dstlen )
{
    FIXME("%s, %s, %s, %lx, %p, %p, %p\n", debugstr_w(server), debugstr_w(path), debugstr_w(env),
          flags, hwnd, dst, dstlen);
    return E_NOTIMPL;
}

/*****************************************************************************
 *          PerfOpen [WINSPOOL.@]
 */
DWORD WINAPI PerfOpen(LPWSTR context)
{
    FIXME("%s: stub\n", debugstr_w(context));
    return ERROR_SUCCESS;
}

/*****************************************************************************
 *          PerfClose [WINSPOOL.@]
 */
DWORD WINAPI PerfClose(void)
{
    FIXME("stub\n");
    return ERROR_SUCCESS;
}

/*****************************************************************************
 *          PerfCollect [WINSPOOL.@]
 */
DWORD WINAPI PerfCollect(LPWSTR query, LPVOID *data, LPDWORD size, LPDWORD obj_count)
{
    FIXME("%s, %p, %p, %p: stub\n", debugstr_w(query), data, size, obj_count);
    *size = 0;
    *obj_count = 0;
    return ERROR_SUCCESS;
}

/*****************************************************************************
 *          GetSpoolFileHandle [WINSPOOL.@]
 */
HANDLE WINAPI GetSpoolFileHandle( HANDLE printer )
{
    FIXME( "%p: stub\n", printer );
    return INVALID_HANDLE_VALUE;
}

/*****************************************************************************
 *          SeekPrinter [WINSPOOL.@]
 */
BOOL WINAPI SeekPrinter(HANDLE printer, LARGE_INTEGER distance,
        LARGE_INTEGER *pos, DWORD method, BOOL bwrite)
{
    HANDLE handle = get_backend_handle(printer);

    TRACE("(%p %I64d %p %lx %x)\n", printer, distance.QuadPart, pos, method, bwrite);

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    return backend->fpSeekPrinter(handle, distance, pos, method, bwrite);
}
