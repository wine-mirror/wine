/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"

/***********************************************************************
 *           GetACP               (KERNEL32.148)
 */
UINT GetACP(void)
{
    return 1252;    /* Windows 3.1 ISO Latin */
}

/***********************************************************************
 *           GetCPInfo            (KERNEL32.154)
 */
BOOL GetCPInfo(UINT codepage, LPCPINFO cpinfo)
{
    cpinfo->MaxCharSize = 1;
    cpinfo->DefaultChar[0] = '?';

    return 1;
}

/***********************************************************************
 *              GetOEMCP                (KERNEL32.248)
 */
UINT GetOEMCP(void)
{
    return 437;    /* MS-DOS United States */
}

