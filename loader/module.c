/*
 * Modules
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "windef.h"
#include "winerror.h"
#include "class.h"
#include "file.h"
#include "global.h"
#include "heap.h"
#include "module.h"
#include "neexe.h"
#include "pe_image.h"
#include "dosexe.h"
#include "process.h"
#include "thread.h"
#include "selectors.h"
#include "stackframe.h"
#include "task.h"
#include "debug.h"
#include "callback.h"
#include "loadorder.h"
#include "elfdll.h"

DECLARE_DEBUG_CHANNEL(module)
DECLARE_DEBUG_CHANNEL(win32)


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
    	ERR(module,"tried to lookup 0x%04x in win32 module handler!\n",hmod);
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


    TRACE( module, "(%s,%s,%p) - CALL\n", 
           wm->modname, typeName[type], lpReserved );

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
        ERR( module, "wine_modref type %d not handled.\n", wm->type );
        retv = FALSE;
        break;
    }

    TRACE( module, "(%s,%s,%p) - RETURN %d\n", 
           wm->modname, typeName[type], lpReserved, retv );

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

    TRACE( module, "(%s,%p) - START\n", 
           wm->modname, lpReserved );

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

    TRACE( module, "(%s,%p) - END\n", 
           wm->modname, lpReserved );

    return retv;
}

/*************************************************************************
 *		MODULE_DllProcessDetach
 * 
 * Send DLL process detach notifications.  See the comment about calling 
 * sequence at MODULE_DllProcessAttach.  Unless the bForceDetach flag
 * is set, only DLLs with zero refcount are notified.
 *
 * NOTE: Assumes that the process critical section is held!
 *
 */
void MODULE_DllProcessDetach( BOOL bForceDetach, LPVOID lpReserved )
{
    WINE_MODREF *wm;

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
HMODULE MODULE_CreateDummyModule( const OFSTRUCT *ofs, LPCSTR modName )
{
    HMODULE hModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    char *pStr,*s;
    int len;
    const char* basename;

    INT of_size = sizeof(OFSTRUCT) - sizeof(ofs->szPathName)
                    + strlen(ofs->szPathName) + 1;
    INT size = sizeof(NE_MODULE) +
                 /* loaded file info */
                 of_size +
                 /* segment table: DS,CS */
                 2 * sizeof(SEGTABLEENTRY) +
                 /* name table */
                 9 +
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
    pModule->expected_version = 0x030a;
    pModule->self             = hModule;

    /* Set loaded file information */
    memcpy( pModule + 1, ofs, of_size );
    ((OFSTRUCT *)(pModule+1))->cBytes = of_size - 1;

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
    if ( modName )
        basename = modName;
    else
    {
        basename = strrchr(ofs->szPathName,'\\');
        if (!basename) basename = ofs->szPathName;
        else basename++;
    }
    len = strlen(basename);
    if ((s = strchr(basename,'.'))) len = s - basename;
    if (len > 8) len = 8;
    *pStr = len;
    strncpy( pStr+1, basename, len );
    if (len < 8) pStr[len+1] = 0;
    pStr += 9;

    /* All tables zero terminated */
    pModule->res_table = pModule->import_table = pModule->entry_table =
		(int)pStr - (int)pModule;

    NE_RegisterModule( pModule );
    return hModule;
}


/***********************************************************************
 *           MODULE_GetWndProcEntry16  (not a Windows API function)
 *
 * Return an entry point from the WPROCS dll.
 */
FARPROC16 MODULE_GetWndProcEntry16( LPCSTR name )
{
    FARPROC16 ret = NULL;

    if (__winelib)
    {
        /* FIXME: hack for Winelib */
        extern LRESULT ColorDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT FileOpenDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT FileSaveDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT FindTextDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT PrintDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT PrintSetupDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT ReplaceTextDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);

        if (!strcmp(name,"ColorDlgProc"))
            return (FARPROC16)ColorDlgProc16;
        if (!strcmp(name,"FileOpenDlgProc"))
            return (FARPROC16)FileOpenDlgProc16;
        if (!strcmp(name,"FileSaveDlgProc"))
            return (FARPROC16)FileSaveDlgProc16;
        if (!strcmp(name,"FindTextDlgProc"))
            return (FARPROC16)FindTextDlgProc16;
        if (!strcmp(name,"PrintDlgProc"))
            return (FARPROC16)PrintDlgProc16;
        if (!strcmp(name,"PrintSetupDlgProc"))
            return (FARPROC16)PrintSetupDlgProc16;
        if (!strcmp(name,"ReplaceTextDlgProc"))
            return (FARPROC16)ReplaceTextDlgProc16;
        FIXME(module,"No mapping for %s(), add one in library/miscstubs.c\n",name);
        assert( FALSE );
        return NULL;
    }
    else
    {
        WORD ordinal;
        static HMODULE hModule = 0;

        if (!hModule) hModule = GetModuleHandle16( "WPROCS" );
        ordinal = NE_GetOrdinal( hModule, name );
        if (!(ret = NE_GetEntryPoint( hModule, ordinal )))
        {            
            WARN( module, "%s not found\n", name );
            assert( FALSE );
        }
    }
    return ret;
}


/**********************************************************************
 *	    MODULE_FindModule32
 *
 * Find a (loaded) win32 module depending on path
 * The handling of '.' is a bit weird, but we need it that way, 
 * for sometimes the programs use '<name>.exe' and '<name>.dll' and
 * this is the only way to differentiate. (mainly hypertrm.exe)
 *
 * RETURNS
 *	the module handle if found
 * 	0 if not
 */
WINE_MODREF *MODULE_FindModule(
	LPCSTR path	/* [in] pathname of module/library to be found */
) {
    LPSTR	filename;
    LPSTR	dotptr;
    WINE_MODREF	*wm;

    if (!(filename = strrchr( path, '\\' )))
    	filename = HEAP_strdupA( GetProcessHeap(), 0, path );
    else 
    	filename = HEAP_strdupA( GetProcessHeap(), 0, filename+1 );
    dotptr=strrchr(filename,'.');

    for ( wm = PROCESS_Current()->modref_list; wm; wm=wm->next ) {
    	LPSTR	xmodname,xdotptr;

	assert (wm->modname);
	xmodname = HEAP_strdupA( GetProcessHeap(), 0, wm->modname );
	xdotptr=strrchr(xmodname,'.');
	if (	(xdotptr && !dotptr) ||
		(!xdotptr && dotptr)
	) {
	    if (dotptr)	*dotptr		= '\0';
	    if (xdotptr) *xdotptr	= '\0';
	}
	if (!strcasecmp( filename, xmodname)) {
	    HeapFree( GetProcessHeap(), 0, filename );
	    HeapFree( GetProcessHeap(), 0, xmodname );
	    return wm;
	}
	if (dotptr) *dotptr='.';
	/* FIXME: add paths, shortname */
	HeapFree( GetProcessHeap(), 0, xmodname );
    }
    /* if that fails, try looking for the filename... */
    for ( wm = PROCESS_Current()->modref_list; wm; wm=wm->next ) {
    	LPSTR	xlname,xdotptr;

	assert (wm->longname);
	xlname = strrchr(wm->longname,'\\');
	if (!xlname) 
	    xlname = wm->longname;
	else
	    xlname++;
	xlname = HEAP_strdupA( GetProcessHeap(), 0, xlname );
	xdotptr=strrchr(xlname,'.');
	if (	(xdotptr && !dotptr) ||
		(!xdotptr && dotptr)
	) {
	    if (dotptr)	*dotptr		= '\0';
	    if (xdotptr) *xdotptr	= '\0';
	}
	if (!strcasecmp( filename, xlname)) {
	    HeapFree( GetProcessHeap(), 0, filename );
	    HeapFree( GetProcessHeap(), 0, xlname );
	    return wm;
	}
	if (dotptr) *dotptr='.';
	/* FIXME: add paths, shortname */
	HeapFree( GetProcessHeap(), 0, xlname );
    }
    HeapFree( GetProcessHeap(), 0, filename );
    return NULL;
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
 * SCS_OS216_BINARY: A 16bit OS/2 based application ( Not implemented )
 *
 * Returns TRUE if the file is an executable in which case
 * the value pointed by lpBinaryType is set.
 * Returns FALSE if the file is not an executable or if the function fails.
 *
 * To do so it opens the file and reads in the header information
 * if the extended header information is not presend it will
 * assume that that the file is a DOS executable.
 * If the extended header information is present it will
 * determine if the file is an 16 or 32 bit Windows executable
 * by check the flags in the header.
 *
 * Note that .COM and .PIF files are only recognized by their
 * file name extension; but Windows does it the same way ...
 */
static BOOL MODULE_GetBinaryType( HFILE hfile, OFSTRUCT *ofs, 
                                  LPDWORD lpBinaryType )
{
    IMAGE_DOS_HEADER mz_header;
    char magic[4], *ptr;

    /* Seek to the start of the file and read the DOS header information.
     */
    if ( _llseek( hfile, 0, SEEK_SET ) >= 0  &&
         _lread( hfile, &mz_header, sizeof(mz_header) ) == sizeof(mz_header) )
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
                if ( ( mz_header.e_crlc == 0 && mz_header.e_lfarlc == 0 ) ||
                     ( mz_header.e_lfarlc >= sizeof(IMAGE_DOS_HEADER) ) )
                    if ( mz_header.e_lfanew >= sizeof(IMAGE_DOS_HEADER) &&
                         _llseek( hfile, mz_header.e_lfanew, SEEK_SET ) >= 0 &&
                         _lread( hfile, magic, sizeof(magic) ) == sizeof(magic) )
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
                     * header.      This is a bit misleading, but it is
                     * documented in the SDK. ( for more details see
                     * the neexe.h file )
                     */
                     *lpBinaryType = SCS_WOW_BINARY;
                     return TRUE;
                }
                else
                {
                    /* Unknown extended header, so abort.
                     */
                    return FALSE;
                }
            }
        }
    }

    /* If we get here, we don't even have a correct MZ header.
     * Try to check the file extension for known types ...
     */
    ptr = strrchr( ofs->szPathName, '.' );
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
    HFILE hfile;
    OFSTRUCT ofs;

    TRACE( win32, "%s\n", lpApplicationName );

    /* Sanity check.
     */
    if ( lpApplicationName == NULL || lpBinaryType == NULL )
        return FALSE;

    /* Open the file indicated by lpApplicationName for reading.
     */
    if ( (hfile = OpenFile( lpApplicationName, &ofs, OF_READ )) == HFILE_ERROR )
        return FALSE;

    /* Check binary type
     */
    ret = MODULE_GetBinaryType( hfile, &ofs, lpBinaryType );

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

    TRACE( win32, "%s\n", debugstr_w(lpApplicationName) );

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

/**********************************************************************
 *	    MODULE_CreateUnixProcess
 */
static BOOL MODULE_CreateUnixProcess( LPCSTR filename, LPCSTR lpCmdLine,
                                      LPSTARTUPINFOA lpStartupInfo,
                                      LPPROCESS_INFORMATION lpProcessInfo,
                                      BOOL useWine )
{
    DOS_FULL_NAME full_name;
    const char *unixfilename = filename;
    const char *argv[256], **argptr;
    BOOL iconic = FALSE;

    /* Get Unix file name and iconic flag */

    if ( lpStartupInfo->dwFlags & STARTF_USESHOWWINDOW )
        if (    lpStartupInfo->wShowWindow == SW_SHOWMINIMIZED
             || lpStartupInfo->wShowWindow == SW_SHOWMINNOACTIVE )
            iconic = TRUE;

    if (    strchr(filename, '/') 
         || strchr(filename, ':') 
         || strchr(filename, '\\') )
    {
        if ( DOSFS_GetFullName( filename, TRUE, &full_name ) )
            unixfilename = full_name.long_name;
    }

    if ( !unixfilename )
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return FALSE;
    }

    /* Build argument list */

    argptr = argv;
    if ( !useWine )
    {
        char *p = strdup(lpCmdLine);
        *argptr++ = unixfilename;
        if (iconic) *argptr++ = "-iconic";
        while (1)
        {
            while (*p && (*p == ' ' || *p == '\t')) *p++ = '\0';
            if (!*p) break;
            *argptr++ = p;
            while (*p && *p != ' ' && *p != '\t') p++;
        }
    }
    else
    {
        *argptr++ = "wine";
        if (iconic) *argptr++ = "-iconic";
        *argptr++ = lpCmdLine;
    }
    *argptr++ = 0;

    /* Fork and execute */

    if ( !fork() )
    {
        /* Note: don't use Wine routines here, as this process
                 has not been correctly initialized! */

        execvp( argv[0], (char**)argv );

        /* Failed ! */
        if ( useWine )
            fprintf( stderr, "CreateProcess: can't exec 'wine %s'\n", 
                             lpCmdLine );
        exit( 1 );
    }

    /* Fake success return value */

    memset( lpProcessInfo, '\0', sizeof( *lpProcessInfo ) );
    lpProcessInfo->hProcess = INVALID_HANDLE_VALUE;
    lpProcessInfo->hThread  = INVALID_HANDLE_VALUE;

    SetLastError( ERROR_SUCCESS );
    return TRUE;
}

/***********************************************************************
 *           WinExec16   (KERNEL.166)
 */
HINSTANCE16 WINAPI WinExec16( LPCSTR lpCmdLine, UINT16 nCmdShow )
{
    return WinExec( lpCmdLine, nCmdShow );
}

/***********************************************************************
 *           WinExec   (KERNEL32.566)
 */
HINSTANCE WINAPI WinExec( LPCSTR lpCmdLine, UINT nCmdShow )
{
    LOADPARAMS params;
    UINT16 paramCmdShow[2];

    if (!lpCmdLine)
        return 2;  /* File not found */

    /* Set up LOADPARAMS buffer for LoadModule */

    memset( &params, '\0', sizeof(params) );
    params.lpCmdLine    = (LPSTR)lpCmdLine;
    params.lpCmdShow    = paramCmdShow;
    params.lpCmdShow[0] = 2;
    params.lpCmdShow[1] = nCmdShow;

    /* Now load the executable file */

    return LoadModule( NULL, &params );
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
    PDB *pdb;
    TDB *tdb;

    memset( &startup, '\0', sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = params->lpCmdShow? params->lpCmdShow[1] : 0;

    if ( !CreateProcessA( name, params->lpCmdLine,
                          NULL, NULL, FALSE, 0, params->lpEnvAddress,
                          NULL, &startup, &info ) )
    {
        hInstance = GetLastError();
        if ( hInstance < 32 ) return hInstance;

        FIXME( module, "Strange error set by CreateProcess: %d\n", hInstance );
        return 11;
    }
    
    /* Get 16-bit hInstance/hTask from process */
    pdb = PROCESS_IdToPDB( info.dwProcessId );
    tdb = pdb? (TDB *)GlobalLock16( pdb->task ) : NULL;
    hInstance = tdb && tdb->hInstance? tdb->hInstance : pdb? pdb->task : 0;

    /* Close off the handles */
    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );

    return hInstance;
}


static void get_executable_name( LPCSTR line, LPSTR name, int namelen,
                                 LPCSTR *after, BOOL extension )
{
    int len = 0;
    LPCSTR p = NULL, pcmd = NULL;

    while ( *line == ' ' ) line++;
    if ( *line == '"' )
    {
        line++;      /* skip '"' */
        if ((pcmd = strchr(line, '"'))) /* closing '"' available, too ? */
            len = ++pcmd - line;
    }
    else
    {
	if((p = strchr(line, ' '))) 
	{
		len = p - line;
		pcmd = p+1;
	}
	else
		len = strlen(line);
	
	len++;
    }
    if(len > (namelen - 4)) len = namelen - 4;
    lstrcpynA(name, line, len);
    if(extension && (strrchr(name, '.') <= strrchr(name, '\\')) )
        	strcat(name, ".exe");
    if (after) *after = pcmd;
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
    HFILE hFile;
    OFSTRUCT ofs;
    DWORD type;
    char name[256];
    LPCSTR cmdline;

    /* Get name and command line */

    if (!lpApplicationName && !lpCommandLine)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return FALSE;
    }

    name[0] = '\0';

    if (lpApplicationName) {
       get_executable_name( lpApplicationName, name, sizeof(name), NULL, TRUE );
    }
    else {
       get_executable_name( lpCommandLine, name, sizeof ( name ), NULL, TRUE );
    }
    if (!lpCommandLine) 
      cmdline = lpApplicationName;
    else cmdline = lpCommandLine;

    /* Warn if unsupported features are used */

    if (dwCreationFlags & DEBUG_PROCESS)
        FIXME(module, "(%s,...): DEBUG_PROCESS ignored\n", name);
    if (dwCreationFlags & DEBUG_ONLY_THIS_PROCESS)
        FIXME(module, "(%s,...): DEBUG_ONLY_THIS_PROCESS ignored\n", name);
    if (dwCreationFlags & CREATE_SUSPENDED)
        FIXME(module, "(%s,...): CREATE_SUSPENDED ignored\n", name);
    if (dwCreationFlags & DETACHED_PROCESS)
        FIXME(module, "(%s,...): DETACHED_PROCESS ignored\n", name);
    if (dwCreationFlags & CREATE_NEW_CONSOLE)
        FIXME(module, "(%s,...): CREATE_NEW_CONSOLE ignored\n", name);
    if (dwCreationFlags & NORMAL_PRIORITY_CLASS)
        FIXME(module, "(%s,...): NORMAL_PRIORITY_CLASS ignored\n", name);
    if (dwCreationFlags & IDLE_PRIORITY_CLASS)
        FIXME(module, "(%s,...): IDLE_PRIORITY_CLASS ignored\n", name);
    if (dwCreationFlags & HIGH_PRIORITY_CLASS)
        FIXME(module, "(%s,...): HIGH_PRIORITY_CLASS ignored\n", name);
    if (dwCreationFlags & REALTIME_PRIORITY_CLASS)
        FIXME(module, "(%s,...): REALTIME_PRIORITY_CLASS ignored\n", name);
    if (dwCreationFlags & CREATE_NEW_PROCESS_GROUP)
        FIXME(module, "(%s,...): CREATE_NEW_PROCESS_GROUP ignored\n", name);
    if (dwCreationFlags & CREATE_UNICODE_ENVIRONMENT)
        FIXME(module, "(%s,...): CREATE_UNICODE_ENVIRONMENT ignored\n", name);
    if (dwCreationFlags & CREATE_SEPARATE_WOW_VDM)
        FIXME(module, "(%s,...): CREATE_SEPARATE_WOW_VDM ignored\n", name);
    if (dwCreationFlags & CREATE_SHARED_WOW_VDM)
        FIXME(module, "(%s,...): CREATE_SHARED_WOW_VDM ignored\n", name);
    if (dwCreationFlags & CREATE_DEFAULT_ERROR_MODE)
        FIXME(module, "(%s,...): CREATE_DEFAULT_ERROR_MODE ignored\n", name);
    if (dwCreationFlags & CREATE_NO_WINDOW)
        FIXME(module, "(%s,...): CREATE_NO_WINDOW ignored\n", name);
    if (dwCreationFlags & PROFILE_USER)
        FIXME(module, "(%s,...): PROFILE_USER ignored\n", name);
    if (dwCreationFlags & PROFILE_KERNEL)
        FIXME(module, "(%s,...): PROFILE_KERNEL ignored\n", name);
    if (dwCreationFlags & PROFILE_SERVER)
        FIXME(module, "(%s,...): PROFILE_SERVER ignored\n", name);
    if (lpCurrentDirectory)
        FIXME(module, "(%s,...): lpCurrentDirectory %s ignored\n", 
                      name, lpCurrentDirectory);
    if (lpStartupInfo->lpDesktop)
        FIXME(module, "(%s,...): lpStartupInfo->lpDesktop %s ignored\n", 
                      name, lpStartupInfo->lpDesktop);
    if (lpStartupInfo->lpTitle)
        FIXME(module, "(%s,...): lpStartupInfo->lpTitle %s ignored\n", 
                      name, lpStartupInfo->lpTitle);
    if (lpStartupInfo->dwFlags & STARTF_USECOUNTCHARS)
        FIXME(module, "(%s,...): STARTF_USECOUNTCHARS (%ld,%ld) ignored\n", 
                      name, lpStartupInfo->dwXCountChars, lpStartupInfo->dwYCountChars);
    if (lpStartupInfo->dwFlags & STARTF_USEFILLATTRIBUTE)
        FIXME(module, "(%s,...): STARTF_USEFILLATTRIBUTE %lx ignored\n", 
                      name, lpStartupInfo->dwFillAttribute);
    if (lpStartupInfo->dwFlags & STARTF_RUNFULLSCREEN)
        FIXME(module, "(%s,...): STARTF_RUNFULLSCREEN ignored\n", name);
    if (lpStartupInfo->dwFlags & STARTF_FORCEONFEEDBACK)
        FIXME(module, "(%s,...): STARTF_FORCEONFEEDBACK ignored\n", name);
    if (lpStartupInfo->dwFlags & STARTF_FORCEOFFFEEDBACK)
        FIXME(module, "(%s,...): STARTF_FORCEOFFFEEDBACK ignored\n", name);
    if (lpStartupInfo->dwFlags & STARTF_USEHOTKEY)
        FIXME(module, "(%s,...): STARTF_USEHOTKEY ignored\n", name);


    /* When in WineLib, always fork new Unix process */

    if ( __winelib )
        return MODULE_CreateUnixProcess( name, cmdline, 
                                         lpStartupInfo, lpProcessInfo, TRUE );

    /* Check for special case: second instance of NE module */

    lstrcpynA( ofs.szPathName, name, sizeof( ofs.szPathName ) );
    retv = NE_CreateProcess( HFILE_ERROR, &ofs, cmdline, lpEnvironment, 
                             lpProcessAttributes, lpThreadAttributes,
                             bInheritHandles, lpStartupInfo, lpProcessInfo );

    /* Load file and create process */

    if ( !retv )
    {
        /* Open file and determine executable type */

        if ( (hFile = OpenFile( name, &ofs, OF_READ )) == HFILE_ERROR )
        {
            SetLastError( ERROR_FILE_NOT_FOUND );
            return FALSE;
        }

        if ( !MODULE_GetBinaryType( hFile, &ofs, &type ) )
        {
            CloseHandle( hFile );

            /* FIXME: Try Unix executable only when appropriate! */
            if ( MODULE_CreateUnixProcess( name, cmdline, 
                                           lpStartupInfo, lpProcessInfo, FALSE ) )
                return TRUE;

            SetLastError( ERROR_BAD_FORMAT );
            return FALSE;
        }


        /* Create process */

        switch ( type )
        {
        case SCS_32BIT_BINARY:
            retv = PE_CreateProcess( hFile, &ofs, cmdline, lpEnvironment, 
                                     lpProcessAttributes, lpThreadAttributes,
                                     bInheritHandles, lpStartupInfo, lpProcessInfo );
            break;
    
        case SCS_DOS_BINARY:
            retv = MZ_CreateProcess( hFile, &ofs, cmdline, lpEnvironment, 
                                     lpProcessAttributes, lpThreadAttributes,
                                     bInheritHandles, lpStartupInfo, lpProcessInfo );
            break;

        case SCS_WOW_BINARY:
            retv = NE_CreateProcess( hFile, &ofs, cmdline, lpEnvironment, 
                                     lpProcessAttributes, lpThreadAttributes,
                                     bInheritHandles, lpStartupInfo, lpProcessInfo );
            break;

        case SCS_PIF_BINARY:
        case SCS_POSIX_BINARY:
        case SCS_OS216_BINARY:
            FIXME( module, "Unsupported executable type: %ld\n", type );
            /* fall through */
    
        default:
            SetLastError( ERROR_BAD_FORMAT );
            retv = FALSE;
            break;
        }

        CloseHandle( hFile );
    }
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

    TRACE(win32, "(%s,%s,...)\n", debugstr_w(lpApplicationName), debugstr_w(lpCommandLine));

    if (lpStartupInfo->lpReserved)
      FIXME(win32,"StartupInfo.lpReserved is used, please report (%s)\n", debugstr_w(lpStartupInfo->lpReserved));
      
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
 *              GetModuleHandle         (KERNEL32.237)
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

HMODULE WINAPI GetModuleHandleW(LPCWSTR module)
{
    HMODULE hModule;
    LPSTR modulea = HEAP_strdupWtoA( GetProcessHeap(), 0, module );
    hModule = GetModuleHandleA( modulea );
    HeapFree( GetProcessHeap(), 0, modulea );
    return hModule;
}


/***********************************************************************
 *              GetModuleFileName32A      (KERNEL32.235)
 */
DWORD WINAPI GetModuleFileNameA( 
	HMODULE hModule,	/* [in] module handle (32bit) */
	LPSTR lpFileName,	/* [out] filenamebuffer */
        DWORD size		/* [in] size of filenamebuffer */
) {                   
    WINE_MODREF *wm = MODULE32_LookupHMODULE( hModule );

    if (!wm) /* can happen on start up or the like */
    	return 0;

    if (PE_HEADER(wm->module)->OptionalHeader.MajorOperatingSystemVersion >= 4.0)
      lstrcpynA( lpFileName, wm->longname, size );
    else
      lstrcpynA( lpFileName, wm->shortname, size );
       
    TRACE(module, "%s\n", lpFileName );
    return strlen(lpFileName);
}                   
 

/***********************************************************************
 *              GetModuleFileName32W      (KERNEL32.236)
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
 *           LoadLibraryEx32W   (KERNEL.513)
 * FIXME
 */
HMODULE WINAPI LoadLibraryEx32W16( LPCSTR libname, HANDLE16 hf,
                                       DWORD flags )
{
    TRACE(module,"(%s,%d,%08lx)\n",libname,hf,flags);
    return LoadLibraryExA(libname, hf,flags);
}

/***********************************************************************
 *           LoadLibraryExA   (KERNEL32)
 */
HMODULE WINAPI LoadLibraryExA(LPCSTR libname, HFILE hfile, DWORD flags)
{
	WINE_MODREF *wm;

	if(!libname)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	EnterCriticalSection(&PROCESS_Current()->crit_section);

	wm = MODULE_LoadLibraryExA( libname, hfile, flags );

	if(wm && !MODULE_DllProcessAttach(wm, NULL))
	{
		WARN(module, "Attach failed for module '%s', \n", libname);
		MODULE_FreeLibrary(wm);
		SetLastError(ERROR_DLL_INIT_FAILED);
		wm = NULL;
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
	DWORD err;
	WINE_MODREF *pwm;
	int i;
	module_loadorder_t *plo;

	EnterCriticalSection(&PROCESS_Current()->crit_section);

	/* Check for already loaded module */
	if((pwm = MODULE_FindModule(libname))) 
	{
		if(!(pwm->flags & WINE_MODREF_MARKER))
			pwm->refCount++;
		TRACE(module, "Already loaded module '%s' at 0x%08x, count=%d, \n", libname, pwm->module, pwm->refCount);
		LeaveCriticalSection(&PROCESS_Current()->crit_section);
		return pwm;
	}

	plo = MODULE_GetLoadOrder(libname);

	for(i = 0; i < MODULE_LOADORDER_NTYPES; i++)
	{
		switch(plo->loadorder[i])
		{
		case MODULE_LOADORDER_DLL:
			TRACE(module, "Trying native dll '%s'\n", libname);
			pwm = PE_LoadLibraryExA(libname, flags, &err);
			break;

		case MODULE_LOADORDER_ELFDLL:
			TRACE(module, "Trying elfdll '%s'\n", libname);
			pwm = ELFDLL_LoadLibraryExA(libname, flags, &err);
			break;

		case MODULE_LOADORDER_SO:
			TRACE(module, "Trying so-library '%s'\n", libname);
			pwm = ELF_LoadLibraryExA(libname, flags, &err);
			break;

		case MODULE_LOADORDER_BI:
			TRACE(module, "Trying built-in '%s'\n", libname);
			pwm = BUILTIN32_LoadLibraryExA(libname, flags, &err);
			break;

		default:
			ERR(module, "Got invalid loadorder type %d (%s index %d)\n", plo->loadorder[i], plo->modulename, i);
		/* Fall through */

		case MODULE_LOADORDER_INVALID:	/* We ignore this as it is an empty entry */
			pwm = NULL;
			break;
		}

		if(pwm)
		{
			/* Initialize DLL just loaded */
			TRACE(module, "Loaded module '%s' at 0x%08x, \n", libname, pwm->module);

			/* Set the refCount here so that an attach failure will */
			/* decrement the dependencies through the MODULE_FreeLibrary call. */
			pwm->refCount++;

			LeaveCriticalSection(&PROCESS_Current()->crit_section);
			return pwm;
		}

		if(err != ERROR_FILE_NOT_FOUND)
			break;
	}

	ERR(module, "Failed to load module '%s'; error=0x%08lx, \n", libname, err);
	SetLastError(err);
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
 *           LoadLibraryExW       (KERNEL32)
 */
HMODULE WINAPI LoadLibraryExW(LPCWSTR libnameW,HFILE hfile,DWORD flags)
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
		case MODULE32_PE:	PE_UnloadLibrary(wm);		break;
		case MODULE32_ELF:	ELF_UnloadLibrary(wm);		break;
		case MODULE32_ELFDLL:	ELFDLL_UnloadLibrary(wm);	break;
		case MODULE32_BI:	BUILTIN32_UnloadLibrary(wm);	break;

		default:
			ERR(module, "Invalid or unhandled MODREF type %d encountered (wm=%p)\n", wm->type, wm);
		}
	}
}

/***********************************************************************
 *           FreeLibrary
 */
BOOL WINAPI FreeLibrary(HINSTANCE hLibModule)
{
    BOOL retv = TRUE;
    WINE_MODREF *wm;

    EnterCriticalSection( &PROCESS_Current()->crit_section );

    wm = MODULE32_LookupHMODULE( hLibModule );
    if ( !wm )
        SetLastError( ERROR_INVALID_HANDLE );
    else
        retv = MODULE_FreeLibrary( wm );

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
    TRACE( module, "(%s) refCount: %d\n", wm->modname, wm->refCount );

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
    TRACE( module, "(%s) - START\n", wm->modname );

    /* Recursively decrement reference counts */
    MODULE_DecRefCount( wm );

    /* Call process detach notifications */
    MODULE_DllProcessDetach( FALSE, NULL );

    MODULE_FlushModrefs();

    TRACE( module, "(%s) - END\n", wm->modname );

    return FALSE;
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
    	WARN(module,"hModule may not be 0!\n");
	return (FARPROC16)0;
    }
    if (HIWORD(hModule))
    {
    	WARN( module, "hModule is Win32 handle (%08x)\n", hModule );
	return (FARPROC16)0;
    }
    hModule = GetExePtr( hModule );
    if (HIWORD(name)) {
        ordinal = NE_GetOrdinal( hModule, name );
        TRACE(module, "%04x '%s'\n",
                        hModule, name );
    } else {
        ordinal = LOWORD(name);
        TRACE(module, "%04x %04x\n",
                        hModule, ordinal );
    }
    if (!ordinal) return (FARPROC16)0;
    ret = NE_GetEntryPoint( hModule, ordinal );
    TRACE(module,"returning %08x\n",(UINT)ret);
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
        TRACE(module, "%04x '%s'\n",
                        hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
    }
    else
    {
        ordinal = LOWORD(name);
        TRACE(module, "%04x %04x\n",
                        hModule, ordinal );
    }
    if (!ordinal) return (FARPROC16)0;

    ret = NE_GetEntryPoint( hModule, ordinal );

    TRACE(module, "returning %08x\n", (UINT)ret );
    return ret;
}


/***********************************************************************
 *           GetProcAddress32   		(KERNEL32.257)
 */
FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR function )
{
    return MODULE_GetProcAddress( hModule, function, TRUE );
}

/***********************************************************************
 *           WIN16_GetProcAddress32   		(KERNEL.453)
 */
FARPROC WINAPI GetProcAddress32_16( HMODULE hModule, LPCSTR function )
{
    return MODULE_GetProcAddress( hModule, function, FALSE );
}

/***********************************************************************
 *           MODULE_GetProcAddress32   		(internal)
 */
FARPROC MODULE_GetProcAddress( 
	HMODULE hModule, 	/* [in] current module handle */
	LPCSTR function,	/* [in] function to be looked up */
	BOOL snoop )
{
    WINE_MODREF	*wm = MODULE32_LookupHMODULE( hModule );
    FARPROC	retproc;

    if (HIWORD(function))
	TRACE(win32,"(%08lx,%s)\n",(DWORD)hModule,function);
    else
	TRACE(win32,"(%08lx,%p)\n",(DWORD)hModule,function);
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
    	ERR(module,"wine_modref type %d not handled.\n",wm->type);
    	SetLastError(ERROR_INVALID_HANDLE);
    	return (FARPROC)0;
    }
}


/***********************************************************************
 *           RtlImageNtHeaders   (NTDLL)
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

