/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"

/***********************************************************************
 *           VirtualAlloc             (KERNEL32.548)
 */
LPVOID VirtualAlloc(LPVOID lpvAddress, DWORD cbSize,
                   DWORD fdwAllocationType, DWORD fdwProtect)
{
    char *ptr;

    printf("VirtualAlloc: size = %ld, address=%p\n", cbSize, lpvAddress);
    ptr = malloc(cbSize + 65536);
    if(ptr)
    {
        /* Round it up to the next 64K boundary and zero it.
         */
        ptr = (void *)(((unsigned long)ptr & 0xFFFF0000L) + 0x00010000L);
        memset(ptr, 0, cbSize);
    }
    printf("VirtualAlloc: got pointer %p\n", ptr);
    return ptr;
}

/***********************************************************************
 *           VirtualFree               (KERNEL32.550)
 */
BOOL VirtualFree(LPVOID lpvAddress, DWORD cbSize, DWORD fdwFreeType)
{
    if(lpvAddress)
        free(lpvAddress);
    return 1;
}

