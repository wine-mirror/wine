/*
 * Copyright 2014 Austin English
 * Copyright 2023 Hans Leidekker for CodeWeavers
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

#define COBJMACROS

#include "initguid.h"
#include "objidl.h"
#include "wbemcli.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systeminfo);

enum format_flags
{
    FORMAT_STRING,
    FORMAT_DATE,
    FORMAT_LOCALE,
    FORMAT_SIZE,
};

struct sysinfo
{
    const WCHAR *item;
    const WCHAR *class;
    const WCHAR *property; /* hardcoded value if class is NULL */
    void (*callback)( IWbemServices *services, enum format_flags flags, UINT32 ); /* called if class and property are NULL */
    enum format_flags flags;
};

static void output_processors( IWbemServices *services, enum format_flags flags, UINT32 width )
{
    IEnumWbemClassObject *iter;
    IWbemClassObject *obj;
    DWORD i, num_cpus = 0, count;
    VARIANT value;
    BSTR str;
    HRESULT hr;

    str = SysAllocString( L"Win32_Processor" );
    hr = IWbemServices_CreateInstanceEnum( services, str, 0, NULL, &iter );
    SysFreeString( str );
    if (FAILED( hr )) return;

    while (IEnumWbemClassObject_Skip( iter, WBEM_INFINITE, 1 ) == S_OK) num_cpus++;

    fwprintf( stdout, L"Processor(s):%*s %u Processor(s) Installed.\n", width - wcslen(L"Processor(s)"), " ", num_cpus );
    IEnumWbemClassObject_Reset( iter );

    for (i = 0; i < num_cpus; i++)
    {
        hr = IEnumWbemClassObject_Next( iter, WBEM_INFINITE, 1, &obj, &count );
        if (FAILED( hr )) goto done;

        hr = IWbemClassObject_Get( obj, L"Caption", 0, &value, NULL, NULL );
        if (FAILED( hr ))
        {
            IWbemClassObject_Release( obj );
            goto done;
        }
        fwprintf( stdout, L"%*s[%02u]: %s", width + 2, " ", i + 1, V_BSTR(&value) );
        VariantClear( &value );

        hr = IWbemClassObject_Get( obj, L"Manufacturer", 0, &value, NULL, NULL );
        if (FAILED( hr ))
        {
            IWbemClassObject_Release( obj );
            goto done;
        }
        fwprintf( stdout, L" %s", V_BSTR(&value) );
        VariantClear( &value );

        hr = IWbemClassObject_Get( obj, L"MaxClockSpeed", 0, &value, NULL, NULL );
        if (FAILED( hr ))
        {
            IWbemClassObject_Release( obj );
            goto done;
        }
        fwprintf( stdout, L" ~%u Mhz\n", V_I4(&value) );

        IWbemClassObject_Release( obj );
    }

done:
    IEnumWbemClassObject_Release( iter );
}

static void output_hotfixes( IWbemServices *services, enum format_flags flags, UINT32 width )
{
    IEnumWbemClassObject *iter;
    IWbemClassObject *obj;
    DWORD i, num_hotfixes = 0, count;
    VARIANT value;
    BSTR str;
    HRESULT hr;

    str = SysAllocString( L"Win32_QuickFixEngineering" );
    hr = IWbemServices_CreateInstanceEnum( services, str, 0, NULL, &iter );
    SysFreeString( str );
    if (FAILED( hr )) return;

    while (IEnumWbemClassObject_Skip( iter, WBEM_INFINITE, 1 ) == S_OK) num_hotfixes++;

    fwprintf( stdout, L"Hotfix(es):%*s %u Hotfix(es) Installed.\n", width - wcslen(L"Hotfix(es)"), " ", num_hotfixes );
    IEnumWbemClassObject_Reset( iter );

    for (i = 0; i < num_hotfixes; i++)
    {
        hr = IEnumWbemClassObject_Next( iter, WBEM_INFINITE, 1, &obj, &count );
        if (FAILED( hr )) goto done;

        hr = IWbemClassObject_Get( obj, L"Caption", 0, &value, NULL, NULL );
        if (FAILED( hr ))
        {
            IWbemClassObject_Release( obj );
            goto done;
        }
        fwprintf( stdout, L"%*s[%02u]: %s\n", width + 2, " ", i + 1, V_BSTR(&value) );
        VariantClear( &value );

        IWbemClassObject_Release( obj );
    }

done:
    IEnumWbemClassObject_Release( iter );
}

static void output_nics( IWbemServices *services, enum format_flags flags, UINT32 width )
{
    IEnumWbemClassObject *iter;
    IWbemClassObject *obj;
    DWORD i, num_nics = 0, count;
    VARIANT value;
    SAFEARRAY *sa;
    LONG bound = -1, j;
    BSTR str;
    HRESULT hr;

    str = SysAllocString( L"Win32_NetworkAdapterConfiguration" );
    hr = IWbemServices_CreateInstanceEnum( services, str, 0, NULL, &iter );
    SysFreeString( str );
    if (FAILED( hr )) return;

    while (IEnumWbemClassObject_Skip( iter, WBEM_INFINITE, 1 ) == S_OK) num_nics++;

    fwprintf( stdout, L"Network Card(s):%*s %u NICs(s) Installed.\n", width - wcslen(L"Network Card(s)"), " ", num_nics );
    IEnumWbemClassObject_Reset( iter );

    for (i = 0; i < num_nics; i++)
    {
        hr = IEnumWbemClassObject_Next( iter, WBEM_INFINITE, 1, &obj, &count );
        if (FAILED( hr )) goto done;

        hr = IWbemClassObject_Get( obj, L"Description", 0, &value, NULL, NULL );
        if (FAILED( hr ))
        {
            IWbemClassObject_Release( obj );
            goto done;
        }
        fwprintf( stdout, L"%*s[%02u]: %s\n", width + 2, " ", i + 1, V_BSTR(&value) );
        VariantClear( &value );

        /* FIXME: Connection Name, DHCP Server */

        hr = IWbemClassObject_Get( obj, L"DHCPEnabled", 0, &value, NULL, NULL );
        if (FAILED( hr ))
        {
            IWbemClassObject_Release( obj );
            goto done;
        }
        fwprintf( stdout, L"%*s      DHCP Enabled: %s\n", width + 2, " ", V_BOOL(&value) ? L"Yes" : L"No" );

        hr = IWbemClassObject_Get( obj, L"IPAddress", 0, &value, NULL, NULL );
        if (FAILED( hr ))
        {
            IWbemClassObject_Release( obj );
            goto done;
        }
        if (V_VT( &value ) == (VT_BSTR | VT_ARRAY))
        {
            sa = V_ARRAY( &value );
            SafeArrayGetUBound( sa, 1, &bound );
            if (bound >= 0)
            {
                fwprintf( stdout, L"%*s      IP Addresse(es)\n", width + 2, " " );
                for (j = 0; j <= bound; j++)
                {
                    SafeArrayGetElement( sa, &j, &str );
                    fwprintf( stdout, L"%*s      [%02u]: %s\n", width + 2, " ", j + 1, str );
                    SysFreeString( str );
                }
            }
        }
        VariantClear( &value );
        IWbemClassObject_Release( obj );
    }

done:
    IEnumWbemClassObject_Release( iter );
}

static void output_timezone( IWbemServices *services, enum format_flags flags, UINT32 width )
{
    WCHAR name[64], timezone[256] = {};
    DWORD count = sizeof(name);
    HKEY key_current = 0, key_timezones = 0, key_name = 0;

    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\TimeZoneInformation", 0,
                       KEY_READ, &key_current )) goto done;
    if (RegQueryValueExW( key_current, L"TimeZoneKeyName", NULL, NULL, (BYTE *)name, &count )) goto done;
    if (RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones", 0,
                       KEY_READ, &key_timezones )) goto done;
    if (RegOpenKeyExW( key_timezones, name, 0, KEY_READ, &key_name )) goto done;
    count = sizeof(timezone);
    RegQueryValueExW( key_name, L"Display", NULL, NULL, (BYTE *)timezone, &count );

done:
    fwprintf( stdout, L"Time Zone:%*s %s\n", width - wcslen(L"Time Zone"), " ", timezone );
    RegCloseKey( key_name );
    RegCloseKey( key_timezones );
    RegCloseKey( key_current );
}

static const struct sysinfo sysinfo_map[] =
{
    { L"Host Name", L"Win32_ComputerSystem", L"Name" },
    { L"OS Name", L"Win32_OperatingSystem", L"Caption" },
    { L"OS Version", L"Win32_OperatingSystem", L"Version" }, /* FIXME build number */
    { L"OS Manufacturer", L"Win32_OperatingSystem", L"Manufacturer" },
    { L"OS Configuration", NULL, L"Standalone Workstation" },
    { L"OS Build Type", L"Win32_OperatingSystem", L"BuildType" },
    { L"Registered Owner", L"Win32_OperatingSystem", L"RegisteredUser" },
    { L"Registered Organization", L"Win32_OperatingSystem", L"Organization" },
    { L"Product ID", L"Win32_OperatingSystem", L"SerialNumber" },
    { L"Original Install Date", L"Win32_OperatingSystem", L"InstallDate", NULL, FORMAT_DATE },
    { L"System Boot Time", L"Win32_OperatingSystem", L"LastBootUpTime", NULL, FORMAT_DATE },
    { L"System Manufacturer", L"Win32_ComputerSystem", L"Manufacturer" },
    { L"System Model", L"Win32_ComputerSystem", L"Model" },
    { L"System Type", L"Win32_ComputerSystem", L"SystemType" },
    { L"Processor(s)", NULL, NULL, output_processors },
    { L"BIOS Version", L"Win32_BIOS", L"SMBIOSBIOSVersion" },
    { L"Windows Directory", L"Win32_OperatingSystem", L"WindowsDirectory" },
    { L"System Directory", L"Win32_OperatingSystem", L"SystemDirectory" },
    { L"Boot Device", L"Win32_OperatingSystem", L"BootDevice" },
    { L"System Locale", L"Win32_OperatingSystem", L"Locale", NULL, FORMAT_LOCALE },
    { L"Input Locale", L"Win32_OperatingSystem", L"Locale", NULL, FORMAT_LOCALE }, /* FIXME */
    { L"Time Zone", NULL, NULL, output_timezone },
    { L"Total Physical Memory", L"Win32_OperatingSystem", L"TotalVisibleMemorySize", NULL, FORMAT_SIZE },
    { L"Available Physical Memory", L"Win32_OperatingSystem", L"FreePhysicalMemory", NULL, FORMAT_SIZE },
    { L"Virtual Memory: Max Size", L"Win32_OperatingSystem", L"TotalVirtualMemorySize", NULL, FORMAT_SIZE },
    { L"Virtual Memory: Available", L"Win32_OperatingSystem", L"FreeVirtualMemory", NULL, FORMAT_SIZE },
    /* FIXME Virtual Memory: In Use */
    { L"Page File Location(s)", L"Win32_PageFileUsage", L"Name" },
    { L"Domain", L"Win32_ComputerSystem", L"Domain" },
    /* FIXME Logon Server */
    { L"Hotfix(s)", NULL, NULL, output_hotfixes },
    { L"Network Card(s)", NULL, NULL, output_nics },
    /* FIXME Hyper-V Requirements */
};

static void output_item( IWbemServices *services, const struct sysinfo *info, UINT32 width )
{
    HRESULT hr;
    IWbemClassObject *obj = NULL;
    BSTR str;
    VARIANT value;

    if (!info->class)
    {
        if (info->property)
            fwprintf( stdout, L"%s:%*s %s\n", info->item, width - wcslen(info->item), " ", info->property );
        else
            info->callback( services, info->flags, width );
        return;
    }

    if (!(str = SysAllocString( info->class ))) return;
    hr = IWbemServices_GetObject( services, str, 0, NULL, &obj, NULL );
    SysFreeString( str );
    if (FAILED( hr )) return;

    hr = IWbemClassObject_Get( obj, info->property, 0, &value, NULL, NULL );
    if (FAILED( hr ))
    {
        IWbemClassObject_Release( obj );
        return;
    }

    switch (info->flags)
    {
    case FORMAT_DATE:
    {
        SYSTEMTIME st;
        WCHAR date[32] = {}, time[32] = {};

        /* assume UTC */
        memset( &st, 0, sizeof(st) );
        swscanf( V_BSTR(&value), L"%04u%02u%02u%02u%02u%02u",
                 &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond );
        GetDateFormatW( LOCALE_SYSTEM_DEFAULT, 0, &st, NULL, date, ARRAY_SIZE(date) );
        GetTimeFormatW( LOCALE_SYSTEM_DEFAULT, 0, &st, NULL, time, ARRAY_SIZE(time) );
        fwprintf( stdout, L"%s:%*s %s, %s\n", info->item, width - wcslen(info->item), " ", date, time );
        break;
    }
    case FORMAT_LOCALE:
    {
        UINT32 lcid;
        WCHAR name[32] = {}, displayname[LOCALE_NAME_MAX_LENGTH] = {};

        swscanf( V_BSTR(&value), L"%x", &lcid );
        LCIDToLocaleName( lcid, name, ARRAY_SIZE(name), 0 );
        GetLocaleInfoW( lcid, LOCALE_SENGLISHDISPLAYNAME, displayname, ARRAY_SIZE(displayname) );
        fwprintf( stdout, L"%s:%*s %s;%s\n", info->item, width - wcslen(info->item), " ", name, displayname );
        break;
    }
    case FORMAT_SIZE:
    {
        UINT64 size = 0;
        swscanf( V_BSTR(&value), L"%I64u", &size );
        fwprintf( stdout, L"%s:%*s %I64u MB\n", info->item, width - wcslen(info->item), " ", size / 1024 );
        break;
    }
    default:
        fwprintf( stdout, L"%s:%*s %s\n", info->item, width - wcslen(info->item), " ", V_BSTR(&value) );
        break;
    }
    VariantClear( &value );
}

static void output_sysinfo( void )
{
    IWbemLocator *locator;
    IWbemServices *services = NULL;
    UINT32 i, len, width = 0;
    HRESULT hr;
    BSTR path;

    CoInitialize( NULL );
    CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL );

    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (void **)&locator );
    if (hr != S_OK) return;

    if (!(path = SysAllocString( L"ROOT\\CIMV2" ))) goto done;
    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    SysFreeString( path );
    if (hr != S_OK) goto done;

    for (i = 0; i < ARRAY_SIZE(sysinfo_map); i++) if ((len = wcslen( sysinfo_map[i].item )) > width) width = len;
    width++;

    for (i = 0; i < ARRAY_SIZE(sysinfo_map); i++) output_item( services, &sysinfo_map[i], width );

done:
    if (services) IWbemServices_Release( services );
    IWbemLocator_Release( locator );
    CoUninitialize();
}

int __cdecl wmain( int argc, WCHAR *argv[] )
{
    if (argc > 1)
    {
        int i;
        FIXME( "stub:" );
        for (i = 0; i < argc; i++) FIXME( " %s", wine_dbgstr_w(argv[i]) );
        FIXME( "\n" );
        return 0;
    }

    output_sysinfo();
    return 0;
}
