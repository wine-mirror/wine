/*
 * AILinux Wine PE export coverage probe.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Independently executable PE program. All symbols are inspected dynamically,
 * so missing DLLs do not prevent the probe itself from starting.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

struct library_probe { const wchar_t *dll; const char *symbol; const char *category; };
static const struct library_probe probes[] = {
    { L"kernel32.dll", "GetTickCount64", "core" },
    { L"kernelbase.dll", "GetCurrentPackageFullName", "packages" },
    { L"ntdll.dll", "RtlGetVersion", "nt" },
    { L"advapi32.dll", "RegOpenKeyExW", "registry" },
    { L"ole32.dll", "CoInitializeEx", "com" },
    { L"oleaut32.dll", "VariantInit", "automation" },
    { L"combase.dll", "CoCreateInstance", "com" },
    { L"shell32.dll", "SHGetKnownFolderPath", "shell" },
    { L"shcore.dll", "GetDpiForMonitor", "dpi" },
    { L"ws2_32.dll", "WSAStartup", "network" },
    { L"winhttp.dll", "WinHttpOpen", "network" },
    { L"wininet.dll", "InternetOpenW", "network" },
    { L"bcrypt.dll", "BCryptOpenAlgorithmProvider", "crypto" },
    { L"crypt32.dll", "CryptProtectData", "crypto" },
    { L"ucrtbase.dll", "malloc", "crt" },
    { L"vcruntime140.dll", "__CxxFrameHandler3", "vc-runtime" },
    { L"msvcp140.dll", "?_Xbad_alloc@std@@YAXXZ", "vc-runtime" },
    { L"user32.dll", "MessageBoxW", "gui" },
    { L"gdi32.dll", "CreateFontW", "gui" },
    { L"d3d11.dll", "D3D11CreateDevice", "d3d" },
    { L"dxgi.dll", "CreateDXGIFactory", "d3d" },
    { L"d3d12.dll", "D3D12CreateDevice", "d3d" },
    { L"xaudio2_9.dll", "XAudio2Create", "audio" },
    { L"mfplat.dll", "MFStartup", "media" }
};
int main(void)
{
    unsigned int i, missing = 0, total = sizeof(probes) / sizeof(probes[0]);
    printf("AILinux Wine PE library probe, bits=%u\n", (unsigned int)(sizeof(void *) * 8));
    for (i = 0; i < total; ++i)
    {
        HMODULE mod = LoadLibraryW(probes[i].dll);
        FARPROC symbol = mod ? GetProcAddress(mod, probes[i].symbol) : NULL;
        printf("%-14s %-22ls %-36s %s\n", probes[i].category,
               probes[i].dll, probes[i].symbol, symbol ? "PASS" : "MISSING");
        if (!symbol) ++missing;
        if (mod) FreeLibrary(mod);
    }
    printf("SUMMARY checked=%u missing=%u\n", total, missing);
    /* No hard failure: optional libraries are coverage signals, not mandatory APIs. */
    return 0;
}
