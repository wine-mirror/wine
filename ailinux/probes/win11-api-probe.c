/*
 * AILinux Wine Win32/WinRT/Media Foundation behavioral smoke test
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Uses dynamically resolved exports to report missing platform APIs.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <winhttp.h>
#include <stdio.h>
#include <wchar.h>

#ifndef APPMODEL_ERROR_NO_PACKAGE
#define APPMODEL_ERROR_NO_PACKAGE 15700L
#endif
#define MF_VERSION_2_70 0x00020070
#define MFSTARTUP_LITE 1
typedef void *HSTRING;
typedef LONG (WINAPI *package_fn)(UINT32 *, WCHAR *);
typedef HRESULT (WINAPI *roinit_fn)(UINT);
typedef void (WINAPI *rouninit_fn)(void);
typedef HRESULT (WINAPI *create_string_fn)(const WCHAR *, UINT32, HSTRING *);
typedef const WCHAR *(WINAPI *get_string_fn)(HSTRING, UINT32 *);
typedef HRESULT (WINAPI *delete_string_fn)(HSTRING);
typedef HRESULT (WINAPI *mfstartup_fn)(ULONG, DWORD);
typedef HRESULT (WINAPI *mfshutdown_fn)(void);
typedef UINT (WINAPI *dpi_fn)(void);
typedef void * (WINAPI *thread_dpi_fn)(void *);
typedef HINTERNET (WINAPI *winhttp_open_fn)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
typedef BOOL (WINAPI *winhttp_close_fn)(HINTERNET);
static unsigned int passed, failed;
static void check(const char *name, int ok) {
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (ok) ++passed; else ++failed;
}
static FARPROC symbol(HMODULE module, const char *name) {
    return module ? GetProcAddress(module, name) : NULL;
}
static void package_api(void) {
    HMODULE dll = LoadLibraryW(L"kernelbase.dll");
    package_fn call = (package_fn)(void *)symbol(dll, "GetCurrentPackageFullName");
    UINT32 len = 0;
    LONG status = call ? call(&len, NULL) : -1;
    printf("APPMODEL code=%ld length=%u\n", (long)status, (unsigned)len);
    check("AppModel unpackaged desktop process", call && status == APPMODEL_ERROR_NO_PACKAGE);
    if (dll) FreeLibrary(dll);
}
static void winrt_api(void) {
    const WCHAR value[] = L"AILinux-WinRT";
    HSTRING hstr = NULL;
    UINT32 length = 0;
    const WCHAR *data;
    HRESULT init_hr = E_NOTIMPL, create_hr = E_NOTIMPL;
    HMODULE dll = LoadLibraryW(L"combase.dll");
    roinit_fn init = (roinit_fn)(void *)symbol(dll, "RoInitialize");
    rouninit_fn uninit = (rouninit_fn)(void *)symbol(dll, "RoUninitialize");
    create_string_fn create = (create_string_fn)(void *)symbol(dll, "WindowsCreateString");
    get_string_fn raw = (get_string_fn)(void *)symbol(dll, "WindowsGetStringRawBuffer");
    delete_string_fn destroy = (delete_string_fn)(void *)symbol(dll, "WindowsDeleteString");
    if (init) init_hr = init(1); /* RO_INIT_MULTITHREADED */
    if (SUCCEEDED(init_hr) && create)
        create_hr = create(value, (UINT32)wcslen(value), &hstr);
    data = (hstr && raw) ? raw(hstr, &length) : NULL;
    printf("WINRT init=0x%08lx create=0x%08lx len=%u\n",
           (unsigned long)(ULONG)init_hr, (unsigned long)(ULONG)create_hr, (unsigned)length);
    check("WinRT HSTRING roundtrip",
          SUCCEEDED(init_hr) && SUCCEEDED(create_hr) && data &&
          length == (UINT32)wcslen(value) && wmemcmp(data, value, length) == 0);
    if (hstr && destroy) destroy(hstr);
    if (SUCCEEDED(init_hr) && uninit) uninit();
    if (dll) FreeLibrary(dll);
}
static void mf_api(void) {
    HMODULE dll = LoadLibraryW(L"mfplat.dll");
    mfstartup_fn start = (mfstartup_fn)(void *)symbol(dll, "MFStartup");
    mfshutdown_fn stop = (mfshutdown_fn)(void *)symbol(dll, "MFShutdown");
    HRESULT result = start ? start(MF_VERSION_2_70, MFSTARTUP_LITE) : E_NOTIMPL;
    printf("MFSTARTUP hresult=0x%08lx\n", (unsigned long)(ULONG)result);
    check("Media Foundation startup/shutdown", SUCCEEDED(result) && stop != NULL);
    if (SUCCEEDED(result) && stop) stop();
    if (dll) FreeLibrary(dll);
}
static void dpi_api(void) {
    HMODULE dll = LoadLibraryW(L"user32.dll");
    dpi_fn get = (dpi_fn)(void *)symbol(dll, "GetDpiForSystem");
    UINT value = get ? get() : 0;
    printf("DPI system=%u\n", value);
    check("System DPI API", get && value >= 48 && value <= 768);
    if (dll) FreeLibrary(dll);
}
static void http_api(void) {
    HMODULE dll = LoadLibraryW(L"winhttp.dll");
    winhttp_open_fn open = (winhttp_open_fn)(void *)symbol(dll, "WinHttpOpen");
    winhttp_close_fn close = (winhttp_close_fn)(void *)symbol(dll, "WinHttpCloseHandle");
    HINTERNET handle = open ? open(L"AILinuxProbe/0.1", 1, NULL, NULL, 0) : NULL;
    check("WinHTTP session open/close", handle && close);
    if (handle && close) close(handle);
    if (dll) FreeLibrary(dll);
}
int main(void) {
    printf("AILinux Win11-API probe bits=%u\n", (unsigned)(8 * sizeof(void *)));
    package_api(); winrt_api(); mf_api(); dpi_api(); http_api();
    printf("SUMMARY pass=%u fail=%u\n", passed, failed);
    return failed ? 1 : 0;
}
