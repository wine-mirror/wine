/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 */

#include <stdio.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           GetCurrentThreadId   (KERNEL32.200)
 */

int GetCurrentThreadId(void)
{
        return getpid();
}

/***********************************************************************
 *           GetThreadContext         (KERNEL32.294)
 */
BOOL GetThreadContext(HANDLE hThread, void *lpContext)
{
        return FALSE;
}

