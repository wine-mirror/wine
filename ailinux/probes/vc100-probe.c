/* SPDX-License-Identifier: LGPL-2.1-or-later */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    HMODULE crt = LoadLibraryW(L"msvcr100.dll");
    HMODULE cpp = LoadLibraryW(L"msvcp100.dll");
    typedef void *(__cdecl *alloc_func)(size_t);
    typedef void (__cdecl *free_func)(void *);
    FARPROC alloc_symbol = crt ? GetProcAddress(crt, "malloc") : NULL;
    FARPROC free_symbol = crt ? GetProcAddress(crt, "free") : NULL;
    alloc_func allocate = NULL;
    free_func release = NULL;
    int passed = 0;
    if (alloc_symbol && free_symbol && cpp) {
        memcpy(&allocate, &alloc_symbol, sizeof(allocate));
        memcpy(&release, &free_symbol, sizeof(release));
        char *mem = (char *)allocate(64);
        if (mem) {
            strcpy(mem, "ailinux-vc2010-test");
            passed = strcmp(mem, "ailinux-vc2010-test") == 0;
            release(mem);
        }
    }
    printf("VC100 bits=%u crt=%s cpp=%s malloc_free=%s\n", (unsigned)(sizeof(void*) * 8),
           crt ? "loaded" : "missing", cpp ? "loaded" : "missing",
           passed ? "PASS" : "FAIL");
    if (cpp) FreeLibrary(cpp);
    if (crt) FreeLibrary(crt);
    return passed ? 0 : 1;
}
