/*
 * Wine Conformance Test EXE
 *
 * Copyright 2003, 2004 Jakob Eriksson   (for Solid Form Sweden AB)
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003 Ferenc Wagner
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
 *
 * This program is dedicated to Anna Lindh,
 * Swedish Minister of Foreign Affairs.
 * Anna was murdered September 11, 2003.
 *
 */

#define COBJMACROS
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <commctrl.h>
#include <winternl.h>
#include <mshtml.h>

#include "winetest.h"
#include "resource.h"

/* Don't submit the results if more than SKIP_LIMIT tests have been skipped */
#define SKIP_LIMIT 10

/* Don't submit the results if more than FAILURES_LIMIT tests have failed */
#define FAILURES_LIMIT 50

/* Maximum output size for individual test */
#define MAX_OUTPUT_SIZE (32 * 1024)

struct wine_test
{
    char *name;
    int subtest_count;
    char **subtests;
    char *exename;
    char *maindllpath;
};

char *tag = NULL;
char *description = NULL;
char *url = NULL;
char *email = NULL;
BOOL aborting = FALSE;
static struct wine_test *wine_tests;
static int nr_of_files, nr_of_tests, nr_of_skips;
static int nr_native_dlls;
static const char whitespace[] = " \t\r\n";
static const char testexe[] = "_test.exe";
static char build_id[64];
static BOOL is_wow64;
static int failures;
static int quiet_mode;
static HANDLE logfile;
static HANDLE junit;

/* filters for running only specific tests */
static char **filters;
static unsigned int nb_filters;
static unsigned int alloc_filters;
static BOOL exclude_tests = FALSE;

/* Needed to check for .NET dlls */
static HMODULE hmscoree;
static HRESULT (WINAPI *pLoadLibraryShim)(LPCWSTR, LPCWSTR, LPVOID, HMODULE *);

/* For SxS DLLs e.g. msvcr90 */
static HANDLE (WINAPI *pCreateActCtxA)(PACTCTXA);
static BOOL (WINAPI *pActivateActCtx)(HANDLE, ULONG_PTR *);
static BOOL (WINAPI *pDeactivateActCtx)(DWORD, ULONG_PTR);
static void (WINAPI *pReleaseActCtx)(HANDLE);

/* To store the current PATH setting (related to .NET only provided dlls) */
static char *curpath;

/* check if test is being filtered out */
static BOOL test_filtered_out( LPCSTR module, LPCSTR testname )
{
    char *p, dllname[MAX_PATH];
    unsigned int i, len;

    strcpy( dllname, module );
    CharLowerA( dllname );
    p = strstr( dllname, testexe );
    if (p) *p = 0;
    len = strlen(dllname);

    if (!nb_filters) return exclude_tests;
    for (i = 0; i < nb_filters; i++)
    {
        if (!strncmp( dllname, filters[i], len ))
        {
            if (!filters[i][len]) return exclude_tests;
            if (filters[i][len] != ':') continue;
            if (testname && !strcmp( testname, &filters[i][len+1] )) return exclude_tests;
            if (!testname && !exclude_tests) return FALSE;
        }
    }
    return !exclude_tests;
}

static void add_filter( const char *name )
{
    if (name[0] == '@')
    {
        char *p, *str, buffer[256];
        FILE *f = fopen( name + 1, "rt" );
        if (!f) return;

        while (fgets( buffer, sizeof(buffer), f ))
        {
            p = buffer;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '#') continue;
            str = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
            *p = 0;
            add_filter( str );
        }
        fclose( f );
        return;
    }

    if (nb_filters >= alloc_filters)
    {
        alloc_filters = max( alloc_filters * 2, 64 );
        filters = xrealloc( filters, alloc_filters * sizeof(*filters) );
    }
    filters[nb_filters++] = xstrdup(name);
}

static HANDLE create_output_file( const char *name )
{
    SECURITY_ATTRIBUTES sa;
    HANDLE file;

    /* make handle inheritable */
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    file = CreateFileA( name, GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        &sa, CREATE_ALWAYS, 0, NULL );

    if (file == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        /* FILE_SHARE_DELETE not supported on win9x */
        file = CreateFileA( name, GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            &sa, CREATE_ALWAYS, 0, NULL );
    }
    return file;
}

static HANDLE create_temp_file( char name[MAX_PATH] )
{
    char tmpdir[MAX_PATH];

    if (!GetTempPathA( MAX_PATH, tmpdir ) ||
        !GetTempFileNameA( tmpdir, "out", 0, name ))
        report (R_FATAL, "Can't name temp file.");

    return create_output_file( name );
}

static void close_temp_file( const char *name, HANDLE file )
{
    CloseHandle( file );
    DeleteFileA( name );
}

static char *flush_temp_file( const char *name, HANDLE file, DWORD *retsize )
{
    DWORD size = SetFilePointer( file, 0, NULL, FILE_CURRENT );
    char *buffer = xalloc( size + 1 );

    SetFilePointer( file, 0, NULL, FILE_BEGIN );
    if (!ReadFile( file, buffer, size, retsize, NULL )) *retsize = 0;
    close_temp_file( name, file );
    buffer[*retsize] = 0;
    return buffer;
}

static char * get_file_version(char * file_name)
{
    static char version[32];
    DWORD size;
    DWORD handle;

    size = GetFileVersionInfoSizeA(file_name, &handle);
    if (size) {
        char * data = xalloc(size);
        if (GetFileVersionInfoA(file_name, handle, size, data)) {
            static const char backslash[] = "\\";
            VS_FIXEDFILEINFO *pFixedVersionInfo;
            UINT len;
            if (VerQueryValueA(data, backslash, (LPVOID *)&pFixedVersionInfo, &len)) {
                sprintf(version, "%ld.%ld.%ld.%ld",
                        pFixedVersionInfo->dwFileVersionMS >> 16,
                        pFixedVersionInfo->dwFileVersionMS & 0xffff,
                        pFixedVersionInfo->dwFileVersionLS >> 16,
                        pFixedVersionInfo->dwFileVersionLS & 0xffff);
            } else
                sprintf(version, "version not found");
        } else
            sprintf(version, "version error %lu", GetLastError());
        free(data);
    } else if (GetLastError() == ERROR_FILE_NOT_FOUND)
        sprintf(version, "dll is missing");
    else
        sprintf(version, "version not present %lu", GetLastError());

    return version;
}

static BOOL running_under_wine (void)
{
    HMODULE module = GetModuleHandleA("ntdll.dll");

    if (!module) return FALSE;
    return (GetProcAddress(module, "wine_server_call") != NULL);
}

static BOOL check_mount_mgr(void)
{
    HANDLE handle = CreateFileA( "\\\\.\\MountPointManager", GENERIC_READ,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE) return FALSE;
    CloseHandle( handle );
    return TRUE;
}

static BOOL check_wow64_registry(void)
{
    char buffer[MAX_PATH];
    DWORD type, size = MAX_PATH;
    HKEY hkey;
    BOOL ret;

    if (!is_wow64) return TRUE;
    if (RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", &hkey ))
        return FALSE;
    ret = !RegQueryValueExA( hkey, "ProgramFilesDir (x86)", NULL, &type, (BYTE *)buffer, &size );
    RegCloseKey( hkey );
    return ret;
}

static BOOL check_display_driver(void)
{
    HWND hwnd = CreateWindowA( "STATIC", "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                               0, 0, GetModuleHandleA(0), 0 );
    if (!hwnd) return FALSE;
    DestroyWindow( hwnd );
    return TRUE;
}

static BOOL running_on_visible_desktop (void)
{
    HWND desktop;
    HMODULE huser32 = GetModuleHandleA("user32.dll");
    HWINSTA (WINAPI *pGetProcessWindowStation)(void);
    BOOL (WINAPI *pGetUserObjectInformationA)(HANDLE,INT,LPVOID,DWORD,LPDWORD);

    pGetProcessWindowStation = (void *)GetProcAddress(huser32, "GetProcessWindowStation");
    pGetUserObjectInformationA = (void *)GetProcAddress(huser32, "GetUserObjectInformationA");

    desktop = GetDesktopWindow();
    if (!GetWindowLongPtrW(desktop, GWLP_WNDPROC)) /* Win9x */
        return IsWindowVisible(desktop);

    if (pGetProcessWindowStation && pGetUserObjectInformationA)
    {
        DWORD len;
        HWINSTA wstation;
        USEROBJECTFLAGS uoflags;

        wstation = pGetProcessWindowStation();
        assert(pGetUserObjectInformationA(wstation, UOI_FLAGS, &uoflags, sizeof(uoflags), &len));
        return (uoflags.dwFlags & WSF_VISIBLE) != 0;
    }
    return IsWindowVisible(desktop);
}

static int running_as_admin (void)
{
    PSID administrators = NULL;
    SID_IDENTIFIER_AUTHORITY nt_authority = { SECURITY_NT_AUTHORITY };
    HANDLE token;
    DWORD groups_size;
    PTOKEN_GROUPS groups;
    DWORD group_index;

    /* Create a well-known SID for the Administrators group. */
    if (! AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                   &administrators))
        return -1;

    /* Get the process token */
    if (! OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        FreeSid(administrators);
        return -1;
    }

    /* Get the group info from the token */
    groups_size = 0;
    GetTokenInformation(token, TokenGroups, NULL, 0, &groups_size);
    groups = xalloc(groups_size);
    if (! GetTokenInformation(token, TokenGroups, groups, groups_size, &groups_size))
    {
        free(groups);
        CloseHandle(token);
        FreeSid(administrators);
        return -1;
    }
    CloseHandle(token);

    /* Now check if the token groups include the Administrators group */
    for (group_index = 0; group_index < groups->GroupCount; group_index++)
    {
        if (EqualSid(groups->Groups[group_index].Sid, administrators))
        {
            free(groups);
            FreeSid(administrators);
            return 1;
        }
    }

    /* If we end up here we didn't find the Administrators group */
    free(groups);
    FreeSid(administrators);
    return 0;
}

static int running_elevated (void)
{
    HANDLE token;
    TOKEN_ELEVATION elevation_info;
    DWORD size;

    /* Get the process token */
    if (! OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return -1;

    /* Get the elevation info from the token */
    if (! GetTokenInformation(token, TokenElevation, &elevation_info,
                              sizeof(TOKEN_ELEVATION), &size))
    {
        CloseHandle(token);
        return -1;
    }
    CloseHandle(token);

    return elevation_info.TokenIsElevated;
}

/* check for native dll when running under wine */
static BOOL is_native_dll( HMODULE module )
{
    static const char builtin_signature[] = "Wine builtin DLL";
    static const char fakedll_signature[] = "Wine placeholder DLL";
    const IMAGE_DOS_HEADER *dos;

    if (!running_under_wine()) return FALSE;
    if (!((ULONG_PTR)module & 1)) return FALSE;  /* not loaded as datafile */
    /* builtin dlls can't be loaded as datafile, so we must have native or fake dll */
    dos = (const IMAGE_DOS_HEADER *)((const char *)module - 1);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    if (dos->e_lfanew >= sizeof(*dos) + 32)
    {
        if (!memcmp( dos + 1, builtin_signature, sizeof(builtin_signature) )) return FALSE;
        if (!memcmp( dos + 1, fakedll_signature, sizeof(fakedll_signature) )) return FALSE;
    }
    return TRUE;
}

/*
 * Windows 8 has a concept of stub DLLs.  When DLLMain is called the user is prompted
 *  to install that component.  To bypass this check we need to look at the version resource.
 */
static BOOL is_stub_dll(const char *filename)
{
    UINT size;
    DWORD ver;
    BOOL isstub = FALSE;
    char *p, *data;

    size = GetFileVersionInfoSizeA(filename, &ver);
    if (!size) return FALSE;

    data = xalloc(size);
    if (GetFileVersionInfoA(filename, ver, size, data))
    {
        char buf[256];

        sprintf(buf, "\\StringFileInfo\\%04x%04x\\OriginalFilename", MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 1200);
        if (VerQueryValueA(data, buf, (void**)&p, &size))
            isstub = !lstrcmpiA("wcodstub.dll", p);
    }
    free(data);

    return isstub;
}

static int disable_crash_dialog(void)
{
    HKEY key;
    DWORD type, data, size;
    int ret = 0;

    if (RegCreateKeyA( HKEY_CURRENT_USER, "Software\\Wine\\WineDbg", &key )) return 0;
    size = sizeof(data);
    if (RegQueryValueExA( key, "ShowCrashDialog", NULL, &type, (BYTE *)&data, &size )) ret = 1;
    else if (type != REG_DWORD || data) ret = 2;
    data = 0;
    RegSetValueExA( key, "ShowCrashDialog", 0, REG_DWORD, (BYTE *)&data, sizeof(data) );
    RegCloseKey( key );
    return ret;
}

static void restore_crash_dialog( int prev )
{
    HKEY key;
    DWORD data = 1;

    if (!prev) return;
    if (RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\WineDbg", &key )) return;
    if (prev == 1) RegDeleteKeyValueA( key, NULL, "ShowCrashDialog" );
    else RegSetValueExA( key, "ShowCrashDialog", 0, REG_DWORD, (BYTE *)&data, sizeof(data) );
    RegCloseKey( key );
}

static void print_version (void)
{
#ifdef __i386__
    static const char platform[] = "i386";
#elif defined(__x86_64__)
    static const char platform[] = "x86_64";
#elif defined(__arm__)
    static const char platform[] = "arm";
#elif defined(__aarch64__)
    static const char platform[] = "arm64";
#else
# error CPU unknown
#endif
    OSVERSIONINFOEXA ver;
    RTL_OSVERSIONINFOEXW rtlver;
    BOOL ext;
    int is_win2k3_r2, is_admin, is_elevated;
    const char *(CDECL *wine_get_build_id)(void);
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    void (CDECL *wine_get_host_version)( const char **sysname, const char **release );
    BOOL (WINAPI *pGetProductInfo)(DWORD, DWORD, DWORD, DWORD, DWORD *);
    NTSTATUS (WINAPI *pRtlGetVersion)(RTL_OSVERSIONINFOEXW *);

    ver.dwOSVersionInfoSize = sizeof(ver);
    if (!(ext = GetVersionExA ((OSVERSIONINFOA *) &ver)))
    {
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	if (!GetVersionExA ((OSVERSIONINFOA *) &ver))
	    report (R_FATAL, "Can't get OS version.");
    }

    /* try to get non-faked values */
    if (ver.dwMajorVersion == 6 && ver.dwMinorVersion == 2)
    {
        rtlver.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

        pRtlGetVersion = (void *)GetProcAddress(hntdll, "RtlGetVersion");
        pRtlGetVersion(&rtlver);

        ver.dwMajorVersion = rtlver.dwMajorVersion;
        ver.dwMinorVersion = rtlver.dwMinorVersion;
        ver.dwBuildNumber = rtlver.dwBuildNumber;
        ver.dwPlatformId = rtlver.dwPlatformId;
        ver.wServicePackMajor = rtlver.wServicePackMajor;
        ver.wServicePackMinor = rtlver.wServicePackMinor;
        ver.wSuiteMask = rtlver.wSuiteMask;
        ver.wProductType = rtlver.wProductType;

        WideCharToMultiByte(CP_ACP, 0, rtlver.szCSDVersion, -1, ver.szCSDVersion, sizeof(ver.szCSDVersion), NULL, NULL);
    }

    xprintf ("    Platform=%s%s\n", platform, is_wow64 ? " (WOW64)" : "");
    xprintf ("    bRunningUnderWine=%d\n", running_under_wine ());
    xprintf ("    bRunningOnVisibleDesktop=%d\n", running_on_visible_desktop ());
    is_admin = running_as_admin ();
    if (0 <= is_admin)
    {
        xprintf ("    Account=%s", is_admin ? "admin" : "non-admin");
        is_elevated = running_elevated ();
        if (0 <= is_elevated)
            xprintf(", %s", is_elevated ? "elevated" : "not elevated");
        xprintf ("\n");
    }
    if (email)
        xprintf ("    Submitter=%s\n", email );
    if (description)
        xprintf ("    Description=%s\n", description );
    if (url)
        xprintf ("    URL=%s\n", url );
    xprintf ("    dwMajorVersion=%lu\n    dwMinorVersion=%lu\n"
             "    dwBuildNumber=%lu\n    PlatformId=%lu\n    szCSDVersion=%s\n",
             ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber,
             ver.dwPlatformId, ver.szCSDVersion);

    wine_get_build_id = (void *)GetProcAddress(hntdll, "wine_get_build_id");
    wine_get_host_version = (void *)GetProcAddress(hntdll, "wine_get_host_version");
    if (wine_get_build_id) xprintf( "    WineBuild=%s\n", wine_get_build_id() );
    if (wine_get_host_version)
    {
        const char *sysname, *release;
        wine_get_host_version( &sysname, &release );
        xprintf( "    Host system=%s\n    Host version=%s\n", sysname, release );
    }
    is_win2k3_r2 = GetSystemMetrics(SM_SERVERR2);
    if(is_win2k3_r2)
        xprintf("    R2 build number=%d\n", is_win2k3_r2);

    if (!ext) return;

    xprintf ("    wServicePackMajor=%d\n    wServicePackMinor=%d\n"
             "    wSuiteMask=%d\n    wProductType=%d\n    wReserved=%d\n",
             ver.wServicePackMajor, ver.wServicePackMinor, ver.wSuiteMask,
             ver.wProductType, ver.wReserved);

    pGetProductInfo = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"),"GetProductInfo");
    if (pGetProductInfo && !running_under_wine())
    {
        DWORD prodtype = 0;

        pGetProductInfo(ver.dwMajorVersion, ver.dwMinorVersion, ver.wServicePackMajor, ver.wServicePackMinor, &prodtype);
        xprintf("    dwProductInfo=%lu\n", prodtype);
    }
}

static void print_language(void)
{
    HMODULE hkernel32;
    BOOL (WINAPI *pGetSystemPreferredUILanguages)(DWORD, PULONG, PZZWSTR, PULONG);
    LANGID (WINAPI *pGetUserDefaultUILanguage)(void);
    LANGID (WINAPI *pGetThreadUILanguage)(void);

    xprintf ("    SystemDefaultLCID=%04lx\n", GetSystemDefaultLCID());
    xprintf ("    UserDefaultLCID=%04lx\n", GetUserDefaultLCID());
    xprintf ("    ThreadLocale=%04lx\n", GetThreadLocale());

    hkernel32 = GetModuleHandleA("kernel32.dll");
    pGetSystemPreferredUILanguages = (void*)GetProcAddress(hkernel32, "GetSystemPreferredUILanguages");
    pGetUserDefaultUILanguage = (void*)GetProcAddress(hkernel32, "GetUserDefaultUILanguage");
    pGetThreadUILanguage = (void*)GetProcAddress(hkernel32, "GetThreadUILanguage");

    if (pGetSystemPreferredUILanguages && !running_under_wine())
    {
        WCHAR langW[32];
        ULONG num, size = ARRAY_SIZE(langW);
        if (pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, &num, langW, &size))
        {
            char lang[32], *p = lang;
            WideCharToMultiByte(CP_ACP, 0, langW, size, lang, sizeof(lang), NULL, NULL);
            for (p += strlen(p) + 1; *p != '\0'; p += strlen(p) + 1) *(p - 1) = ',';
            xprintf ("    SystemPreferredUILanguages=%s\n", lang);
        }
    }
    if (pGetUserDefaultUILanguage)
        xprintf ("    UserDefaultUILanguage=%04x\n", pGetUserDefaultUILanguage());
    if (pGetThreadUILanguage)
        xprintf ("    ThreadUILanguage=%04x\n", pGetThreadUILanguage());
    xprintf ("    KeyboardLayout=%p\n", GetKeyboardLayout(0));
    xprintf ("    Country=%ld\n", GetUserGeoID(GEOCLASS_NATION));
    xprintf ("    ACP=%d\n", GetACP());
}

static inline BOOL is_dot_dir(const char* x)
{
    return ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))));
}

static void remove_dir (const char *dir)
{
    HANDLE  hFind;
    WIN32_FIND_DATAA wfd;
    char path[MAX_PATH];
    size_t dirlen = strlen (dir);

    /* Make sure the directory exists before going further */
    memcpy (path, dir, dirlen);
    strcpy (path + dirlen++, "\\*");
    hFind = FindFirstFileA (path, &wfd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        char *lp = wfd.cFileName;

        if (!lp[0]) lp = wfd.cAlternateFileName; /* ? FIXME not (!lp) ? */
        if (is_dot_dir (lp)) continue;
        strcpy (path + dirlen, lp);
        if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
            remove_dir(path);
        else if (!DeleteFileA(path))
            report (R_WARNING, "Can't delete file %s: error %d",
                    path, GetLastError ());
    } while (FindNextFileA(hFind, &wfd));
    FindClose (hFind);
    if (!RemoveDirectoryA(dir))
        report (R_WARNING, "Can't remove directory %s: error %d",
                dir, GetLastError ());
}

static const char* get_test_source_file(const char* test, const char* subtest)
{
    static char buffer[MAX_PATH];
    int len = strlen(test);

    if (len > 4 && !strcmp( test + len - 4, ".exe" ) &&
        strcmp( test, "ntoskrnl.exe" )) /* the one exception! */
    {
        len = sprintf(buffer, "programs/%s", test) - 4;
        buffer[len] = 0;
    }
    else len = sprintf(buffer, "dlls/%s", test);

    sprintf(buffer + len, "/tests/%s.c", subtest);
    return buffer;
}

static void* extract_rcdata (LPCSTR name, LPCSTR type, DWORD* size)
{
    HRSRC rsrc;
    HGLOBAL hdl;
    LPVOID addr;
    
    if (!(rsrc = FindResourceA(NULL, name, type)) ||
        !(*size = SizeofResource (0, rsrc)) ||
        !(hdl = LoadResource (0, rsrc)) ||
        !(addr = LockResource (hdl)))
        return NULL;
    return addr;
}

/* Fills in the name and exename fields */
static void
extract_test (struct wine_test *test, const char *dir, LPSTR res_name)
{
    BYTE* code;
    DWORD size;
    char *exepos;
    HANDLE hfile;
    DWORD written;

    code = extract_rcdata (res_name, "TESTRES", &size);
    if (!code) report (R_FATAL, "Can't find test resource %s: %d",
                       res_name, GetLastError ());
    test->name = xstrdup( res_name );
    test->exename = strmake("%s\\%s", dir, test->name);
    exepos = strstr (test->name, testexe);
    if (!exepos) report (R_FATAL, "Not an .exe file: %s", test->name);
    *exepos = 0;
    test->name = xrealloc(test->name, exepos - test->name + 1);
    report (R_STEP, "Extracting: %s", test->name);

    hfile = CreateFileA(test->exename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfile == INVALID_HANDLE_VALUE)
        report (R_FATAL, "Failed to open file %s.", test->exename);

    if (!WriteFile(hfile, code, size, &written, NULL))
        report (R_FATAL, "Failed to write file %s.", test->exename);

    CloseHandle(hfile);
}

static HANDLE get_admin_token(void)
{
    TOKEN_ELEVATION_TYPE type;
    TOKEN_LINKED_TOKEN linked;
    DWORD size;

    if (!GetTokenInformation(GetCurrentThreadEffectiveToken(), TokenElevationType, &type, sizeof(type), &size)
            || type == TokenElevationTypeFull)
        return NULL;

    if (!GetTokenInformation(GetCurrentThreadEffectiveToken(), TokenLinkedToken, &linked, sizeof(linked), &size))
        return NULL;
    return linked.LinkedToken;
}

static DWORD wait_process( HANDLE process, DWORD timeout )
{
    DWORD wait, diff = 0, start = GetTickCount();
    MSG msg;

    while (diff < timeout)
    {
        wait = MsgWaitForMultipleObjects( 1, &process, FALSE, timeout - diff, QS_ALLINPUT );
        if (wait != WAIT_OBJECT_0 + 1) return wait;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        diff = GetTickCount() - start;
    }
    return WAIT_TIMEOUT;
}

static void append_path( const char *path)
{
    char *newpath = strmake( "%s;%s", curpath, path );
    SetEnvironmentVariableA("PATH", newpath);
    free(newpath);
}

/* Run a command for MS milliseconds.  If OUT != NULL, also redirect
   stdout to there.

   Return the exit status, -2 if can't create process or the return
   value of WaitForSingleObject.
 */
static int
run_ex (char *cmd, HANDLE out_file, const char *tempdir, DWORD ms, BOOL nocritical, DWORD* pid)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD wait, status, flags;
    UINT old_errmode;
    HANDLE token = NULL;

    GetStartupInfoA (&si);
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle( STD_INPUT_HANDLE );
    si.hStdOutput = out_file;
    si.hStdError  = out_file;
    if (nocritical)
    {
        old_errmode = SetErrorMode(0);
        SetErrorMode(old_errmode | SEM_FAILCRITICALERRORS);
        flags = 0;
    }
    else
        flags = CREATE_DEFAULT_ERROR_MODE;

    /* Some tests cause a UAC prompt on Windows if the user is not elevated,
     * so to allow them to run unattended the relevant test units need to skip
     * the tests if so.
     *
     * However, we still want Wine to run as many tests as possible, so always
     * elevate ourselves. On Wine elevation isn't interactive. */
    if (running_under_wine())
        token = get_admin_token();

    if (!CreateProcessAsUserA(token, NULL, cmd, NULL, NULL, TRUE, flags, NULL, tempdir, &si, &pi))
    {
        if (nocritical) SetErrorMode(old_errmode);
        if (pid) *pid = 0;
        return -2;
    }

    if (nocritical) SetErrorMode(old_errmode);
    CloseHandle (pi.hThread);
    if (pid) *pid = pi.dwProcessId;
    status = wait_process( pi.hProcess, ms );
    switch (status)
    {
    case WAIT_OBJECT_0:
        GetExitCodeProcess (pi.hProcess, &status);
        CloseHandle (pi.hProcess);
        return status;
    case WAIT_FAILED:
        report (R_ERROR, "Wait for '%s' failed: %d", cmd, GetLastError ());
        break;
    case WAIT_TIMEOUT:
        break;
    default:
        report (R_ERROR, "Wait returned %d", status);
        break;
    }
    if (!TerminateProcess (pi.hProcess, 257))
        report (R_ERROR, "TerminateProcess failed: %d", GetLastError ());
    wait = wait_process( pi.hProcess, 5000 );
    switch (wait)
    {
    case WAIT_OBJECT_0:
        break;
    case WAIT_FAILED:
        report (R_ERROR, "Wait for termination of '%s' failed: %d", cmd, GetLastError ());
        break;
    case WAIT_TIMEOUT:
        report (R_ERROR, "Can't kill process '%s'", cmd);
        break;
    default:
        report (R_ERROR, "Waiting for termination: %d", wait);
        break;
    }
    CloseHandle (pi.hProcess);
    return status;
}

static DWORD
get_subtests (const char *tempdir, struct wine_test *test, LPSTR res_name)
{
    char *cmd;
    HANDLE subfile;
    DWORD err, total;
    char *buffer, *index;
    static const char header[] = "Valid test names:";
    int status, allocated;
    char subname[MAX_PATH];

    test->subtest_count = 0;

    subfile = create_temp_file( subname );
    if (subfile == INVALID_HANDLE_VALUE) return GetLastError();

    cmd = strmake("%s --list", test->exename);
    if (test->maindllpath) {
        /* We need to add the path (to the main dll) to PATH */
        append_path(test->maindllpath);
    }
    status = run_ex (cmd, subfile, tempdir, 5000, TRUE, NULL);
    err = GetLastError();
    if (test->maindllpath) {
        /* Restore PATH again */
        SetEnvironmentVariableA("PATH", curpath);
    }
    free(cmd);

    if (status)
    {
        close_temp_file( subname, subfile );
        return status == -2 ? err : status;
    }

    buffer = flush_temp_file( subname, subfile, &total );
    index = strstr (buffer, header);
    if (!index) {
        report (R_ERROR, "Can't parse subtests output of %s",
                test->name);
        return ERROR_INTERNAL_ERROR;
    }
    index += sizeof header;

    allocated = 10;
    test->subtests = xalloc(allocated * sizeof(char*));
    index = strtok (index, whitespace);
    while (index) {
        if (test->subtest_count == allocated) {
            allocated *= 2;
            test->subtests = xrealloc(test->subtests, allocated * sizeof(char*));
        }
        test->subtests[test->subtest_count++] = xstrdup(index);
        index = strtok (NULL, whitespace);
    }
    test->subtests = xrealloc(test->subtests, test->subtest_count * sizeof(char*));
    free( buffer );
    return 0;
}

static char *xmlescape( const char *src, const char *end, int comment )
{
    char *dst = calloc( 1, (end - src) * 6 + 1 ), *tmp;

    while (end > src && (end[-1] == '\r' || end[-1] == '\n')) end--;
    for (tmp = dst; tmp && src < end; src++)
    {
        if (comment && *src == '-') tmp += sprintf( tmp, "&#45;" );
        else if (comment) *tmp++ = *src;
        else if (*src == '&') tmp += sprintf( tmp, "&amp;" );
        else if (*src == '"') tmp += sprintf( tmp, "&quot;" );
        else if (*src == '<') tmp += sprintf( tmp, "&lt;" );
        else if (*src == '>') tmp += sprintf( tmp, "&gt;" );
        else if (*src < ' ' && *src != '\t' && *src != '\r' && *src != '\n')
        {
            char *esc = strmake( "\\x%02x", *src );
            tmp += sprintf( tmp, "%s", esc );
            free( esc );
        }
        else *tmp++ = *src;
    }

    return dst;
}

static void report_test_start( struct wine_test *test, const char *subtest, const char *file )
{
    report( R_STEP, "Running: %s:%s", test->name, subtest );
    xprintf( "%s:%s start %s\n", test->name, subtest, file );
}

/* filter out color escapes and null characters from test output data */
static void *filter_data( const char *data, DWORD size, DWORD *output_size )
{
    DWORD i, j, eol, ignore = 0;
    char *ret;

    if (!(ret = malloc( size + 1 ))) return NULL;
    for (i = j = 0, eol = -1; i < size; i++)
    {
        if (data[i] == '\x1b' && data[i + 1] == '[')
        {
            while (data[i] && data[i] != 'm') i++;
            eol = i;
        }
        else if (data[i]) ret[j++] = data[i];
        if (!strncmp( data + i, " Test succeeded", 15 )) ignore += i + 15 - eol;
        if (data[i] == '\n') eol = i;
    }
    ret[j] = 0;
    *output_size = j - ignore;

    return ret;
}

static void report_test_done( struct wine_test *test, const char *subtest, const char *file, DWORD pid, DWORD ticks,
                              HANDLE out_file, UINT status, const char *data, DWORD size, DWORD *output_size )
{
    char *filtered_data;

    if (!(filtered_data = filter_data( data, size, output_size ))) return;

    if (quiet_mode <= 1 || status || *output_size > MAX_OUTPUT_SIZE) WriteFile( out_file, data, size, &size, NULL );
    xprintf( "%s:%s:%04lx done (%d) in %lds %luB\n", test->name, subtest, pid, status, ticks / 1000, size );
    if (*output_size > MAX_OUTPUT_SIZE) xprintf( "%s:%s:%04lx The test prints too much data (%lu bytes)\n", test->name, subtest, pid, size );

    if (filtered_data && junit)
    {
        int total = 0, fail_total = 0, skip_total = 0, failures = 0;
        const char *next, *line, *ptr, *dir = strrchr( file, '/' );
        float time, last_time = 0;

        for (line = next = filtered_data; *line; line = next)
        {
            int count, todo_count, flaky_count, fail_count, skip_count;

            if ((next = strchr( next, '\n' ))) next += 1;
            else next = line + strlen( line );
            if (!(ptr = strchr( line, ' ' ))) continue;

            if (sscanf( ptr, " %d tests executed (%d marked as todo, %d as flaky, %d failur%*[^)]), %d skipped.",
                        &count, &todo_count, &flaky_count, &fail_count, &skip_count ) == 5)
            {
                total += count;
                fail_total += fail_count;
                skip_total += skip_count;
            }
        }

        output( junit, "  <testsuite name=\"%s:%s\" file=\"%s\" time=\"%f\" tests=\"%d\" failures=\"%d\" skipped=\"%d\">\n",
                test->name, subtest, file, ticks / 1000.0, total, fail_total, skip_total );

        for (line = next = filtered_data; *line; line = next)
        {
            struct { const char *pattern; int length; int error; } patterns[] =
            {
                #define X(x, y) {.pattern = x, .length = strlen(x), .error = y}
                X("Tests skipped: ", -1),
                X("Test failed: ", 1),
                X("Test succeeded inside todo block: ", 1),
                X("Test succeeded inside flaky todo block: ", 1),
                #undef X
            };
            char src[256], *name, *message;
            int i, n, len;

            if ((next = strchr( next, '\n' ))) next += 1;
            else next = line + strlen( line );

            if ((len = sscanf( line, "%255[^:\n]:%d:%f", src, &n, &time )) != 2 && len != 3) continue;
            if (len == 2) time = last_time;

            if (!(ptr = strchr( line, ' ' ))) continue;
            ptr++;

            message = xmlescape( ptr, next, 0 );
            name = strmake( "%s:%d %s", src, n, message );
            for (i = 0; i < ARRAY_SIZE(patterns); i++)
            {
                if (!strncmp( ptr, patterns[i].pattern, patterns[i].length ))
                {
                    output( junit, "    <testcase classname=\"%s:%s\" name=\"%s:%s %s\" file=\"%.*s%s#L%d\" assertions=\"1\" time=\"%f\">",
                             test->name, subtest, test->name, subtest, name, (int)(dir - file + 1), file, src, n, time - last_time );
                    if (patterns[i].error == -1) output( junit, "<system-out>%s</system-out><skipped/>", message );
                    else if (patterns[i].error == 1) output( junit, "<system-out>%s</system-out><failure/>", message );
                    output( junit, "</testcase>\n" );

                    if (patterns[i].error == 1) failures++;
                    last_time = time;
                    break;
                }
            }
            free( name );
            free( message );
        }

        if (total > fail_total + skip_total)
        {
            output( junit, "    <testcase classname=\"%s:%s\" name=\"%d succeeding tests\" file=\"%s\" assertions=\"%d\" time=\"%f\"/>\n",
                     test->name, subtest, total - fail_total - skip_total, file, total - fail_total - skip_total, ticks / 1000.0 - last_time );
        }

        if (status == WAIT_TIMEOUT)
        {
            output( junit, "    <testcase classname=\"%s:%s\" name=\"%s:%s timeout\" file=\"%s\" assertions=\"%d\" time=\"%f\">",
                     test->name, subtest, test->name, subtest, file, total, ticks / 1000.0 );
            output( junit, "<system-out>Test timeout after %f seconds</system-out><failure/>", ticks / 1000.0 );
            output( junit, "</testcase>\n" );
        }
        else if (status != failures)
        {
            output( junit, "    <testcase classname=\"%s:%s\" name=\"%s:%s status %d\" file=\"%s\" assertions=\"%d\" time=\"%f\">",
                     test->name, subtest, test->name, subtest, status, file, total, ticks / 1000.0 );
            output( junit, "<system-out>Test exited with status %d</system-out><failure/>", status );
            output( junit, "</testcase>\n" );
        }
        if (*output_size > MAX_OUTPUT_SIZE)
        {
            output( junit, "    <testcase classname=\"%s:%s\" name=\"%s:%s output overflow\" file=\"%s\" assertions=\"%d\" time=\"%f\">",
                     test->name, subtest, test->name, subtest, file, total, ticks / 1000.0 );
            output( junit, "<system-out>Test prints too much data (%lukB > %ukB)</system-out><failure/>",
                    size / 1024, MAX_OUTPUT_SIZE / 1024 );
            output( junit, "</testcase>\n" );
        }

        output( junit, "  </testsuite>\n" );
    }

    free( filtered_data );
}

static void report_test_skip( struct wine_test *test, const char *subtest, const char *file )
{
    report( R_STEP, "Skipping: %s:%s", test->name, subtest );
    xprintf( "%s:%s skipped %s\n", test->name, subtest, file );
}

static void
run_test (struct wine_test* test, const char* subtest, HANDLE out_file, const char *tempdir)
{
    /* Build the source filename so analysis tools can link to it */
    const char* file = get_test_source_file(test->name, subtest);

    if (test_filtered_out( test->name, subtest ))
    {
        report_test_skip( test, subtest, file );
        nr_of_skips++;
    }
    else
    {
        char *data, tmpname[MAX_PATH];
        HANDLE tmpfile = create_temp_file( tmpname );
        int status;
        DWORD pid, size, output_size = 0, start = GetTickCount();
        char *cmd = strmake("%s %s", test->exename, subtest);

        report_test_start( test, subtest, file );
        /* Flush to disk so we know which test caused Windows to crash if it does */
        FlushFileBuffers(out_file);

        status = run_ex( cmd, tmpfile, tempdir, 120000, FALSE, &pid );
        if (status == -2 && GetLastError()) status = -GetLastError();
        free(cmd);

        data = flush_temp_file( tmpname, tmpfile, &size );
        report_test_done( test, subtest, file, pid, GetTickCount() - start, out_file, status, data, size, &output_size );
        free( data );

        if (status || output_size > MAX_OUTPUT_SIZE) failures++;
    }
    if (failures) report (R_STATUS, "Running tests - %u failures", failures);
}

static BOOL CALLBACK
EnumTestFileProc (HMODULE hModule, LPCSTR lpszType,
                  LPSTR lpszName, LONG_PTR lParam)
{
    if (!test_filtered_out( lpszName, NULL )) (*(int*)lParam)++;
    return TRUE;
}

static const struct clsid_mapping
{
    const char *name;
    CLSID clsid;
} clsid_list[] =
{
    {"oledb32", {0xc8b522d1, 0x5cf3, 0x11ce, {0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d}}},
    {NULL, {0, 0, 0, {0,0,0,0,0,0,0,0}}}
};


static BOOL get_main_clsid(const char *name, CLSID *clsid)
{
    const struct clsid_mapping *mapping;

    for(mapping = clsid_list; mapping->name; mapping++)
    {
        if(!strcasecmp(name, mapping->name))
        {
            *clsid = mapping->clsid;
            return TRUE;
        }
    }
    return FALSE;
}

static HMODULE load_com_dll(const char *name, char **path, char *filename)
{
    HMODULE dll = NULL;
    HKEY hkey;
    char keyname[100];
    char dllname[MAX_PATH];
    char *p;
    CLSID clsid;

    if(!get_main_clsid(name, &clsid)) return NULL;

    sprintf(keyname, "CLSID\\{%08lx-%04x-%04x-%02x%2x-%02x%2x%02x%2x%02x%2x}\\InprocServer32",
            clsid.Data1, clsid.Data2, clsid.Data3, clsid.Data4[0], clsid.Data4[1],
            clsid.Data4[2], clsid.Data4[3], clsid.Data4[4], clsid.Data4[5],
            clsid.Data4[6], clsid.Data4[7]);

    if(RegOpenKeyA(HKEY_CLASSES_ROOT, keyname, &hkey) == ERROR_SUCCESS)
    {
        LONG size = sizeof(dllname);
        if(RegQueryValueA(hkey, NULL, dllname, &size) == ERROR_SUCCESS)
        {
            if ((dll = LoadLibraryExA(dllname, NULL, LOAD_LIBRARY_AS_DATAFILE)))
            {
                strcpy( filename, dllname );
                p = strrchr(dllname, '\\');
                if (p) *p = 0;
                *path = xstrdup( dllname );
            }
        }
        RegCloseKey(hkey);
    }

    return dll;
}

static void get_dll_path(HMODULE dll, char **path, char *filename)
{
    char dllpath[MAX_PATH];

    GetModuleFileNameA(dll, dllpath, MAX_PATH);
    strcpy(filename, dllpath);
    *strrchr(dllpath, '\\') = '\0';
    *path = xstrdup( dllpath );
}

static BOOL CALLBACK
extract_test_proc (HMODULE hModule, LPCSTR lpszType, LPSTR lpszName, LONG_PTR lParam)
{
    const char *tempdir = (const char *)lParam;
    char dllname[MAX_PATH];
    char filename[MAX_PATH];
    WCHAR dllnameW[MAX_PATH];
    HMODULE dll;
    DWORD err;
    HANDLE actctx;
    ULONG_PTR cookie;
    BOOL run;

    if (aborting) return TRUE;

    /* Check if the main dll is present on this system */
    CharLowerA(lpszName);
    strcpy(dllname, lpszName);
    *strstr(dllname, testexe) = 0;

    if (test_filtered_out( lpszName, NULL ))
    {
        nr_of_skips++;
        if (exclude_tests) xprintf ("    %s=skipped\n", dllname);
        return TRUE;
    }
    extract_test (&wine_tests[nr_of_files], tempdir, lpszName);

    if (pCreateActCtxA != NULL && pActivateActCtx != NULL &&
        pDeactivateActCtx != NULL && pReleaseActCtx != NULL)
    {
        ACTCTXA actctxinfo;
        memset(&actctxinfo, 0, sizeof(ACTCTXA));
        actctxinfo.cbSize = sizeof(ACTCTXA);
        actctxinfo.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        actctxinfo.lpSource = wine_tests[nr_of_files].exename;
        actctxinfo.lpResourceName = (LPSTR)CREATEPROCESS_MANIFEST_RESOURCE_ID;
        actctx = pCreateActCtxA(&actctxinfo);
        if (actctx != INVALID_HANDLE_VALUE &&
            ! pActivateActCtx(actctx, &cookie))
        {
            pReleaseActCtx(actctx);
            actctx = INVALID_HANDLE_VALUE;
        }
    } else actctx = INVALID_HANDLE_VALUE;

    wine_tests[nr_of_files].maindllpath = NULL;
    strcpy(filename, dllname);
    dll = LoadLibraryExA(dllname, NULL, LOAD_LIBRARY_AS_DATAFILE);

    if (!dll) dll = load_com_dll(dllname, &wine_tests[nr_of_files].maindllpath, filename);

    if (!dll && pLoadLibraryShim)
    {
        MultiByteToWideChar(CP_ACP, 0, dllname, -1, dllnameW, MAX_PATH);
        if (SUCCEEDED( pLoadLibraryShim(dllnameW, NULL, NULL, &dll) ) && dll)
        {
            get_dll_path(dll, &wine_tests[nr_of_files].maindllpath, filename);
            FreeLibrary(dll);
            dll = LoadLibraryExA(filename, NULL, LOAD_LIBRARY_AS_DATAFILE);
        }
        else dll = 0;
    }

    run = TRUE;
    if (dll)
    {
        if (is_stub_dll(dllname))
        {
            xprintf ("    %s=dll is a stub\n", dllname);
            run = FALSE;
        }
        else if (is_native_dll(dll))
        {
            xprintf ("    %s=dll is native\n", dllname);
            nr_native_dlls++;
            run = FALSE;
        }
        FreeLibrary(dll);
    }

    if (run)
    {
        err = get_subtests( tempdir, &wine_tests[nr_of_files], lpszName );
        switch (err)
        {
        case 0:
            xprintf ("    %s=%s\n", dllname, get_file_version(filename));
            nr_of_tests += wine_tests[nr_of_files].subtest_count;
            nr_of_files++;
            break;
        case STATUS_DLL_NOT_FOUND:
            xprintf ("    %s=dll is missing\n", dllname);
            /* or it is a side-by-side dll but the test has no manifest */
            break;
        case STATUS_ORDINAL_NOT_FOUND:
            xprintf ("    %s=dll is missing an ordinal (%s)\n", dllname, get_file_version(filename));
            break;
        case STATUS_ENTRYPOINT_NOT_FOUND:
            xprintf ("    %s=dll is missing an entrypoint (%s)\n", dllname, get_file_version(filename));
            break;
        case ERROR_SXS_CANT_GEN_ACTCTX:
            xprintf ("    %s=dll is missing the requested side-by-side version\n", dllname);
            break;
        default:
            xprintf ("    %s=load error %lu\n", dllname, err);
            break;
        }
    }

    if (actctx != INVALID_HANDLE_VALUE)
    {
        pDeactivateActCtx(0, cookie);
        pReleaseActCtx(actctx);
    }
    return TRUE;
}

static char *
run_tests (char *logname, char *outdir)
{
    int i;
    char *strres, *eol, *nextline;
    DWORD strsize;
    char tmppath[MAX_PATH], tempdir[MAX_PATH+4];
    BOOL newdir;
    DWORD needed;
    HMODULE kernel32;

    /* Get the current PATH only once */
    needed = GetEnvironmentVariableA("PATH", NULL, 0);
    curpath = xalloc(needed);
    GetEnvironmentVariableA("PATH", curpath, needed);

    SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    if (!GetTempPathA( MAX_PATH, tmppath ))
        report (R_FATAL, "Can't name temporary dir (check %%TEMP%%).");

    if (logname)
    {
        if (!strcmp(logname, "-")) logfile = GetStdHandle( STD_OUTPUT_HANDLE );
        else logfile = create_output_file( logname );
    }
    else
    {
        static char tmpname[MAX_PATH];
        logfile = create_temp_file( tmpname );
        logname = tmpname;
    }
    report (R_OUT, "%s", logname);

    if (logfile == INVALID_HANDLE_VALUE)
        report (R_FATAL, "Could not open logfile: %u", GetLastError());

    if (outdir)
    {
        /* Get a full path so it is still valid after a chdir */
        GetFullPathNameA( outdir, ARRAY_SIZE(tempdir), tempdir, NULL );
    }
    else
    {
        strcpy( tempdir, tmppath );
        strcat( tempdir, "wct" ); /* try stable path for ZoneAlarm */
    }
    newdir = CreateDirectoryA( tempdir, NULL );
    if (!newdir && !outdir)
    {
        if (!GetTempFileNameA( tmppath, "wct", 0, tempdir ))
            report (R_FATAL, "Can't name temporary dir (check %%TEMP%%).");
        DeleteFileA( tempdir );
        newdir = CreateDirectoryA( tempdir, NULL );
    }
    if (!newdir && (!outdir || GetLastError() != ERROR_ALREADY_EXISTS))
        report (R_FATAL, "Could not create directory %s (%d)", tempdir, GetLastError());

    report (R_DIR, "%s", tempdir);

    if (junit)
    {
        output( junit, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
        output( junit, "<testsuites>\n" );
    }
    xprintf ("Version 4\n");
    xprintf ("Tests from build %s\n", build_id[0] ? build_id : "-" );
    xprintf ("Archive: -\n");  /* no longer used */
    xprintf ("Tag: %s\n", tag);
    xprintf ("Build info:\n");
    strres = extract_rcdata ("BUILD_INFO", "STRINGRES", &strsize);
    while (strres) {
        eol = memchr (strres, '\n', strsize);
        if (!eol) {
            nextline = NULL;
            eol = strres + strsize;
        } else {
            strsize -= eol - strres + 1;
            nextline = strsize?eol+1:NULL;
            if (eol > strres && *(eol-1) == '\r') eol--;
        }
        xprintf ("    %.*s\n", (int)(eol-strres), strres);
        strres = nextline;
    }
    xprintf ("Operating system version:\n");
    print_version ();
    print_language ();
    xprintf ("Dll info:\n" );

    report (R_STATUS, "Counting tests");
    if (!EnumResourceNamesA (NULL, "TESTRES", EnumTestFileProc, (LPARAM)&nr_of_files))
        report (R_FATAL, "Can't enumerate test files: %d",
                GetLastError ());
    wine_tests = xalloc(nr_of_files * sizeof wine_tests[0]);

    /* Do this only once during extraction (and version checking) */
    hmscoree = LoadLibraryA("mscoree.dll");
    pLoadLibraryShim = NULL;
    if (hmscoree)
        pLoadLibraryShim = (void *)GetProcAddress(hmscoree, "LoadLibraryShim");
    kernel32 = GetModuleHandleA("kernel32.dll");
    pCreateActCtxA = (void *)GetProcAddress(kernel32, "CreateActCtxA");
    pActivateActCtx = (void *)GetProcAddress(kernel32, "ActivateActCtx");
    pDeactivateActCtx = (void *)GetProcAddress(kernel32, "DeactivateActCtx");
    pReleaseActCtx = (void *)GetProcAddress(kernel32, "ReleaseActCtx");

    report (R_STATUS, "Extracting tests");
    report (R_PROGRESS, 0, nr_of_files);
    nr_of_files = 0;
    nr_of_tests = 0;
    nr_of_skips = 0;
    if (!EnumResourceNamesA (NULL, "TESTRES", extract_test_proc, (LPARAM)tempdir))
        report (R_FATAL, "Can't enumerate test files: %d",
                GetLastError ());

    FreeLibrary(hmscoree);

    if (aborting) return logname;

    xprintf ("Test output:\n" );

    report (R_DELTA, 0, "Extracting: Done");

    if (nr_native_dlls)
        report( R_WARNING, "Some dlls are configured as native, you won't be able to submit results." );

    report (R_STATUS, "Running tests");
    report (R_PROGRESS, 1, nr_of_tests);
    for (i = 0; i < nr_of_files; i++) {
        struct wine_test *test = wine_tests + i;
        int j;

        if (aborting) break;

        if (test->maindllpath) {
            /* We need to add the path (to the main dll) to PATH */
            append_path(test->maindllpath);
        }

	for (j = 0; j < test->subtest_count; j++) {
            if (aborting) break;
            run_test (test, test->subtests[j], logfile, tempdir);
        }

        if (test->maindllpath) {
            /* Restore PATH again */
            SetEnvironmentVariableA("PATH", curpath);
        }
    }
    report (R_DELTA, 0, "Running: Done");
    if (junit)
    {
        output( junit, "</testsuites>\n" );
        CloseHandle( junit );
    }

    report (R_STATUS, "Cleaning up - %u failures", failures);
    if (strcmp(logname, "-") != 0) CloseHandle( logfile );
    logfile = 0;
    if (newdir)
        remove_dir (tempdir);
    free(wine_tests);
    free(curpath);

    return logname;
}

static BOOL WINAPI ctrl_handler(DWORD ctrl_type)
{
    if (ctrl_type == CTRL_C_EVENT) {
        printf("Ignoring Ctrl-C, use Ctrl-Break if you really want to terminate\n");
        return TRUE;
    }

    return FALSE;
}


static BOOL CALLBACK
extract_only_proc (HMODULE hModule, LPCSTR lpszType, LPSTR lpszName, LONG_PTR lParam)
{
    const char *target_dir = (const char *)lParam;
    char filename[MAX_PATH];

    if (test_filtered_out( lpszName, NULL )) return TRUE;

    strcpy(filename, lpszName);
    CharLowerA(filename);

    extract_test( &wine_tests[nr_of_files], target_dir, filename );
    nr_of_files++;
    return TRUE;
}

static void extract_only (const char *target_dir)
{
    BOOL res;

    report (R_DIR, target_dir);
    res = CreateDirectoryA( target_dir, NULL );
    if (!res && GetLastError() != ERROR_ALREADY_EXISTS)
        report (R_FATAL, "Could not create directory: %s (%d)", target_dir, GetLastError ());

    nr_of_files = 0;
    report (R_STATUS, "Counting tests");
    if (!EnumResourceNamesA(NULL, "TESTRES", EnumTestFileProc, (LPARAM)&nr_of_files))
        report (R_FATAL, "Can't enumerate test files: %d", GetLastError ());

    wine_tests = xalloc(nr_of_files * sizeof wine_tests[0] );

    report (R_STATUS, "Extracting tests");
    report (R_PROGRESS, 0, nr_of_files);
    nr_of_files = 0;
    if (!EnumResourceNamesA(NULL, "TESTRES", extract_only_proc, (LPARAM)target_dir))
        report (R_FATAL, "Can't enumerate test files: %d", GetLastError ());

    report (R_DELTA, 0, "Extracting: Done");
}

static void
usage (void)
{
    fprintf (stderr,
"Usage: winetest [OPTION]... [TESTS]\n\n"
" --help    print this message and exit\n"
" --version print the build version and exit\n"
" -c        console mode, no GUI\n"
" -d DIR    Use DIR as temp directory (default: %%TEMP%%\\wct)\n"
" -e        preserve the environment\n"
" -h        print this message and exit\n"
" -i INFO   an optional description of the test platform\n"
" -m MAIL   an email address to enable developers to contact you\n"
" -n        exclude the specified tests\n"
" -p        shutdown when the tests are done\n"
" -q        quiet mode, no output at all\n"
" -J FILE   output junit XML report into FILE\n"
" -o FILE   put report into FILE, do not submit\n"
" -s FILE   submit FILE, do not run tests\n"
" -S URL    URL to submit the results to\n"
" -t TAG    include TAG of characters [-.0-9a-zA-Z] in the report\n"
" -u URL    include TestBot URL in the report\n"
" -x DIR    Extract tests to DIR (default: .\\wct) and exit\n");
}

int __cdecl main( int argc, char *argv[] )
{
    BOOL (WINAPI *pIsWow64Process)(HANDLE hProcess, PBOOL Wow64Process);
    char *logname = NULL, *outdir = NULL, *path = NULL;
    const char *extract = NULL;
    const char *cp, *submit = NULL, *submiturl = NULL, *job_name;
    int reset_env = 1;
    int poweroff = 0;
    int interactive = 1;
    int prev_crash_dialog = 0;
    int i;

    InitCommonControls();

    if (!LoadStringA( 0, IDS_BUILD_ID, build_id, sizeof(build_id) )) build_id[0] = 0;

    pIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"),"IsWow64Process");
    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    if ((job_name = getenv( "CI_JOB_NAME" )))
    {
        tag = strmake( "gitlab-%s", job_name );
        url = getenv( "CI_JOB_URL" );
    }

    for (i = 1; i < argc && argv[i]; i++)
    {
        if (!strcmp(argv[i], "--help")) {
            usage ();
            exit (0);
        }
        else if (!strcmp(argv[i], "--version")) {
            printf("%-12.12s\n", build_id[0] ? build_id : "unknown");
            exit (0);
        }
        else if ((argv[i][0] != '-' && argv[i][0] != '/') || argv[i][2]) {
            add_filter( argv[i] );
        }
        else switch (argv[i][1]) {
        case 'c':
            report (R_TEXTMODE);
            interactive = 0;
            break;
        case 'e':
            reset_env = 0;
            break;
        case 'h':
        case '?':
            usage ();
            exit (0);
        case 'i':
            if (!(description = argv[++i]))
            {
                usage();
                exit( 2 );
            }
            break;
        case 'J':
            if (!(path = argv[++i]))
            {
                usage();
                exit( 2 );
            }
            junit = create_output_file( path );
            break;
        case 'm':
            if (!(email = argv[++i]))
            {
                usage();
                exit( 2 );
            }
            break;
        case 'n':
            exclude_tests = TRUE;
            break;
        case 'p':
            poweroff = 1;
            break;
        case 'q':
            report (R_QUIET);
            interactive = 0;
            quiet_mode++;
            break;
        case 's':
            if (!(submit = argv[++i]))
            {
                usage();
                exit( 2 );
            }
            break;
        case 'S':
            if (!(submiturl = argv[++i]))
            {
                usage();
                exit( 2 );
            }
            break;
        case 'o':
            if (!(logname = argv[++i]))
            {
                usage();
                exit( 2 );
            }
            break;
        case 't':
            if (!(tag = argv[++i]))
            {
                usage();
                exit( 2 );
            }
            if (strlen (tag) > MAXTAGLEN)
                report (R_FATAL, "tag is too long (maximum %d characters)",
                        MAXTAGLEN);
            break;
        case 'u':
            if (!(url = argv[++i]))
            {
                usage();
                exit( 2 );
            }
            break;
        case 'x':
            report (R_TEXTMODE);
            if (!(extract = argv[++i]))
                extract = ".\\wct";

            extract_only (extract);
            break;
        case 'd':
            outdir = argv[++i];
            break;
        default:
            report (R_ERROR, "invalid option: -%c", argv[i][1]);
            usage ();
            exit (2);
        }
    }
    if (submit) {
        if (tag)
            report (R_WARNING, "ignoring tag for submission");
        send_file (submiturl, submit);

    } else if (!extract) {
        int is_win9x = (GetVersion() & 0x80000000) != 0;

        report (R_STATUS, "Starting up");

        if (is_win9x)
            report (R_WARNING, "Running on win9x is not supported. You won't be able to submit results.");

        if (!running_on_visible_desktop ())
            report (R_FATAL, "Tests must be run on a visible desktop");

        if (running_under_wine())
        {
            if (!check_mount_mgr())
                report (R_FATAL, "Mount manager not running, most likely your WINEPREFIX wasn't created correctly.");

            if (!check_wow64_registry())
                report (R_FATAL, "WoW64 keys missing, most likely your WINEPREFIX wasn't created correctly.");

            if (!check_display_driver())
                report (R_FATAL, "Unable to create a window, the display driver is not working.");

            if (!interactive) prev_crash_dialog = disable_crash_dialog();
        }

        SetConsoleCtrlHandler(ctrl_handler, TRUE);

        if (reset_env)
        {
            SetEnvironmentVariableA( "WINETEST_PLATFORM", running_under_wine () ? "wine" : "windows" );
            SetEnvironmentVariableA( "WINETEST_DEBUG", "1" );
            SetEnvironmentVariableA( "WINETEST_INTERACTIVE", "0" );
            SetEnvironmentVariableA( "WINETEST_REPORT_SUCCESS", "0" );
        }
        if (junit)
        {
            SetEnvironmentVariableA( "WINETEST_TIME", "1" );
        }

        if (nb_filters && !exclude_tests)
        {
            run_tests( logname, outdir );
            exit( failures ? 3 : 0 );
        }

        if (tag && (cp = findbadtagchar(tag)))
        {
            report (R_ERROR, "invalid char in tag: %c", *cp);
            usage ();
            exit (2);
        }

        while (!tag) {
            if (!interactive)
                report (R_FATAL, "Please specify a tag (-t option) if "
                        "running noninteractive!");
            if (guiAskTag () == IDABORT) exit (1);
        }
        report (R_TAG);

        while (!email) {
            if (!interactive)
            {
                if (url) break;
                report (R_FATAL, "Please specify an email address (-m option) to enable developers\n"
                        "    to contact you about your report if necessary.");
            }
            if (guiAskEmail () == IDABORT) exit (1);
        }

        if (!build_id[0])
            report( R_WARNING, "You won't be able to submit results without a valid build id.\n"
                    "To submit results, winetest needs to be built from a git checkout." );

        if (!logname) {
            logname = run_tests (NULL, outdir);
            if (aborting) {
                DeleteFileA(logname);
                exit (0);
            }
            if (failures > FAILURES_LIMIT)
                report( R_WARNING,
                        "%d tests failed. There is probably something broken with your setup.\n"
                        "You need to address this before submitting results.", failures );

            if (build_id[0] && nr_of_skips <= SKIP_LIMIT && failures <= FAILURES_LIMIT &&
                !nr_native_dlls && !is_win9x &&
                report (R_ASK, MB_YESNO, "Do you want to submit the test results?") == IDYES)
            {
                if (!send_file (submiturl, logname) && !DeleteFileA(logname))
                    report (R_WARNING, "Can't remove logfile: %u", GetLastError());
                else
                    failures = 0;  /* return success */
            }
        }
        else
        {
            run_tests (logname, outdir);
            report (R_STATUS, "Finished - %u failures", failures);
        }
        if (prev_crash_dialog) restore_crash_dialog( prev_crash_dialog );
    }
    if (poweroff)
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES npr;

        /* enable the shutdown privilege for the current process */
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        {
            LookupPrivilegeValueA(0, "SeShutdownPrivilege", &npr.Privileges[0].Luid);
            npr.PrivilegeCount = 1;
            npr.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &npr, 0, 0, 0);
            CloseHandle(hToken);
        }
        ExitWindowsEx(EWX_SHUTDOWN | EWX_POWEROFF | EWX_FORCEIFHUNG, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER);
    }
    exit( failures ? 3 : 0 );
}
