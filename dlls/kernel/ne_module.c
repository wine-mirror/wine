/*
 * NE modules
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>

#include "winbase.h"
#include "wine/winbase16.h"
#include "winerror.h"
#include "wownt32.h"
#include "wine/library.h"
#include "module.h"
#include "toolhelp.h"
#include "global.h"
#include "file.h"
#include "task.h"
#include "snoop.h"
#include "builtin16.h"
#include "stackframe.h"
#include "excpt.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);

#include "pshpack1.h"
typedef struct _GPHANDLERDEF
{
    WORD selector;
    WORD rangeStart;
    WORD rangeEnd;
    WORD handler;
} GPHANDLERDEF;
#include "poppack.h"

/*
 * Segment table entry
 */
struct ne_segment_table_entry_s
{
    WORD seg_data_offset;   /* Sector offset of segment data	*/
    WORD seg_data_length;   /* Length of segment data		*/
    WORD seg_flags;         /* Flags associated with this segment	*/
    WORD min_alloc;         /* Minimum allocation size for this	*/
};

#define hFirstModule (pThhook->hExeHead)

static NE_MODULE *pCachedModule = 0;  /* Module cached by NE_OpenFile */

typedef struct
{
    void       *module_start;      /* 32-bit address of the module data */
    int         module_size;       /* Size of the module data */
    void       *code_start;        /* 32-bit address of DLL code */
    void       *data_start;        /* 32-bit address of DLL data */
    const char *owner;             /* 32-bit dll that contains this dll */
    const void *rsrc;              /* resources data */
} BUILTIN16_DESCRIPTOR;

/* Table of all built-in DLLs */

#define MAX_DLLS 50

static const BUILTIN16_DESCRIPTOR *builtin_dlls[MAX_DLLS];


static HINSTANCE16 NE_LoadModule( LPCSTR name, BOOL lib_only );
static BOOL16 NE_FreeModule( HMODULE16 hModule, BOOL call_wep );

static HINSTANCE16 MODULE_LoadModule16( LPCSTR libname, BOOL implicit, BOOL lib_only );

static HMODULE16 NE_GetModuleByFilename( LPCSTR name );


static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
        GetExceptionCode() == EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}


/* patch all the flat cs references of the code segment if necessary */
inline static void patch_code_segment( void *code_segment )
{
#ifdef __i386__
    CALLFROM16 *call = code_segment;
    if (call->flatcs == wine_get_cs()) return;  /* nothing to patch */
    while (call->pushl == 0x68)
    {
        call->flatcs = wine_get_cs();
        call++;
    }
#endif
}


/***********************************************************************
 *           find_dll_descr
 *
 * Find a descriptor in the list
 */
static const BUILTIN16_DESCRIPTOR *find_dll_descr( const char *dllname )
{
    int i;
    for (i = 0; i < MAX_DLLS; i++)
    {
        const BUILTIN16_DESCRIPTOR *descr = builtin_dlls[i];
        if (descr)
        {
            NE_MODULE *pModule = (NE_MODULE *)descr->module_start;
            OFSTRUCT *pOfs = (OFSTRUCT *)((LPBYTE)pModule + pModule->fileinfo);
            BYTE *name_table = (BYTE *)pModule + pModule->name_table;

            /* check the dll file name */
            if (!FILE_strcasecmp( pOfs->szPathName, dllname )) return descr;
            /* check the dll module name (without extension) */
            if (!FILE_strncasecmp( dllname, name_table+1, *name_table ) &&
                !strcmp( dllname + *name_table, ".dll" ))
                return descr;
        }
    }
    return NULL;
}


/***********************************************************************
 *           is_builtin_present
 *
 * Check if a builtin dll descriptor is present (because we loaded its 32-bit counterpart).
 */
static BOOL is_builtin_present( LPCSTR name )
{
    char dllname[20], *p;

    if (strlen(name) >= sizeof(dllname)-4) return FALSE;
    strcpy( dllname, name );
    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );
    for (p = dllname; *p; p++) *p = FILE_tolower(*p);

    return (find_dll_descr( dllname ) != NULL);
}


/***********************************************************************
 *           __wine_register_dll_16 (KERNEL32.@)
 *
 * Register a built-in DLL descriptor.
 */
void __wine_register_dll_16( const BUILTIN16_DESCRIPTOR *descr )
{
    int i;

    for (i = 0; i < MAX_DLLS; i++)
    {
        if (builtin_dlls[i]) continue;
        builtin_dlls[i] = descr;
        break;
    }
    assert( i < MAX_DLLS );
}


/***********************************************************************
 *           __wine_unregister_dll_16 (KERNEL32.@)
 *
 * Unregister a built-in DLL descriptor.
 */
void __wine_unregister_dll_16( const BUILTIN16_DESCRIPTOR *descr )
{
    int i;

    for (i = 0; i < MAX_DLLS; i++)
    {
        if (builtin_dlls[i] != descr) continue;
        builtin_dlls[i] = NULL;
        break;
    }
}


/***********************************************************************
 *           NE_DumpModule
 */
void NE_DumpModule( HMODULE16 hModule )
{
    int i, ordinal;
    SEGTABLEENTRY *pSeg;
    BYTE *pstr;
    WORD *pword;
    NE_MODULE *pModule;
    ET_BUNDLE *bundle;
    ET_ENTRY *entry;

    if (!(pModule = NE_GetPtr( hModule )))
    {
        MESSAGE( "**** %04x is not a module handle\n", hModule );
        return;
    }

      /* Dump the module info */
    DPRINTF( "---\n" );
    DPRINTF( "Module %04x:\n", hModule );
    DPRINTF( "count=%d flags=%04x heap=%d stack=%d\n",
             pModule->count, pModule->flags,
             pModule->heap_size, pModule->stack_size );
    DPRINTF( "cs:ip=%04x:%04x ss:sp=%04x:%04x ds=%04x nb seg=%d modrefs=%d\n",
             pModule->cs, pModule->ip, pModule->ss, pModule->sp, pModule->dgroup,
             pModule->seg_count, pModule->modref_count );
    DPRINTF( "os_flags=%d swap_area=%d version=%04x\n",
             pModule->os_flags, pModule->min_swap_area,
             pModule->expected_version );
    if (pModule->flags & NE_FFLAGS_WIN32)
        DPRINTF( "PE module=%p\n", pModule->module32 );

      /* Dump the file info */
    DPRINTF( "---\n" );
    DPRINTF( "Filename: '%s'\n", NE_MODULE_NAME(pModule) );

      /* Dump the segment table */
    DPRINTF( "---\n" );
    DPRINTF( "Segment table:\n" );
    pSeg = NE_SEG_TABLE( pModule );
    for (i = 0; i < pModule->seg_count; i++, pSeg++)
        DPRINTF( "%02x: pos=%d size=%d flags=%04x minsize=%d hSeg=%04x\n",
                 i + 1, pSeg->filepos, pSeg->size, pSeg->flags,
                 pSeg->minsize, pSeg->hSeg );

      /* Dump the resource table */
    DPRINTF( "---\n" );
    DPRINTF( "Resource table:\n" );
    if (pModule->res_table)
    {
        pword = (WORD *)((BYTE *)pModule + pModule->res_table);
        DPRINTF( "Alignment: %d\n", *pword++ );
        while (*pword)
        {
            NE_TYPEINFO *ptr = (NE_TYPEINFO *)pword;
            NE_NAMEINFO *pname = (NE_NAMEINFO *)(ptr + 1);
            DPRINTF( "id=%04x count=%d\n", ptr->type_id, ptr->count );
            for (i = 0; i < ptr->count; i++, pname++)
                DPRINTF( "offset=%d len=%d id=%04x\n",
		      pname->offset, pname->length, pname->id );
            pword = (WORD *)pname;
        }
    }
    else DPRINTF( "None\n" );

      /* Dump the resident name table */
    DPRINTF( "---\n" );
    DPRINTF( "Resident-name table:\n" );
    pstr = (char *)pModule + pModule->name_table;
    while (*pstr)
    {
        DPRINTF( "%*.*s: %d\n", *pstr, *pstr, pstr + 1,
                 *(WORD *)(pstr + *pstr + 1) );
        pstr += *pstr + 1 + sizeof(WORD);
    }

      /* Dump the module reference table */
    DPRINTF( "---\n" );
    DPRINTF( "Module ref table:\n" );
    if (pModule->modref_table)
    {
        pword = (WORD *)((BYTE *)pModule + pModule->modref_table);
        for (i = 0; i < pModule->modref_count; i++, pword++)
        {
            char name[10];
            GetModuleName16( *pword, name, sizeof(name) );
            DPRINTF( "%d: %04x -> '%s'\n", i, *pword, name );
        }
    }
    else DPRINTF( "None\n" );

      /* Dump the entry table */
    DPRINTF( "---\n" );
    DPRINTF( "Entry table:\n" );
    bundle = (ET_BUNDLE *)((BYTE *)pModule+pModule->entry_table);
    do {
        entry = (ET_ENTRY *)((BYTE *)bundle+6);
        DPRINTF( "Bundle %d-%d: %02x\n", bundle->first, bundle->last, entry->type);
        ordinal = bundle->first;
        while (ordinal < bundle->last)
        {
            if (entry->type == 0xff)
                DPRINTF("%d: %02x:%04x (moveable)\n", ordinal++, entry->segnum, entry->offs);
            else
                DPRINTF("%d: %02x:%04x (fixed)\n", ordinal++, entry->segnum, entry->offs);
            entry++;
        }
    } while ( (bundle->next) && (bundle = ((ET_BUNDLE *)((BYTE *)pModule + bundle->next))) );

    /* Dump the non-resident names table */
    DPRINTF( "---\n" );
    DPRINTF( "Non-resident names table:\n" );
    if (pModule->nrname_handle)
    {
        pstr = (char *)GlobalLock16( pModule->nrname_handle );
        while (*pstr)
        {
            DPRINTF( "%*.*s: %d\n", *pstr, *pstr, pstr + 1,
                   *(WORD *)(pstr + *pstr + 1) );
            pstr += *pstr + 1 + sizeof(WORD);
        }
    }
    DPRINTF( "\n" );
}


/***********************************************************************
 *           NE_WalkModules
 *
 * Walk the module list and print the modules.
 */
void NE_WalkModules(void)
{
    HMODULE16 hModule = hFirstModule;
    MESSAGE( "Module Flags Name\n" );
    while (hModule)
    {
        NE_MODULE *pModule = NE_GetPtr( hModule );
        if (!pModule)
        {
            MESSAGE( "Bad module %04x in list\n", hModule );
            return;
        }
        MESSAGE( " %04x  %04x  %.*s\n", hModule, pModule->flags,
                 *((char *)pModule + pModule->name_table),
                 (char *)pModule + pModule->name_table + 1 );
        hModule = pModule->next;
    }
}


/***********************************************************************
 *		EntryAddrProc (KERNEL.667) Wine-specific export
 *
 * Return the entry point for a given ordinal.
 */
FARPROC16 WINAPI EntryAddrProc16( HMODULE16 hModule, WORD ordinal )
{
    FARPROC16 ret = NE_GetEntryPointEx( hModule, ordinal, TRUE );
    CURRENT_STACK16->ecx = hModule; /* FIXME: might be incorrect value */
    return ret;
}

/***********************************************************************
 *           NE_SetEntryPoint
 *
 * Change the value of an entry point. Use with caution!
 * It can only change the offset value, not the selector.
 */
BOOL16 NE_SetEntryPoint( HMODULE16 hModule, WORD ordinal, WORD offset )
{
    NE_MODULE *pModule;
    ET_ENTRY *entry;
    ET_BUNDLE *bundle;
    int i;

    if (!(pModule = NE_GetPtr( hModule ))) return FALSE;
    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

    bundle = (ET_BUNDLE *)((BYTE *)pModule + pModule->entry_table);
    while ((ordinal < bundle->first + 1) || (ordinal > bundle->last))
    {
        bundle = (ET_BUNDLE *)((BYTE *)pModule + bundle->next);
        if (!(bundle->next)) return 0;
    }

    entry = (ET_ENTRY *)((BYTE *)bundle+6);
    for (i=0; i < (ordinal - bundle->first - 1); i++)
        entry++;

    memcpy( &entry->offs, &offset, sizeof(WORD) );
    return TRUE;
}


/***********************************************************************
 *           NE_OpenFile
 */
HANDLE NE_OpenFile( NE_MODULE *pModule )
{
    HANDLE handle;
    char *name;

    TRACE("(%p)\n", pModule );
    /* mjm - removed module caching because it keeps open file handles
             thus preventing CDROMs from being ejected */
    name = NE_MODULE_NAME( pModule );
    if ((handle = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ,
                                   NULL, OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE)
        MESSAGE( "Can't open file '%s' for module %04x\n", name, pModule->self );
    TRACE("opened '%s' -> %p\n", name, handle);
    return handle;
}


/* wrapper for SetFilePointer and ReadFile */
static BOOL read_data( HANDLE handle, LONG offset, void *buffer, DWORD size )
{
    DWORD result;

    if (SetFilePointer( handle, offset, NULL, SEEK_SET ) == INVALID_SET_FILE_POINTER) return FALSE;
    if (!ReadFile( handle, buffer, size, &result, NULL )) return FALSE;
    return (result == size);
}

/***********************************************************************
 *           NE_LoadExeHeader
 */
static HMODULE16 NE_LoadExeHeader( HANDLE handle, LPCSTR path )
{
    IMAGE_DOS_HEADER mz_header;
    IMAGE_OS2_HEADER ne_header;
    int size;
    HMODULE16 hModule;
    NE_MODULE *pModule;
    BYTE *pData, *pTempEntryTable;
    char *buffer, *fastload = NULL;
    int fastload_offset = 0, fastload_length = 0;
    ET_ENTRY *entry;
    ET_BUNDLE *bundle, *oldbundle;
    OFSTRUCT *ofs;

  /* Read a block from either the file or the fast-load area. */
#define READ(offset,size,buffer) \
       ((fastload && ((offset) >= fastload_offset) && \
         ((offset)+(size) <= fastload_offset+fastload_length)) ? \
        (memcpy( buffer, fastload+(offset)-fastload_offset, (size) ), TRUE) : \
         read_data( handle, (offset), (buffer), (size)))

    if (!read_data( handle, 0, &mz_header, sizeof(mz_header)) ||
        (mz_header.e_magic != IMAGE_DOS_SIGNATURE))
        return (HMODULE16)11;  /* invalid exe */

    if (!read_data( handle, mz_header.e_lfanew, &ne_header, sizeof(ne_header) ))
        return (HMODULE16)11;  /* invalid exe */

    if (ne_header.ne_magic == IMAGE_NT_SIGNATURE) return (HMODULE16)21;  /* win32 exe */
    if (ne_header.ne_magic == IMAGE_OS2_SIGNATURE_LX) {
        MESSAGE("Sorry, this is an OS/2 linear executable (LX) file !\n");
        return (HMODULE16)12;
    }
    if (ne_header.ne_magic != IMAGE_OS2_SIGNATURE) return (HMODULE16)11;  /* invalid exe */

    /* We now have a valid NE header */

    size = sizeof(NE_MODULE) +
             /* segment table */
           ne_header.ne_cseg * sizeof(SEGTABLEENTRY) +
             /* resource table */
           ne_header.ne_restab - ne_header.ne_rsrctab +
             /* resident names table */
           ne_header.ne_modtab - ne_header.ne_restab +
             /* module ref table */
           ne_header.ne_cmod * sizeof(WORD) +
             /* imported names table */
           ne_header.ne_enttab - ne_header.ne_imptab +
             /* entry table length */
           ne_header.ne_cbenttab +
             /* entry table extra conversion space */
           sizeof(ET_BUNDLE) +
           2 * (ne_header.ne_cbenttab - ne_header.ne_cmovent*6) +
             /* loaded file info */
           sizeof(OFSTRUCT) - sizeof(ofs->szPathName) + strlen(path) + 1;

    hModule = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT, size );
    if (!hModule) return (HMODULE16)11;  /* invalid exe */

    FarSetOwner16( hModule, hModule );
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    memcpy( pModule, &ne_header, sizeof(ne_header) );
    pModule->count = 0;
    /* check *programs* for default minimal stack size */
    if ( (!(pModule->flags & NE_FFLAGS_LIBMODULE))
         && (pModule->stack_size < 0x1400) )
        pModule->stack_size = 0x1400;
    pModule->module32 = 0;
    pModule->self = hModule;
    pModule->self_loading_sel = 0;
    pData = (BYTE *)(pModule + 1);

    /* Clear internal Wine flags in case they are set in the EXE file */

    pModule->flags &= ~(NE_FFLAGS_BUILTIN | NE_FFLAGS_WIN32);

    /* Read the fast-load area */

    if (ne_header.ne_flagsothers & NE_AFLAGS_FASTLOAD)
    {
        fastload_offset=ne_header.ne_pretthunks << ne_header.ne_align;
        fastload_length=ne_header.ne_psegrefbytes << ne_header.ne_align;
        TRACE("Using fast-load area offset=%x len=%d\n",
                        fastload_offset, fastload_length );
        if ((fastload = HeapAlloc( GetProcessHeap(), 0, fastload_length )) != NULL)
        {
            if (!read_data( handle, fastload_offset, fastload, fastload_length))
            {
                HeapFree( GetProcessHeap(), 0, fastload );
                WARN("Error reading fast-load area!\n");
                fastload = NULL;
            }
        }
    }

    /* Get the segment table */

    pModule->seg_table = pData - (BYTE *)pModule;
    buffer = HeapAlloc( GetProcessHeap(), 0, ne_header.ne_cseg *
                                      sizeof(struct ne_segment_table_entry_s));
    if (buffer)
    {
        int i;
        struct ne_segment_table_entry_s *pSeg;

        if (!READ( mz_header.e_lfanew + ne_header.ne_segtab,
             ne_header.ne_cseg * sizeof(struct ne_segment_table_entry_s),
             buffer ))
        {
            HeapFree( GetProcessHeap(), 0, buffer );
            if (fastload) HeapFree( GetProcessHeap(), 0, fastload );
            GlobalFree16( hModule );
            return (HMODULE16)11;  /* invalid exe */
        }
        pSeg = (struct ne_segment_table_entry_s *)buffer;
        for (i = ne_header.ne_cseg; i > 0; i--, pSeg++)
        {
            memcpy( pData, pSeg, sizeof(*pSeg) );
            pData += sizeof(SEGTABLEENTRY);
        }
        HeapFree( GetProcessHeap(), 0, buffer );
    }
    else
    {
        if (fastload) HeapFree( GetProcessHeap(), 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE16)11;  /* invalid exe */
    }

    /* Get the resource table */

    if (ne_header.ne_rsrctab < ne_header.ne_restab)
    {
        pModule->res_table = pData - (BYTE *)pModule;
        if (!READ(mz_header.e_lfanew + ne_header.ne_rsrctab,
                  ne_header.ne_restab - ne_header.ne_rsrctab,
                  pData ))
            return (HMODULE16)11;  /* invalid exe */
        pData += ne_header.ne_restab - ne_header.ne_rsrctab;
        NE_InitResourceHandler( pModule );
    }
    else pModule->res_table = 0;  /* No resource table */

    /* Get the resident names table */

    pModule->name_table = pData - (BYTE *)pModule;
    if (!READ( mz_header.e_lfanew + ne_header.ne_restab,
               ne_header.ne_modtab - ne_header.ne_restab,
               pData ))
    {
        if (fastload) HeapFree( GetProcessHeap(), 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE16)11;  /* invalid exe */
    }
    pData += ne_header.ne_modtab - ne_header.ne_restab;

    /* Get the module references table */

    if (ne_header.ne_cmod > 0)
    {
        pModule->modref_table = pData - (BYTE *)pModule;
        if (!READ( mz_header.e_lfanew + ne_header.ne_modtab,
                  ne_header.ne_cmod * sizeof(WORD),
                  pData ))
        {
            if (fastload) HeapFree( GetProcessHeap(), 0, fastload );
            GlobalFree16( hModule );
            return (HMODULE16)11;  /* invalid exe */
        }
        pData += ne_header.ne_cmod * sizeof(WORD);
    }
    else pModule->modref_table = 0;  /* No module references */

    /* Get the imported names table */

    pModule->import_table = pData - (BYTE *)pModule;
    if (!READ( mz_header.e_lfanew + ne_header.ne_imptab,
               ne_header.ne_enttab - ne_header.ne_imptab,
               pData ))
    {
        if (fastload) HeapFree( GetProcessHeap(), 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE16)11;  /* invalid exe */
    }
    pData += ne_header.ne_enttab - ne_header.ne_imptab;

    /* Load entry table, convert it to the optimized version used by Windows */

    if ((pTempEntryTable = HeapAlloc( GetProcessHeap(), 0, ne_header.ne_cbenttab)) != NULL)
    {
        BYTE nr_entries, type, *s;

        TRACE("Converting entry table.\n");
        pModule->entry_table = pData - (BYTE *)pModule;
        if (!READ( mz_header.e_lfanew + ne_header.ne_enttab,
                   ne_header.ne_cbenttab, pTempEntryTable ))
        {
            HeapFree( GetProcessHeap(), 0, pTempEntryTable );
            if (fastload) HeapFree( GetProcessHeap(), 0, fastload );
            GlobalFree16( hModule );
            return (HMODULE16)11;  /* invalid exe */
        }

        s = pTempEntryTable;
        TRACE("entry table: offs %04x, len %04x, entries %d\n", ne_header.ne_enttab, ne_header.ne_cbenttab, *s);

        bundle = (ET_BUNDLE *)pData;
        TRACE("first bundle: %p\n", bundle);
        memset(bundle, 0, sizeof(ET_BUNDLE)); /* in case no entry table exists */
        entry = (ET_ENTRY *)((BYTE *)bundle+6);

        while ((nr_entries = *s++))
        {
            if ((type = *s++))
            {
                bundle->last += nr_entries;
                if (type == 0xff)
                while (nr_entries--)
                {
                    entry->type   = type;
                    entry->flags  = *s++;
                    s += 2;
                    entry->segnum = *s++;
                    entry->offs   = *(WORD *)s; s += 2;
                    /*TRACE(module, "entry: %p, type: %d, flags: %d, segnum: %d, offs: %04x\n", entry, entry->type, entry->flags, entry->segnum, entry->offs);*/
                    entry++;
                }
                else
                while (nr_entries--)
                {
                    entry->type   = type;
                    entry->flags  = *s++;
                    entry->segnum = type;
                    entry->offs   = *(WORD *)s; s += 2;
                    /*TRACE(module, "entry: %p, type: %d, flags: %d, segnum: %d, offs: %04x\n", entry, entry->type, entry->flags, entry->segnum, entry->offs);*/
                    entry++;
                }
            }
            else
            {
                if (bundle->first == bundle->last)
                {
                    bundle->first += nr_entries;
                    bundle->last += nr_entries;
                }
                else
                {
                    oldbundle = bundle;
                    oldbundle->next = ((int)entry - (int)pModule);
                    bundle = (ET_BUNDLE *)entry;
                    TRACE("new bundle: %p\n", bundle);
                    bundle->first = bundle->last =
                        oldbundle->last + nr_entries;
                    bundle->next = 0;
                    (BYTE *)entry += sizeof(ET_BUNDLE);
                }
            }
        }
        HeapFree( GetProcessHeap(), 0, pTempEntryTable );
    }
    else
    {
        if (fastload) HeapFree( GetProcessHeap(), 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE16)11;  /* invalid exe */
    }

    pData += ne_header.ne_cbenttab + sizeof(ET_BUNDLE) +
        2 * (ne_header.ne_cbenttab - ne_header.ne_cmovent*6);

    if ((DWORD)entry > (DWORD)pData)
       ERR("converted entry table bigger than reserved space !!!\nentry: %p, pData: %p. Please report !\n", entry, pData);

    /* Store the filename information */

    pModule->fileinfo = pData - (BYTE *)pModule;
    size = sizeof(OFSTRUCT) - sizeof(ofs->szPathName) + strlen(path) + 1;
    ofs = (OFSTRUCT *)pData;
    ofs->cBytes = size - 1;
    ofs->fFixedDisk = 1;
    strcpy( ofs->szPathName, path );
    pData += size;

    /* Free the fast-load area */

#undef READ
    if (fastload) HeapFree( GetProcessHeap(), 0, fastload );

    /* Get the non-resident names table */

    if (ne_header.ne_cbnrestab)
    {
        pModule->nrname_handle = GlobalAlloc16( 0, ne_header.ne_cbnrestab );
        if (!pModule->nrname_handle)
        {
            GlobalFree16( hModule );
            return (HMODULE16)11;  /* invalid exe */
        }
        FarSetOwner16( pModule->nrname_handle, hModule );
        buffer = GlobalLock16( pModule->nrname_handle );
        if (!read_data( handle, ne_header.ne_nrestab, buffer, ne_header.ne_cbnrestab ))
        {
            GlobalFree16( pModule->nrname_handle );
            GlobalFree16( hModule );
            return (HMODULE16)11;  /* invalid exe */
        }
    }
    else pModule->nrname_handle = 0;

    /* Allocate a segment for the implicitly-loaded DLLs */

    if (pModule->modref_count)
    {
        pModule->dlls_to_init = GlobalAlloc16( GMEM_ZEROINIT,
                                               (pModule->modref_count+1)*sizeof(HMODULE16) );
        if (!pModule->dlls_to_init)
        {
            if (pModule->nrname_handle) GlobalFree16( pModule->nrname_handle );
            GlobalFree16( hModule );
            return (HMODULE16)11;  /* invalid exe */
        }
        FarSetOwner16( pModule->dlls_to_init, hModule );
    }
    else pModule->dlls_to_init = 0;

    NE_RegisterModule( pModule );
    SNOOP16_RegisterDLL(pModule,path);
    return hModule;
}


/***********************************************************************
 *           NE_LoadDLLs
 *
 * Load all DLLs implicitly linked to a module.
 */
static BOOL NE_LoadDLLs( NE_MODULE *pModule )
{
    int i;
    WORD *pModRef = (WORD *)((char *)pModule + pModule->modref_table);
    WORD *pDLLs = (WORD *)GlobalLock16( pModule->dlls_to_init );

    for (i = 0; i < pModule->modref_count; i++, pModRef++)
    {
        char buffer[260], *p;
        BYTE *pstr = (BYTE *)pModule + pModule->import_table + *pModRef;
        memcpy( buffer, pstr + 1, *pstr );
        *(buffer + *pstr) = 0; /* terminate it */

        TRACE("Loading '%s'\n", buffer );
        if (!(*pModRef = GetModuleHandle16( buffer )))
        {
            /* If the DLL is not loaded yet, load it and store */
            /* its handle in the list of DLLs to initialize.   */
            HMODULE16 hDLL;

            /* Append .DLL to name if no extension present */
            if (!(p = strrchr( buffer, '.')) || strchr( p, '/' ) || strchr( p, '\\'))
                    strcat( buffer, ".DLL" );

            if ((hDLL = MODULE_LoadModule16( buffer, TRUE, TRUE )) < 32)
            {
                /* FIXME: cleanup what was done */

                MESSAGE( "Could not load '%s' required by '%.*s', error=%d\n",
                     buffer, *((BYTE*)pModule + pModule->name_table),
                     (char *)pModule + pModule->name_table + 1, hDLL );
                return FALSE;
            }
            *pModRef = GetExePtr( hDLL );
            *pDLLs++ = *pModRef;
        }
        else  /* Increment the reference count of the DLL */
        {
            NE_MODULE *pOldDLL = NE_GetPtr( *pModRef );
            if (pOldDLL) pOldDLL->count++;
        }
    }
    return TRUE;
}


/**********************************************************************
 *	    NE_DoLoadModule
 *
 * Load first instance of NE module from file.
 *
 * pModule must point to a module structure prepared by NE_LoadExeHeader.
 * This routine must never be called twice on a module.
 *
 */
static HINSTANCE16 NE_DoLoadModule( NE_MODULE *pModule )
{
    /* Allocate the segments for this module */

    if (!NE_CreateAllSegments( pModule ))
        return ERROR_NOT_ENOUGH_MEMORY; /* 8 */

    /* Load the referenced DLLs */

    if (!NE_LoadDLLs( pModule ))
        return ERROR_FILE_NOT_FOUND; /* 2 */

    /* Load the segments */

    NE_LoadAllSegments( pModule );

    /* Make sure the usage count is 1 on the first loading of  */
    /* the module, even if it contains circular DLL references */

    pModule->count = 1;

    return NE_GetInstance( pModule );
}

/**********************************************************************
 *	    NE_LoadModule
 *
 * Load first instance of NE module. (Note: caller is responsible for
 * ensuring the module isn't already loaded!)
 *
 * If the module turns out to be an executable module, only a
 * handle to a module stub is returned; this needs to be initialized
 * by calling NE_DoLoadModule later, in the context of the newly
 * created process.
 *
 * If lib_only is TRUE, however, the module is perforce treated
 * like a DLL module, even if it is an executable module.
 *
 */
static HINSTANCE16 NE_LoadModule( LPCSTR name, BOOL lib_only )
{
    NE_MODULE *pModule;
    HMODULE16 hModule;
    HINSTANCE16 hInstance;
    HFILE16 hFile;
    OFSTRUCT ofs;

    /* Open file */
    if ((hFile = OpenFile16( name, &ofs, OF_READ )) == HFILE_ERROR16)
        return (HMODULE16)2;  /* File not found */

    hModule = NE_LoadExeHeader( DosFileHandleToWin32Handle(hFile), ofs.szPathName );
    _lclose16( hFile );
    if (hModule < 32) return hModule;

    pModule = NE_GetPtr( hModule );
    if ( !pModule ) return hModule;

    if ( !lib_only && !( pModule->flags & NE_FFLAGS_LIBMODULE ) )
        return hModule;

    hInstance = NE_DoLoadModule( pModule );
    if ( hInstance < 32 )
    {
        /* cleanup ... */
        NE_FreeModule( hModule, 0 );
    }

    return hInstance;
}


/***********************************************************************
 *           NE_DoLoadBuiltinModule
 *
 * Load a built-in Win16 module. Helper function for NE_LoadBuiltinModule.
 */
static HMODULE16 NE_DoLoadBuiltinModule( const BUILTIN16_DESCRIPTOR *descr )
{
    NE_MODULE *pModule;
    int minsize;
    SEGTABLEENTRY *pSegTable;
    HMODULE16 hModule;

    hModule = GLOBAL_CreateBlock( GMEM_MOVEABLE, descr->module_start,
                                  descr->module_size, 0, WINE_LDT_FLAGS_DATA );
    if (!hModule) return 0;
    FarSetOwner16( hModule, hModule );

    pModule = (NE_MODULE *)GlobalLock16( hModule );
    pModule->self = hModule;
    /* NOTE: (Ab)use the hRsrcMap parameter for resource data pointer */
    pModule->hRsrcMap = (void *)descr->rsrc;

    /* Allocate the code segment */

    pSegTable = NE_SEG_TABLE( pModule );
    pSegTable->hSeg = GLOBAL_CreateBlock( GMEM_FIXED, descr->code_start,
                                          pSegTable->minsize, hModule,
                                          WINE_LDT_FLAGS_CODE|WINE_LDT_FLAGS_32BIT );
    if (!pSegTable->hSeg) return 0;
    patch_code_segment( descr->code_start );
    pSegTable++;

    /* Allocate the data segment */

    minsize = pSegTable->minsize ? pSegTable->minsize : 0x10000;
    minsize += pModule->heap_size;
    if (minsize > 0x10000) minsize = 0x10000;
    pSegTable->hSeg = GlobalAlloc16( GMEM_FIXED, minsize );
    if (!pSegTable->hSeg) return 0;
    FarSetOwner16( pSegTable->hSeg, hModule );
    if (pSegTable->minsize) memcpy( GlobalLock16( pSegTable->hSeg ),
                                    descr->data_start, pSegTable->minsize);
    if (pModule->heap_size)
        LocalInit16( GlobalHandleToSel16(pSegTable->hSeg), pSegTable->minsize, minsize );

    if (descr->rsrc) NE_InitResourceHandler(pModule);

    NE_RegisterModule( pModule );

    /* make sure the 32-bit library containing this one is loaded too */
    LoadLibraryA( descr->owner );

    return hModule;
}


/***********************************************************************
 *           NE_LoadBuiltinModule
 *
 * Load a built-in module.
 */
static HMODULE16 NE_LoadBuiltinModule( LPCSTR name )
{
    const BUILTIN16_DESCRIPTOR *descr;
    char error[256], dllname[20], *p;
    int file_exists;
    void *handle;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;

    if (strlen(name) >= sizeof(dllname)-4) return (HMODULE16)2;

    strcpy( dllname, name );
    p = strrchr( dllname, '.' );
    if (!p) strcat( dllname, ".dll" );
    for (p = dllname; *p; p++) *p = FILE_tolower(*p);

    if ((descr = find_dll_descr( dllname )))
        return NE_DoLoadBuiltinModule( descr );

    if ((handle = wine_dll_load( dllname, error, sizeof(error), &file_exists )))
    {
        if ((descr = find_dll_descr( dllname )))
            return NE_DoLoadBuiltinModule( descr );

        ERR( "loaded .so but dll %s still not found\n", dllname );
    }
    else
    {
        if (!file_exists) WARN("cannot open .so lib for 16-bit builtin %s: %s\n", name, error);
        else ERR("failed to load .so lib for 16-bit builtin %s: %s\n", name, error );
    }
    return (HMODULE16)2;
}


/**********************************************************************
 *	    MODULE_LoadModule16
 *
 * Load a NE module in the order of the loadorder specification.
 * The caller is responsible that the module is not loaded already.
 *
 */
static HINSTANCE16 MODULE_LoadModule16( LPCSTR libname, BOOL implicit, BOOL lib_only )
{
    HINSTANCE16 hinst = 2;
    enum loadorder_type loadorder[LOADORDER_NTYPES];
    int i;
    const char *filetype = "";
    const char *ptr;

    /* strip path information */

    if (libname[0] && libname[1] == ':') libname += 2;  /* strip drive specification */
    if ((ptr = strrchr( libname, '\\' ))) libname = ptr + 1;
    if ((ptr = strrchr( libname, '/' ))) libname = ptr + 1;

    if (is_builtin_present(libname))
    {
        TRACE( "forcing loadorder to builtin for %s\n", debugstr_a(libname) );
        /* force builtin loadorder since the dll is already in memory */
        loadorder[0] = LOADORDER_BI;
        loadorder[1] = LOADORDER_INVALID;
    }
    else MODULE_GetLoadOrder(loadorder, libname, FALSE);

    for(i = 0; i < LOADORDER_NTYPES; i++)
    {
        if (loadorder[i] == LOADORDER_INVALID) break;

        switch(loadorder[i])
        {
        case LOADORDER_DLL:
            TRACE("Trying native dll '%s'\n", libname);
            hinst = NE_LoadModule(libname, lib_only);
            filetype = "native";
            break;

        case LOADORDER_BI:
            TRACE("Trying built-in '%s'\n", libname);
            hinst = NE_LoadBuiltinModule(libname);
            filetype = "builtin";
            break;

        default:
            hinst = 2;
            break;
        }

        if(hinst >= 32)
        {
            TRACE_(loaddll)("Loaded module '%s' : %s\n", libname, filetype);
            if(!implicit)
            {
                HMODULE16 hModule;
                NE_MODULE *pModule;

                hModule = GetModuleHandle16(libname);
                if(!hModule)
                {
                    ERR("Serious trouble. Just loaded module '%s' (hinst=0x%04x), but can't get module handle. Filename too long ?\n",
                        libname, hinst);
                    return 6;   /* ERROR_INVALID_HANDLE seems most appropriate */
                }

                pModule = NE_GetPtr(hModule);
                if(!pModule)
                {
                    ERR("Serious trouble. Just loaded module '%s' (hinst=0x%04x), but can't get NE_MODULE pointer\n",
                        libname, hinst);
                    return 6;   /* ERROR_INVALID_HANDLE seems most appropriate */
                }

                TRACE("Loaded module '%s' at 0x%04x.\n", libname, hinst);

                /*
                 * Call initialization routines for all loaded DLLs. Note that
                 * when we load implicitly linked DLLs this will be done by InitTask().
                 */
                if(pModule->flags & NE_FFLAGS_LIBMODULE)
                {
                    NE_InitializeDLLs(hModule);
                    NE_DllProcessAttach(hModule);
                }
            }
            return hinst;
        }

        if(hinst != 2)
        {
            /* We quit searching when we get another error than 'File not found' */
            break;
        }
    }
    return hinst;       /* The last error that occurred */
}


/**********************************************************************
 *          NE_CreateThread
 *
 * Create the thread for a 16-bit module.
 */
static HINSTANCE16 NE_CreateThread( NE_MODULE *pModule, WORD cmdShow, LPCSTR cmdline )
{
    HANDLE hThread;
    TDB *pTask;
    HTASK16 hTask;
    HINSTANCE16 instance = 0;

    if (!(hTask = TASK_SpawnTask( pModule, cmdShow, cmdline + 1, *cmdline, &hThread )))
        return 0;

    /* Post event to start the task */
    PostEvent16( hTask );

    /* Wait until we get the instance handle */
    do
    {
        DirectedYield16( hTask );
        if (!IsTask16( hTask ))  /* thread has died */
        {
            DWORD exit_code;
            WaitForSingleObject( hThread, INFINITE );
            GetExitCodeThread( hThread, &exit_code );
            CloseHandle( hThread );
            return exit_code;
        }
        if (!(pTask = GlobalLock16( hTask ))) break;
        instance = pTask->hInstance;
        GlobalUnlock16( hTask );
    } while (!instance);

    CloseHandle( hThread );
    return instance;
}


/**********************************************************************
 *          LoadModule      (KERNEL.45)
 */
HINSTANCE16 WINAPI LoadModule16( LPCSTR name, LPVOID paramBlock )
{
    BOOL lib_only = !paramBlock || (paramBlock == (LPVOID)-1);
    LOADPARAMS16 *params;
    HMODULE16 hModule;
    NE_MODULE *pModule;
    LPSTR cmdline;
    WORD cmdShow;

    /* Load module */

    if ( (hModule = NE_GetModuleByFilename(name) ) != 0 )
    {
        /* Special case: second instance of an already loaded NE module */

        if ( !( pModule = NE_GetPtr( hModule ) ) ) return (HINSTANCE16)11;
        if ( pModule->module32 ) return (HINSTANCE16)21;

        /* Increment refcount */

        pModule->count++;
    }
    else
    {
        /* Main case: load first instance of NE module */

        if ( (hModule = MODULE_LoadModule16( name, FALSE, lib_only )) < 32 )
            return hModule;

        if ( !(pModule = NE_GetPtr( hModule )) )
            return (HINSTANCE16)11;
    }

    /* If library module, we just retrieve the instance handle */

    if ( ( pModule->flags & NE_FFLAGS_LIBMODULE ) || lib_only )
        return NE_GetInstance( pModule );

    /*
     *  At this point, we need to create a new process.
     *
     *  pModule points either to an already loaded module, whose refcount
     *  has already been incremented (to avoid having the module vanish
     *  in the meantime), or else to a stub module which contains only header
     *  information.
     */
    params = (LOADPARAMS16 *)paramBlock;
    cmdShow = ((WORD *)MapSL(params->showCmd))[1];
    cmdline = MapSL( params->cmdLine );
    return NE_CreateThread( pModule, cmdShow, cmdline );
}


/**********************************************************************
 *          NE_StartTask
 *
 * Startup code for a new 16-bit task.
 */
DWORD NE_StartTask(void)
{
    TDB *pTask = TASK_GetCurrent();
    NE_MODULE *pModule = NE_GetPtr( pTask->hModule );
    HINSTANCE16 hInstance, hPrevInstance;
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );
    WORD sp;

    if ( pModule->count > 0 )
    {
        /* Second instance of an already loaded NE module */
        /* Note that the refcount was already incremented by the parent */

        hPrevInstance = NE_GetInstance( pModule );

        if ( pModule->dgroup )
            if ( NE_CreateSegment( pModule, pModule->dgroup ) )
                NE_LoadSegment( pModule, pModule->dgroup );

        hInstance = NE_GetInstance( pModule );
        TRACE("created second instance %04x[%d] of instance %04x.\n", hInstance, pModule->dgroup, hPrevInstance);

    }
    else
    {
        /* Load first instance of NE module */

        pModule->flags |= NE_FFLAGS_GUI;  /* FIXME: is this necessary? */

        hInstance = NE_DoLoadModule( pModule );
        hPrevInstance = 0;
    }

    if ( hInstance >= 32 )
    {
        CONTEXT86 context;

        /* Enter instance handles into task struct */

        pTask->hInstance = hInstance;
        pTask->hPrevInstance = hPrevInstance;

        /* Use DGROUP for 16-bit stack */

        if (!(sp = pModule->sp))
            sp = pSegTable[pModule->ss-1].minsize + pModule->stack_size;
        sp &= ~1;
        sp -= sizeof(STACK16FRAME);
        pTask->teb->cur_stack = MAKESEGPTR( GlobalHandleToSel16(hInstance), sp );

        /* Registers at initialization must be:
         * ax   zero
         * bx   stack size in bytes
         * cx   heap size in bytes
         * si   previous app instance
         * di   current app instance
         * bp   zero
         * es   selector to the PSP
         * ds   dgroup of the application
         * ss   stack selector
         * sp   top of the stack
         */
        memset( &context, 0, sizeof(context) );
        context.SegCs  = GlobalHandleToSel16(pSegTable[pModule->cs - 1].hSeg);
        context.SegDs  = GlobalHandleToSel16(pTask->hInstance);
        context.SegEs  = pTask->hPDB;
        context.Eip    = pModule->ip;
        context.Ebx    = pModule->stack_size;
        context.Ecx    = pModule->heap_size;
        context.Edi    = pTask->hInstance;
        context.Esi    = pTask->hPrevInstance;

        /* Now call 16-bit entry point */

        TRACE("Starting main program: cs:ip=%04lx:%04lx ds=%04lx ss:sp=%04x:%04x\n",
              context.SegCs, context.Eip, context.SegDs,
              SELECTOROF(pTask->teb->cur_stack),
              OFFSETOF(pTask->teb->cur_stack) );

        WOWCallback16Ex( 0, WCB16_REGS, 0, NULL, (DWORD *)&context );
        ExitThread( LOWORD(context.Eax) );
    }
    return hInstance;  /* error code */
}

/***********************************************************************
 *           LoadLibrary     (KERNEL.95)
 *           LoadLibrary16   (KERNEL32.35)
 */
HINSTANCE16 WINAPI LoadLibrary16( LPCSTR libname )
{
    return LoadModule16(libname, (LPVOID)-1 );
}


/**********************************************************************
 *	    MODULE_CallWEP
 *
 * Call a DLL's WEP, allowing it to shut down.
 * FIXME: we always pass the WEP WEP_FREE_DLL, never WEP_SYSTEM_EXIT
 */
static BOOL16 MODULE_CallWEP( HMODULE16 hModule )
{
    BOOL16 ret;
    FARPROC16 WEP = GetProcAddress16( hModule, "WEP" );
    if (!WEP) return FALSE;

    __TRY
    {
        WORD args[1];
        DWORD dwRet;

        args[0] = WEP_FREE_DLL;
        WOWCallback16Ex( (DWORD)WEP, WCB16_PASCAL, sizeof(args), args, &dwRet );
        ret = LOWORD(dwRet);
    }
    __EXCEPT(page_fault)
    {
        WARN("Page fault\n");
        ret = 0;
    }
    __ENDTRY

    return ret;
}


/**********************************************************************
 *	    NE_FreeModule
 *
 * Implementation of FreeModule16().
 */
static BOOL16 NE_FreeModule( HMODULE16 hModule, BOOL call_wep )
{
    HMODULE16 *hPrevModule;
    NE_MODULE *pModule;
    HMODULE16 *pModRef;
    int i;

    if (!(pModule = NE_GetPtr( hModule ))) return FALSE;
    hModule = pModule->self;

    TRACE("%04x count %d\n", hModule, pModule->count );

    if (((INT16)(--pModule->count)) > 0 ) return TRUE;
    else pModule->count = 0;

    if (pModule->flags & NE_FFLAGS_BUILTIN)
        return FALSE;  /* Can't free built-in module */

    if (call_wep && !(pModule->flags & NE_FFLAGS_WIN32))
    {
        /* Free the objects owned by the DLL module */
        NE_CallUserSignalProc( hModule, USIG16_DLL_UNLOAD );

        if (pModule->flags & NE_FFLAGS_LIBMODULE)
            MODULE_CallWEP( hModule );
        else
            call_wep = FALSE;  /* We are freeing a task -> no more WEPs */
    }


    /* Clear magic number just in case */

    pModule->magic = pModule->self = 0;

      /* Remove it from the linked list */

    hPrevModule = &hFirstModule;
    while (*hPrevModule && (*hPrevModule != hModule))
    {
        hPrevModule = &(NE_GetPtr( *hPrevModule ))->next;
    }
    if (*hPrevModule) *hPrevModule = pModule->next;

    /* Free the referenced modules */

    pModRef = (HMODULE16*)NE_MODULE_TABLE( pModule );
    for (i = 0; i < pModule->modref_count; i++, pModRef++)
    {
        NE_FreeModule( *pModRef, call_wep );
    }

    /* Free the module storage */

    GlobalFreeAll16( hModule );

    /* Remove module from cache */

    if (pCachedModule == pModule) pCachedModule = NULL;
    return TRUE;
}


/**********************************************************************
 *	    FreeModule    (KERNEL.46)
 */
BOOL16 WINAPI FreeModule16( HMODULE16 hModule )
{
    return NE_FreeModule( hModule, TRUE );
}


/***********************************************************************
 *           FreeLibrary     (KERNEL.96)
 *           FreeLibrary16   (KERNEL32.36)
 */
void WINAPI FreeLibrary16( HINSTANCE16 handle )
{
    TRACE("%04x\n", handle );
    FreeModule16( handle );
}


/**********************************************************************
 *	    GetModuleName    (KERNEL.27)
 */
BOOL16 WINAPI GetModuleName16( HINSTANCE16 hinst, LPSTR buf, INT16 count )
{
    NE_MODULE *pModule;
    BYTE *p;

    if (!(pModule = NE_GetPtr( hinst ))) return FALSE;
    p = (BYTE *)pModule + pModule->name_table;
    if (count > *p) count = *p + 1;
    if (count > 0)
    {
        memcpy( buf, p + 1, count - 1 );
        buf[count-1] = '\0';
    }
    return TRUE;
}


/**********************************************************************
 *	    GetModuleUsage    (KERNEL.48)
 */
INT16 WINAPI GetModuleUsage16( HINSTANCE16 hModule )
{
    NE_MODULE *pModule = NE_GetPtr( hModule );
    return pModule ? pModule->count : 0;
}


/**********************************************************************
 *	    GetExpWinVer    (KERNEL.167)
 */
WORD WINAPI GetExpWinVer16( HMODULE16 hModule )
{
    NE_MODULE *pModule = NE_GetPtr( hModule );
    if ( !pModule ) return 0;

    /*
     * For built-in modules, fake the expected version the module should
     * have according to the Windows version emulated by Wine
     */
    if ( !pModule->expected_version )
    {
        OSVERSIONINFOA versionInfo;
        versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);

        if ( GetVersionExA( &versionInfo ) )
            pModule->expected_version =
                     (versionInfo.dwMajorVersion & 0xff) << 8
                   | (versionInfo.dwMinorVersion & 0xff);
    }

    return pModule->expected_version;
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

/**********************************************************************
 *	    GetModuleHandle    (KERNEL.47)
 *
 * Find a module from a module name.
 *
 * NOTE: The current implementation works the same way the Windows 95 one
 *	 does. Do not try to 'fix' it, fix the callers.
 *	 + It does not do ANY extension handling (except that strange .EXE bit)!
 *	 + It does not care about paths, just about basenames. (same as Windows)
 *
 * RETURNS
 *   LOWORD:
 *	the win16 module handle if found
 * 	0 if not
 *   HIWORD (undocumented, see "Undocumented Windows", chapter 5):
 *	Always hFirstModule
 */
DWORD WINAPI WIN16_GetModuleHandle( SEGPTR name )
{
    if (HIWORD(name) == 0)
        return MAKELONG(GetExePtr( (HINSTANCE16)name), hFirstModule );
    return MAKELONG(GetModuleHandle16( MapSL(name)), hFirstModule );
}

/**********************************************************************
 *	    NE_GetModuleByFilename
 */
static HMODULE16 NE_GetModuleByFilename( LPCSTR name )
{
    HMODULE16   hModule;
    LPSTR       s, p;
    BYTE        len, *name_table;
    char        tmpstr[MAX_PATH];
    NE_MODULE *pModule;

    lstrcpynA(tmpstr, name, sizeof(tmpstr));

    /* If the base filename of 'name' matches the base filename of the module
     * filename of some module (case-insensitive compare):
     * Return its handle.
     */

    /* basename: search backwards in passed name to \ / or : */
    s = tmpstr + strlen(tmpstr);
    while (s > tmpstr)
    {
        if (s[-1]=='/' || s[-1]=='\\' || s[-1]==':')
                break;
        s--;
    }

    /* search this in loaded filename list */
    for (hModule = hFirstModule; hModule ; hModule = pModule->next)
    {
        char            *loadedfn;
        OFSTRUCT        *ofs;

        pModule = NE_GetPtr( hModule );
        if (!pModule) break;
        if (!pModule->fileinfo) continue;
        if (pModule->flags & NE_FFLAGS_WIN32) continue;

        ofs = (OFSTRUCT*)((BYTE *)pModule + pModule->fileinfo);
        loadedfn = ((char*)ofs->szPathName) + strlen(ofs->szPathName);
        /* basename: search backwards in pathname to \ / or : */
        while (loadedfn > (char*)ofs->szPathName)
        {
            if (loadedfn[-1]=='/' || loadedfn[-1]=='\\' || loadedfn[-1]==':')
                    break;
            loadedfn--;
        }
        /* case insensitive compare ... */
        if (!FILE_strcasecmp(loadedfn, s))
            return hModule;
    }
    /* If basename (without ext) matches the module name of a module:
     * Return its handle.
     */

    if ( (p = strrchr( s, '.' )) != NULL ) *p = '\0';
    len = strlen(s);

    for (hModule = hFirstModule; hModule ; hModule = pModule->next)
    {
        pModule = NE_GetPtr( hModule );
        if (!pModule) break;
        if (pModule->flags & NE_FFLAGS_WIN32) continue;

        name_table = (BYTE *)pModule + pModule->name_table;
        if ((*name_table == len) && !FILE_strncasecmp(s, name_table+1, len))
            return hModule;
    }

    return 0;
}

/***********************************************************************
 *           GetProcAddress16   (KERNEL32.37)
 * Get procaddress in 16bit module from win32... (kernel32 undoc. ordinal func)
 */
FARPROC16 WINAPI WIN32_GetProcAddress16( HMODULE hModule, LPCSTR name )
{
    if (!hModule) return 0;
    if (HIWORD(hModule))
    {
        WARN("hModule is Win32 handle (%p)\n", hModule );
        return 0;
    }
    return GetProcAddress16( LOWORD(hModule), name );
}

/**********************************************************************
 *	    ModuleFirst    (TOOLHELP.59)
 */
BOOL16 WINAPI ModuleFirst16( MODULEENTRY *lpme )
{
    lpme->wNext = hFirstModule;
    return ModuleNext16( lpme );
}


/**********************************************************************
 *	    ModuleNext    (TOOLHELP.60)
 */
BOOL16 WINAPI ModuleNext16( MODULEENTRY *lpme )
{
    NE_MODULE *pModule;
    char *name;

    if (!lpme->wNext) return FALSE;
    if (!(pModule = NE_GetPtr( lpme->wNext ))) return FALSE;
    name = (char *)pModule + pModule->name_table;
    memcpy( lpme->szModule, name + 1, min(*name, MAX_MODULE_NAME) );
    lpme->szModule[min(*name, MAX_MODULE_NAME)] = '\0';
    lpme->hModule = lpme->wNext;
    lpme->wcUsage = pModule->count;
    lstrcpynA( lpme->szExePath, NE_MODULE_NAME(pModule), sizeof(lpme->szExePath) );
    lpme->wNext = pModule->next;
    return TRUE;
}


/**********************************************************************
 *	    ModuleFindName    (TOOLHELP.61)
 */
BOOL16 WINAPI ModuleFindName16( MODULEENTRY *lpme, LPCSTR name )
{
    lpme->wNext = GetModuleHandle16( name );
    return ModuleNext16( lpme );
}


/**********************************************************************
 *	    ModuleFindHandle    (TOOLHELP.62)
 */
BOOL16 WINAPI ModuleFindHandle16( MODULEENTRY *lpme, HMODULE16 hModule )
{
    hModule = GetExePtr( hModule );
    lpme->wNext = hModule;
    return ModuleNext16( lpme );
}


/***************************************************************************
 *          IsRomModule    (KERNEL.323)
 */
BOOL16 WINAPI IsRomModule16( HMODULE16 unused )
{
    return FALSE;
}

/***************************************************************************
 *          IsRomFile    (KERNEL.326)
 */
BOOL16 WINAPI IsRomFile16( HFILE16 unused )
{
    return FALSE;
}

/***********************************************************************
 *           create_dummy_module
 *
 * Create a dummy NE module for Win32 or Winelib.
 */
static HMODULE16 create_dummy_module( HMODULE module32 )
{
    HMODULE16 hModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    char *pStr,*s;
    unsigned int len;
    const char* basename;
    OFSTRUCT *ofs;
    int of_size, size;
    char filename[MAX_PATH];
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( module32 );

    if (!nt) return (HMODULE16)11;  /* invalid exe */

    /* Extract base filename */
    GetModuleFileNameA( module32, filename, sizeof(filename) );
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
    pModule->flags            = NE_FFLAGS_WIN32;
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
    pModule->expected_version = ((nt->OptionalHeader.MajorSubsystemVersion & 0xff) << 8 ) |
                                (nt->OptionalHeader.MinorSubsystemVersion & 0xff);
    if (nt->FileHeader.Characteristics & IMAGE_FILE_DLL)
        pModule->flags |= NE_FFLAGS_LIBMODULE | NE_FFLAGS_SINGLEDATA;

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
    pModule->res_table = pModule->import_table = pModule->entry_table = (int)pStr - (int)pModule;

    NE_RegisterModule( pModule );
    LoadLibraryA( filename );  /* increment the ref count of the 32-bit module */
    return hModule;
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

/***************************************************************************
 *		MapHModuleLS			(KERNEL32.@)
 */
HMODULE16 WINAPI MapHModuleLS(HMODULE hmod)
{
    HMODULE16 ret;
    NE_MODULE *pModule;

    if (!hmod)
        return TASK_GetCurrent()->hInstance;
    if (!HIWORD(hmod))
        return LOWORD(hmod); /* we already have a 16 bit module handle */
    pModule = (NE_MODULE*)GlobalLock16(hFirstModule);
    while (pModule)  {
        if (pModule->module32 == hmod)
            return pModule->self;
        pModule = (NE_MODULE*)GlobalLock16(pModule->next);
    }
    if ((ret = create_dummy_module( hmod )) < 32)
    {
        SetLastError(ret);
        ret = 0;
    }
    return ret;
}

/***************************************************************************
 *		MapHModuleSL			(KERNEL32.@)
 */
HMODULE WINAPI MapHModuleSL(HMODULE16 hmod)
{
    NE_MODULE *pModule;

    if (!hmod) {
        TDB *pTask = TASK_GetCurrent();
        hmod = pTask->hModule;
    }
    pModule = (NE_MODULE*)GlobalLock16(hmod);
    if ((pModule->magic!=IMAGE_OS2_SIGNATURE) || !(pModule->flags & NE_FFLAGS_WIN32))
        return 0;
    return pModule->module32;
}

/***************************************************************************
 *		MapHInstLS			(KERNEL32.@)
 *		MapHInstLS			(KERNEL.472)
 */
void WINAPI MapHInstLS( CONTEXT86 *context )
{
    context->Eax = MapHModuleLS( (HMODULE)context->Eax );
}

/***************************************************************************
 *		MapHInstSL			(KERNEL32.@)
 *		MapHInstSL			(KERNEL.473)
 */
void WINAPI MapHInstSL( CONTEXT86 *context )
{
    context->Eax = (DWORD)MapHModuleSL( context->Eax );
}

/***************************************************************************
 *		MapHInstLS_PN			(KERNEL32.@)
 */
void WINAPI MapHInstLS_PN( CONTEXT86 *context )
{
    if (context->Eax) context->Eax = MapHModuleLS( (HMODULE)context->Eax );
}

/***************************************************************************
 *		MapHInstSL_PN			(KERNEL32.@)
 */
void WINAPI MapHInstSL_PN( CONTEXT86 *context )
{
    if (context->Eax) context->Eax = (DWORD)MapHModuleSL( context->Eax );
}
