/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "wincon.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *           SetConsoleCtrlHandler               (KERNEL32.459)
 */
BOOL SetConsoleCtrlHandler(HANDLER_ROUTINE * func,  BOOL a)
{
	return 0;
}

