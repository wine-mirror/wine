/*
 * Modules
 *
 * Copyright 1995 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "wine/winbase16.h"
#include "winerror.h"
#include "winternl.h"
#include "heap.h"
#include "file.h"
#include "module.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(win32);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);

WINE_MODREF *MODULE_modref_list = NULL;

static WINE_MODREF *exe_modref;
static int free_lib_count;   /* recursion depth of FreeLibrary calls */
static int process_detaching;  /* set on process detach to avoid deadlocks with thread detach */

static CRITICAL_SECTION loader_section = CRITICAL_SECTION_INIT( "loader_section" );

/***********************************************************************
 *           wait_input_idle
 *
 * Wrapper to call WaitForInputIdle USER function
 */
typedef DWORD (WINAPI *WaitForInputIdle_ptr)( HANDLE hProcess, DWORD dwTimeOut );

static DWORD wait_input_idle( HANDLE process, DWORD timeout )
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    if (mod)
    {
        WaitForInputIdle_ptr ptr = (WaitForInputIdle_ptr)GetProcAddress( mod, "WaitForInputIdle" );
        if (ptr) return ptr( process, timeout );
    }
    return 0;
}


/*************************************************************************
 *		MODULE32_LookupHMODULE
 * looks for the referenced HMODULE in the current process
 * NOTE: Assumes that the process critical section is held!
 */
static WINE_MODREF *MODULE32_LookupHMODULE( HMODULE hmod )
{
    WINE_MODREF	*wm;

    if (!hmod)
    	return exe_modref;

    if (!HIWORD(hmod)) {
    	ERR("tried to lookup 0x%04x in win32 module handler!\n",hmod);
        SetLastError( ERROR_INVALID_HANDLE );
	return NULL;
    }
    for ( wm = MODULE_modref_list; wm; wm=wm->next )
	if (wm->module == hmod)
	    return wm;
    SetLastError( ERROR_INVALID_HANDLE );
    return NULL;
}

/*************************************************************************
 *		MODULE_AllocModRef
 *
 * Allocate a WINE_MODREF structure and add it to the process list
 * NOTE: Assumes that the process critical section is held!
 */
WINE_MODREF *MODULE_AllocModRef( HMODULE hModule, LPCSTR filename )
{
    WINE_MODREF *wm;

    DWORD long_len = strlen( filename );
    DWORD short_len = GetShortPathNameA( filename, NULL, 0 );

    if ((wm = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                         sizeof(*wm) + long_len + short_len + 1 )))
    {
        wm->module = hModule;
        wm->tlsindex = -1;

        wm->filename = wm->data;
        memcpy( wm->filename, filename, long_len + 1 );
        if ((wm->modname = strrchr( wm->filename, '\\' ))) wm->modname++;
        else wm->modname = wm->filename;

        wm->short_filename = wm->filename + long_len + 1;
        GetShortPathNameA( wm->filename, wm->short_filename, short_len + 1 );
        if ((wm->short_modname = strrchr( wm->short_filename, '\\' ))) wm->short_modname++;
        else wm->short_modname = wm->short_filename;

        wm->next = MODULE_modref_list;
        if (wm->next) wm->next->prev = wm;
        MODULE_modref_list = wm;

        if (!(RtlImageNtHeader(hModule)->FileHeader.Characteristics & IMAGE_FILE_DLL))
        {
            if (!exe_modref) exe_modref = wm;
            else FIXME( "Trying to load second .EXE file: %s\n", filename );
        }
    }
    return wm;
}

/*************************************************************************
 *		MODULE_InitDLL
 */
static BOOL MODULE_InitDLL( WINE_MODREF *wm, DWORD type, LPVOID lpReserved )
{
    BOOL retv = TRUE;

    static LPCSTR typeName[] = { "PROCESS_DETACH", "PROCESS_ATTACH",
                                 "THREAD_ATTACH", "THREAD_DETACH" };
    assert( wm );

    /* Skip calls for modules loaded with special load flags */

    if (wm->flags & WINE_MODREF_DONT_RESOLVE_REFS) return TRUE;

    TRACE("(%s,%s,%p) - CALL\n", wm->modname, typeName[type], lpReserved );

    /* Call the initialization routine */
    retv = PE_InitDLL( wm->module, type, lpReserved );

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
 */
BOOL MODULE_DllProcessAttach( WINE_MODREF *wm, LPVOID lpReserved )
{
    BOOL retv = TRUE;
    int i;

    RtlEnterCriticalSection( &loader_section );

    if (!wm)
    {
        wm = exe_modref;
        PE_InitTls();
    }
    assert( wm );

    /* prevent infinite recursion in case of cyclical dependencies */
    if (    ( wm->flags & WINE_MODREF_MARKER )
         || ( wm->flags & WINE_MODREF_PROCESS_ATTACHED ) )
        goto done;

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
        retv = MODULE_InitDLL( wm, DLL_PROCESS_ATTACH, lpReserved );
        if ( retv )
            wm->flags |= WINE_MODREF_PROCESS_ATTACHED;
    }

    /* Re-insert MODREF at head of list */
    if ( retv && wm->prev )
    {
        wm->prev->next = wm->next;
        if ( wm->next ) wm->next->prev = wm->prev;

        wm->prev = NULL;
        wm->next = MODULE_modref_list;
        MODULE_modref_list = wm->next->prev = wm;
    }

    /* Remove recursion flag */
    wm->flags &= ~WINE_MODREF_MARKER;

    TRACE("(%s,%p) - END\n", wm->modname, lpReserved );

 done:
    RtlLeaveCriticalSection( &loader_section );
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

    RtlEnterCriticalSection( &loader_section );
    if (bForceDetach) process_detaching = 1;
    do
    {
        for ( wm = MODULE_modref_list; wm; wm = wm->next )
        {
            /* Check whether to detach this DLL */
            if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
                continue;
            if ( wm->refCount > 0 && !bForceDetach )
                continue;

            /* Call detach notification */
            wm->flags &= ~WINE_MODREF_PROCESS_ATTACHED;
            MODULE_InitDLL( wm, DLL_PROCESS_DETACH, lpReserved );

            /* Restart at head of WINE_MODREF list, as entries might have
               been added and/or removed while performing the call ... */
            break;
        }
    } while ( wm );

    RtlLeaveCriticalSection( &loader_section );
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

    /* don't do any attach calls if process is exiting */
    if (process_detaching) return;
    /* FIXME: there is still a race here */

    RtlEnterCriticalSection( &loader_section );

    PE_InitTls();

    for ( wm = MODULE_modref_list; wm; wm = wm->next )
        if ( !wm->next )
            break;

    for ( ; wm; wm = wm->prev )
    {
        if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
            continue;
        if ( wm->flags & WINE_MODREF_NO_DLL_CALLS )
            continue;

        MODULE_InitDLL( wm, DLL_THREAD_ATTACH, lpReserved );
    }

    RtlLeaveCriticalSection( &loader_section );
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

    /* don't do any detach calls if process is exiting */
    if (process_detaching) return;
    /* FIXME: there is still a race here */

    RtlEnterCriticalSection( &loader_section );

    for ( wm = MODULE_modref_list; wm; wm = wm->next )
    {
        if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
            continue;
        if ( wm->flags & WINE_MODREF_NO_DLL_CALLS )
            continue;

        MODULE_InitDLL( wm, DLL_THREAD_DETACH, lpReserved );
    }

    RtlLeaveCriticalSection( &loader_section );
}

/****************************************************************************
 *              DisableThreadLibraryCalls (KERNEL32.@)
 *
 * Don't call DllEntryPoint for DLL_THREAD_{ATTACH,DETACH} if set.
 */
BOOL WINAPI DisableThreadLibraryCalls( HMODULE hModule )
{
    WINE_MODREF *wm;
    BOOL retval = TRUE;

    RtlEnterCriticalSection( &loader_section );

    wm = MODULE32_LookupHMODULE( hModule );
    if ( !wm )
        retval = FALSE;
    else
        wm->flags |= WINE_MODREF_NO_DLL_CALLS;

    RtlLeaveCriticalSection( &loader_section );

    return retval;
}


/***********************************************************************
 *           MODULE_CreateDummyModule
 *
 * Create a dummy NE module for Win32 or Winelib.
 */
HMODULE16 MODULE_CreateDummyModule( LPCSTR filename, HMODULE module32 )
{
    HMODULE16 hModule;
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
                 ((of_size + 3) & ~3) +
                 /* segment table: DS,CS */
                 2 * sizeof(SEGTABLEENTRY) +
                 /* name table */
                 len + 2 +
                 /* several empty tables */
                 8;

    hModule = GlobalAlloc16( GMEM_MOVEABLE | GMEM_ZEROINIT, size );
    if (!hModule) return (HMODULE16)11;  /* invalid exe */

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
        IMAGE_NT_HEADERS *nt = RtlImageNtHeader( module32 );
        pModule->expected_version = ((nt->OptionalHeader.MajorSubsystemVersion & 0xff) << 8 ) |
                                     (nt->OptionalHeader.MinorSubsystemVersion & 0xff);
        pModule->flags |= NE_FFLAGS_WIN32;
        if (nt->FileHeader.Characteristics & IMAGE_FILE_DLL)
            pModule->flags |= NE_FFLAGS_LIBMODULE | NE_FFLAGS_SINGLEDATA;
    }

    /* Set loaded file information */
    ofs = (OFSTRUCT *)(pModule + 1);
    memset( ofs, 0, of_size );
    ofs->cBytes = of_size < 256 ? of_size : 255;   /* FIXME */
    strcpy( ofs->szPathName, filename );

    pSegment = (SEGTABLEENTRY*)((char*)(pModule + 1) + ((of_size + 3) & ~3));
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
 *	    MODULE_FindModule
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

    for ( wm = MODULE_modref_list; wm; wm = wm->next )
    {
        if ( !FILE_strcasecmp( dllname, wm->modname ) )
            break;
        if ( !FILE_strcasecmp( dllname, wm->filename ) )
            break;
        if ( !FILE_strcasecmp( dllname, wm->short_modname ) )
            break;
        if ( !FILE_strcasecmp( dllname, wm->short_filename ) )
            break;
    }

    return wm;
}


/* Check whether a file is an OS/2 or a very old Windows executable
 * by testing on import of KERNEL.
 *
 * FIXME: is reading the module imports the only way of discerning
 *        old Windows binaries from OS/2 ones ? At least it seems so...
 */
static enum binary_type MODULE_Decide_OS2_OldWin(HANDLE hfile, const IMAGE_DOS_HEADER *mz,
                                                 const IMAGE_OS2_HEADER *ne)
{
    DWORD currpos = SetFilePointer( hfile, 0, NULL, SEEK_CUR);
    enum binary_type ret = BINARY_OS216;
    LPWORD modtab = NULL;
    LPSTR nametab = NULL;
    DWORD len;
    int i;

    /* read modref table */
    if ( (SetFilePointer( hfile, mz->e_lfanew + ne->ne_modtab, NULL, SEEK_SET ) == -1)
      || (!(modtab = HeapAlloc( GetProcessHeap(), 0, ne->ne_cmod*sizeof(WORD))))
      || (!(ReadFile(hfile, modtab, ne->ne_cmod*sizeof(WORD), &len, NULL)))
      || (len != ne->ne_cmod*sizeof(WORD)) )
	goto broken;

    /* read imported names table */
    if ( (SetFilePointer( hfile, mz->e_lfanew + ne->ne_imptab, NULL, SEEK_SET ) == -1)
      || (!(nametab = HeapAlloc( GetProcessHeap(), 0, ne->ne_enttab - ne->ne_imptab)))
      || (!(ReadFile(hfile, nametab, ne->ne_enttab - ne->ne_imptab, &len, NULL)))
      || (len != ne->ne_enttab - ne->ne_imptab) )
	goto broken;

    for (i=0; i < ne->ne_cmod; i++)
    {
	LPSTR module = &nametab[modtab[i]];
	TRACE("modref: %.*s\n", module[0], &module[1]);
	if (!(strncmp(&module[1], "KERNEL", module[0])))
	{ /* very old Windows file */
	    MESSAGE("This seems to be a very old (pre-3.0) Windows executable. Expect crashes, especially if this is a real-mode binary !\n");
            ret = BINARY_WIN16;
	    goto good;
	}
    }

broken:
    ERR("Hmm, an error occurred. Is this binary file broken ?\n");

good:
    HeapFree( GetProcessHeap(), 0, modtab);
    HeapFree( GetProcessHeap(), 0, nametab);
    SetFilePointer( hfile, currpos, NULL, SEEK_SET); /* restore filepos */
    return ret;
}

/***********************************************************************
 *           MODULE_GetBinaryType
 */
enum binary_type MODULE_GetBinaryType( HANDLE hfile )
{
    union
    {
        struct
        {
            unsigned char magic[4];
            unsigned char ignored[12];
            unsigned short type;
        } elf;
        IMAGE_DOS_HEADER mz;
    } header;

    char magic[4];
    DWORD len;

    /* Seek to the start of the file and read the header information. */
    if (SetFilePointer( hfile, 0, NULL, SEEK_SET ) == -1)
        return BINARY_UNKNOWN;
    if (!ReadFile( hfile, &header, sizeof(header), &len, NULL ) || len != sizeof(header))
        return BINARY_UNKNOWN;

    if (!memcmp( header.elf.magic, "\177ELF", 4 ))
    {
        /* FIXME: we don't bother to check byte order, architecture, etc. */
        switch(header.elf.type)
        {
        case 2: return BINARY_UNIX_EXE;
        case 3: return BINARY_UNIX_LIB;
        }
        return BINARY_UNKNOWN;
    }

    /* Not ELF, try DOS */

    if (header.mz.e_magic == IMAGE_DOS_SIGNATURE)
    {
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
        if ((header.mz.e_cparhdr << 4) < sizeof(IMAGE_DOS_HEADER))
            return BINARY_DOS;
        if (header.mz.e_crlc && (header.mz.e_lfarlc < sizeof(IMAGE_DOS_HEADER)))
            return BINARY_DOS;
        if (header.mz.e_lfanew < sizeof(IMAGE_DOS_HEADER))
            return BINARY_DOS;
        if (SetFilePointer( hfile, header.mz.e_lfanew, NULL, SEEK_SET ) == -1)
            return BINARY_DOS;
        if (!ReadFile( hfile, magic, sizeof(magic), &len, NULL ) || len != sizeof(magic))
            return BINARY_DOS;

        /* Reading the magic field succeeded so
         * we will try to determine what type it is.
         */
        if (!memcmp( magic, "PE\0\0", 4 ))
        {
            IMAGE_FILE_HEADER FileHeader;

            if (ReadFile( hfile, &FileHeader, sizeof(FileHeader), &len, NULL ) && len == sizeof(FileHeader))
            {
                if (FileHeader.Characteristics & IMAGE_FILE_DLL) return BINARY_PE_DLL;
                return BINARY_PE_EXE;
            }
            return BINARY_DOS;
        }

        if (!memcmp( magic, "NE", 2 ))
        {
            /* This is a Windows executable (NE) header.  This can
             * mean either a 16-bit OS/2 or a 16-bit Windows or even a
             * DOS program (running under a DOS extender).  To decide
             * which, we'll have to read the NE header.
             */
            IMAGE_OS2_HEADER ne;
            if (    SetFilePointer( hfile, header.mz.e_lfanew, NULL, SEEK_SET ) != -1
                    && ReadFile( hfile, &ne, sizeof(ne), &len, NULL )
                    && len == sizeof(ne) )
            {
                switch ( ne.ne_exetyp )
                {
                case 2:  return BINARY_WIN16;
                case 5:  return BINARY_DOS;
                default: return MODULE_Decide_OS2_OldWin(hfile, &header.mz, &ne);
                }
            }
            /* Couldn't read header, so abort. */
            return BINARY_DOS;
        }

        /* Unknown extended header, but this file is nonetheless DOS-executable. */
        return BINARY_DOS;
    }

    return BINARY_UNKNOWN;
}

/***********************************************************************
 *             GetBinaryTypeA                     [KERNEL32.@]
 *             GetBinaryType                      [KERNEL32.@]
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
BOOL WINAPI GetBinaryTypeA( LPCSTR lpApplicationName, LPDWORD lpBinaryType )
{
    BOOL ret = FALSE;
    HANDLE hfile;
    char *ptr;

    TRACE_(win32)("%s\n", lpApplicationName );

    /* Sanity check.
     */
    if ( lpApplicationName == NULL || lpBinaryType == NULL )
        return FALSE;

    /* Open the file indicated by lpApplicationName for reading.
     */
    hfile = CreateFileA( lpApplicationName, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, 0, 0 );
    if ( hfile == INVALID_HANDLE_VALUE )
        return FALSE;

    /* Check binary type
     */
    switch(MODULE_GetBinaryType( hfile ))
    {
    case BINARY_UNKNOWN:
        /* try to determine from file name */
        ptr = strrchr( lpApplicationName, '.' );
        if (!ptr) break;
        if (!FILE_strcasecmp( ptr, ".COM" ))
        {
            *lpBinaryType = SCS_DOS_BINARY;
            ret = TRUE;
        }
        else if (!FILE_strcasecmp( ptr, ".PIF" ))
        {
            *lpBinaryType = SCS_PIF_BINARY;
            ret = TRUE;
        }
        break;
    case BINARY_PE_EXE:
    case BINARY_PE_DLL:
        *lpBinaryType = SCS_32BIT_BINARY;
        ret = TRUE;
        break;
    case BINARY_WIN16:
        *lpBinaryType = SCS_WOW_BINARY;
        ret = TRUE;
        break;
    case BINARY_OS216:
        *lpBinaryType = SCS_OS216_BINARY;
        ret = TRUE;
        break;
    case BINARY_DOS:
        *lpBinaryType = SCS_DOS_BINARY;
        ret = TRUE;
        break;
    case BINARY_UNIX_EXE:
    case BINARY_UNIX_LIB:
        ret = FALSE;
        break;
    }

    CloseHandle( hfile );
    return ret;
}

/***********************************************************************
 *             GetBinaryTypeW                      [KERNEL32.@]
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
 *           WinExec     (KERNEL.166)
 */
HINSTANCE16 WINAPI WinExec16( LPCSTR lpCmdLine, UINT16 nCmdShow )
{
    LPCSTR p, args = NULL;
    LPCSTR name_beg, name_end;
    LPSTR name, cmdline;
    int arglen;
    HINSTANCE16 ret;
    char buffer[MAX_PATH];

    if (*lpCmdLine == '"') /* has to be only one and only at beginning ! */
    {
      name_beg = lpCmdLine+1;
      p = strchr ( lpCmdLine+1, '"' );
      if (p)
      {
	  name_end = p;
	  args = strchr ( p, ' ' );
      }
      else /* yes, even valid with trailing '"' missing */
	  name_end = lpCmdLine+strlen(lpCmdLine);
    }
    else
    {
      name_beg = lpCmdLine;
      args = strchr( lpCmdLine, ' ' );
      name_end = args ? args : lpCmdLine+strlen(lpCmdLine);
    }

    if ((name_beg == lpCmdLine) && (!args))
    { /* just use the original cmdline string as file name */
        name = (LPSTR)lpCmdLine;
    }
    else
    {
        if (!(name = HeapAlloc( GetProcessHeap(), 0, name_end - name_beg + 1 )))
            return ERROR_NOT_ENOUGH_MEMORY;
        memcpy( name, name_beg, name_end - name_beg );
        name[name_end - name_beg] = '\0';
    }

    if (args)
    {
	args++;
	arglen = strlen(args);
	cmdline = HeapAlloc( GetProcessHeap(), 0, 2 + arglen );
	cmdline[0] = (BYTE)arglen;
	strcpy( cmdline + 1, args );
    }
    else
    {
	cmdline = HeapAlloc( GetProcessHeap(), 0, 2 );
	cmdline[0] = cmdline[1] = 0;
    }

    TRACE("name: '%s', cmdline: '%.*s'\n", name, cmdline[0], &cmdline[1]);

    if (SearchPathA( NULL, name, ".exe", sizeof(buffer), buffer, NULL ))
    {
        LOADPARAMS16 params;
        WORD showCmd[2];
        showCmd[0] = 2;
        showCmd[1] = nCmdShow;

        params.hEnvironment = 0;
        params.cmdLine = MapLS( cmdline );
        params.showCmd = MapLS( showCmd );
        params.reserved = 0;

        ret = LoadModule16( buffer, &params );
        UnMapLS( params.cmdLine );
        UnMapLS( params.showCmd );
    }
    else ret = GetLastError();

    HeapFree( GetProcessHeap(), 0, cmdline );
    if (name != lpCmdLine) HeapFree( GetProcessHeap(), 0, name );

    if (ret == 21)  /* 32-bit module */
    {
        DWORD count;
        ReleaseThunkLock( &count );
        ret = LOWORD( WinExec( lpCmdLine, nCmdShow ) );
        RestoreThunkLock( count );
    }
    return ret;
}

/***********************************************************************
 *           WinExec   (KERNEL32.@)
 */
UINT WINAPI WinExec( LPCSTR lpCmdLine, UINT nCmdShow )
{
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char *cmdline;
    UINT ret;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = nCmdShow;

    /* cmdline needs to be writeable for CreateProcess */
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(lpCmdLine)+1 ))) return 0;
    strcpy( cmdline, lpCmdLine );

    if (CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
                        0, NULL, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == 0xFFFFFFFF)
            WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        ret = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((ret = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", ret );
        ret = 11;
    }
    HeapFree( GetProcessHeap(), 0, cmdline );
    return ret;
}

/**********************************************************************
 *	    LoadModule    (KERNEL32.@)
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

    if (!name) return (HINSTANCE)ERROR_FILE_NOT_FOUND;

    if (!SearchPathA( NULL, name, ".exe", sizeof(filename), filename, NULL ) &&
        !SearchPathA( NULL, name, NULL, sizeof(filename), filename, NULL ))
        return (HINSTANCE)GetLastError();

    len = (BYTE)params->lpCmdLine[0];
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(filename) + len + 2 )))
        return (HINSTANCE)ERROR_NOT_ENOUGH_MEMORY;

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
        if (wait_input_idle( info.hProcess, 30000 ) ==  0xFFFFFFFF )
            WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        hInstance = (HINSTANCE)33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((hInstance = (HINSTANCE)GetLastError()) >= (HINSTANCE)32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", hInstance );
        hInstance = (HINSTANCE)11;
    }

    HeapFree( GetProcessHeap(), 0, cmdline );
    return hInstance;
}


/***********************************************************************
 *              GetModuleHandleA         (KERNEL32.@)
 *              GetModuleHandle32        (KERNEL.488)
 */
HMODULE WINAPI GetModuleHandleA(LPCSTR module)
{
    WINE_MODREF *wm;

    if ( module == NULL )
        wm = exe_modref;
    else
        wm = MODULE_FindModule( module );

    return wm? wm->module : 0;
}

/***********************************************************************
 *		GetModuleHandleW (KERNEL32.@)
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
 *              GetModuleFileNameA      (KERNEL32.@)
 *              GetModuleFileName32     (KERNEL.487)
 *
 * GetModuleFileNameA seems to *always* return the long path;
 * it's only GetModuleFileName16 that decides between short/long path
 * by checking if exe version >= 4.0.
 * (SDK docu doesn't mention this)
 */
DWORD WINAPI GetModuleFileNameA(
	HMODULE hModule,	/* [in] module handle (32bit) */
	LPSTR lpFileName,	/* [out] filenamebuffer */
        DWORD size )		/* [in] size of filenamebuffer */
{
    RtlEnterCriticalSection( &loader_section );

    lpFileName[0] = 0;
    if (!hModule && !(NtCurrentTeb()->tibflags & TEBF_WIN32))
    {
        /* 16-bit task - get current NE module name */
        NE_MODULE *pModule = NE_GetPtr( GetCurrentTask() );
        if (pModule) GetLongPathNameA(NE_MODULE_NAME(pModule), lpFileName, size);
    }
    else
    {
        WINE_MODREF *wm = MODULE32_LookupHMODULE( hModule );
        if (wm) lstrcpynA( lpFileName, wm->filename, size );
    }

    RtlLeaveCriticalSection( &loader_section );
    TRACE("%s\n", lpFileName );
    return strlen(lpFileName);
}


/***********************************************************************
 *              GetModuleFileNameW      (KERNEL32.@)
 */
DWORD WINAPI GetModuleFileNameW( HMODULE hModule, LPWSTR lpFileName, DWORD size )
{
    LPSTR fnA = HeapAlloc( GetProcessHeap(), 0, size * 2 );
    if (!fnA) return 0;
    GetModuleFileNameA( hModule, fnA, size * 2 );
    if (size > 0 && !MultiByteToWideChar( CP_ACP, 0, fnA, -1, lpFileName, size ))
        lpFileName[size-1] = 0;
    HeapFree( GetProcessHeap(), 0, fnA );
    return strlenW(lpFileName);
}


/***********************************************************************
 *           LoadLibraryExA   (KERNEL32.@)
 */
HMODULE WINAPI LoadLibraryExA(LPCSTR libname, HANDLE hfile, DWORD flags)
{
	WINE_MODREF *wm;

	if(!libname)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

        if (flags & LOAD_LIBRARY_AS_DATAFILE)
        {
            char filename[256];
            HMODULE hmod = 0;

            /* This method allows searching for the 'native' libraries only */
            if (SearchPathA( NULL, libname, ".dll", sizeof(filename), filename, NULL ))
            {
                /* FIXME: maybe we should use the hfile parameter instead */
                HANDLE hFile = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ,
                                            NULL, OPEN_EXISTING, 0, 0 );
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    HANDLE mapping;
                    switch (MODULE_GetBinaryType( hFile ))
                    {
                    case BINARY_PE_EXE:
                    case BINARY_PE_DLL:
                        mapping = CreateFileMappingA( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
                        if (mapping)
                        {
                            hmod = (HMODULE)MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
                            CloseHandle( mapping );
                        }
                        break;
                    default:
                        break;
                    }
                    CloseHandle( hFile );
                }
                if (hmod) return (HMODULE)((ULONG_PTR)hmod + 1);
            }
            flags |= DONT_RESOLVE_DLL_REFERENCES; /* Just in case */
            /* Fallback to normal behaviour */
        }

        RtlEnterCriticalSection( &loader_section );

	wm = MODULE_LoadLibraryExA( libname, hfile, flags );
	if ( wm )
	{
		if ( !MODULE_DllProcessAttach( wm, NULL ) )
		{
			WARN_(module)("Attach failed for module '%s'.\n", libname);
			MODULE_FreeLibrary(wm);
			SetLastError(ERROR_DLL_INIT_FAILED);
			wm = NULL;
		}
	}

        RtlLeaveCriticalSection( &loader_section );
	return wm ? wm->module : 0;
}

/***********************************************************************
 *      allocate_lib_dir
 *
 * helper for MODULE_LoadLibraryExA.  Allocate space to hold the directory
 * portion of the provided name and put the name in it.
 *
 */
static LPCSTR allocate_lib_dir(LPCSTR libname)
{
    LPCSTR p, pmax;
    LPSTR result;
    int length;

    pmax = libname;
    if ((p = strrchr( pmax, '\\' ))) pmax = p + 1;
    if ((p = strrchr( pmax, '/' ))) pmax = p + 1; /* Naughty.  MSDN says don't */
    if (pmax == libname && pmax[0] && pmax[1] == ':') pmax += 2;

    length = pmax - libname;

    result = HeapAlloc (GetProcessHeap(), 0, length+1);

    if (result)
    {
        strncpy (result, libname, length);
        result [length] = '\0';
    }

    return result;
}

/***********************************************************************
 *	MODULE_LoadLibraryExA	(internal)
 *
 * Load a PE style module according to the load order.
 *
 * The HFILE parameter is not used and marked reserved in the SDK. I can
 * only guess that it should force a file to be mapped, but I rather
 * ignore the parameter because it would be extremely difficult to
 * integrate this with different types of module representations.
 *
 * libdir is used to support LOAD_WITH_ALTERED_SEARCH_PATH during the recursion
 *        on this function.  When first called from LoadLibraryExA it will be
 *        NULL but thereafter it may point to a buffer containing the path
 *        portion of the library name.  Note that the recursion all occurs
 *        within a Critical section (see LoadLibraryExA) so the use of a
 *        static is acceptable.
 *        (We have to use a static variable at some point anyway, to pass the
 *        information from BUILTIN32_dlopen through dlopen and the builtin's
 *        init function into load_library).
 * allocated_libdir is TRUE in the stack frame that allocated libdir
 */
WINE_MODREF *MODULE_LoadLibraryExA( LPCSTR libname, HANDLE hfile, DWORD flags )
{
	DWORD err = GetLastError();
	WINE_MODREF *pwm;
	int i;
	enum loadorder_type loadorder[LOADORDER_NTYPES];
	LPSTR filename;
        const char *filetype = "";
        DWORD found;
        BOOL allocated_libdir = FALSE;
        static LPCSTR libdir = NULL; /* See above */

	if ( !libname ) return NULL;

	filename = HeapAlloc ( GetProcessHeap(), 0, MAX_PATH + 1 );
	if ( !filename ) return NULL;
        *filename = 0; /* Just in case we don't set it before goto error */

        RtlEnterCriticalSection( &loader_section );

        if ((flags & LOAD_WITH_ALTERED_SEARCH_PATH) && FILE_contains_path(libname))
        {
            if (!(libdir = allocate_lib_dir(libname))) goto error;
            allocated_libdir = TRUE;
        }

        if (!libdir || allocated_libdir)
            found = SearchPathA(NULL, libname, ".dll", MAX_PATH, filename, NULL);
        else
            found = DIR_SearchAlternatePath(libdir, libname, ".dll", MAX_PATH, filename, NULL);

	/* build the modules filename */
        if (!found)
	{
            if (!MODULE_GetBuiltinPath( libname, ".dll", filename, MAX_PATH )) goto error;
	}

	/* Check for already loaded module */
	if (!(pwm = MODULE_FindModule(filename)) && !FILE_contains_path(libname))
        {
	    LPSTR	fn = HeapAlloc ( GetProcessHeap(), 0, MAX_PATH + 1 );
	    if (fn)
	    {
	    	/* since the default loading mechanism uses a more detailed algorithm
		 * than SearchPath (like using PATH, which can even be modified between
		 * two attempts of loading the same DLL), the look-up above (with
		 * SearchPath) can have put the file in system directory, whereas it
		 * has already been loaded but with a different path. So do a specific
		 * look-up with filename (without any path)
	     	 */
	    	strcpy ( fn, libname );
	    	/* if the filename doesn't have an extension append .DLL */
	    	if (!strrchr( fn, '.')) strcat( fn, ".dll" );
	    	if ((pwm = MODULE_FindModule( fn )) != NULL)
		   strcpy( filename, fn );
		HeapFree( GetProcessHeap(), 0, fn );
	    }
	}
	if (pwm)
	{
		pwm->refCount++;

                if ((pwm->flags & WINE_MODREF_DONT_RESOLVE_REFS) &&
		    !(flags & DONT_RESOLVE_DLL_REFERENCES))
                {
                    pwm->flags &= ~WINE_MODREF_DONT_RESOLVE_REFS;
                    PE_fixup_imports( pwm );
		}
		TRACE("Already loaded module '%s' at 0x%08x, count=%d\n", filename, pwm->module, pwm->refCount);
                if (allocated_libdir)
                {
                    HeapFree ( GetProcessHeap(), 0, (LPSTR)libdir );
                    libdir = NULL;
                }
                RtlLeaveCriticalSection( &loader_section );
		HeapFree ( GetProcessHeap(), 0, filename );
		return pwm;
	}

        MODULE_GetLoadOrder( loadorder, filename, TRUE);

	for(i = 0; i < LOADORDER_NTYPES; i++)
	{
                if (loadorder[i] == LOADORDER_INVALID) break;
                SetLastError( ERROR_FILE_NOT_FOUND );

		switch(loadorder[i])
		{
		case LOADORDER_DLL:
			TRACE("Trying native dll '%s'\n", filename);
			pwm = PE_LoadLibraryExA(filename, flags);
                        filetype = "native";
			break;

		case LOADORDER_SO:
			TRACE("Trying so-library '%s'\n", filename);
                        pwm = ELF_LoadLibraryExA(filename, flags);
                        filetype = "so";
			break;

		case LOADORDER_BI:
			TRACE("Trying built-in '%s'\n", filename);
			pwm = BUILTIN32_LoadLibraryExA(filename, flags);
                        filetype = "builtin";
			break;

                default:
			pwm = NULL;
			break;
		}

		if(pwm)
		{
			/* Initialize DLL just loaded */
			TRACE("Loaded module '%s' at 0x%08x\n", filename, pwm->module);
                        if (!TRACE_ON(module))
                            TRACE_(loaddll)("Loaded module '%s' : %s\n", filename, filetype);
			/* Set the refCount here so that an attach failure will */
			/* decrement the dependencies through the MODULE_FreeLibrary call. */
			pwm->refCount = 1;

                        if (allocated_libdir)
                        {
                            HeapFree ( GetProcessHeap(), 0, (LPSTR)libdir );
                            libdir = NULL;
                        }
                        RtlLeaveCriticalSection( &loader_section );
                        SetLastError( err );  /* restore last error */
			HeapFree ( GetProcessHeap(), 0, filename );
			return pwm;
		}

		if(GetLastError() != ERROR_FILE_NOT_FOUND)
                {
                    WARN("Loading of %s DLL %s failed (error %ld).\n",
                         filetype, filename, GetLastError());
                    break;
                }
	}

 error:
        if (allocated_libdir)
        {
            HeapFree ( GetProcessHeap(), 0, (LPSTR)libdir );
            libdir = NULL;
        }
        RtlLeaveCriticalSection( &loader_section );
	WARN("Failed to load module '%s'; error=%ld\n", filename, GetLastError());
	HeapFree ( GetProcessHeap(), 0, filename );
	return NULL;
}

/***********************************************************************
 *           LoadLibraryA         (KERNEL32.@)
 */
HMODULE WINAPI LoadLibraryA(LPCSTR libname) {
	return LoadLibraryExA(libname,0,0);
}

/***********************************************************************
 *           LoadLibraryW         (KERNEL32.@)
 */
HMODULE WINAPI LoadLibraryW(LPCWSTR libnameW)
{
    return LoadLibraryExW(libnameW,0,0);
}

/***********************************************************************
 *           LoadLibrary32        (KERNEL.452)
 *           LoadSystemLibrary32  (KERNEL.482)
 */
HMODULE WINAPI LoadLibrary32_16( LPCSTR libname )
{
    HMODULE hModule;
    DWORD count;

    ReleaseThunkLock( &count );
    hModule = LoadLibraryA( libname );
    RestoreThunkLock( count );
    return hModule;
}

/***********************************************************************
 *           LoadLibraryExW       (KERNEL32.@)
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

	for(wm = MODULE_modref_list; wm; wm = next)
	{
		next = wm->next;

		if(wm->refCount)
			continue;

		/* Unlink this modref from the chain */
		if(wm->next)
                        wm->next->prev = wm->prev;
		if(wm->prev)
                        wm->prev->next = wm->next;
		if(wm == MODULE_modref_list)
			MODULE_modref_list = wm->next;

                TRACE(" unloading %s\n", wm->filename);
                if (!TRACE_ON(module))
                    TRACE_(loaddll)("Unloaded module '%s' : %s\n", wm->filename,
                                    wm->dlhandle ? "builtin" : "native" );

                if (wm->dlhandle) wine_dll_unload( wm->dlhandle );
                else UnmapViewOfFile( (LPVOID)wm->module );
                FreeLibrary16(wm->hDummyMod);
                HeapFree( GetProcessHeap(), 0, wm->deps );
                HeapFree( GetProcessHeap(), 0, wm );
	}
}

/***********************************************************************
 *           FreeLibrary   (KERNEL32.@)
 *           FreeLibrary32 (KERNEL.486)
 */
BOOL WINAPI FreeLibrary(HINSTANCE hLibModule)
{
    BOOL retv = FALSE;
    WINE_MODREF *wm;

    if (!hLibModule)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if ((ULONG_PTR)hLibModule & 1)
    {
        /* this is a LOAD_LIBRARY_AS_DATAFILE module */
        char *ptr = (char *)hLibModule - 1;
        UnmapViewOfFile( ptr );
        return TRUE;
    }

    RtlEnterCriticalSection( &loader_section );

    /* if we're stopping the whole process (and forcing the removal of all
     * DLLs) the library will be freed anyway
     */
    if (process_detaching) retv = TRUE;
    else
    {
        free_lib_count++;
        if ((wm = MODULE32_LookupHMODULE( hLibModule ))) retv = MODULE_FreeLibrary( wm );
        free_lib_count--;
    }

    RtlLeaveCriticalSection( &loader_section );

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
    if ( free_lib_count <= 1 )
    {
        MODULE_DllProcessDetach( FALSE, NULL );
        SERVER_START_REQ( unload_dll )
        {
            req->base = (void *)wm->module;
            wine_server_call( req );
        }
        SERVER_END_REQ;
        MODULE_FlushModrefs();
    }

    TRACE("END\n");

    return TRUE;
}


/***********************************************************************
 *           FreeLibraryAndExitThread (KERNEL32.@)
 */
VOID WINAPI FreeLibraryAndExitThread(HINSTANCE hLibModule, DWORD dwExitCode)
{
    FreeLibrary(hLibModule);
    ExitThread(dwExitCode);
}

/***********************************************************************
 *           PrivateLoadLibrary       (KERNEL32.@)
 *
 * FIXME: rough guesswork, don't know what "Private" means
 */
HINSTANCE16 WINAPI PrivateLoadLibrary(LPCSTR libname)
{
    return LoadLibrary16(libname);
}



/***********************************************************************
 *           PrivateFreeLibrary       (KERNEL32.@)
 *
 * FIXME: rough guesswork, don't know what "Private" means
 */
void WINAPI PrivateFreeLibrary(HINSTANCE16 handle)
{
    FreeLibrary16(handle);
}


/***********************************************************************
 *           GetProcAddress16   (KERNEL32.37)
 * Get procaddress in 16bit module from win32... (kernel32 undoc. ordinal func)
 */
FARPROC16 WINAPI WIN32_GetProcAddress16( HMODULE hModule, LPCSTR name )
{
    if (!hModule) {
    	WARN("hModule may not be 0!\n");
	return (FARPROC16)0;
    }
    if (HIWORD(hModule))
    {
    	WARN("hModule is Win32 handle (%08x)\n", hModule );
	return (FARPROC16)0;
    }
    return GetProcAddress16( LOWORD(hModule), name );
}

/***********************************************************************
 *           GetProcAddress   (KERNEL.50)
 */
FARPROC16 WINAPI GetProcAddress16( HMODULE16 hModule, LPCSTR name )
{
    WORD ordinal;
    FARPROC16 ret;

    if (!hModule) hModule = GetCurrentTask();
    hModule = GetExePtr( hModule );

    if (HIWORD(name) != 0)
    {
        ordinal = NE_GetOrdinal( hModule, name );
        TRACE("%04x '%s'\n", hModule, name );
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
 *           GetProcAddress   		(KERNEL32.@)
 */
FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR function )
{
    return MODULE_GetProcAddress( hModule, function, -1, TRUE );
}

/***********************************************************************
 *           GetProcAddress32   		(KERNEL.453)
 */
FARPROC WINAPI GetProcAddress32_16( HMODULE hModule, LPCSTR function )
{
    return MODULE_GetProcAddress( hModule, function, -1, FALSE );
}

/***********************************************************************
 *           MODULE_GetProcAddress   		(internal)
 */
FARPROC MODULE_GetProcAddress(
	HMODULE hModule, 	/* [in] current module handle */
	LPCSTR function,	/* [in] function to be looked up */
	int hint,
	BOOL snoop )
{
    WINE_MODREF	*wm;
    FARPROC	retproc = 0;

    if (HIWORD(function))
	TRACE_(win32)("(%08lx,%s (%d))\n",(DWORD)hModule,function,hint);
    else
	TRACE_(win32)("(%08lx,%p)\n",(DWORD)hModule,function);

    RtlEnterCriticalSection( &loader_section );
    if ((wm = MODULE32_LookupHMODULE( hModule )))
    {
        retproc = wm->find_export( wm, function, hint, snoop );
        if (!retproc) SetLastError(ERROR_PROC_NOT_FOUND);
    }
    RtlLeaveCriticalSection( &loader_section );
    return retproc;
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
         && (gpHandler = MapSL( gpPtr )) != NULL )
    {
        while (gpHandler->selector)
        {
            if (    SELECTOROF(address) == gpHandler->selector
                 && OFFSETOF(address)   >= gpHandler->rangeStart
                 && OFFSETOF(address)   <  gpHandler->rangeEnd  )
                return MAKESEGPTR( gpHandler->selector, gpHandler->handler );
            gpHandler++;
        }
    }

    return 0;
}
