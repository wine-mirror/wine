/* 
 * Definitions for inter-dll snooping
 */
#ifndef __WINE_SNOOP_H
#define __WINE_SNOOP_H

#include "module.h"

extern void SNOOP_RegisterDLL(HMODULE32,LPCSTR,DWORD);
extern FARPROC32 SNOOP_GetProcAddress32(HMODULE32,LPCSTR,DWORD,FARPROC32);
extern void (*fnSNOOP16_RegisterDLL)(NE_MODULE*,LPCSTR);
extern FARPROC16 (*fnSNOOP16_GetProcAddress16)(HMODULE16,DWORD,FARPROC16);
extern void SNOOP16_Init();
#endif
