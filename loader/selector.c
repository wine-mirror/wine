/*
 * Selector manipulation functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef WINELIB

#ifdef __linux__
#include <sys/mman.h>
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/mman.h>
#include <linux/a.out.h>
#include <linux/ldt.h>
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/mman.h>
#include <machine/segments.h>
#endif

#include "windows.h"
#include "ldt.h"
#include "wine.h"
#include "global.h"
#include "dlls.h"
#include "neexe.h"
#include "prototypes.h"
#include "module.h"
#include "stddebug.h"
/* #define DEBUG_SELECTORS */
#include "debug.h"


#define MAX_ENV_SIZE 16384  /* Max. environment size (ought to be dynamic) */

static HANDLE EnvironmentHandle = 0;


extern char WindowsPath[256];

extern char **environ;



WNDPROC GetWndProcEntry16( char *name )
{
    WORD ordinal;
    static HMODULE hModule = 0;

    if (!hModule) hModule = GetModuleHandle( "WINPROCS" );
    ordinal = MODULE_GetOrdinal( hModule, name );
    return MODULE_GetEntryPoint( hModule, ordinal );
}


/***********************************************************************
 *           GetDOSEnvironment   (KERNEL.131)
 */
SEGPTR GetDOSEnvironment(void)
{
    return WIN16_GlobalLock( EnvironmentHandle );
}


/**********************************************************************
 *           CreateEnvironment
 */
static HANDLE CreateEnvironment(void)
{
    HANDLE handle;
    char **e;
    char *p;

    handle = GlobalAlloc( GMEM_MOVEABLE, MAX_ENV_SIZE );
    if (!handle) return 0;
    p = (char *) GlobalLock( handle );

    /*
     * Fill environment with Windows path, the Unix environment,
     * and program name.
     */
    strcpy(p, "PATH=");
    strcat(p, WindowsPath);
    p += strlen(p) + 1;

    for (e = environ; *e; e++)
    {
	if (strncasecmp(*e, "path", 4))
	{
	    strcpy(p, *e);
	    p += strlen(p) + 1;
	}
    }

    *p++ = '\0';

    /*
     * Display environment
     */
    p = (char *) GlobalLock( handle );
    dprintf_selectors(stddeb, "Environment at %p\n", p);
    for (; *p; p += strlen(p) + 1) dprintf_selectors(stddeb, "    %s\n", p);

    return handle;
}



/**********************************************************************
 *					CreateSelectors
 */
void CreateSelectors(void)
{
    if(!EnvironmentHandle) EnvironmentHandle = CreateEnvironment();
}


#endif /* ifndef WINELIB */
