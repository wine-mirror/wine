#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
static void probe(const char *dll, const char *symbol, int comma) {
 HMODULE m=LoadLibraryA(dll);
 FARPROC p=m ? GetProcAddress(m,symbol) : NULL;
 printf("%s\"%s!%s\":%s",comma?",":"",dll,symbol,p?"true":"false");
 if(m) FreeLibrary(m);
}
int main(void) {
 OSVERSIONINFOW os={0};
 os.dwOSVersionInfoSize=sizeof(os);
 typedef LONG (WINAPI *rtl_version_t)(OSVERSIONINFOW *);
 HMODULE ntdll=GetModuleHandleW(L"ntdll.dll");
 rtl_version_t version=(rtl_version_t)(void *)GetProcAddress(ntdll,"RtlGetVersion");
 BOOL valid=version && version(&os)==0;
 printf("{\"schema\":1,\"arch_bits\":%u,\"windows_version\":\"%lu.%lu.%lu\",\"version_queried\":%s,\"api\":{",
  (unsigned)(sizeof(void*)*8),(unsigned long)os.dwMajorVersion,
  (unsigned long)os.dwMinorVersion,(unsigned long)os.dwBuildNumber,valid?"true":"false");
 probe("kernel32.dll","GetPackageFamilyName",0);
 probe("kernel32.dll","GetSystemTimePreciseAsFileTime",1);
 probe("ntdll.dll","RtlGetVersion",1);
 probe("combase.dll","WindowsCreateString",1);
 probe("mfplat.dll","MFStartup",1);
 probe("d3d12.dll","D3D12CreateDevice",1);
 probe("d3d11.dll","D3D11CreateDevice",1);
 probe("winhttp.dll","WinHttpOpen",1);
 probe("bcrypt.dll","BCryptGenRandom",1);
 puts("}}");
 return 0;
}
