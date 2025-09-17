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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "wine/winbase16.h"
#include "wownt32.h"
#include "winternl.h"
#include "ddk/ntddk.h"
#include "kernel16_private.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#pragma pack(push,1)
typedef struct _GPHANDLERDEF
{
    WORD selector;
    WORD rangeStart;
    WORD rangeEnd;
    WORD handler;
} GPHANDLERDEF;
#pragma pack(pop)

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

static HINSTANCE16 NE_LoadModule( LPCSTR name, BOOL lib_only );
static BOOL16 NE_FreeModule( HMODULE16 hModule, BOOL call_wep );

static HINSTANCE16 MODULE_LoadModule16( LPCSTR libname, BOOL implicit, BOOL lib_only );

static HMODULE16 NE_GetModuleByFilename( LPCSTR name );


/* patch all the flat cs references of the code segment if necessary */
static inline void patch_code_segment( NE_MODULE *pModule )
{
    int i;
    CALLFROM16 *call;
    SEGTABLEENTRY *pSeg = NE_SEG_TABLE( pModule );

    for (i = 0; i < pModule->ne_cseg; i++, pSeg++)
        if (!(pSeg->flags & NE_SEGFLAGS_DATA)) break;  /* found the code segment */

    call = GlobalLock16( pSeg->hSeg );

    /* patch glue code address and code selector */
    for (i = 0; call[i].pushl == 0x68; i++)
    {
        if (call[i].ret[0] == 0xca66 || call[i].ret[0] == 0xcb66)  /* register entry point? */
            call[i].glue = __wine_call_from_16_regs;
        else
            call[i].glue = __wine_call_from_16;
        call[i].flatcs = get_cs();
    }

    if (TRACE_ON(relay))  /* patch relay functions to all point to relay_call_from_16 */
        for (i = 0; call[i].pushl == 0x68; i++) call[i].relay = relay_call_from_16;
}


/***********************************************************************
 *           contains_path
 */
static inline BOOL contains_path( LPCSTR name )
{
    return ((*name && (name[1] == ':')) || strchr(name, '/') || strchr(name, '\\'));
}


/***********************************************************************
 *           NE_GetPtr
 */
NE_MODULE *NE_GetPtr( HMODULE16 hModule )
{
    return GlobalLock16( GetExePtr(hModule) );
}


/**********************************************************************
 *           NE_RegisterModule
 */
static void NE_RegisterModule( NE_MODULE *pModule )
{
    pModule->next = hFirstModule;
    hFirstModule = pModule->self;
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
        ERR( "**** %04x is not a module handle\n", hModule );
        return;
    }

      /* Dump the module info */
    TRACE( "---\n" );
    TRACE( "Module %04x:\n", hModule );
    TRACE( "count=%d flags=%04x heap=%d stack=%d\n",
             pModule->count, pModule->ne_flags,
             pModule->ne_heap, pModule->ne_stack );
    TRACE( "cs:ip=%04x:%04x ss:sp=%04x:%04x ds=%04x nb seg=%d modrefs=%d\n",
             SELECTOROF(pModule->ne_csip), OFFSETOF(pModule->ne_csip),
             SELECTOROF(pModule->ne_sssp), OFFSETOF(pModule->ne_sssp),
             pModule->ne_autodata, pModule->ne_cseg, pModule->ne_cmod );
    TRACE( "os_flags=%d swap_area=%d version=%04x\n",
             pModule->ne_exetyp, pModule->ne_swaparea, pModule->ne_expver );
    if (pModule->ne_flags & NE_FFLAGS_WIN32)
        TRACE( "PE module=%p\n", pModule->module32 );

      /* Dump the file info */
    TRACE( "---\n" );
    TRACE( "Filename: '%s'\n", NE_MODULE_NAME(pModule) );

      /* Dump the segment table */
    TRACE( "---\n" );
    TRACE( "Segment table:\n" );
    pSeg = NE_SEG_TABLE( pModule );
    for (i = 0; i < pModule->ne_cseg; i++, pSeg++)
        TRACE( "%02x: pos=%d size=%d flags=%04x minsize=%d hSeg=%04x\n",
                 i + 1, pSeg->filepos, pSeg->size, pSeg->flags,
                 pSeg->minsize, pSeg->hSeg );

      /* Dump the resource table */
    TRACE( "---\n" );
    TRACE( "Resource table:\n" );
    if (pModule->ne_rsrctab)
    {
        pword = (WORD *)((BYTE *)pModule + pModule->ne_rsrctab);
        TRACE( "Alignment: %d\n", *pword++ );
        while (*pword)
        {
            NE_TYPEINFO *ptr = (NE_TYPEINFO *)pword;
            NE_NAMEINFO *pname = (NE_NAMEINFO *)(ptr + 1);
            TRACE( "id=%04x count=%d\n", ptr->type_id, ptr->count );
            for (i = 0; i < ptr->count; i++, pname++)
                TRACE( "offset=%d len=%d id=%04x\n",
		      pname->offset, pname->length, pname->id );
            pword = (WORD *)pname;
        }
    }
    else TRACE( "None\n" );

      /* Dump the resident name table */
    TRACE( "---\n" );
    TRACE( "Resident-name table:\n" );
    pstr = (BYTE*) pModule + pModule->ne_restab;
    while (*pstr)
    {
        TRACE( "%*.*s: %d\n", *pstr, *pstr, pstr + 1,
                 *(WORD *)(pstr + *pstr + 1) );
        pstr += *pstr + 1 + sizeof(WORD);
    }

      /* Dump the module reference table */
    TRACE( "---\n" );
    TRACE( "Module ref table:\n" );
    if (pModule->ne_modtab)
    {
        pword = (WORD *)((BYTE *)pModule + pModule->ne_modtab);
        for (i = 0; i < pModule->ne_cmod; i++, pword++)
        {
            char name[10];
            GetModuleName16( *pword, name, sizeof(name) );
            TRACE( "%d: %04x -> '%s'\n", i, *pword, name );
        }
    }
    else TRACE( "None\n" );

      /* Dump the entry table */
    TRACE( "---\n" );
    TRACE( "Entry table:\n" );
    bundle = (ET_BUNDLE *)((BYTE *)pModule+pModule->ne_enttab);
    do {
        entry = (ET_ENTRY *)((BYTE *)bundle+6);
        TRACE( "Bundle %d-%d: %02x\n", bundle->first, bundle->last, entry->type);
        ordinal = bundle->first;
        while (ordinal < bundle->last)
        {
            if (entry->type == 0xff)
                TRACE("%d: %02x:%04x (moveable)\n", ordinal++, entry->segnum, entry->offs);
            else
                TRACE("%d: %02x:%04x (fixed)\n", ordinal++, entry->segnum, entry->offs);
            entry++;
        }
    } while ( (bundle->next) && (bundle = ((ET_BUNDLE *)((BYTE *)pModule + bundle->next))) );

    /* Dump the non-resident names table */
    TRACE( "---\n" );
    TRACE( "Non-resident names table:\n" );
    if (pModule->nrname_handle)
    {
        pstr = GlobalLock16( pModule->nrname_handle );
        while (*pstr)
        {
            TRACE( "%*.*s: %d\n", *pstr, *pstr, pstr + 1,
                   *(WORD *)(pstr + *pstr + 1) );
            pstr += *pstr + 1 + sizeof(WORD);
        }
    }
    TRACE( "\n" );
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
        MESSAGE( " %04x  %04x  %.*s\n", hModule, pModule->ne_flags,
                 *((char *)pModule + pModule->ne_restab),
                 (char *)pModule + pModule->ne_restab + 1 );
        hModule = pModule->next;
    }
}


/***********************************************************************
 *           NE_InitResourceHandler
 *
 * Fill in 'resloader' fields in the resource table.
 */
static void NE_InitResourceHandler( HMODULE16 hModule )
{
    static FARPROC16 proc;

    NE_TYPEINFO *pTypeInfo;
    NE_MODULE *pModule;

    if (!(pModule = NE_GetPtr( hModule )) || !pModule->ne_rsrctab) return;

    TRACE("InitResourceHandler[%04x]\n", hModule );

    if (!proc) proc = GetProcAddress16( GetModuleHandle16("KERNEL"), "DefResourceHandler" );

    pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->ne_rsrctab + 2);
    while(pTypeInfo->type_id)
    {
        pTypeInfo->resloader = proc;
        pTypeInfo = (NE_TYPEINFO *)((char*)(pTypeInfo + 1) + pTypeInfo->count * sizeof(NE_NAMEINFO));
    }
}


/***********************************************************************
 *           NE_GetOrdinal
 *
 * Lookup the ordinal for a given name.
 */
WORD NE_GetOrdinal( HMODULE16 hModule, const char *name )
{
    char buffer[256], *p;
    BYTE *cpnt;
    BYTE len;
    NE_MODULE *pModule;

    if (!(pModule = NE_GetPtr( hModule ))) return 0;
    if (pModule->ne_flags & NE_FFLAGS_WIN32) return 0;

    TRACE("(%04x,'%s')\n", hModule, name );

      /* First handle names of the form '#xxxx' */

    if (name[0] == '#') return atoi( name + 1 );

      /* Now copy and uppercase the string */

    strcpy( buffer, name );
    for (p = buffer; *p; p++) *p = RtlUpperChar(*p);
    len = p - buffer;

      /* First search the resident names */

    cpnt = (BYTE *)pModule + pModule->ne_restab;

      /* Skip the first entry (module name) */
    cpnt += *cpnt + 1 + sizeof(WORD);
    while (*cpnt)
    {
        if ((*cpnt == len) && !memcmp( cpnt+1, buffer, len ))
        {
            WORD ordinal;
            memcpy( &ordinal, cpnt + *cpnt + 1, sizeof(ordinal) );
            TRACE("  Found: ordinal=%d\n", ordinal );
            return ordinal;
        }
        cpnt += *cpnt + 1 + sizeof(WORD);
    }

      /* Now search the non-resident names table */

    if (!pModule->nrname_handle) return 0;  /* No non-resident table */
    cpnt = GlobalLock16( pModule->nrname_handle );

      /* Skip the first entry (module description string) */
    cpnt += *cpnt + 1 + sizeof(WORD);
    while (*cpnt)
    {
        if ((*cpnt == len) && !memcmp( cpnt+1, buffer, len ))
        {
            WORD ordinal;
            memcpy( &ordinal, cpnt + *cpnt + 1, sizeof(ordinal) );
            TRACE("  Found: ordinal=%d\n", ordinal );
            return ordinal;
        }
        cpnt += *cpnt + 1 + sizeof(WORD);
    }
    return 0;
}


/***********************************************************************
 *		NE_GetEntryPoint
 */
FARPROC16 WINAPI NE_GetEntryPoint( HMODULE16 hModule, WORD ordinal )
{
    return NE_GetEntryPointEx( hModule, ordinal, TRUE );
}

/***********************************************************************
 *		NE_GetEntryPointEx
 */
FARPROC16 NE_GetEntryPointEx( HMODULE16 hModule, WORD ordinal, BOOL16 snoop )
{
    NE_MODULE *pModule;
    WORD sel, offset, i;

    ET_ENTRY *entry;
    ET_BUNDLE *bundle;

    if (!(pModule = NE_GetPtr( hModule ))) return 0;

    bundle = (ET_BUNDLE *)((BYTE *)pModule + pModule->ne_enttab);
    while ((ordinal < bundle->first + 1) || (ordinal > bundle->last))
    {
        if (!(bundle->next))
            return 0;
        bundle = (ET_BUNDLE *)((BYTE *)pModule + bundle->next);
    }

    entry = (ET_ENTRY *)((BYTE *)bundle+6);
    for (i=0; i < (ordinal - bundle->first - 1); i++)
        entry++;

    sel = entry->segnum;
    memcpy( &offset, &entry->offs, sizeof(WORD) );

    if (sel == 0xfe) sel = 0xffff;  /* constant entry */
    else sel = GlobalHandleToSel16(NE_SEG_TABLE(pModule)[sel-1].hSeg);
    if (sel==0xffff)
        return (FARPROC16)MAKESEGPTR( sel, offset );
    if (!snoop)
        return (FARPROC16)MAKESEGPTR( sel, offset );
    else
        return SNOOP16_GetProcAddress16(hModule,ordinal,(FARPROC16)MAKESEGPTR( sel, offset ));
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

    bundle = (ET_BUNDLE *)((BYTE *)pModule + pModule->ne_enttab);
    while ((ordinal < bundle->first + 1) || (ordinal > bundle->last))
    {
        bundle = (ET_BUNDLE *)((BYTE *)pModule + bundle->next);
        if (!bundle->next) return FALSE;
    }

    entry = (ET_ENTRY *)((BYTE *)bundle+6);
    for (i=0; i < (ordinal - bundle->first - 1); i++)
        entry++;

    memcpy( &entry->offs, &offset, sizeof(WORD) );
    return TRUE;
}


/***********************************************************************
 *           build_bundle_data
 *
 * Build the entry table bundle data from the on-disk format. Helper for build_module.
 */
static void *build_bundle_data( NE_MODULE *pModule, void *dest, const BYTE *table )
{
    ET_BUNDLE *oldbundle, *bundle = dest;
    ET_ENTRY *entry;
    BYTE nr_entries, type;

    memset(bundle, 0, sizeof(ET_BUNDLE)); /* in case no entry table exists */
    entry = (ET_ENTRY *)((BYTE *)bundle+6);

    while ((nr_entries = *table++))
    {
        if ((type = *table++))
        {
            bundle->last += nr_entries;
            if (type == 0xff)
            {
                while (nr_entries--)
                {
                    entry->type   = type;
                    entry->flags  = *table++;
                    table += sizeof(WORD);
                    entry->segnum = *table++;
                    entry->offs   = *(const WORD *)table;
                    table += sizeof(WORD);
                    entry++;
                }
            }
            else
            {
                while (nr_entries--)
                {
                    entry->type   = type;
                    entry->flags  = *table++;
                    entry->segnum = type;
                    entry->offs   = *(const WORD *)table;
                    table += sizeof(WORD);
                    entry++;
                }
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
                oldbundle->next = (char *)entry - (char *)pModule;
                bundle = (ET_BUNDLE *)entry;
                bundle->first = bundle->last = oldbundle->last + nr_entries;
                bundle->next = 0;
                entry = (ET_ENTRY*)(((BYTE*)entry)+sizeof(ET_BUNDLE));
            }
        }
    }
    return entry;
}


/***********************************************************************
 *           build_module
 *
 * Build the in-memory module from the on-disk data.
 */
static HMODULE16 build_module( const void *mapping, SIZE_T mapping_size, LPCSTR path )
{
    const IMAGE_DOS_HEADER *mz_header = mapping;
    const IMAGE_OS2_HEADER *ne_header;
    const struct ne_segment_table_entry_s *pSeg;
    const void *ptr;
    int i;
    size_t size;
    HMODULE16 hModule;
    NE_MODULE *pModule;
    BYTE *buffer, *pData, *end;
    OFSTRUCT *ofs;

    if (mapping_size < sizeof(*mz_header)) return ERROR_BAD_FORMAT;
    if (mz_header->e_magic != IMAGE_DOS_SIGNATURE) return ERROR_BAD_FORMAT;
    ne_header = (const IMAGE_OS2_HEADER *)((const char *)mapping + mz_header->e_lfanew);
    if (mz_header->e_lfanew + sizeof(*ne_header) > mapping_size) return ERROR_BAD_FORMAT;
    if (ne_header->ne_magic == IMAGE_NT_SIGNATURE) return 21;  /* win32 exe */
    if (ne_header->ne_magic == IMAGE_OS2_SIGNATURE_LX)
    {
        MESSAGE("Sorry, %s is an OS/2 linear executable (LX) file!\n", path);
        return 12;
    }
    if (ne_header->ne_magic != IMAGE_OS2_SIGNATURE) return ERROR_BAD_FORMAT;

    /* We now have a valid NE header */

    /* check to be able to fall back to loading OS/2 programs as DOS
     * FIXME: should this check be reversed in order to be less strict?
     * (only fail for OS/2 ne_exetyp 0x01 here?) */
    if ((ne_header->ne_exetyp != 0x02 /* Windows */)
        && (ne_header->ne_exetyp != 0x04) /* Windows 386 */)
        return ERROR_BAD_FORMAT;

    size = sizeof(NE_MODULE) +
             /* segment table */
           ne_header->ne_cseg * sizeof(SEGTABLEENTRY) +
             /* resource table */
           ne_header->ne_restab - ne_header->ne_rsrctab +
             /* resident names table */
           ne_header->ne_modtab - ne_header->ne_restab +
             /* module ref table */
           ne_header->ne_cmod * sizeof(WORD) +
             /* imported names table */
           ne_header->ne_enttab - ne_header->ne_imptab +
             /* entry table length */
           ne_header->ne_cbenttab +
             /* entry table extra conversion space */
           sizeof(ET_BUNDLE) +
           2 * (ne_header->ne_cbenttab - ne_header->ne_cmovent*6) +
             /* loaded file info */
           sizeof(OFSTRUCT) - sizeof(ofs->szPathName) + strlen(path) + 1;

    hModule = GlobalAlloc16( GMEM_FIXED | GMEM_ZEROINIT, size );
    if (!hModule)
    {
        ERR_(winediag)( "Failed to create module for %s, 16-bit LDT support may be missing.\n",
                        debugstr_a(path) );
        return ERROR_BAD_FORMAT;
    }

    FarSetOwner16( hModule, hModule );
    pModule = GlobalLock16( hModule );
    memcpy( pModule, ne_header, sizeof(*ne_header) );
    pModule->count = 0;
    /* check programs for default minimal stack size */
    if (!(pModule->ne_flags & NE_FFLAGS_LIBMODULE) && (pModule->ne_stack < 0x1400))
        pModule->ne_stack = 0x1400;

    pModule->self         = hModule;
    pModule->mapping      = mapping;
    pModule->mapping_size = mapping_size;

    pData = (BYTE *)(pModule + 1);

    /* Clear internal Wine flags in case they are set in the EXE file */

    pModule->ne_flags &= ~(NE_FFLAGS_BUILTIN | NE_FFLAGS_WIN32);

    /* Get the segment table */

    pModule->ne_segtab = pData - (BYTE *)pModule;
    if (!(pSeg = NE_GET_DATA( pModule, mz_header->e_lfanew + ne_header->ne_segtab,
                              ne_header->ne_cseg * sizeof(struct ne_segment_table_entry_s) )))
        goto failed;
    for (i = ne_header->ne_cseg; i > 0; i--, pSeg++)
    {
        memcpy( pData, pSeg, sizeof(*pSeg) );
        pData += sizeof(SEGTABLEENTRY);
    }

    /* Get the resource table */

    if (ne_header->ne_rsrctab < ne_header->ne_restab)
    {
        pModule->ne_rsrctab = pData - (BYTE *)pModule;
        if (!NE_READ_DATA( pModule, pData, mz_header->e_lfanew + ne_header->ne_rsrctab,
                           ne_header->ne_restab - ne_header->ne_rsrctab )) goto failed;
        pData += ne_header->ne_restab - ne_header->ne_rsrctab;
    }
    else pModule->ne_rsrctab = 0;  /* No resource table */

    /* Get the resident names table */

    pModule->ne_restab = pData - (BYTE *)pModule;
    if (!NE_READ_DATA( pModule, pData, mz_header->e_lfanew + ne_header->ne_restab,
                       ne_header->ne_modtab - ne_header->ne_restab )) goto failed;
    pData += ne_header->ne_modtab - ne_header->ne_restab;

    /* Get the module references table */

    if (ne_header->ne_cmod > 0)
    {
        pModule->ne_modtab = pData - (BYTE *)pModule;
        if (!NE_READ_DATA( pModule, pData, mz_header->e_lfanew + ne_header->ne_modtab,
                           ne_header->ne_cmod * sizeof(WORD) )) goto failed;
        pData += ne_header->ne_cmod * sizeof(WORD);
    }
    else pModule->ne_modtab = 0;  /* No module references */

    /* Get the imported names table */

    pModule->ne_imptab = pData - (BYTE *)pModule;
    if (!NE_READ_DATA( pModule, pData, mz_header->e_lfanew + ne_header->ne_imptab,
                       ne_header->ne_enttab - ne_header->ne_imptab )) goto failed;
    pData += ne_header->ne_enttab - ne_header->ne_imptab;

    /* Load entry table, convert it to the optimized version used by Windows */

    pModule->ne_enttab = pData - (BYTE *)pModule;
    if (!(ptr = NE_GET_DATA( pModule, mz_header->e_lfanew + ne_header->ne_enttab,
                             ne_header->ne_cbenttab ))) goto failed;
    end = build_bundle_data( pModule, pData, ptr );

    pData += ne_header->ne_cbenttab + sizeof(ET_BUNDLE) +
        2 * (ne_header->ne_cbenttab - ne_header->ne_cmovent*6);

    if (end > pData)
    {
        FIXME( "not enough space for entry table for %s\n", debugstr_a(path) );
        goto failed;
    }

    /* Store the filename information */

    pModule->fileinfo = pData - (BYTE *)pModule;
    ofs = (OFSTRUCT *)pData;
    ofs->cBytes = sizeof(OFSTRUCT) - sizeof(ofs->szPathName) + strlen(path);
    ofs->fFixedDisk = 1;
    strcpy( ofs->szPathName, path );
    pData += ofs->cBytes + 1;

    /* Get the non-resident names table */

    if (ne_header->ne_cbnrestab)
    {
        pModule->nrname_handle = GlobalAlloc16( 0, ne_header->ne_cbnrestab );
        if (!pModule->nrname_handle) goto failed;
        FarSetOwner16( pModule->nrname_handle, hModule );
        buffer = GlobalLock16( pModule->nrname_handle );
        if (!NE_READ_DATA( pModule, buffer, ne_header->ne_nrestab, ne_header->ne_cbnrestab ))
        {
            GlobalFree16( pModule->nrname_handle );
            pModule->nrname_handle = 0;
        }
    }
    else pModule->nrname_handle = 0;

    /* Allocate a segment for the implicitly-loaded DLLs */

    if (pModule->ne_cmod)
    {
        pModule->dlls_to_init = GlobalAlloc16( GMEM_ZEROINIT,
                                               (pModule->ne_cmod+1)*sizeof(HMODULE16) );
        if (!pModule->dlls_to_init)
        {
            if (pModule->nrname_handle) GlobalFree16( pModule->nrname_handle );
            goto failed;
        }
        FarSetOwner16( pModule->dlls_to_init, hModule );
    }
    else pModule->dlls_to_init = 0;

    NE_RegisterModule( pModule );
    return hModule;

failed:
    GlobalFree16( hModule );
    return ERROR_BAD_FORMAT;
}


/***********************************************************************
 *           NE_LoadDLLs
 *
 * Load all DLLs implicitly linked to a module.
 */
static BOOL NE_LoadDLLs( NE_MODULE *pModule )
{
    int i;
    WORD *pModRef = (WORD *)((char *)pModule + pModule->ne_modtab);
    WORD *pDLLs = GlobalLock16( pModule->dlls_to_init );

    for (i = 0; i < pModule->ne_cmod; i++, pModRef++)
    {
        char buffer[260], *p;
        BYTE *pstr = (BYTE *)pModule + pModule->ne_imptab + *pModRef;
        memcpy( buffer, pstr + 1, *pstr );
        *(buffer + *pstr) = 0; /* terminate it */

        TRACE("Loading '%s'\n", buffer );
        if (!(*pModRef = GetModuleHandle16( buffer )))
        {
            /* If the DLL is not loaded yet, load it and store */
            /* its handle in the list of DLLs to initialize.   */
            HMODULE16 hDLL;

            /* Append .DLL (Windows >= 3.00) or .EXE (Windows < 3.00) to name if no extension present */
            if (!(p = strrchr( buffer, '.')) || strchr( p, '/' ) || strchr( p, '\\'))
                    strcat( buffer, (GetExeVersion16() >= 0x0300) ? ".DLL" : ".EXE" );

            if ((hDLL = MODULE_LoadModule16( buffer, TRUE, TRUE )) < 32)
            {
                /* FIXME: cleanup what was done */

                MESSAGE( "Could not load '%s' required by '%.*s', error=%d\n",
                     buffer, *((BYTE*)pModule + pModule->ne_restab),
                     (char *)pModule + pModule->ne_restab + 1, hDLL );
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
 * pModule must point to a module structure prepared by build_module.
 * This routine must never be called twice on a module.
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
    HANDLE mapping;
    void *ptr;
    MEMORY_BASIC_INFORMATION info;

    /* Open file */
    if ((hFile = OpenFile16( name, &ofs, OF_READ|OF_SHARE_DENY_WRITE )) == HFILE_ERROR16)
        return ERROR_FILE_NOT_FOUND;

    mapping = CreateFileMappingW( DosFileHandleToWin32Handle(hFile), NULL, PAGE_WRITECOPY, 0, 0, NULL );
    _lclose16( hFile );
    if (!mapping) return ERROR_BAD_FORMAT;

    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 0 );
    CloseHandle( mapping );
    if (!ptr) return ERROR_BAD_FORMAT;

    VirtualQuery( ptr, &info, sizeof(info) );
    hModule = build_module( ptr, info.RegionSize, ofs.szPathName );

    if (hModule < 32)
    {
        UnmapViewOfFile( ptr );
        return hModule;
    }

    SNOOP16_RegisterDLL( hModule, ofs.szPathName );
    NE_InitResourceHandler( hModule );

    pModule = NE_GetPtr( hModule );

    if ( !lib_only && !( pModule->ne_flags & NE_FFLAGS_LIBMODULE ) )
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
static HMODULE16 NE_DoLoadBuiltinModule( const IMAGE_DOS_HEADER *mz_header, const char *file_name,
                                         HMODULE owner32 )
{
    NE_MODULE *pModule;
    HMODULE16 hModule;
    HINSTANCE16 hInstance;
    SIZE_T mapping_size = ~0UL;  /* assume builtins don't contain invalid offsets... */

    hModule = build_module( mz_header, mapping_size, file_name );
    if (hModule < 32) return hModule;
    pModule = GlobalLock16( hModule );
    pModule->ne_flags |= NE_FFLAGS_BUILTIN;
    pModule->owner32 = owner32;

    /* fake the expected version the module should have according to the current Windows version */
    pModule->ne_expver = MAKEWORD( NtCurrentTeb()->Peb->OSMajorVersion,
                                   NtCurrentTeb()->Peb->OSMinorVersion );

    hInstance = NE_DoLoadModule( pModule );
    if (hInstance < 32) NE_FreeModule( hModule, 0 );

    NE_InitResourceHandler( hModule );

    if (pModule->ne_heap)
    {
        SEGTABLEENTRY *pSeg = NE_SEG_TABLE( pModule ) + pModule->ne_autodata - 1;
        unsigned int size = pSeg->minsize + pModule->ne_heap;
        if (size > 0xfffe) size = 0xfffe;
        LocalInit16( GlobalHandleToSel16(pSeg->hSeg), pSeg->minsize, size );
    }

    patch_code_segment( pModule );
    *(ULONG *)mz_header->e_res2 = NtCurrentTeb()->Peb->SpareUlongs[0];

    return hInstance;
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
    HMODULE16 hModule;
    HMODULE mod32 = 0;
    NE_MODULE *pModule;
    const IMAGE_DOS_HEADER *descr = NULL;
    const char *file_name = NULL;
    char dllname[32];
    const char *basename, *main_module, *p;

    /* strip path information */

    basename = libname;
    if (basename[0] && basename[1] == ':') basename += 2;  /* strip drive specification */
    if ((p = strrchr( basename, '\\' ))) basename = p + 1;
    if ((p = strrchr( basename, '/' ))) basename = p + 1;

    if (strlen(basename) < sizeof(dllname)-6)
    {
        DWORD count;
        char *q;

        ReleaseThunkLock( &count );

        strcpy( dllname, basename );
        q = strrchr( dllname, '.' );
        if (!q) strcat( dllname, (GetExeVersion16() >= 0x0300) ? ".dll" : ".exe" );
        for (q = dllname; *q; q++) if (*q >= 'A' && *q <= 'Z') *q += 32;

        strcpy( q, "16" );
        if ((mod32 = LoadLibraryA( dllname )))
        {
            if (!(descr = (void *)GetProcAddress( mod32, "__wine_spec_dos_header" )))
            {
                WARN( "loaded %s but does not contain a 16-bit module\n", debugstr_a(dllname) );
                FreeLibrary( mod32 );
            }
            else
            {
                TRACE( "found %s with embedded 16-bit module\n", debugstr_a(dllname) );
                file_name = basename;

                /* if module has a 32-bit owner, match the load order of the owner */
                if ((main_module = (void *)GetProcAddress( mod32, "__wine_spec_main_module" )))
                {
                    LDR_DATA_TABLE_ENTRY *ldr;
                    HMODULE main_owner = LoadLibraryA( main_module );

                    if (!main_owner)
                    {
                        WARN( "couldn't load owner %s for 16-bit dll %s\n", main_module, dllname );
                        FreeLibrary( mod32 );
                        RestoreThunkLock( count );
                        return ERROR_FILE_NOT_FOUND;
                    }
                    /* check if module was loaded native */
                    if (LdrFindEntryForAddress( main_owner, &ldr ) || !(ldr->Flags & LDR_WINE_INTERNAL))
                    {
                        FreeLibrary( mod32 );
                        descr = NULL;
                    }
                    FreeLibrary( main_owner );
                }
            }
        }
        RestoreThunkLock( count );

        /* loading the 32-bit library can have the side effect of loading the module */
        /* if so, simply incr the ref count and return the module */
        if (descr && (hModule = GetModuleHandle16( libname )))
        {
            TRACE( "module %s already loaded by owner\n", libname );
            pModule = NE_GetPtr( hModule );
            if (pModule) pModule->count++;
            FreeLibrary( mod32 );
            return hModule;
        }
    }

    if (descr)
    {
        TRACE("Trying built-in '%s'\n", libname);
        hinst = NE_DoLoadBuiltinModule( descr, file_name, mod32 );
        if (hinst > 32) TRACE_(loaddll)("Loaded module %s : builtin\n", debugstr_a(file_name));
        else FreeLibrary( mod32 );
    }
    else
    {
        TRACE("Trying native dll '%s'\n", libname);
        hinst = NE_LoadModule(libname, lib_only);
        if (hinst > 32) TRACE_(loaddll)("Loaded module %s : native\n", debugstr_a(libname));
    }

    if (hinst > 32 && !implicit)
    {
        hModule = GetModuleHandle16(libname);
        if(!hModule)
        {
            ERR("Serious trouble. Just loaded module '%s' (hinst=0x%04x), but can't get module handle. Filename too long ?\n",
                libname, hinst);
            return ERROR_INVALID_HANDLE;
        }

        pModule = NE_GetPtr(hModule);
        if(!pModule)
        {
            ERR("Serious trouble. Just loaded module '%s' (hinst=0x%04x), but can't get NE_MODULE pointer\n",
                libname, hinst);
            return ERROR_INVALID_HANDLE;
        }

        TRACE("Loaded module '%s' at 0x%04x.\n", libname, hinst);

        /*
         * Call initialization routines for all loaded DLLs. Note that
         * when we load implicitly linked DLLs this will be done by InitTask().
         */
        if(pModule->ne_flags & NE_FFLAGS_LIBMODULE)
        {
            NE_InitializeDLLs(hModule);
            NE_DllProcessAttach(hModule);
        }
        else DOSMEM_InitDosMemory();  /* we will be running a 16-bit task, setup DOS memory */
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
    WORD cmdShow = 1; /* SW_SHOWNORMAL but we don't want to include winuser.h here */

    if (name == NULL) return 0;

    TRACE("name %s, paramBlock %p\n", name, paramBlock);

    /* Load module */

    if ( (hModule = NE_GetModuleByFilename(name) ) != 0 )
    {
        /* Special case: second instance of an already loaded NE module */

        if ( !( pModule = NE_GetPtr( hModule ) ) ) return ERROR_BAD_FORMAT;
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
            return ERROR_BAD_FORMAT;
    }

    /* If library module, we just retrieve the instance handle */

    if ( ( pModule->ne_flags & NE_FFLAGS_LIBMODULE ) || lib_only )
        return NE_GetInstance( pModule );

    /*
     *  At this point, we need to create a new process.
     *
     *  pModule points either to an already loaded module, whose refcount
     *  has already been incremented (to avoid having the module vanish
     *  in the meantime), or else to a stub module which contains only header
     *  information.
     */
    params = paramBlock;
    if (params->showCmd)
        cmdShow = ((WORD *)MapSL( params->showCmd ))[1];
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

        if ( pModule->ne_autodata )
            if ( NE_CreateSegment( pModule, pModule->ne_autodata ) )
                NE_LoadSegment( pModule, pModule->ne_autodata );

        hInstance = NE_GetInstance( pModule );
        TRACE("created second instance %04x[%d] of instance %04x.\n", hInstance, pModule->ne_autodata, hPrevInstance);

    }
    else
    {
        /* Load first instance of NE module */

        pModule->ne_flags |= NE_FFLAGS_GUI;  /* FIXME: is this necessary? */

        hInstance = NE_DoLoadModule( pModule );
        hPrevInstance = 0;
    }

    if ( hInstance >= 32 )
    {
        CONTEXT context;

        /* Enter instance handles into task struct */

        pTask->hInstance = hInstance;
        pTask->hPrevInstance = hPrevInstance;

        /* Use DGROUP for 16-bit stack */

        if (!(sp = OFFSETOF(pModule->ne_sssp)))
            sp = pSegTable[SELECTOROF(pModule->ne_sssp)-1].minsize + pModule->ne_stack;
        sp &= ~1;
        CURRENT_SS = GlobalHandleToSel16(hInstance);
        CURRENT_SP = sp - sizeof(STACK16FRAME);

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
        context.SegCs  = GlobalHandleToSel16(pSegTable[SELECTOROF(pModule->ne_csip) - 1].hSeg);
        context.SegDs  = GlobalHandleToSel16(pTask->hInstance);
        context.SegEs  = pTask->hPDB;
        context.Eip    = OFFSETOF(pModule->ne_csip);
        context.Ebx    = pModule->ne_stack;
        context.Ecx    = pModule->ne_heap;
        context.Edi    = pTask->hInstance;
        context.Esi    = pTask->hPrevInstance;

        /* Now call 16-bit entry point */

        TRACE("Starting main program: cs:ip=%04lx:%04lx ds=%04lx ss:sp=%04x:%04x\n",
              context.SegCs, context.Eip, context.SegDs, CURRENT_SS, CURRENT_SP);

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
    __EXCEPT_PAGE_FAULT
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

    if (call_wep && !(pModule->ne_flags & NE_FFLAGS_WIN32))
    {
        /* Free the objects owned by the DLL module */
        NE_CallUserSignalProc( hModule, USIG16_DLL_UNLOAD );

        if (pModule->ne_flags & NE_FFLAGS_LIBMODULE)
            MODULE_CallWEP( hModule );
        else
            call_wep = FALSE;  /* We are freeing a task -> no more WEPs */
    }

    TRACE_(loaddll)("Unloaded module %s : %s\n", debugstr_a(NE_MODULE_NAME(pModule)),
                    (pModule->ne_flags & NE_FFLAGS_BUILTIN) ? "builtin" : "native");

    /* Clear magic number just in case */

    pModule->ne_magic = pModule->self = 0;
    if (pModule->owner32) FreeLibrary( pModule->owner32 );
    else if (pModule->mapping) UnmapViewOfFile( pModule->mapping );

      /* Remove it from the linked list */

    hPrevModule = &hFirstModule;
    while (*hPrevModule && (*hPrevModule != hModule))
    {
        hPrevModule = &(NE_GetPtr( *hPrevModule ))->next;
    }
    if (*hPrevModule) *hPrevModule = pModule->next;

    /* Free the referenced modules */

    pModRef = (HMODULE16*)((char *)pModule + pModule->ne_modtab);
    for (i = 0; i < pModule->ne_cmod; i++, pModRef++)
    {
        NE_FreeModule( *pModRef, call_wep );
    }

    /* Free the module storage */

    GlobalFreeAll16( hModule );
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


/***********************************************************************
 *          GetModuleHandle16 (KERNEL32.@)
 */
HMODULE16 WINAPI GetModuleHandle16( LPCSTR name )
{
    HMODULE16	hModule;
    LPSTR	s;
    BYTE	len, *name_table;
    char	tmpstr[MAX_PATH];
    NE_MODULE *pModule;

    TRACE("(%s)\n", name);

    if (!HIWORD(name)) return GetExePtr(LOWORD(name));

    len = strlen(name);
    if (!len) return 0;

    lstrcpynA(tmpstr, name, sizeof(tmpstr));

    /* If 'name' matches exactly the module name of a module:
     * Return its handle.
     */
    for (hModule = hFirstModule; hModule ; hModule = pModule->next)
    {
	pModule = NE_GetPtr( hModule );
        if (!pModule) break;
        if (pModule->ne_flags & NE_FFLAGS_WIN32) continue;

        name_table = (BYTE *)pModule + pModule->ne_restab;
        if ((*name_table == len) && !strncmp(name, (char*) name_table+1, len))
            return hModule;
    }

    /* If uppercased 'name' matches exactly the module name of a module:
     * Return its handle
     */
    for (s = tmpstr; *s; s++) *s = RtlUpperChar(*s);

    for (hModule = hFirstModule; hModule ; hModule = pModule->next)
    {
	pModule = NE_GetPtr( hModule );
        if (!pModule) break;
        if (pModule->ne_flags & NE_FFLAGS_WIN32) continue;

        name_table = (BYTE *)pModule + pModule->ne_restab;
	/* FIXME: the _strnicmp is WRONG. It should not be case insensitive,
	 * but case sensitive! (Unfortunately Winword 6 and subdlls have
	 * lowercased module names, but try to load uppercase DLLs, so this
	 * 'i' compare is just a quickfix until the loader handles that
	 * correctly. -MM 990705
	 */
        if ((*name_table == len) && !_strnicmp(tmpstr, (const char*)name_table+1, len))
            return hModule;
    }

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
        char		*loadedfn;
	OFSTRUCT	*ofs;

	pModule = NE_GetPtr( hModule );
        if (!pModule) break;
	if (!pModule->fileinfo) continue;
        if (pModule->ne_flags & NE_FFLAGS_WIN32) continue;

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
	if (!stricmp(loadedfn, s))
	    return hModule;
    }
    return 0;
}


/**********************************************************************
 *	    GetModuleName    (KERNEL.27)
 */
BOOL16 WINAPI GetModuleName16( HINSTANCE16 hinst, LPSTR buf, INT16 count )
{
    NE_MODULE *pModule;
    BYTE *p;

    if (!(pModule = NE_GetPtr( hinst ))) return FALSE;
    p = (BYTE *)pModule + pModule->ne_restab;
    if (count > *p) count = *p + 1;
    if (count > 0)
    {
        memcpy( buf, p + 1, count - 1 );
        buf[count-1] = '\0';
    }
    return TRUE;
}


/**********************************************************************
 *	    GetModuleFileName      (KERNEL.49)
 *
 * See also: GetModuleFileNameA
 *
 * This function returns short paths when the modules version field is < 4.0).
 *
 * Even if invoked by second instance of a program,
 * it still returns path of first one.
 */
INT16 WINAPI GetModuleFileName16( HINSTANCE16 hModule, LPSTR lpFileName,
                                  INT16 nSize )
{
    NE_MODULE *pModule;

    /* Win95 does not query hModule if set to 0 !
     * Is this wrong or maybe Win3.1 only ? */
    if (!hModule) hModule = GetCurrentTask();

    if (!(pModule = NE_GetPtr( hModule ))) return 0;
    lstrcpynA( lpFileName, NE_MODULE_NAME(pModule), nSize );
    if (pModule->ne_expver < 0x400)
        GetShortPathNameA(NE_MODULE_NAME(pModule), lpFileName, nSize);
    TRACE("%04x -> '%s'\n", hModule, lpFileName );
    return strlen(lpFileName);
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
    return pModule->ne_expver;
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
    LOADPARAMS16 params;
    WORD showCmd[2];

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

    showCmd[0] = 2;
    showCmd[1] = nCmdShow;

    params.hEnvironment = 0;
    params.cmdLine = MapLS( cmdline );
    params.showCmd = MapLS( showCmd );
    params.reserved = 0;

    if (SearchPathA( NULL, name, ".exe", sizeof(buffer), buffer, NULL ))
    {
        ret = LoadModule16( buffer, &params );
    }
    else if (!contains_path( name ))  /* try 16-bit builtin */
    {
        lstrcpynA( buffer, name, sizeof(buffer) );
        if (strlen( buffer ) < sizeof(buffer) - 4 && !strchr( buffer, '.' )) strcat( buffer, ".exe" );
        ret = LoadModule16( buffer, &params );
        if (ret == ERROR_FILE_NOT_FOUND) ret = 21;  /* it might be a 32-bit builtin too */
    }
    else ret = ERROR_FILE_NOT_FOUND;

    UnMapLS( params.cmdLine );
    UnMapLS( params.showCmd );

    HeapFree( GetProcessHeap(), 0, cmdline );
    if (name != lpCmdLine) HeapFree( GetProcessHeap(), 0, name );

    if (ret == 21 || ret == ERROR_BAD_FORMAT)  /* 32-bit module or unknown executable*/
    {
        LOADPARAMS16 params;
        WORD showCmd[2];
        showCmd[0] = 2;
        showCmd[1] = nCmdShow;

        arglen = strlen( lpCmdLine );
        cmdline = HeapAlloc( GetProcessHeap(), 0, arglen + 1 );
        cmdline[0] = (BYTE)arglen;
        memcpy( cmdline + 1, lpCmdLine, arglen );

        params.hEnvironment = 0;
        params.cmdLine = MapLS( cmdline );
        params.showCmd = MapLS( showCmd );
        params.reserved = 0;

        ret = LoadModule16( "winoldap.mod", &params );
        UnMapLS( params.cmdLine );
        UnMapLS( params.showCmd );
    }
    return ret;
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
    if (!ordinal) return NULL;

    ret = NE_GetEntryPoint( hModule, ordinal );

    TRACE("returning %p\n", ret );
    return ret;
}


/***************************************************************************
 *              HasGPHandler                    (KERNEL.338)
 */
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
        if (pModule->ne_flags & NE_FFLAGS_WIN32) continue;

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
        if (!stricmp(loadedfn, s))
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
        if (pModule->ne_flags & NE_FFLAGS_WIN32) continue;

        name_table = (BYTE *)pModule + pModule->ne_restab;
        if ((*name_table == len) && !_strnicmp(s, (const char*)name_table+1, len))
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
    char *pStr;
    unsigned int len;
    const char *basename, *s;
    OFSTRUCT *ofs;
    int of_size, size;
    char filename[MAX_PATH];
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( module32 );

    if (!nt) return ERROR_BAD_FORMAT;

    /* Extract base filename */
    len = GetModuleFileNameA( module32, filename, sizeof(filename) );
    if (!len || len >= sizeof(filename)) return ERROR_BAD_FORMAT;
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
    if (!hModule) return ERROR_BAD_FORMAT;

    FarSetOwner16( hModule, hModule );
    pModule = GlobalLock16( hModule );

    /* Set all used entries */
    pModule->ne_magic         = IMAGE_OS2_SIGNATURE;
    pModule->count            = 1;
    pModule->next             = 0;
    pModule->ne_flags         = NE_FFLAGS_WIN32;
    pModule->ne_autodata      = 0;
    pModule->ne_sssp          = MAKESEGPTR( 0, 1 );
    pModule->ne_csip          = MAKESEGPTR( 0, 2 );
    pModule->ne_heap          = 0;
    pModule->ne_stack         = 0;
    pModule->ne_cseg          = 2;
    pModule->ne_cmod          = 0;
    pModule->ne_cbnrestab     = 0;
    pModule->fileinfo         = sizeof(NE_MODULE);
    pModule->ne_exetyp        = NE_OSFLAGS_WINDOWS;
    pModule->self             = hModule;
    pModule->module32         = module32;

    /* Set version and flags */
    pModule->ne_expver = ((nt->OptionalHeader.MajorSubsystemVersion & 0xff) << 8 ) |
                          (nt->OptionalHeader.MinorSubsystemVersion & 0xff);
    if (nt->FileHeader.Characteristics & IMAGE_FILE_DLL)
        pModule->ne_flags |= NE_FFLAGS_LIBMODULE | NE_FFLAGS_SINGLEDATA;

    /* Set loaded file information */
    ofs = (OFSTRUCT *)(pModule + 1);
    memset( ofs, 0, of_size );
    ofs->cBytes = of_size < 256 ? of_size : 255;   /* FIXME */
    strcpy( ofs->szPathName, filename );

    pSegment = (SEGTABLEENTRY*)((char*)(pModule + 1) + ((of_size + 3) & ~3));
    pModule->ne_segtab = (char *)pSegment - (char *)pModule;
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
    pModule->ne_restab = pStr - (char *)pModule;
    *pStr = len;
    lstrcpynA( pStr+1, basename, len+1 );
    pStr += len+2;

    /* All tables zero terminated */
    pModule->ne_rsrctab = pModule->ne_imptab = pModule->ne_enttab = pStr - (char *)pModule;

    NE_RegisterModule( pModule );
    pModule->owner32 = LoadLibraryA( filename );  /* increment the ref count of the 32-bit module */
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
    pModule = GlobalLock16(hFirstModule);
    while (pModule)  {
        if (pModule->module32 == hmod)
            return pModule->self;
        pModule = GlobalLock16(pModule->next);
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
    pModule = GlobalLock16(hmod);
    if ((pModule->ne_magic != IMAGE_OS2_SIGNATURE) || !(pModule->ne_flags & NE_FFLAGS_WIN32))
        return 0;
    return pModule->module32;
}

/***************************************************************************
 *		MapHInstLS			(KERNEL.472)
 */
void WINAPI MapHInstLS16( CONTEXT *context )
{
    context->Eax = MapHModuleLS( (HMODULE)context->Eax );
}

/***************************************************************************
 *		MapHInstSL			(KERNEL.473)
 */
void WINAPI MapHInstSL16( CONTEXT *context )
{
    context->Eax = (DWORD)MapHModuleSL( context->Eax );
}

/***************************************************************************
 *		MapHInstLS			(KERNEL32.@)
 */
__ASM_STDCALL_FUNC( MapHInstLS, 0,
                   "pushl %eax\n\t"
                   "call " __ASM_STDCALL("MapHModuleLS",4) "\n\t"
                   "ret" )

/***************************************************************************
 *		MapHInstSL			(KERNEL32.@)
 */
__ASM_STDCALL_FUNC( MapHInstSL, 0,
                   "pushl %eax\n\t"
                   "call " __ASM_STDCALL("MapHModuleSL",4) "\n\t"
                   "ret" )

/***************************************************************************
 *		MapHInstLS_PN			(KERNEL32.@)
 */
__ASM_STDCALL_FUNC( MapHInstLS_PN, 0,
                   "testl %eax,%eax\n\t"
                   "jz 1f\n\t"
                   "pushl %eax\n\t"
                   "call " __ASM_STDCALL("MapHModuleLS",4) "\n"
                   "1:\tret" )

/***************************************************************************
 *		MapHInstSL_PN			(KERNEL32.@)
 */
__ASM_STDCALL_FUNC( MapHInstSL_PN, 0,
                   "andl $0xffff,%eax\n\t"
                   "jz 1f\n\t"
                   "pushl %eax\n\t"
                   "call " __ASM_STDCALL("MapHModuleSL",4) "\n"
                   "1:\tret" )
