/*
 * W32SCOMB
 * DLL for Win32s
 *
 * Copyright (c) 1997 Andreas Mohr
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "windows.h"
#include "module.h"
#include "ldt.h"
#include "selectors.h"
#include "winbase.h"
#include "heap.h"

/***********************************************************************
 *           Get16DLLAddress       (KERNEL32)
 *
 * This function is used by a Win32s DLL if it wants to call a Win16 function.
 * A 16:16 segmented pointer to the function is returned.
 * Written without any docu.
 */
FARPROC16 WINAPI Get16DLLAddress(HMODULE32 handle, LPSTR func_name) {
       if ( (strcasecmp(func_name, "StackLinearToSegmented"))
        && (strcasecmp(func_name, "CoThkCommon")) ) {
               fprintf(stderr, "Get16DLLAddress() called for function %s(). Please report to Andreas Mohr.\n", func_name);
       }
        if (!handle) {
               handle = (HMODULE32)LoadLibrary16("WIN32S16");
               FreeLibrary16(handle);
       }
       return WIN32_GetProcAddress16(handle,func_name);
}

