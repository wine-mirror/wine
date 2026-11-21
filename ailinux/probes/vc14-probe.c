/* SPDX-License-Identifier: LGPL-2.1-or-later */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    HMODULE c = LoadLibraryW(L"vcruntime140.dll");
    HMODULE s = LoadLibraryW(L"msvcp140.dll");
    FARPROC memcpy_fn = c ? GetProcAddress(c, "memcpy") : NULL;
    FARPROC alloc_fn = s ? GetProcAddress(s, "?_Xbad_alloc@std@@YAXXZ") : NULL;
    char input[] = "vc14-dynamic-test", output[sizeof(input)];
    DWORD copied = 0;
    int ok = 0;
    printf("VC14_DLL_CHECK bits=%u crt=%s cxx=%s\n", (unsigned)(8*sizeof(void *)), c?"loaded":"missing", s?"loaded":"missing");
    if (memcpy_fn && alloc_fn) {
        typedef void * (__cdecl *cpy_t)(void *, const void *, size_t);
        memset(output, 0, sizeof(output));
        { cpy_t fn; memcpy(&fn, &memcpy_fn, sizeof(fn)); fn(output,input,sizeof(input)); }
        ok = memcmp(output,input,sizeof(input))==0;
        copied = (DWORD)sizeof(input);
    }
    printf("VC14_BEHAVIOR=%s copied=%lu\n",ok?"PASS":"FAIL",(unsigned long)copied);
    if(s) FreeLibrary(s);
    if(c) FreeLibrary(c);
    return ok?0:1;
}
