/*
 * Copyright 2002 Dmitry Timoshkov for Codeweavers
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

#include "winbase.h"
#include "winnt.h"
#include "winternl.h"

#include "module.h"
#include "file.h"
#include "wine/exception.h"
#include "excpt.h"
#include "wine/debug.h"
#include "wine/server.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);
WINE_DECLARE_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);

static int free_lib_count;   /* recursion depth of FreeLibrary calls */

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}


/******************************************************************
 *		LdrDisableThreadCalloutsForDll (NTDLL.@)
 *
 */
NTSTATUS WINAPI LdrDisableThreadCalloutsForDll(HMODULE hModule)
{
    WINE_MODREF *wm;
    NTSTATUS    ret = STATUS_SUCCESS;

    RtlEnterCriticalSection( &loader_section );

    wm = MODULE32_LookupHMODULE( hModule );
    if ( !wm )
        ret = STATUS_DLL_NOT_FOUND;
    else
        wm->flags |= WINE_MODREF_NO_DLL_CALLS;

    RtlLeaveCriticalSection( &loader_section );

    return ret;
}

/**********************************************************************
 *	    MODULE_FindModule
 *
 * Find a (loaded) win32 module depending on path
 *	LPCSTR path: [in] pathname of module/library to be found
 *
 * The loader_section must be locked while calling this function
 * RETURNS
 *	the module handle if found
 * 	0 if not
 */
WINE_MODREF *MODULE_FindModule(LPCSTR path)
{
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

/******************************************************************
 *		LdrGetDllHandle (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI LdrGetDllHandle(ULONG x, ULONG y, PUNICODE_STRING name, HMODULE *base)
{
    WINE_MODREF *wm;

    TRACE("%08lx %08lx %s %p\n",
          x, y, name ? debugstr_wn(name->Buffer, name->Length) : NULL, base);

    if (x != 0 || y != 0)
        FIXME("Unknown behavior, please report\n");

    /* FIXME: we should store module name information as unicode */
    if (name)
    {
        STRING str;

        RtlUnicodeStringToAnsiString( &str, name, TRUE );

        wm = MODULE_FindModule( str.Buffer );
        RtlFreeAnsiString( &str );
    }
    else
        wm = exe_modref;

    if (!wm)
    {
        *base = 0;
        return STATUS_DLL_NOT_FOUND;
    }

    *base = wm->module;
    return STATUS_SUCCESS;
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
	TRACE("(%p,%s (%d))\n",hModule,function,hint);
    else
	TRACE("(%p,%p)\n",hModule,function);

    RtlEnterCriticalSection( &loader_section );
    if ((wm = MODULE32_LookupHMODULE( hModule )))
    {
        retproc = wm->find_export( wm, function, hint, snoop );
    }
    RtlLeaveCriticalSection( &loader_section );
    return retproc;
}


/******************************************************************
 *		LdrGetProcedureAddress (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI LdrGetProcedureAddress(HMODULE base, PANSI_STRING name, ULONG ord, PVOID *address)
{
    WARN("%p %s %ld %p\n", base, name ? debugstr_an(name->Buffer, name->Length) : NULL, ord, address);

    *address = MODULE_GetProcAddress( base, name ? name->Buffer : (LPSTR)ord, -1, TRUE );

    return (*address) ? STATUS_SUCCESS : STATUS_PROCEDURE_NOT_FOUND;
}


/******************************************************************
 *		LdrShutdownProcess (NTDLL.@)
 *
 */
NTSTATUS    WINAPI  LdrShutdownProcess(void)
{
    TRACE("()\n");
    MODULE_DllProcessDetach( TRUE, (LPVOID)1 );
    return STATUS_SUCCESS; /* FIXME */
}

/******************************************************************
 *		LdrShutdownThread (NTDLL.@)
 *
 */
NTSTATUS WINAPI LdrShutdownThread(void)
{
    WINE_MODREF *wm;
    TRACE("()\n");

    /* don't do any detach calls if process is exiting */
    if (process_detaching) return STATUS_SUCCESS;
    /* FIXME: there is still a race here */

    RtlEnterCriticalSection( &loader_section );

    for ( wm = MODULE_modref_list; wm; wm = wm->next )
    {
        if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
            continue;
        if ( wm->flags & WINE_MODREF_NO_DLL_CALLS )
            continue;

        MODULE_InitDLL( wm, DLL_THREAD_DETACH, NULL );
    }

    RtlLeaveCriticalSection( &loader_section );
    return STATUS_SUCCESS; /* FIXME */
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

    for (wm = MODULE_modref_list; wm; wm = next)
    {
        next = wm->next;

        if (wm->refCount)
            continue;

        /* Unlink this modref from the chain */
        if (wm->next)
            wm->next->prev = wm->prev;
        if (wm->prev)
            wm->prev->next = wm->next;
        if (wm == MODULE_modref_list)
            MODULE_modref_list = wm->next;

        TRACE(" unloading %s\n", wm->filename);
        if (!TRACE_ON(module))
            TRACE_(loaddll)("Unloaded module '%s' : %s\n", wm->filename,
                            wm->dlhandle ? "builtin" : "native" );

        SERVER_START_REQ( unload_dll )
        {
            req->base = (void *)wm->module;
            wine_server_call( req );
        }
        SERVER_END_REQ;

        if (wm->dlhandle) wine_dll_unload( wm->dlhandle );
        else UnmapViewOfFile( (LPVOID)wm->module );
        FreeLibrary16( wm->hDummyMod );
        RtlFreeHeap( ntdll_get_process_heap(), 0, wm->deps );
        RtlFreeHeap( ntdll_get_process_heap(), 0, wm );
    }
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

/******************************************************************
 *		LdrUnloadDll (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI LdrUnloadDll( HMODULE hModule )
{
    NTSTATUS retv = STATUS_SUCCESS;

    TRACE("(%p)\n", hModule);

    RtlEnterCriticalSection( &loader_section );

    /* if we're stopping the whole process (and forcing the removal of all
     * DLLs) the library will be freed anyway
     */
    if (!process_detaching)
    {
        WINE_MODREF *wm;

        free_lib_count++;
        if ((wm = MODULE32_LookupHMODULE( hModule )) != NULL)
        {
            TRACE("(%s) - START\n", wm->modname);

            /* Recursively decrement reference counts */
            MODULE_DecRefCount( wm );

            /* Call process detach notifications */
            if ( free_lib_count <= 1 )
            {
                MODULE_DllProcessDetach( FALSE, NULL );
                MODULE_FlushModrefs();
            }

            TRACE("END\n");
        }
        else
            retv = STATUS_DLL_NOT_FOUND;

        free_lib_count--;
    }

    RtlLeaveCriticalSection( &loader_section );

    return retv;
}

/***********************************************************************
 *           RtlImageNtHeader   (NTDLL.@)
 */
PIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE hModule)
{
    IMAGE_NT_HEADERS *ret;

    __TRY
    {
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)hModule;

        ret = NULL;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE)
        {
            ret = (IMAGE_NT_HEADERS *)((char *)dos + dos->e_lfanew);
            if (ret->Signature != IMAGE_NT_SIGNATURE) ret = NULL;
        }
    }
    __EXCEPT(page_fault)
    {
        return NULL;
    }
    __ENDTRY
    return ret;
}


/***********************************************************************
 *           RtlImageDirectoryEntryToData   (NTDLL.@)
 */
PVOID WINAPI RtlImageDirectoryEntryToData( HMODULE module, BOOL image, WORD dir, ULONG *size )
{
    const IMAGE_NT_HEADERS *nt;
    DWORD addr;

    if ((ULONG_PTR)module & 1)  /* mapped as data file */
    {
        module = (HMODULE)((ULONG_PTR)module & ~1);
        image = FALSE;
    }
    if (!(nt = RtlImageNtHeader( module ))) return NULL;
    if (dir >= nt->OptionalHeader.NumberOfRvaAndSizes) return NULL;
    if (!(addr = nt->OptionalHeader.DataDirectory[dir].VirtualAddress)) return NULL;
    *size = nt->OptionalHeader.DataDirectory[dir].Size;
    if (image || addr < nt->OptionalHeader.SizeOfHeaders) return (char *)module + addr;

    /* not mapped as image, need to find the section containing the virtual address */
    return RtlImageRvaToVa( nt, module, addr, NULL );
}


/***********************************************************************
 *           RtlImageRvaToSection   (NTDLL.@)
 */
PIMAGE_SECTION_HEADER WINAPI RtlImageRvaToSection( const IMAGE_NT_HEADERS *nt,
                                                   HMODULE module, DWORD rva )
{
    int i;
    IMAGE_SECTION_HEADER *sec = (IMAGE_SECTION_HEADER*)((char*)&nt->OptionalHeader +
                                                        nt->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        if ((sec->VirtualAddress <= rva) && (sec->VirtualAddress + sec->SizeOfRawData > rva))
            return sec;
    }
    return NULL;
}


/***********************************************************************
 *           RtlImageRvaToVa   (NTDLL.@)
 */
PVOID WINAPI RtlImageRvaToVa( const IMAGE_NT_HEADERS *nt, HMODULE module,
                              DWORD rva, IMAGE_SECTION_HEADER **section )
{
    IMAGE_SECTION_HEADER *sec;

    if (section && *section)  /* try this section first */
    {
        sec = *section;
        if ((sec->VirtualAddress <= rva) && (sec->VirtualAddress + sec->SizeOfRawData > rva))
            goto found;
    }
    if (!(sec = RtlImageRvaToSection( nt, module, rva ))) return NULL;
 found:
    if (section) *section = sec;
    return (char *)module + sec->PointerToRawData + (rva - sec->VirtualAddress);
}
