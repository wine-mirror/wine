/* SPDX-License-Identifier: LGPL-2.1-or-later */
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    UINT32 GdiplusVersion;
    void *DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;
} startup_input;
typedef int (WINAPI *gdip_start_t)(ULONG_PTR *, const startup_input *, void *);
typedef void (WINAPI *gdip_stop_t)(ULONG_PTR);
typedef HRESULT (WINAPI *compile_t)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO *,
    ID3DInclude *, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob **, ID3DBlob **);
static int gdiplus_check(void) {
    HMODULE m = LoadLibraryW(L"gdiplus.dll");
    gdip_start_t start = m ? (gdip_start_t)(void *)GetProcAddress(m, "GdiplusStartup") : NULL;
    gdip_stop_t stop = m ? (gdip_stop_t)(void *)GetProcAddress(m, "GdiplusShutdown") : NULL;
    startup_input in = {1, NULL, FALSE, FALSE};
    ULONG_PTR token = 0;
    int status = (start && stop) ? start(&token, &in, NULL) : -1;
    int ok = status == 0 && token != 0;
    if (ok) stop(token);
    if (m) FreeLibrary(m);
    printf("%s GDI+ initialize/shutdown (status=%d)\n", ok?"PASS":"FAIL",status);
    return ok;
}
static int xml_check(void) {
    CLSID clsid;
    IUnknown *obj = NULL;
    HRESULT hr = CLSIDFromProgID(L"Msxml2.DOMDocument.6.0", &clsid);
    if (SUCCEEDED(hr)) hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER,
                                              &IID_IUnknown, (void **)&obj);
    int ok = SUCCEEDED(hr) && obj;
    if (obj) IUnknown_Release(obj);
    printf("%s MSXML6 DOM COM activation (HRESULT=0x%08lx)\n",
           ok?"PASS":"FAIL",(unsigned long)(ULONG)hr);
    return ok;
}
static int shader_check(void) {
    static const char shader[] = "float4 main() : SV_Target { return float4(0.25,0.5,0.75,1); }";
    HMODULE m = LoadLibraryW(L"d3dcompiler_47.dll");
    compile_t compile = m ? (compile_t)(void *)GetProcAddress(m, "D3DCompile") : NULL;
    ID3DBlob *code = NULL, *errors = NULL;
    HRESULT hr = compile ? compile(shader, strlen(shader), "ailinux-probe", NULL, NULL,
                                   "main", "ps_5_0", 0, 0, &code, &errors) : E_NOTIMPL;
    int ok = SUCCEEDED(hr) && code && ID3D10Blob_GetBufferSize(code) > 0;
    if (errors) ID3D10Blob_Release(errors);
    if (code) ID3D10Blob_Release(code);
    if (m) FreeLibrary(m);
    printf("%s D3DCompiler_47 HLSL shader compilation (HRESULT=0x%08lx)\n",
           ok?"PASS":"FAIL",(unsigned long)(ULONG)hr);
    return ok;
}
int main(void) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    int xml = SUCCEEDED(hr) ? xml_check() : 0;
    if (SUCCEEDED(hr)) CoUninitialize();
    int gdip = gdiplus_check();
    int shader = shader_check();
    printf("EXTRAS_SUMMARY bits=%u pass=%d fail=%d\n", (unsigned)(8*sizeof(void*)),
           xml+gdip+shader,3-(xml+gdip+shader));
    return (xml && gdip && shader) ? 0 : 1;
}
