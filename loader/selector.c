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
#include "if1632.h"
#include "prototypes.h"
#include "module.h"
#include "stddebug.h"
/* #define DEBUG_SELECTORS */
#include "debug.h"


#define MAX_ENV_SIZE 16384  /* Max. environment size (ought to be dynamic) */

static HANDLE EnvironmentHandle = 0;


extern char WindowsPath[256];

extern char **Argv;
extern int Argc;
extern char **environ;


/**********************************************************************
 *  Check whether pseudo-functions like __0040H for direct memory
 *  access are referenced and return 1 if so.
 *  FIXME: Reading and writing to the returned selectors has no effect
 *         (e.g. reading from the Bios data segment (esp. clock!) )
 */

unsigned int GetMemoryReference( char *dll_name, char *function,
                                 WORD *sel, WORD *offset )
{
  static HANDLE memory_handles[ 10 ] = { 0,0,0,0,0,0,0,0,0,0 };
  static char *memory_names[ 10 ] = { "segment 0xA000",
					"segment 0xB000",
					"segment 0xB800",
					"Bios-Rom",
					"segment 0xD000",
					"segment 0x0000",
					"segment 0xE000",
					"segment 0xF000",
					"segment 0xC000",
					"Bios data segment" };
  short nr;

  if( strcasecmp( dll_name, "KERNEL" ) ) 
    return 0;

  if( HIWORD( function ) ) {
    if( ( *function != '_' ) || ( *(function+1) != '_' ) )
      return 0;
    if( !strcasecmp( function, "__A000H" ) ) nr = 0;
    else if( !strcasecmp( function, "__B000H" ) ) nr = 1;
    else if( !strcasecmp( function, "__B800H" ) ) nr = 2;
    else if( !strcasecmp( function, "__ROMBIOS" ) ) nr = 3;
    else if( !strcasecmp( function, "__D000H" ) ) nr = 4;
    else if( !strcasecmp( function, "__0000H" ) ) nr = 5;
    else if( !strcasecmp( function, "__E000H" ) ) nr = 6;
    else if( !strcasecmp( function, "__F000H" ) ) nr = 7;
    else if( !strcasecmp( function, "__C000H" ) ) nr = 8;
    else if( !strcasecmp( function, "__0040H" ) ) nr = 9;
     else
      return 0;
  }
  else {
    switch( LOWORD( function ) ) {
    case 174: nr = 0; break;
    case 181: nr = 1; break;
    case 182: nr = 2; break;
    case 173: nr = 3; break;
    case 179: nr = 4; break;
    case 183: nr = 5; break;
    case 190: nr = 6; break;
    case 194: nr = 7; break;
    case 195: nr = 8; break;
    case 193: nr = 9; break;
    default: return 0;
    }
  }
  
  if( !memory_handles[ nr ] ) {
    fprintf( stderr, "Warning: Direct access to %s!\n", memory_names[ nr ] );
    memory_handles[ nr ] = GlobalAlloc( GMEM_FIXED, 65535 );
  }
  *sel = *offset = memory_handles[ nr ];
  return 1;
}
 


unsigned int GetEntryDLLName( char * dll_name, char * function,
                              WORD* sel, WORD *offset )
{
    HMODULE hModule;
    struct dll_table_s *dll_table;
    int ordinal, addr;

    if( GetMemoryReference( dll_name, function, sel, offset ) )
        return 0;

    hModule = GetModuleHandle( dll_name );
    ordinal = MODULE_GetOrdinal( hModule, function );
    if (!ordinal) return 1;
    addr = MODULE_GetEntryPoint( hModule, ordinal );
    if (!addr) return 1;
#ifdef WINESTAT
    if ((dll_table = FindDLLTable(dll_name)) != NULL)
    {
        dll_table->dll_table[ordinal].used++;
    }
#endif
    *offset = LOWORD(addr);
    *sel    = HIWORD(addr);
    return 0;
}


unsigned int GetEntryDLLOrdinal( char * dll_name, int ordinal,
                                 WORD *sel, WORD *offset )
{
    HMODULE hModule;
    struct dll_table_s *dll_table;
    int addr;

    if( GetMemoryReference( dll_name, (char*)ordinal, sel, offset ) )
        return 0;

    hModule = GetModuleHandle( dll_name );
    addr = MODULE_GetEntryPoint( hModule, ordinal );
    if (!addr) return 1;
#ifdef WINESTAT
    if ((dll_table = FindDLLTable(dll_name)) != NULL)
        dll_table->dll_table[ordinal].used++;
#endif
    *offset = LOWORD(addr);
    *sel    = HIWORD(addr);
    return 0;
}


WNDPROC GetWndProcEntry16( char *name )
{
    WORD sel, offset;

    GetEntryDLLName( "WINPROCS", name, &sel, &offset );
    return (WNDPROC) MAKELONG( offset, sel );
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
