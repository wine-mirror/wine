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

WINE_MODREF *exe_modref;
int process_detaching = 0;  /* set on process detach to avoid deadlocks with thread detach */

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
 *              MODULE_InitDLL
 */
BOOL MODULE_InitDLL( WINE_MODREF *wm, DWORD type, LPVOID lpReserved )
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

/****************************************************************************
 *              DisableThreadLibraryCalls (KERNEL32.@)
 *
 * Don't call DllEntryPoint for DLL_THREAD_{ATTACH,DETACH} if set.
 */
BOOL WINAPI DisableThreadLibraryCalls( HMODULE hModule )
{
    NTSTATUS    nts = LdrDisableThreadCalloutsForDll( hModule );
    if (nts == STATUS_SUCCESS) return TRUE;

    SetLastError( RtlNtStatusToDosError( nts ) );
    return FALSE;
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
        FIXME("Strange error set by CreateProcess: %p\n", hInstance );
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
    NTSTATUS            nts;
    HMODULE             ret;

    if (module)
    {
        UNICODE_STRING      wstr;

        RtlCreateUnicodeStringFromAsciiz(&wstr, module);
        nts = LdrGetDllHandle(0, 0, &wstr, &ret);
        RtlFreeUnicodeString( &wstr );
    }
    else
        nts = LdrGetDllHandle(0, 0, NULL, &ret);
    if (nts != STATUS_SUCCESS)
    {
        ret = 0;
        SetLastError( RtlNtStatusToDosError( nts ) );
    }

    return ret;
}

/***********************************************************************
 *		GetModuleHandleW (KERNEL32.@)
 */
HMODULE WINAPI GetModuleHandleW(LPCWSTR module)
{
    NTSTATUS            nts;
    HMODULE             ret;

    if (module)
    {
        UNICODE_STRING      wstr;

        RtlInitUnicodeString( &wstr, module );
        nts = LdrGetDllHandle( 0, 0, &wstr, &ret);
    }
    else
        nts = LdrGetDllHandle( 0, 0, NULL, &ret);

    if (nts != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( nts ) );
        ret = 0;
    }
    return ret;
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
    DWORD       len = 0;

    lpFileName[0] = 0;

    RtlEnterCriticalSection( &loader_section );
    if (!hModule && !(NtCurrentTeb()->tibflags & TEBF_WIN32))
    {
        /* 16-bit task - get current NE module name */
        NE_MODULE *pModule = NE_GetPtr( GetCurrentTask() );
        if (pModule) len = GetLongPathNameA(NE_MODULE_NAME(pModule), lpFileName, size);
    }
    else if (hModule || LdrGetDllHandle( 0, 0, NULL, &hModule ) == STATUS_SUCCESS)
    {
        LDR_MODULE* pldr;
        NTSTATUS    nts;

        nts = LdrFindEntryForAddress( hModule, &pldr );
        if (nts == STATUS_SUCCESS)
        {
            WideCharToMultiByte( CP_ACP, 0, 
                                 pldr->FullDllName.Buffer, pldr->FullDllName.Length,
                                 lpFileName, size, NULL, NULL );
            len = min(size, pldr->FullDllName.Length / sizeof(WCHAR));
        }
        else SetLastError( RtlNtStatusToDosError( nts ) );
    }
    RtlLeaveCriticalSection( &loader_section );

    TRACE( "%s\n", debugstr_an(lpFileName, len) );
    return len;
}


/***********************************************************************
 *              GetModuleFileNameW      (KERNEL32.@)
 */
DWORD WINAPI GetModuleFileNameW( HMODULE hModule, LPWSTR lpFileName, DWORD size )
{
    DWORD       len = 0;

    lpFileName[0] = 0;

    RtlEnterCriticalSection( &loader_section );
    if (!hModule && !(NtCurrentTeb()->tibflags & TEBF_WIN32))
    {
        /* 16-bit task - get current NE module name */
        NE_MODULE *pModule = NE_GetPtr( GetCurrentTask() );
        if (pModule)
        {
            WCHAR    path[MAX_PATH];

            MultiByteToWideChar( CP_ACP, 0, NE_MODULE_NAME(pModule), -1, 
                                 path, MAX_PATH );
            len = GetLongPathNameW(path, lpFileName, size);
        }
    }
    else if (hModule || LdrGetDllHandle( 0, 0, NULL, &hModule ) == STATUS_SUCCESS)
    {
        LDR_MODULE* pldr;
        NTSTATUS    nts;

        nts = LdrFindEntryForAddress( hModule, &pldr );
        if (nts == STATUS_SUCCESS)
        {
            len = min(size, pldr->FullDllName.Length / sizeof(WCHAR));
            strncpyW(lpFileName, pldr->FullDllName.Buffer, len);
            if (len < size) lpFileName[len] = 0;
        }
        else SetLastError( RtlNtStatusToDosError( nts ) );

    }
    RtlLeaveCriticalSection( &loader_section );

    TRACE( "%s\n", debugstr_wn(lpFileName, len) );
    return len;
}

/******************************************************************
 *		load_library_as_datafile
 */
static BOOL load_library_as_datafile( LPCWSTR name, HMODULE* hmod)
{
    static const WCHAR dotDLL[] = {'.','d','l','l',0};

    WCHAR filenameW[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE mapping;

    *hmod = 0;

    if (SearchPathW( NULL, (LPCWSTR)name, dotDLL, sizeof(filenameW) / sizeof(filenameW[0]),
                     filenameW, NULL ))
    {
        hFile = CreateFileW( filenameW, GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, 0, 0 );
    }
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    switch (MODULE_GetBinaryType( hFile ))
    {
    case BINARY_PE_EXE:
    case BINARY_PE_DLL:
        mapping = CreateFileMappingW( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
        if (mapping)
        {
            *hmod = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
            CloseHandle( mapping );
        }
        break;
    default:
        break;
    }
    CloseHandle( hFile );

    return *hmod != 0;
}

/******************************************************************
 *		LoadLibraryExA          (KERNEL32.@)
 *
 * The HFILE parameter is not used and marked reserved in the SDK. I can
 * only guess that it should force a file to be mapped, but I rather
 * ignore the parameter because it would be extremely difficult to
 * integrate this with different types of module representations.
 *
 */
HMODULE WINAPI LoadLibraryExA(LPCSTR libname, HANDLE hfile, DWORD flags)
{
    UNICODE_STRING      wstr;
    NTSTATUS            nts;
    HMODULE             hModule;

    if (!libname)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    RtlCreateUnicodeStringFromAsciiz( &wstr, libname );

    if (flags & LOAD_LIBRARY_AS_DATAFILE)
    {
        /* The method in load_library_as_datafile allows searching for the
         * 'native' libraries only
         */
        if (load_library_as_datafile( wstr.Buffer, &hModule))
        {
            RtlFreeUnicodeString( &wstr );
            return (HMODULE)((ULONG_PTR)hModule + 1);
        }
        flags |= DONT_RESOLVE_DLL_REFERENCES; /* Just in case */
        /* Fallback to normal behaviour */
    }

    nts = LdrLoadDll(NULL, flags, &wstr, &hModule);
    if (nts != STATUS_SUCCESS)
    {
        hModule = 0;
        SetLastError( RtlNtStatusToDosError( nts ) );
    }
    RtlFreeUnicodeString( &wstr );

    return hModule;
}

/***********************************************************************
 *           LoadLibraryExW       (KERNEL32.@)
 */
HMODULE WINAPI LoadLibraryExW(LPCWSTR libnameW, HANDLE hfile, DWORD flags)
{
    UNICODE_STRING      wstr;
    NTSTATUS            nts;
    HMODULE             hModule;

    if (!libnameW)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (flags & LOAD_LIBRARY_AS_DATAFILE)
    {
        /* The method in load_library_as_datafile allows searching for the
         * 'native' libraries only
         */
        if (load_library_as_datafile(libnameW, &hModule))
            return (HMODULE)((ULONG_PTR)hModule + 1);
        flags |= DONT_RESOLVE_DLL_REFERENCES; /* Just in case */
        /* Fallback to normal behaviour */
    }

    RtlInitUnicodeString( &wstr, libnameW );
    nts = LdrLoadDll(NULL, flags, &wstr, &hModule);
    if (nts != STATUS_SUCCESS)
    {
        hModule = 0;
        SetLastError( RtlNtStatusToDosError( nts ) );
    }
    return hModule;
}

/***********************************************************************
 *           LoadLibraryA         (KERNEL32.@)
 */
HMODULE WINAPI LoadLibraryA(LPCSTR libname)
{
    return LoadLibraryExA(libname, 0, 0);
}

/***********************************************************************
 *           LoadLibraryW         (KERNEL32.@)
 */
HMODULE WINAPI LoadLibraryW(LPCWSTR libnameW)
{
    return LoadLibraryExW(libnameW, 0, 0);
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
 *           FreeLibrary   (KERNEL32.@)
 *           FreeLibrary32 (KERNEL.486)
 */
BOOL WINAPI FreeLibrary(HINSTANCE hLibModule)
{
    BOOL                retv = FALSE;
    NTSTATUS            nts;

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

    if ((nts = LdrUnloadDll( hLibModule )) == STATUS_SUCCESS) retv = TRUE;
    else SetLastError( RtlNtStatusToDosError( nts ) );

    return retv;
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
    	WARN("hModule is Win32 handle (%p)\n", hModule );
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
    NTSTATUS    nts;
    FARPROC     fp;

    if (HIWORD(function))
    {
        ANSI_STRING     str;

        RtlInitAnsiString( &str, function );
        nts = LdrGetProcedureAddress( hModule, &str, 0, (void**)&fp );
    }
    else
        nts = LdrGetProcedureAddress( hModule, NULL, (DWORD)function, (void**)&fp );
    if (nts != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError( nts ) );
        fp = NULL;
    }
    return fp;
}

/***********************************************************************
 *           GetProcAddress32   		(KERNEL.453)
 */
FARPROC WINAPI GetProcAddress32_16( HMODULE hModule, LPCSTR function )
{
    /* FIXME: we used to disable snoop when returning proc for Win16 subsystem */
    return GetProcAddress( hModule, function );
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
