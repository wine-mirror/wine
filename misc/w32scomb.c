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

/***********************************************************************
 *           Get16DLLAddress       (KERNEL32)
 *
 * rough guesswork, but seems to work
 */
FARPROC16 Get16DLLAddress(HMODULE32 handle, LPSTR name) {
        if (!handle) handle=GetModuleHandle16("WIN32S16");
        return (FARPROC16)WIN32_GetProcAddress16(handle, name);
}
