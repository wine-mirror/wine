/* 
 * Definitions for inter-win32-dll snooping
 */
#ifndef __WINE_SNOOP_H
#define __WINE_SNOOP_H
extern void SNOOP_RegisterDLL(HMODULE32 hmod,LPCSTR name,DWORD nrofordinals);
extern FARPROC32 SNOOP_GetProcAddress32(HMODULE32 hmod,LPCSTR name,DWORD ordinal,FARPROC32 origfun);
#endif
