/*
 * Modules
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "windef.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "winerror.h"
#include "file.h"
#include "global.h"
#include "heap.h"
#include "module.h"
#include "snoop.h"
#include "neexe.h"
#include "pe_image.h"
#include "dosexe.h"
#include "process.h"
#include "syslevel.h"
#include "thread.h"
#include "selectors.h"
#include "stackframe.h"
#include "task.h"
#include "debugtools.h"
#include "callback.h"
#include "loadorder.h"
#include "elfdll.h"
#include "server.h"

DEFAULT_DEBUG_CHANNEL(module);
DECLARE_DEBUG_CHANNEL(win32);


/*************************************************************************
 *		MODULE32_LookupHMODULE
 * looks for the referenced HMODULE in the current process
 */
WINE_MODREF *MODULE32_LookupHMODULE( HMODULE hmod )
{
    WINE_MODREF	*wm;

    if (!hmod) 
    	return PROCESS_Current()->exe_modref;

    if (!HIWORD(hmod)) {
    	ERR("tried to lookup 0x%04x in win32 module handler!\n",hmod);
	return NULL;
    }
    for ( wm = PROCESS_Current()->modref_list; wm; wm=wm->next )
	if (wm->module == hmod)
	    return wm;
    return NULL;
}

/*************************************************************************
 *		MODULE_InitDll
 */
static BOOL MODULE_InitDll( WINE_MODREF *wm, DWORD type, LPVOID lpReserved )
{
    BOOL retv = TRUE;

    static LPCSTR typeName[] = { "PROCESS_DETACH", "PROCESS_ATTACH", 
                                 "THREAD_ATTACH", "THREAD_DETACH" };
    assert( wm );


    /* Skip calls for modules loaded with special load flags */

    if (    ( wm->flags & WINE_MODREF_DONT_RESOLVE_REFS )
         || ( wm->flags & WINE_MODREF_LOAD_AS_DATAFILE ) )
        return TRUE;


    TRACE("(%s,%s,%p) - CALL\n", wm->modname, typeName[type], lpReserved );

    /* Call the initialization routine */
    switch ( wm->type )
    {
    case MODULE32_PE:
        retv = PE_InitDLL( wm, type, lpReserved );
        break;

    case MODULE32_ELF:
        /* no need to do that, dlopen() already does */
        break;

    default:
        ERR("wine_modref type %d not handled.\n", wm->type );
        retv = FALSE;
        break;
    }

    /* The state of the module list may have changed due to the call
       to PE_InitDLL. We cannot assume that this module has not been
       deleted.  */
    TRACE("(%p,%s,%p) - RETURN %d\n", wm, typeName[type], lpReserved, retv );

    return retv;
}

/*************************************************************************
 *		MODULE_DllProcessAttach
 * 
 * Send the process attach notification to all DLLs the given module
 * depends on (recursively). This is somewhat complicated due to the fact that
 *
 * - we have to respect the module dependencies, i.e. modules implicitly
 *   referenced by another module have to be initialized before the module
 *   itself can be initialized
 * 
 * - the initialization routine of a DLL can itself call LoadLibrary,
 *   thereby introducing a whole new set of dependencies (even involving
 *   the 'old' modules) at any time during the whole process
 *
 * (Note that this routine can be recursively entered not only directly
 *  from itself, but also via LoadLibrary from one of the called initialization
 *  routines.)
 *
 * Furthermore, we need to rearrange the main WINE_MODREF list to allow
 * the process *detach* notifications to be sent in the correct order.
 * This must not only take into account module dependencies, but also 
 * 'hidden' dependencies created by modules calling LoadLibrary in their
 * attach notification routine.
 *
 * The strategy is rather simple: we move a WINE_MODREF to the head of the
 * list after the attach notification has returned.  This implies that the
 * detach notifications are called in the reverse of the sequence the attach
 * notifications *returned*.
 *
 * NOTE: Assumes that the process critical section is held!
 *
 */
BOOL MODULE_DllProcessAttach( WINE_MODREF *wm, LPVOID lpReserved )
{
    BOOL retv = TRUE;
    int i;
    assert( wm );

    /* prevent infinite recursion in case of cyclical dependencies */
    if (    ( wm->flags & WINE_MODREF_MARKER )
         || ( wm->flags & WINE_MODREF_PROCESS_ATTACHED ) )
        return retv;

    TRACE("(%s,%p) - START\n", wm->modname, lpReserved );

    /* Tag current MODREF to prevent recursive loop */
    wm->flags |= WINE_MODREF_MARKER;

    /* Recursively attach all DLLs this one depends on */
    for ( i = 0; retv && i < wm->nDeps; i++ )
        if ( wm->deps[i] )
            retv = MODULE_DllProcessAttach( wm->deps[i], lpReserved );

    /* Call DLL entry point */
    if ( retv )
    {
        retv = MODULE_InitDll( wm, DLL_PROCESS_ATTACH, lpReserved );
        if ( retv )
            wm->flags |= WINE_MODREF_PROCESS_ATTACHED;
    }

    /* Re-insert MODREF at head of list */
    if ( retv && wm->prev )
    {
        wm->prev->next = wm->next;
        if ( wm->next ) wm->next->prev = wm->prev;

        wm->prev = NULL;
        wm->next = PROCESS_Current()->modref_list;
        PROCESS_Current()->modref_list = wm->next->prev = wm;
    }

    /* Remove recursion flag */
    wm->flags &= ~WINE_MODREF_MARKER;

    TRACE("(%s,%p) - END\n", wm->modname, lpReserved );

    return retv;
}

/*************************************************************************
 *		MODULE_DllProcessDetach
 * 
 * Send DLL process detach notifications.  See the comment about calling 
 * sequence at MODULE_DllProcessAttach.  Unless the bForceDetach flag
 * is set, only DLLs with zero refcount are notified.
 */
void MODULE_DllProcessDetach( BOOL bForceDetach, LPVOID lpReserved )
{
    WINE_MODREF *wm;

    EnterCriticalSection( &PROCESS_Current()->crit_section );

    do
    {
        for ( wm = PROCESS_Current()->modref_list; wm; wm = wm->next )
        {
            /* Check whether to detach this DLL */
            if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
                continue;
            if ( wm->refCount > 0 && !bForceDetach )
                continue;

            /* Call detach notification */
            wm->flags &= ~WINE_MODREF_PROCESS_ATTACHED;
            MODULE_InitDll( wm, DLL_PROCESS_DETACH, lpReserved );

            /* Restart at head of WINE_MODREF list, as entries might have
               been added and/or removed while performing the call ... */
            break;
        }
    } while ( wm );

    LeaveCriticalSection( &PROCESS_Current()->crit_section );
}

/*************************************************************************
 *		MODULE_DllThreadAttach
 * 
 * Send DLL thread attach notifications. These are sent in the
 * reverse sequence of process detach notification.
 *
 */
void MODULE_DllThreadAttach( LPVOID lpReserved )
{
    WINE_MODREF *wm;

    EnterCriticalSection( &PROCESS_Current()->crit_section );

    for ( wm = PROCESS_Current()->modref_list; wm; wm = wm->next )
        if ( !wm->next )
            break;

    for ( ; wm; wm = wm->prev )
    {
        if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
            continue;
        if ( wm->flags & WINE_MODREF_NO_DLL_CALLS )
            continue;

        MODULE_InitDll( wm, DLL_THREAD_ATTACH, lpReserved );
    }

    LeaveCriticalSection( &PROCESS_Current()->crit_section );
}

/*************************************************************************
 *		MODULE_DllThreadDetach
 * 
 * Send DLL thread detach notifications. These are sent in the
 * same sequence as process detach notification.
 *
 */
void MODULE_DllThreadDetach( LPVOID lpReserved )
{
    WINE_MODREF *wm;

    EnterCriticalSection( &PROCESS_Current()->crit_section );

    for ( wm = PROCESS_Current()->modref_list; wm; wm = wm->next )
    {
        if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
            continue;
        if ( wm->flags & WINE_MODREF_NO_DLL_CALLS )
            continue;

        MODULE_InitDll( wm, DLL_THREAD_DETACH, lpReserved );
    }

    LeaveCriticalSection( &PROCESS_Current()->crit_section );
}

/****************************************************************************
 *              DisableThreadLibraryCalls (KERNEL32.74)
 *
 * Don't call DllEntryPoint for DLL_THREAD_{ATTACH,DETACH} if set.
 */
BOOL WINAPI DisableThreadLibraryCalls( HMODULE hModule )
{
    WINE_MODREF *wm;
    BOOL retval = TRUE;

    EnterCriticalSection( &PROCESS_Current()->crit_section );

    wm = MODULE32_LookupHMODULE( hModule );
    if ( !wm )
        retval = FALSE;
    else
        wm->flags |= WINE_MODREF_NO_DLL_CALLS;

    LeaveCriticalSection( &PROCESS_Current()->crit_section );

    return retval;
}


/***********************************************************************
 *           MODULE_CreateDummyModule
 *
 * Create a dummy NE module for Win32 or Winelib.
 */
HMODULE MODULE_CreateDummyModule( LPCSTR filename, HMODULE module32 )
{
    HMODULE hModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    char *pStr,*s;
    unsigned int len;
    const char* basename;
    OFSTRUCT *ofs;
    int of_size, size;

    /* Extract base filename */
    basename = strrchr(filename, '\\');
    if (!basename) basename = filename;
    else basename++;
    len = strlen(basename);
    if ((s = strchr(basename, '.'))) len = s - basename;

    /* Allocate module */
    of_size = sizeof(OFSTRUCT) - sizeof(ofs->szPathName)
                    + strlen(filename) + 1;
    size = sizeof(NE_MODULE) +
                 /* loaded file info */
                 of_size +
                 /* segment table: DS,CS */
                 2 * sizeof(SEGTABLEENTRY) +
                 /* name table */
                 len + 2 +
                 /* several empty tables */
                 8;

    hModule = GlobalAlloc16( GMEM_MOVEABLE | GMEM_ZEROINIT, size );
    if (!hModule) return (HMODULE)11;  /* invalid exe */

    FarSetOwner16( hModule, hModule );
    pModule = (NE_MODULE *)GlobalLock16( hModule );

    /* Set all used entries */
    pModule->magic            = IMAGE_OS2_SIGNATURE;
    pModule->count            = 1;
    pModule->next             = 0;
    pModule->flags            = 0;
    pModule->dgroup           = 0;
    pModule->ss               = 1;
    pModule->cs               = 2;
    pModule->heap_size        = 0;
    pModule->stack_size       = 0;
    pModule->seg_count        = 2;
    pModule->modref_count     = 0;
    pModule->nrname_size      = 0;
    pModule->fileinfo         = sizeof(NE_MODULE);
    pModule->os_flags         = NE_OSFLAGS_WINDOWS;
    pModule->self             = hModule;
    pModule->module32         = module32;

    /* Set version and flags */
    if (module32)
    {
        pModule->expected_version =
            ((PE_HEADER(module32)->OptionalHeader.MajorSubsystemVersion & 0xff) << 8 ) |
             (PE_HEADER(module32)->OptionalHeader.MinorSubsystemVersion & 0xff);
        pModule->flags |= NE_FFLAGS_WIN32;
        if (PE_HEADER(module32)->FileHeader.Characteristics & IMAGE_FILE_DLL)
            pModule->flags |= NE_FFLAGS_LIBMODULE | NE_FFLAGS_SINGLEDATA;
    }

    /* Set loaded file information */
    ofs = (OFSTRUCT *)(pModule + 1);
    memset( ofs, 0, of_size );
    ofs->cBytes = of_size < 256 ? of_size : 255;   /* FIXME */
    strcpy( ofs->szPathName, filename );

    pSegment = (SEGTABLEENTRY*)((char*)(pModule + 1) + of_size);
    pModule->seg_table = (int)pSegment - (int)pModule;
    /* Data segment */
    pSegment->size    = 0;
    pSegment->flags   = NE_SEGFLAGS_DATA;
    pSegment->minsize = 0x1000;
    pSegment++;
    /* Code segment */
    pSegment->flags   = 0;
    pSegment++;

    /* Module name */
    pStr = (char *)pSegment;
    pModule->name_table = (int)pStr - (int)pModule;
    assert(len<256);
    *pStr = len;
    lstrcpynA( pStr+1, basename, len+1 );
    pStr += len+2;

    /* All tables zero terminated */
    pModule->res_table = pModule->import_table = pModule->entry_table =
		(int)pStr - (int)pModule;

    NE_RegisterModule( pModule );
    return hModule;
}


/**********************************************************************
 *	    MODULE_FindModule32
 *
 * Find a (loaded) win32 module depending on path
 *
 * RETURNS
 *	the module handle if found
 * 	0 if not
 */
WINE_MODREF *MODULE_FindModule(
	LPCSTR path	/* [in] pathname of module/library to be found */
) {
    WINE_MODREF	*wm;
    char dllname[260], *p;

    /* Append .DLL to name if no extension present */
    strcpy( dllname, path );
    if (!(p = strrchr( dllname, '.')) || strchr( p, '/' ) || strchr( p, '\\'))
            strcat( dllname, ".DLL" );

    for ( wm = PROCESS_Current()->modref_list; wm; wm = wm->next )
    {
        if ( !strcasecmp( dllname, wm->modname ) )
            break;
        if ( !strcasecmp( dllname, wm->filename ) )
            break;
        if ( !strcasecmp( dllname, wm->short_modname ) )
            break;
        if ( !strcasecmp( dllname, wm->short_filename ) )
            break;
    }

    return wm;
}

/***********************************************************************
 *           MODULE_GetBinaryType
 *
 * The GetBinaryType function determines whether a file is executable
 * or not and if it is it returns what type of executable it is.
 * The type of executable is a property that determines in which
 * subsystem an executable file runs under.
 *
 * Binary types returned:
 * SCS_32BIT_BINARY: A Win32 based application
 * SCS_DOS_BINARY: An MS-Dos based application
 * SCS_WOW_BINARY: A Win16 based application
 * SCS_PIF_BINARY: A PIF file that executes an MS-Dos based app
 * SCS_POSIX_BINARY: A POSIX based application ( Not implemented )
 * SCS_OS216_BINARY: A 16bit OS/2 based application
 *
 * Returns TRUE if the file is an executable in which case
 * the value pointed by lpBinaryType is set.
 * Returns FALSE if the file is not an executable or if the function fails.
 *
 * To do so it opens the file and reads in the header information
 * if the extended header information is not present it will
 * assume that the file is a DOS executable.
 * If the extended header information is present it will
 * determine if the file is a 16 or 32 bit Windows executable
 * by check the flags in the header.
 *
 * Note that .COM and .PIF files are only recognized by their
 * file name extension; but Windows does it the same way ...
 */
BOOL MODULE_GetBinaryType( HANDLE hfile, LPCSTR filename, LPDWORD lpBinaryType )
{
    IMAGE_DOS_HEADER mz_header;
    char magic[4], *ptr;
    DWORD len;

    /* Seek to the start of the file and read the DOS header information.
     */
    if (    SetFilePointer( hfile, 0, NULL, SEEK_SET ) != -1  
         && ReadFile( hfile, &mz_header, sizeof(mz_header), &len, NULL )
         && len == sizeof(mz_header) )
    {
        /* Now that we have the header check the e_magic field
         * to see if this is a dos image.
         */
        if ( mz_header.e_magic == IMAGE_DOS_SIGNATURE )
        {
            BOOL lfanewValid = FALSE;
            /* We do have a DOS image so we will now try to seek into
             * the file by the amount indicated by the field
             * "Offset to extended header" and read in the
             * "magic" field information at that location.
             * This will tell us if there is more header information
             * to read or not.
             */
            /* But before we do we will make sure that header
             * structure encompasses the "Offset to extended header"
             * field.
             */
            if ( (mz_header.e_cparhdr<<4) >= sizeof(IMAGE_DOS_HEADER) )
                if ( ( mz_header.e_crlc == 0 ) ||
                     ( mz_header.e_lfarlc >= sizeof(IMAGE_DOS_HEADER) ) )
                    if (    mz_header.e_lfanew >= sizeof(IMAGE_DOS_HEADER)
                         && SetFilePointer( hfile, mz_header.e_lfanew, NULL, SEEK_SET ) != -1  
                         && ReadFile( hfile, magic, sizeof(magic), &len, NULL )
                         && len == sizeof(magic) )
                        lfanewValid = TRUE;

            if ( !lfanewValid )
            {
                /* If we cannot read this "extended header" we will
                 * assume that we have a simple DOS executable.
                 */
                *lpBinaryType = SCS_DOS_BINARY;
                return TRUE;
            }
            else
            {
                /* Reading the magic field succeeded so
                 * we will try to determine what type it is.
                 */
                if ( *(DWORD*)magic      == IMAGE_NT_SIGNATURE )
                {
                    /* This is an NT signature.
                     */
                    *lpBinaryType = SCS_32BIT_BINARY;
                    return TRUE;
                }
                else if ( *(WORD*)magic == IMAGE_OS2_SIGNATURE )
                {
                    /* The IMAGE_OS2_SIGNATURE indicates that the
                     * "extended header is a Windows executable (NE)
                     * header."  This can mean either a 16-bit OS/2
                     * or a 16-bit Windows or even a DOS program 
                     * (running under a DOS extender).  To decide
                     * which, we'll have to read the NE header.
                     */

                     IMAGE_OS2_HEADER ne;
                     if (    SetFilePointer( hfile, mz_header.e_lfanew, NULL, SEEK_SET ) != -1  
                          && ReadFile( hfile, &ne, sizeof(ne), &len, NULL )
                          && len == sizeof(ne) )
                     {
                         switch ( ne.ne_exetyp )
                         {
                         case 2:  *lpBinaryType = SCS_WOW_BINARY;   return TRUE;
                         case 5:  *lpBinaryType = SCS_DOS_BINARY;   return TRUE;
                         default: *lpBinaryType = SCS_OS216_BINARY; return TRUE;
                         }
                     }
                     /* Couldn't read header, so abort. */
                     return FALSE;
                }
                else
                {
                    /* Unknown extended header, but this file is nonetheless
		       DOS-executable.
                     */
                    *lpBinaryType = SCS_DOS_BINARY;
	            return TRUE;
                }
            }
        }
    }

    /* If we get here, we don't even have a correct MZ header.
     * Try to check the file extension for known types ...
     */
    ptr = strrchr( filename, '.' );
    if ( ptr && !strchr( ptr, '\\' ) && !strchr( ptr, '/' ) )
    {
        if ( !lstrcmpiA( ptr, ".COM" ) )
        {
            *lpBinaryType = SCS_DOS_BINARY;
            return TRUE;
        }

        if ( !lstrcmpiA( ptr, ".PIF" ) )
        {
            *lpBinaryType = SCS_PIF_BINARY;
            return TRUE;
        }
    }

    return FALSE;
}

/***********************************************************************
 *             GetBinaryTypeA                     [KERNEL32.280]
 */
BOOL WINAPI GetBinaryTypeA( LPCSTR lpApplicationName, LPDWORD lpBinaryType )
{
    BOOL ret = FALSE;
    HANDLE hfile;

    TRACE_(win32)("%s\n", lpApplicationName );

    /* Sanity check.
     */
    if ( lpApplicationName == NULL || lpBinaryType == NULL )
        return FALSE;

    /* Open the file indicated by lpApplicationName for reading.
     */
    hfile = CreateFileA( lpApplicationName, GENERIC_READ, 0,
                         NULL, OPEN_EXISTING, 0, -1 );
    if ( hfile == INVALID_HANDLE_VALUE )
        return FALSE;

    /* Check binary type
     */
    ret = MODULE_GetBinaryType( hfile, lpApplicationName, lpBinaryType );

    /* Close the file.
     */
    CloseHandle( hfile );

    return ret;
}

/***********************************************************************
 *             GetBinaryTypeW                      [KERNEL32.281]
 */
BOOL WINAPI GetBinaryTypeW( LPCWSTR lpApplicationName, LPDWORD lpBinaryType )
{
    BOOL ret = FALSE;
    LPSTR strNew = NULL;

    TRACE_(win32)("%s\n", debugstr_w(lpApplicationName) );

    /* Sanity check.
     */
    if ( lpApplicationName == NULL || lpBinaryType == NULL )
        return FALSE;

    /* Convert the wide string to a ascii string.
     */
    strNew = HEAP_strdupWtoA( GetProcessHeap(), 0, lpApplicationName );

    if ( strNew != NULL )
    {
        ret = GetBinaryTypeA( strNew, lpBinaryType );

        /* Free the allocated string.
         */
        HeapFree( GetProcessHeap(), 0, strNew );
    }

    return ret;
}


/***********************************************************************
 *           WinExec16   (KERNEL.166)
 */
HINSTANCE16 WINAPI WinExec16( LPCSTR lpCmdLine, UINT16 nCmdShow )
{
    LPCSTR p;
    LPSTR name, cmdline;
    int len;
    HINSTANCE16 ret;
    char buffer[MAX_PATH];

    if ((p = strchr( lpCmdLine, ' ' )))
    {
        if (!(name = HeapAlloc( GetProcessHeap(), 0, p - lpCmdLine + 1 )))
            return ERROR_NOT_ENOUGH_MEMORY;
        memcpy( name, lpCmdLine, p - lpCmdLine );
        name[p - lpCmdLine] = 0;
        p++;
        len = strlen(p);
        cmdline = SEGPTR_ALLOC( len + 2 );
        cmdline[0] = (BYTE)len;
        strcpy( cmdline + 1, p );
    }
    else
    {
        name = (LPSTR)lpCmdLine;
        cmdline = SEGPTR_ALLOC(2);
        cmdline[0] = cmdline[1] = 0;
    }

    if (SearchPathA( NULL, name, ".exe", sizeof(buffer), buffer, NULL ))
    {
        LOADPARAMS16 params;
        WORD *showCmd = SEGPTR_ALLOC( 2*sizeof(WORD) );
        showCmd[0] = 2;
        showCmd[1] = nCmdShow;

        params.hEnvironment = 0;
        params.cmdLine = SEGPTR_GET(cmdline);
        params.showCmd = SEGPTR_GET(showCmd);
        params.reserved = 0;

        ret = LoadModule16( buffer, &params );

        SEGPTR_FREE( showCmd );
        SEGPTR_FREE( cmdline );
    }
    else ret = GetLastError();

    if (name != lpCmdLine) HeapFree( GetProcessHeap(), 0, name );

    if (ret == 21)  /* 32-bit module */
    {
        SYSLEVEL_ReleaseWin16Lock();
        ret = WinExec( lpCmdLine, nCmdShow );
        SYSLEVEL_RestoreWin16Lock();
    }
    return ret;
}

/***********************************************************************
 *           WinExec   (KERNEL32.566)
 */
HINSTANCE WINAPI WinExec( LPCSTR lpCmdLine, UINT nCmdShow )
{
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    HINSTANCE hInstance;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = nCmdShow;

    if (CreateProcessA( NULL, (LPSTR)lpCmdLine, NULL, NULL, FALSE,
                        0, NULL, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (Callout.WaitForInputIdle ( info.hProcess, 30000 ) == 0xFFFFFFFF)
            WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        hInstance = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((hInstance = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", hInstance );
        hInstance = 11;
    }

    return hInstance;
}

/**********************************************************************
 *	    LoadModule    (KERNEL32.499)
 */
HINSTANCE WINAPI LoadModule( LPCSTR name, LPVOID paramBlock ) 
{
    LOADPARAMS *params = (LOADPARAMS *)paramBlock;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    HINSTANCE hInstance;
    LPSTR cmdline, p;
    char filename[MAX_PATH];
    BYTE len;

    if (!name) return ERROR_FILE_NOT_FOUND;

    if (!SearchPathA( NULL, name, ".exe", sizeof(filename), filename, NULL ) &&
        !SearchPathA( NULL, name, NULL, sizeof(filename), filename, NULL ))
        return GetLastError();

    len = (BYTE)params->lpCmdLine[0];
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(filename) + len + 2 )))
        return ERROR_NOT_ENOUGH_MEMORY;

    strcpy( cmdline, filename );
    p = cmdline + strlen(cmdline);
    *p++ = ' ';
    memcpy( p, params->lpCmdLine + 1, len );
    p[len] = 0;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    if (params->lpCmdShow)
    {
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = params->lpCmdShow[1];
    }
    
    if (CreateProcessA( filename, cmdline, NULL, NULL, FALSE, 0,
                        params->lpEnvAddress, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if ( Callout.WaitForInputIdle ( info.hProcess, 30000 ) ==  0xFFFFFFFF ) 
            WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        hInstance = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((hInstance = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", hInstance );
        hInstance = 11;
    }

    HeapFree( GetProcessHeap(), 0, cmdline );
    return hInstance;
}


/*************************************************************************
 *               get_file_name
 *
 * Helper for CreateProcess: retrieve the file name to load from the
 * app name and command line. Store the file name in buffer, and
 * return a possibly modified command line.
 */
static LPSTR get_file_name( LPCSTR appname, LPSTR cmdline, LPSTR buffer, int buflen )
{
    char *name, *pos, *ret = NULL;
    const char *p;

    /* if we have an app name, everything is easy */

    if (appname)
    {
        /* use the unmodified app name as file name */
        lstrcpynA( buffer, appname, buflen );
        if (!(ret = cmdline))
        {
            /* no command-line, create one */
            if ((ret = HeapAlloc( GetProcessHeap(), 0, strlen(appname) + 3 )))
                sprintf( ret, "\"%s\"", appname );
        }
        return ret;
    }

    if (!cmdline)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    /* first check for a quoted file name */

    if ((cmdline[0] == '"') && ((p = strchr( cmdline + 1, '"' ))))
    {
        int len = p - cmdline - 1;
        /* extract the quoted portion as file name */
        if (!(name = HeapAlloc( GetProcessHeap(), 0, len + 1 ))) return NULL;
        memcpy( name, cmdline + 1, len );
        name[len] = 0;

        if (SearchPathA( NULL, name, ".exe", buflen, buffer, NULL ) ||
            SearchPathA( NULL, name, NULL, buflen, buffer, NULL ))
            ret = cmdline;  /* no change necessary */
        goto done;
    }

    /* now try the command-line word by word */

    if (!(name = HeapAlloc( GetProcessHeap(), 0, strlen(cmdline) + 1 ))) return NULL;
    pos = name;
    p = cmdline;

    while (*p)
    {
        do *pos++ = *p++; while (*p && *p != ' ');
        *pos = 0;
        TRACE("trying '%s'\n", name );
        if (SearchPathA( NULL, name, ".exe", buflen, buffer, NULL ) ||
            SearchPathA( NULL, name, NULL, buflen, buffer, NULL ))
        {
            ret = cmdline;
            break;
        }
    }

    if (!ret || !strchr( name, ' ' )) goto done;  /* no change necessary */

    /* now build a new command-line with quotes */

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, strlen(cmdline) + 3 ))) goto done;
    sprintf( ret, "\"%s\"%s", name, p );

 done:
    HeapFree( GetProcessHeap(), 0, name );
    return ret;
}


/**********************************************************************
 *       CreateProcessA          (KERNEL32.171)
 */
BOOL WINAPI CreateProcessA( LPCSTR lpApplicationName, LPSTR lpCommandLine, 
                            LPSECURITY_ATTRIBUTES lpProcessAttributes,
                            LPSECURITY_ATTRIBUTES lpThreadAttributes,
                            BOOL bInheritHandles, DWORD dwCreationFlags,
                            LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,
                            LPSTARTUPINFOA lpStartupInfo,
                            LPPROCESS_INFORMATION lpProcessInfo )
{
    BOOL retv = FALSE;
    HANDLE hFile;
    DWORD type;
    char name[MAX_PATH];
    LPSTR tidy_cmdline;

    /* Process the AppName and/or CmdLine to get module name and path */

    TRACE("app '%s' cmdline '%s'\n", lpApplicationName, lpCommandLine );

    if (!(tidy_cmdline = get_file_name( lpApplicationName, lpCommandLine, name, sizeof(name) )))
        return FALSE;

    /* Warn if unsupported features are used */

    if (dwCreationFlags & DETACHED_PROCESS)
        FIXME("(%s,...): DETACHED_PROCESS ignored\n", name);
    if (dwCreationFlags & CREATE_NEW_CONSOLE)
        FIXME("(%s,...): CREATE_NEW_CONSOLE ignored\n", name);
    if (dwCreationFlags & NORMAL_PRIORITY_CLASS)
        FIXME("(%s,...): NORMAL_PRIORITY_CLASS ignored\n", name);
    if (dwCreationFlags & IDLE_PRIORITY_CLASS)
        FIXME("(%s,...): IDLE_PRIORITY_CLASS ignored\n", name);
    if (dwCreationFlags & HIGH_PRIORITY_CLASS)
        FIXME("(%s,...): HIGH_PRIORITY_CLASS ignored\n", name);
    if (dwCreationFlags & REALTIME_PRIORITY_CLASS)
        FIXME("(%s,...): REALTIME_PRIORITY_CLASS ignored\n", name);
    if (dwCreationFlags & CREATE_NEW_PROCESS_GROUP)
        FIXME("(%s,...): CREATE_NEW_PROCESS_GROUP ignored\n", name);
    if (dwCreationFlags & CREATE_UNICODE_ENVIRONMENT)
        FIXME("(%s,...): CREATE_UNICODE_ENVIRONMENT ignored\n", name);
    if (dwCreationFlags & CREATE_SEPARATE_WOW_VDM)
        FIXME("(%s,...): CREATE_SEPARATE_WOW_VDM ignored\n", name);
    if (dwCreationFlags & CREATE_SHARED_WOW_VDM)
        FIXME("(%s,...): CREATE_SHARED_WOW_VDM ignored\n", name);
    if (dwCreationFlags & CREATE_DEFAULT_ERROR_MODE)
        FIXME("(%s,...): CREATE_DEFAULT_ERROR_MODE ignored\n", name);
    if (dwCreationFlags & CREATE_NO_WINDOW)
        FIXME("(%s,...): CREATE_NO_WINDOW ignored\n", name);
    if (dwCreationFlags & PROFILE_USER)
        FIXME("(%s,...): PROFILE_USER ignored\n", name);
    if (dwCreationFlags & PROFILE_KERNEL)
        FIXME("(%s,...): PROFILE_KERNEL ignored\n", name);
    if (dwCreationFlags & PROFILE_SERVER)
        FIXME("(%s,...): PROFILE_SERVER ignored\n", name);
    if (lpCurrentDirectory)
        FIXME("(%s,...): lpCurrentDirectory %s ignored\n", 
                      name, lpCurrentDirectory);
    if (lpStartupInfo->lpDesktop)
        FIXME("(%s,...): lpStartupInfo->lpDesktop %s ignored\n", 
                      name, lpStartupInfo->lpDesktop);
    if (lpStartupInfo->lpTitle)
        FIXME("(%s,...): lpStartupInfo->lpTitle %s ignored\n", 
                      name, lpStartupInfo->lpTitle);
    if (lpStartupInfo->dwFlags & STARTF_USECOUNTCHARS)
        FIXME("(%s,...): STARTF_USECOUNTCHARS (%ld,%ld) ignored\n", 
                      name, lpStartupInfo->dwXCountChars, lpStartupInfo->dwYCountChars);
    if (lpStartupInfo->dwFlags & STARTF_USEFILLATTRIBUTE)
        FIXME("(%s,...): STARTF_USEFILLATTRIBUTE %lx ignored\n", 
                      name, lpStartupInfo->dwFillAttribute);
    if (lpStartupInfo->dwFlags & STARTF_RUNFULLSCREEN)
        FIXME("(%s,...): STARTF_RUNFULLSCREEN ignored\n", name);
    if (lpStartupInfo->dwFlags & STARTF_FORCEONFEEDBACK)
        FIXME("(%s,...): STARTF_FORCEONFEEDBACK ignored\n", name);
    if (lpStartupInfo->dwFlags & STARTF_FORCEOFFFEEDBACK)
        FIXME("(%s,...): STARTF_FORCEOFFFEEDBACK ignored\n", name);
    if (lpStartupInfo->dwFlags & STARTF_USEHOTKEY)
        FIXME("(%s,...): STARTF_USEHOTKEY ignored\n", name);

    /* Open file and determine executable type */

    hFile = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, -1 );
    if (hFile == INVALID_HANDLE_VALUE) goto done;

    if ( !MODULE_GetBinaryType( hFile, name, &type ) )
    {
        CloseHandle( hFile );
        retv = PROCESS_Create( -1, name, tidy_cmdline, lpEnvironment, 
                               lpProcessAttributes, lpThreadAttributes,
                               bInheritHandles, dwCreationFlags,
                               lpStartupInfo, lpProcessInfo );
        goto done;
    }

    /* Create process */

    switch ( type )
    {
    case SCS_32BIT_BINARY:
    case SCS_WOW_BINARY:
    case SCS_DOS_BINARY:
        retv = PROCESS_Create( hFile, name, tidy_cmdline, lpEnvironment, 
                               lpProcessAttributes, lpThreadAttributes,
                               bInheritHandles, dwCreationFlags,
                               lpStartupInfo, lpProcessInfo );
        break;

    case SCS_PIF_BINARY:
    case SCS_POSIX_BINARY:
    case SCS_OS216_BINARY:
        FIXME("Unsupported executable type: %ld\n", type );
        /* fall through */

    default:
        SetLastError( ERROR_BAD_FORMAT );
        break;
    }
    CloseHandle( hFile );

 done:
    if (tidy_cmdline != lpCommandLine) HeapFree( GetProcessHeap(), 0, tidy_cmdline );
    return retv;
}

/**********************************************************************
 *       CreateProcessW          (KERNEL32.172)
 * NOTES
 *  lpReserved is not converted
 */
BOOL WINAPI CreateProcessW( LPCWSTR lpApplicationName, LPWSTR lpCommandLine, 
                                LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                BOOL bInheritHandles, DWORD dwCreationFlags,
                                LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
                                LPSTARTUPINFOW lpStartupInfo,
                                LPPROCESS_INFORMATION lpProcessInfo )
{   BOOL ret;
    STARTUPINFOA StartupInfoA;
    
    LPSTR lpApplicationNameA = HEAP_strdupWtoA (GetProcessHeap(),0,lpApplicationName);
    LPSTR lpCommandLineA = HEAP_strdupWtoA (GetProcessHeap(),0,lpCommandLine);
    LPSTR lpCurrentDirectoryA = HEAP_strdupWtoA (GetProcessHeap(),0,lpCurrentDirectory);

    memcpy (&StartupInfoA, lpStartupInfo, sizeof(STARTUPINFOA));
    StartupInfoA.lpDesktop = HEAP_strdupWtoA (GetProcessHeap(),0,lpStartupInfo->lpDesktop);
    StartupInfoA.lpTitle = HEAP_strdupWtoA (GetProcessHeap(),0,lpStartupInfo->lpTitle);

    TRACE_(win32)("(%s,%s,...)\n", debugstr_w(lpApplicationName), debugstr_w(lpCommandLine));

    if (lpStartupInfo->lpReserved)
      FIXME_(win32)("StartupInfo.lpReserved is used, please report (%s)\n", debugstr_w(lpStartupInfo->lpReserved));
      
    ret = CreateProcessA(  lpApplicationNameA,  lpCommandLineA, 
                             lpProcessAttributes, lpThreadAttributes,
                             bInheritHandles, dwCreationFlags,
                             lpEnvironment, lpCurrentDirectoryA,
                             &StartupInfoA, lpProcessInfo );

    HeapFree( GetProcessHeap(), 0, lpCurrentDirectoryA );
    HeapFree( GetProcessHeap(), 0, lpCommandLineA );
    HeapFree( GetProcessHeap(), 0, StartupInfoA.lpDesktop );
    HeapFree( GetProcessHeap(), 0, StartupInfoA.lpTitle );

    return ret;
}

/***********************************************************************
 *              GetModuleHandleA         (KERNEL32.237)
 */
HMODULE WINAPI GetModuleHandleA(LPCSTR module)
{
    WINE_MODREF *wm;

    if ( module == NULL )
        wm = PROCESS_Current()->exe_modref;
    else
        wm = MODULE_FindModule( module );

    return wm? wm->module : 0;
}

/***********************************************************************
 *		GetModuleHandleW
 */
HMODULE WINAPI GetModuleHandleW(LPCWSTR module)
{
    HMODULE hModule;
    LPSTR modulea = HEAP_strdupWtoA( GetProcessHeap(), 0, module );
    hModule = GetModuleHandleA( modulea );
    HeapFree( GetProcessHeap(), 0, modulea );
    return hModule;
}


/***********************************************************************
 *              GetModuleFileNameA      (KERNEL32.235)
 *
 * GetModuleFileNameA seems to *always* return the long path;
 * it's only GetModuleFileName16 that decides between short/long path
 * by checking if exe version >= 4.0.
 * (SDK docu doesn't mention this)
 */
DWORD WINAPI GetModuleFileNameA( 
	HMODULE hModule,	/* [in] module handle (32bit) */
	LPSTR lpFileName,	/* [out] filenamebuffer */
        DWORD size		/* [in] size of filenamebuffer */
) {                   
    WINE_MODREF *wm = MODULE32_LookupHMODULE( hModule );

    if (!wm) /* can happen on start up or the like */
    	return 0;

    lstrcpynA( lpFileName, wm->filename, size );
       
    TRACE("%s\n", lpFileName );
    return strlen(lpFileName);
}                   
 

/***********************************************************************
 *              GetModuleFileNameW      (KERNEL32.236)
 */
DWORD WINAPI GetModuleFileNameW( HMODULE hModule, LPWSTR lpFileName,
                                   DWORD size )
{
    LPSTR fnA = (char*)HeapAlloc( GetProcessHeap(), 0, size );
    DWORD res = GetModuleFileNameA( hModule, fnA, size );
    lstrcpynAtoW( lpFileName, fnA, size );
    HeapFree( GetProcessHeap(), 0, fnA );
    return res;
}


/***********************************************************************
 *           LoadLibraryExA   (KERNEL32)
 */
HMODULE WINAPI LoadLibraryExA(LPCSTR libname, HANDLE hfile, DWORD flags)
{
	WINE_MODREF *wm;

	if(!libname)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	EnterCriticalSection(&PROCESS_Current()->crit_section);

	wm = MODULE_LoadLibraryExA( libname, hfile, flags );
	if ( wm )
	{
		if ( !MODULE_DllProcessAttach( wm, NULL ) )
		{
			WARN_(module)("Attach failed for module '%s', \n", libname);
			MODULE_FreeLibrary(wm);
			SetLastError(ERROR_DLL_INIT_FAILED);
			wm = NULL;
		}
	}

	LeaveCriticalSection(&PROCESS_Current()->crit_section);

	return wm ? wm->module : 0;
}

/***********************************************************************
 *	MODULE_LoadLibraryExA	(internal)
 *
 * Load a PE style module according to the load order.
 *
 * The HFILE parameter is not used and marked reserved in the SDK. I can
 * only guess that it should force a file to be mapped, but I rather
 * ignore the parameter because it would be extremely difficult to
 * integrate this with different types of module represenations.
 *
 */
WINE_MODREF *MODULE_LoadLibraryExA( LPCSTR libname, HFILE hfile, DWORD flags )
{
	DWORD err = GetLastError();
	WINE_MODREF *pwm;
	int i;
	module_loadorder_t *plo;

	EnterCriticalSection(&PROCESS_Current()->crit_section);

	/* Check for already loaded module */
	if((pwm = MODULE_FindModule(libname))) 
	{
		if(!(pwm->flags & WINE_MODREF_MARKER))
			pwm->refCount++;
		TRACE("Already loaded module '%s' at 0x%08x, count=%d, \n", libname, pwm->module, pwm->refCount);
		LeaveCriticalSection(&PROCESS_Current()->crit_section);
		return pwm;
	}

	plo = MODULE_GetLoadOrder(libname);

	for(i = 0; i < MODULE_LOADORDER_NTYPES; i++)
	{
                SetLastError( ERROR_FILE_NOT_FOUND );
		switch(plo->loadorder[i])
		{
		case MODULE_LOADORDER_DLL:
			TRACE("Trying native dll '%s'\n", libname);
			pwm = PE_LoadLibraryExA(libname, flags);
			break;

		case MODULE_LOADORDER_ELFDLL:
			TRACE("Trying elfdll '%s'\n", libname);
			if (!(pwm = BUILTIN32_LoadLibraryExA(libname, flags)))
                            pwm = ELFDLL_LoadLibraryExA(libname, flags);
			break;

		case MODULE_LOADORDER_SO:
			TRACE("Trying so-library '%s'\n", libname);
			if (!(pwm = BUILTIN32_LoadLibraryExA(libname, flags)))
                            pwm = ELF_LoadLibraryExA(libname, flags);
			break;

		case MODULE_LOADORDER_BI:
			TRACE("Trying built-in '%s'\n", libname);
			pwm = BUILTIN32_LoadLibraryExA(libname, flags);
			break;

		default:
			ERR("Got invalid loadorder type %d (%s index %d)\n", plo->loadorder[i], plo->modulename, i);
		/* Fall through */

		case MODULE_LOADORDER_INVALID:	/* We ignore this as it is an empty entry */
			pwm = NULL;
			break;
		}

		if(pwm)
		{
			/* Initialize DLL just loaded */
			TRACE("Loaded module '%s' at 0x%08x, \n", libname, pwm->module);

			/* Set the refCount here so that an attach failure will */
			/* decrement the dependencies through the MODULE_FreeLibrary call. */
			pwm->refCount++;

			LeaveCriticalSection(&PROCESS_Current()->crit_section);
                        SetLastError( err );  /* restore last error */
			return pwm;
		}

		if(GetLastError() != ERROR_FILE_NOT_FOUND)
			break;
	}

	WARN("Failed to load module '%s'; error=0x%08lx, \n", libname, GetLastError());
	LeaveCriticalSection(&PROCESS_Current()->crit_section);
	return NULL;
}

/***********************************************************************
 *           LoadLibraryA         (KERNEL32)
 */
HMODULE WINAPI LoadLibraryA(LPCSTR libname) {
	return LoadLibraryExA(libname,0,0);
}

/***********************************************************************
 *           LoadLibraryW         (KERNEL32)
 */
HMODULE WINAPI LoadLibraryW(LPCWSTR libnameW)
{
    return LoadLibraryExW(libnameW,0,0);
}

/***********************************************************************
 *           LoadLibrary32_16   (KERNEL.452)
 */
HMODULE WINAPI LoadLibrary32_16( LPCSTR libname )
{
    HMODULE hModule;

    SYSLEVEL_ReleaseWin16Lock();
    hModule = LoadLibraryA( libname );
    SYSLEVEL_RestoreWin16Lock();

    return hModule;
}

/***********************************************************************
 *           LoadLibraryExW       (KERNEL32)
 */
HMODULE WINAPI LoadLibraryExW(LPCWSTR libnameW,HANDLE hfile,DWORD flags)
{
    LPSTR libnameA = HEAP_strdupWtoA( GetProcessHeap(), 0, libnameW );
    HMODULE ret = LoadLibraryExA( libnameA , hfile, flags );

    HeapFree( GetProcessHeap(), 0, libnameA );
    return ret;
}

/***********************************************************************
 *           MODULE_FlushModrefs
 *
 * NOTE: Assumes that the process critical section is held!
 *
 * Remove all unused modrefs and call the internal unloading routines
 * for the library type.
 */
static void MODULE_FlushModrefs(void)
{
	WINE_MODREF *wm, *next;

	for(wm = PROCESS_Current()->modref_list; wm; wm = next)
	{
		next = wm->next;

		if(wm->refCount)
			continue;

		/* Unlink this modref from the chain */
		if(wm->next)
                        wm->next->prev = wm->prev;
		if(wm->prev)
                        wm->prev->next = wm->next;
		if(wm == PROCESS_Current()->modref_list)
			PROCESS_Current()->modref_list = wm->next;

		/* 
		 * The unloaders are also responsible for freeing the modref itself
		 * because the loaders were responsible for allocating it.
		 */
		switch(wm->type)
		{
                case MODULE32_PE:       if ( !(wm->flags & WINE_MODREF_INTERNAL) )
                                               PE_UnloadLibrary(wm);
                                        else
                                               BUILTIN32_UnloadLibrary(wm);
					break;
		case MODULE32_ELF:	ELF_UnloadLibrary(wm);		break;
		case MODULE32_ELFDLL:	ELFDLL_UnloadLibrary(wm);	break;

		default:
			ERR("Invalid or unhandled MODREF type %d encountered (wm=%p)\n", wm->type, wm);
		}
	}
}

/***********************************************************************
 *           FreeLibrary
 */
BOOL WINAPI FreeLibrary(HINSTANCE hLibModule)
{
    BOOL retv = FALSE;
    WINE_MODREF *wm;

    EnterCriticalSection( &PROCESS_Current()->crit_section );
    PROCESS_Current()->free_lib_count++;

    wm = MODULE32_LookupHMODULE( hLibModule );
    if ( !wm || !hLibModule )
        SetLastError( ERROR_INVALID_HANDLE );
    else
        retv = MODULE_FreeLibrary( wm );

    PROCESS_Current()->free_lib_count--;
    LeaveCriticalSection( &PROCESS_Current()->crit_section );

    return retv;
}

/***********************************************************************
 *           MODULE_DecRefCount
 *
 * NOTE: Assumes that the process critical section is held!
 */
static void MODULE_DecRefCount( WINE_MODREF *wm )
{
    int i;

    if ( wm->flags & WINE_MODREF_MARKER )
        return;

    if ( wm->refCount <= 0 )
        return;

    --wm->refCount;
    TRACE("(%s) refCount: %d\n", wm->modname, wm->refCount );

    if ( wm->refCount == 0 )
    {
        wm->flags |= WINE_MODREF_MARKER;

        for ( i = 0; i < wm->nDeps; i++ )
            if ( wm->deps[i] )
                MODULE_DecRefCount( wm->deps[i] );

        wm->flags &= ~WINE_MODREF_MARKER;
    }
}

/***********************************************************************
 *           MODULE_FreeLibrary
 *
 * NOTE: Assumes that the process critical section is held!
 */
BOOL MODULE_FreeLibrary( WINE_MODREF *wm )
{
    TRACE("(%s) - START\n", wm->modname );

    /* Recursively decrement reference counts */
    MODULE_DecRefCount( wm );

    /* Call process detach notifications */
    if ( PROCESS_Current()->free_lib_count <= 1 )
    {
        struct unload_dll_request *req = get_req_buffer();

        MODULE_DllProcessDetach( FALSE, NULL );
        req->base = (void *)wm->module;
        server_call_noerr( REQ_UNLOAD_DLL );
    
        MODULE_FlushModrefs();
    }

    TRACE("END\n");

    return TRUE;
}


/***********************************************************************
 *           FreeLibraryAndExitThread
 */
VOID WINAPI FreeLibraryAndExitThread(HINSTANCE hLibModule, DWORD dwExitCode)
{
    FreeLibrary(hLibModule);
    ExitThread(dwExitCode);
}

/***********************************************************************
 *           PrivateLoadLibrary       (KERNEL32)
 *
 * FIXME: rough guesswork, don't know what "Private" means
 */
HINSTANCE WINAPI PrivateLoadLibrary(LPCSTR libname)
{
        return (HINSTANCE)LoadLibrary16(libname);
}



/***********************************************************************
 *           PrivateFreeLibrary       (KERNEL32)
 *
 * FIXME: rough guesswork, don't know what "Private" means
 */
void WINAPI PrivateFreeLibrary(HINSTANCE handle)
{
	FreeLibrary16((HINSTANCE16)handle);
}


/***********************************************************************
 *           WIN32_GetProcAddress16   (KERNEL32.36)
 * Get procaddress in 16bit module from win32... (kernel32 undoc. ordinal func)
 */
FARPROC16 WINAPI WIN32_GetProcAddress16( HMODULE hModule, LPCSTR name )
{
    WORD	ordinal;
    FARPROC16	ret;

    if (!hModule) {
    	WARN("hModule may not be 0!\n");
	return (FARPROC16)0;
    }
    if (HIWORD(hModule))
    {
    	WARN("hModule is Win32 handle (%08x)\n", hModule );
	return (FARPROC16)0;
    }
    hModule = GetExePtr( hModule );
    if (HIWORD(name)) {
        ordinal = NE_GetOrdinal( hModule, name );
        TRACE("%04x '%s'\n", hModule, name );
    } else {
        ordinal = LOWORD(name);
        TRACE("%04x %04x\n", hModule, ordinal );
    }
    if (!ordinal) return (FARPROC16)0;
    ret = NE_GetEntryPoint( hModule, ordinal );
    TRACE("returning %08x\n",(UINT)ret);
    return ret;
}

/***********************************************************************
 *           GetProcAddress16   (KERNEL.50)
 */
FARPROC16 WINAPI GetProcAddress16( HMODULE16 hModule, SEGPTR name )
{
    WORD ordinal;
    FARPROC16 ret;

    if (!hModule) hModule = GetCurrentTask();
    hModule = GetExePtr( hModule );

    if (HIWORD(name) != 0)
    {
        ordinal = NE_GetOrdinal( hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
        TRACE("%04x '%s'\n", hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
    }
    else
    {
        ordinal = LOWORD(name);
        TRACE("%04x %04x\n", hModule, ordinal );
    }
    if (!ordinal) return (FARPROC16)0;

    ret = NE_GetEntryPoint( hModule, ordinal );

    TRACE("returning %08x\n", (UINT)ret );
    return ret;
}


/***********************************************************************
 *           GetProcAddress   		(KERNEL32.257)
 */
FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR function )
{
    return MODULE_GetProcAddress( hModule, function, TRUE );
}

/***********************************************************************
 *           GetProcAddress32   		(KERNEL.453)
 */
FARPROC WINAPI GetProcAddress32_16( HMODULE hModule, LPCSTR function )
{
    return MODULE_GetProcAddress( hModule, function, FALSE );
}

/***********************************************************************
 *           MODULE_GetProcAddress   		(internal)
 */
FARPROC MODULE_GetProcAddress( 
	HMODULE hModule, 	/* [in] current module handle */
	LPCSTR function,	/* [in] function to be looked up */
	BOOL snoop )
{
    WINE_MODREF	*wm = MODULE32_LookupHMODULE( hModule );
    FARPROC	retproc;

    if (HIWORD(function))
	TRACE_(win32)("(%08lx,%s)\n",(DWORD)hModule,function);
    else
	TRACE_(win32)("(%08lx,%p)\n",(DWORD)hModule,function);
    if (!wm) {
    	SetLastError(ERROR_INVALID_HANDLE);
        return (FARPROC)0;
    }
    switch (wm->type)
    {
    case MODULE32_PE:
     	retproc = PE_FindExportedFunction( wm, function, snoop );
	if (!retproc) SetLastError(ERROR_PROC_NOT_FOUND);
	return retproc;
    case MODULE32_ELF:
    	retproc = ELF_FindExportedFunction( wm, function);
	if (!retproc) SetLastError(ERROR_PROC_NOT_FOUND);
	return retproc;
    default:
    	ERR("wine_modref type %d not handled.\n",wm->type);
    	SetLastError(ERROR_INVALID_HANDLE);
    	return (FARPROC)0;
    }
}


/***********************************************************************
 *           RtlImageNtHeader   (NTDLL)
 */
PIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE hModule)
{
    /* basically:
     * return  hModule+(((IMAGE_DOS_HEADER*)hModule)->e_lfanew); 
     * but we could get HMODULE16 or the like (think builtin modules)
     */

    WINE_MODREF	*wm = MODULE32_LookupHMODULE( hModule );
    if (!wm || (wm->type != MODULE32_PE)) return (PIMAGE_NT_HEADERS)0;
    return PE_HEADER(wm->module);
}


/***************************************************************************
 *              HasGPHandler                    (KERNEL.338)
 */

#include "pshpack1.h"
typedef struct _GPHANDLERDEF
{
    WORD selector;
    WORD rangeStart;
    WORD rangeEnd;
    WORD handler;
} GPHANDLERDEF;
#include "poppack.h"

SEGPTR WINAPI HasGPHandler16( SEGPTR address )
{
    HMODULE16 hModule;
    int gpOrdinal;
    SEGPTR gpPtr;
    GPHANDLERDEF *gpHandler;
   
    if (    (hModule = FarGetOwner16( SELECTOROF(address) )) != 0
         && (gpOrdinal = NE_GetOrdinal( hModule, "__GP" )) != 0
         && (gpPtr = (SEGPTR)NE_GetEntryPointEx( hModule, gpOrdinal, FALSE )) != 0
         && !IsBadReadPtr16( gpPtr, sizeof(GPHANDLERDEF) )
         && (gpHandler = PTR_SEG_TO_LIN( gpPtr )) != NULL )
    {
        while (gpHandler->selector)
        {
            if (    SELECTOROF(address) == gpHandler->selector
                 && OFFSETOF(address)   >= gpHandler->rangeStart
                 && OFFSETOF(address)   <  gpHandler->rangeEnd  )
                return PTR_SEG_OFF_TO_SEGPTR( gpHandler->selector,
                                              gpHandler->handler );
            gpHandler++;
        }
    }

    return 0;
}

