/*
 * W32SCOMB
 * DLL for Win32s
 *
 * Copyright (c) 1997 Andreas Mohr
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "windef.h"
#include "wine/winbase16.h"
#include "module.h"
#include "ldt.h"
#include "selectors.h"
#include "heap.h"

/***********************************************************************
 *           Get16DLLAddress       (KERNEL32)
 *
 * This function is used by a Win32s DLL if it wants to call a Win16 function.
 * A 16:16 segmented pointer to the function is returned.
 * Written without any docu.
 */
SEGPTR WINAPI Get16DLLAddress(HMODULE handle, LPSTR func_name) {
	HANDLE ThunkHeap = HeapCreate(HEAP_WINE_SEGPTR | HEAP_WINE_CODESEG, 0, 64);
        LPBYTE x;
	LPVOID tmpheap = HeapAlloc(ThunkHeap, 0, 32);
	SEGPTR thunk = HEAP_GetSegptr(ThunkHeap, 0, tmpheap);
	DWORD proc_16;
	WORD cs;

        if (!handle) handle=GetModuleHandle16("WIN32S16");
        proc_16 = (DWORD)WIN32_GetProcAddress16(handle, func_name);

        x=PTR_SEG_TO_LIN(thunk);
        *x++=0xba; *(DWORD*)x=proc_16;x+=4;             /* movl proc_16, $edx */
        *x++=0xea; *(DWORD*)x=(DWORD)GetProcAddress(GetModuleHandleA("KERNEL32"),"QT_Thunk");x+=4;     /* jmpl QT_Thunk */
	GET_CS(cs); *(WORD*)x=(WORD)cs;
        return thunk;
}
