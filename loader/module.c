/*
 * Modules
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "windows.h"
#include "dlls.h"
#include "dos_fs.h"
#include "file.h"
#include "global.h"
#include "ldt.h"
#include "module.h"
#include "neexe.h"
#include "stackframe.h"
#include "task.h"
#include "toolhelp.h"
#include "stddebug.h"
#include "debug.h"

#include "callback.h"
#include "wine.h"

static HMODULE hFirstModule = 0;
static HMODULE hCachedModule = 0;  /* Module cached by MODULE_OpenFile */

#ifndef WINELIB
static HANDLE hInitialStack32 = 0;
#endif
/***********************************************************************
 *           MODULE_LoadBuiltin
 *
 * Load a built-in module. If the 'force' parameter is FALSE, we only
 * load the module if it has not been disabled via the -dll option.
 */
#ifndef WINELIB
static HMODULE MODULE_LoadBuiltin( LPCSTR name, BOOL force )
{
    HMODULE hModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    struct dll_table_s *table;
    int i;
    char dllname[16], *p;

    /* Fix the name in case we have a full path and extension */

    if ((p = strrchr( name, '\\' ))) name = p + 1;
    strncpy( dllname, name, 15 );
    dllname[15] = '\0';
    if ((p = strrchr( dllname, '.' ))) *p = '\0';

    for (i = 0, table = dll_builtin_table; i < N_BUILTINS; i++, table++)
        if (!lstrcmpi( table->name, dllname )) break;
    if (i >= N_BUILTINS) return 0;
    if (!table->used && !force) return 0;

    hModule = GLOBAL_CreateBlock( GMEM_MOVEABLE, table->module_start,
                                  table->module_end - table->module_start,
                                  0, FALSE, FALSE, FALSE, NULL );
    if (!hModule) return 0;
    FarSetOwner( hModule, hModule );

    table->hModule = hModule;

    dprintf_module( stddeb, "Built-in %s: hmodule=%04x\n",
                    table->name, hModule );

    /* Allocate the code segment */

    pModule = (NE_MODULE *)GlobalLock( hModule );
    pSegTable = NE_SEG_TABLE( pModule );

    pSegTable->selector = GLOBAL_CreateBlock( GMEM_FIXED, table->code_start,
                                              pSegTable->minsize, hModule,
                                              TRUE, TRUE, FALSE, NULL );
    if (!pSegTable->selector) return 0;
    pSegTable++;

    /* Allocate the data segment */

    pSegTable->selector = GLOBAL_Alloc( GMEM_FIXED, pSegTable->minsize,
                                        hModule, FALSE, FALSE, FALSE );
    if (!pSegTable->selector) return 0;
    memcpy( GlobalLock( pSegTable->selector ),
            table->data_start, pSegTable->minsize );

    pModule->next = hFirstModule;
    hFirstModule = hModule;
    return hModule;
}
#endif

/***********************************************************************
 *           MODULE_Init
 *
 * Create the built-in modules.
 */
BOOL MODULE_Init(void)
{
    /* For these, built-in modules are always used */

#ifndef WINELIB32
    if (!MODULE_LoadBuiltin( "KERNEL", TRUE ) ||
        !MODULE_LoadBuiltin( "GDI", TRUE ) ||
        !MODULE_LoadBuiltin( "USER", TRUE ) ||
        !MODULE_LoadBuiltin( "WINPROCS", TRUE )) return FALSE;
#endif
    /* Initialize KERNEL.178 (__WINFLAGS) with the correct flags value */

    MODULE_SetEntryPoint( GetModuleHandle( "KERNEL" ), 178, GetWinFlags() );
    return TRUE;
}


/***********************************************************************
 *           MODULE_PrintModule
 */
void MODULE_PrintModule( HMODULE hmodule )
{
    int i, ordinal;
    SEGTABLEENTRY *pSeg;
    BYTE *pstr;
    WORD *pword;
    NE_MODULE *pModule = (NE_MODULE *)GlobalLock( hmodule );

      /* Dump the module info */

    printf( "Module "NPFMT":\n", hmodule );
    printf( "count=%d flags=%04x heap=%d stack=%d\n",
            pModule->count, pModule->flags,
            pModule->heap_size, pModule->stack_size );
    printf( "cs:ip=%04x:%04x ss:sp=%04x:%04x ds=%04x nb seg=%d modrefs=%d\n",
           pModule->cs, pModule->ip, pModule->ss, pModule->sp, pModule->dgroup,
           pModule->seg_count, pModule->modref_count );
    printf( "os_flags=%d swap_area=%d version=%04x\n",
            pModule->os_flags, pModule->min_swap_area,
            pModule->expected_version );

      /* Dump the file info */

    printf( "Filename: '%s'\n",
         ((LOADEDFILEINFO *)((BYTE *)pModule + pModule->fileinfo))->filename );

      /* Dump the segment table */

    printf( "\nSegment table:\n" );
    pSeg = NE_SEG_TABLE( pModule );
    for (i = 0; i < pModule->seg_count; i++, pSeg++)
        printf( "%02x: pos=%d size=%d flags=%04x minsize=%d sel="NPFMT"\n",
                i + 1, pSeg->filepos, pSeg->size, pSeg->flags,
                pSeg->minsize, pSeg->selector );

      /* Dump the resource table */

    printf( "\nResource table:\n" );
    if (pModule->res_table)
    {
        pword = (WORD *)((BYTE *)pModule + pModule->res_table);
        printf( "Alignment: %d\n", *pword++ );
        while (*pword)
        {
            struct resource_typeinfo_s *ptr = (struct resource_typeinfo_s *)pword;
            struct resource_nameinfo_s *pname = (struct resource_nameinfo_s *)(ptr + 1);
            printf( "id=%04x count=%d\n", ptr->type_id, ptr->count );
            for (i = 0; i < ptr->count; i++, pname++)
                printf( "offset=%d len=%d id=%04x\n",
                       pname->offset, pname->length, pname->id );
            pword = (WORD *)pname;
        }
    }
    else printf( "None\n" );

      /* Dump the resident name table */

    printf( "\nResident-name table:\n" );
    pstr = (char *)pModule + pModule->name_table;
    while (*pstr)
    {
        printf( "%*.*s: %d\n", *pstr, *pstr, pstr + 1,
                *(WORD *)(pstr + *pstr + 1) );
        pstr += *pstr + 1 + sizeof(WORD);
    }

      /* Dump the module reference table */

    printf( "\nModule ref table:\n" );
    if (pModule->modref_table)
    {
        pword = (WORD *)((BYTE *)pModule + pModule->modref_table);
        for (i = 0; i < pModule->modref_count; i++, pword++)
        {
            char *name = (char *)pModule + pModule->import_table + *pword;
            printf( "%d: %04x -> '%*.*s'\n",
                    i, *pword, *name, *name, name + 1 );
        }
    }
    else printf( "None\n" );

      /* Dump the entry table */

    printf( "\nEntry table:\n" );
    pstr = (char *)pModule + pModule->entry_table;
    ordinal = 1;
    while (*pstr)
    {
        printf( "Bundle %d-%d: %02x\n", ordinal, ordinal + *pstr - 1, pstr[1]);
        if (!pstr[1])
        {
            ordinal += *pstr;
            pstr += 2;
        }
        else if ((BYTE)pstr[1] == 0xff)  /* moveable */
        {
            struct entry_tab_movable_s *pe = (struct entry_tab_movable_s*)(pstr+2);
            for (i = 0; i < *pstr; i++, pe++)
                printf( "%d: %02x:%04x (moveable)\n",
                        ordinal++, pe->seg_number, pe->offset );
            pstr = (char *)pe;
        }
        else  /* fixed */
        {
            struct entry_tab_fixed_s *pe = (struct entry_tab_fixed_s*)(pstr+2);
            for (i = 0; i < *pstr; i++, pe++)
                printf( "%d: %04x (fixed)\n",
                        ordinal++, pe->offset[0] + (pe->offset[1] << 8) );
            pstr = (char *)pe;
        }
    }

    /* Dump the non-resident names table */

    printf( "\nNon-resident names table:\n" );
    if (pModule->nrname_handle)
    {
        pstr = (char *)GlobalLock( pModule->nrname_handle );
        while (*pstr)
        {
            printf( "%*.*s: %d\n", *pstr, *pstr, pstr + 1,
                   *(WORD *)(pstr + *pstr + 1) );
            pstr += *pstr + 1 + sizeof(WORD);
        }
    }
    printf( "\n" );
}


/***********************************************************************
 *           MODULE_OpenFile
 */
int MODULE_OpenFile( HMODULE hModule )
{
    NE_MODULE *pModule;
    char *name;
    const char *unixName;

    static int cachedfd = -1;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_module( stddeb, "MODULE_OpenFile("NPFMT") cache: mod="NPFMT" fd=%d\n",
                    hModule, hCachedModule, cachedfd );
    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return -1;
    if (hCachedModule == hModule) return cachedfd;
    close( cachedfd );
    hCachedModule = hModule;
    name = ((LOADEDFILEINFO*)((char*)pModule + pModule->fileinfo))->filename;
    if (!(unixName = DOSFS_GetUnixFileName( name, TRUE )) ||
        (cachedfd = open( unixName, O_RDONLY )) == -1)
        fprintf( stderr, "MODULE_OpenFile: can't open file '%s' for module "NPFMT"\n",
                 name, hModule );
    dprintf_module( stddeb, "MODULE_OpenFile: opened '%s' -> %d\n",
                    name, cachedfd );
    return cachedfd;
}


/***********************************************************************
 *           MODULE_Ne2MemFlags
 *
 * This function translates NE segment flags to GlobalAlloc flags
 */
static WORD MODULE_Ne2MemFlags(WORD flags)
{ 
    WORD memflags = 0;
#if 0
    if (flags & NE_SEGFLAGS_DISCARDABLE) 
      memflags |= GMEM_DISCARDABLE;
    if (flags & NE_SEGFLAGS_MOVEABLE || 
	( ! (flags & NE_SEGFLAGS_DATA) &&
	  ! (flags & NE_SEGFLAGS_LOADED) &&
	  ! (flags & NE_SEGFLAGS_ALLOCATED)
	 )
	)
      memflags |= GMEM_MOVEABLE;
    memflags |= GMEM_ZEROINIT;
#else
    memflags = GMEM_ZEROINIT | GMEM_FIXED;
    return memflags;
#endif
}

/***********************************************************************
 *           MODULE_AllocateSegment (WINPROCS.26)
 */

DWORD MODULE_AllocateSegment(WORD wFlags, WORD wSize, WORD wElem)
{
    WORD size = wSize << wElem;
    HANDLE hMem = GlobalAlloc( MODULE_Ne2MemFlags(wFlags), size);
#ifdef WINELIB32
    return (DWORD)GlobalLock(hMem);
#else
    WORD selector = HIWORD(GlobalLock(hMem));
    return MAKELONG(hMem, selector);
#endif
}

/***********************************************************************
 *           MODULE_CreateSegments
 */
#ifndef WINELIB32
static BOOL MODULE_CreateSegments( HMODULE hModule )
{
    SEGTABLEENTRY *pSegment;
    NE_MODULE *pModule;
    int i, minsize;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return FALSE;
    pSegment = NE_SEG_TABLE( pModule );
    for (i = 1; i <= pModule->seg_count; i++, pSegment++)
    {
        minsize = pSegment->minsize ? pSegment->minsize : 0x10000;
        if (i == pModule->ss) minsize += pModule->stack_size;
	/* The DGROUP is allocated by MODULE_CreateInstance */
        if (i == pModule->dgroup) continue;
        pSegment->selector = GLOBAL_Alloc( MODULE_Ne2MemFlags(pSegment->flags),
                                      minsize, hModule,
                                      !(pSegment->flags & NE_SEGFLAGS_DATA),
                                      FALSE,
                            FALSE /*pSegment->flags & NE_SEGFLAGS_READONLY*/ );
        if (!pSegment->selector) return FALSE;
    }

    pModule->dgroup_entry = pModule->dgroup ? pModule->seg_table +
                            (pModule->dgroup - 1) * sizeof(SEGTABLEENTRY) : 0;
    return TRUE;
}
#endif


/***********************************************************************
 *           MODULE_GetInstance
 */
#ifndef WINELIB32
static HINSTANCE MODULE_GetInstance( HMODULE hModule )
{
    SEGTABLEENTRY *pSegment;
    NE_MODULE *pModule;
    
    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;
    if (pModule->dgroup == 0) return hModule;

    pSegment = NE_SEG_TABLE( pModule ) + pModule->dgroup - 1;
    
    return pSegment->selector;
}
#endif


/***********************************************************************
 *           MODULE_CreateInstance
 */
HINSTANCE MODULE_CreateInstance( HMODULE hModule, LOADPARAMS *params )
{
    SEGTABLEENTRY *pSegment;
    NE_MODULE *pModule;
    int minsize;
    HINSTANCE hNewInstance, hPrevInstance;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;
    if (pModule->dgroup == 0) return hModule;

    pSegment = NE_SEG_TABLE( pModule ) + pModule->dgroup - 1;
    hPrevInstance = pSegment->selector;

      /* if it's a library, create a new instance only the first time */
    if (hPrevInstance)
    {
        if (pModule->flags & NE_FFLAGS_LIBMODULE) return hPrevInstance;
        if (params == (LOADPARAMS*)-1) return hPrevInstance;
    }

    minsize = pSegment->minsize ? pSegment->minsize : 0x10000;
    if (pModule->ss == pModule->dgroup) minsize += pModule->stack_size;
    minsize += pModule->heap_size;
    hNewInstance = GLOBAL_Alloc( GMEM_ZEROINIT | GMEM_FIXED,
                                 minsize, hModule, FALSE, FALSE, FALSE );
    if (!hNewInstance) return 0;
    pSegment->selector = hNewInstance;
    return hNewInstance;
}


/***********************************************************************
 *           MODULE_LoadExeHeader
 */
HMODULE MODULE_LoadExeHeader( int fd, OFSTRUCT *ofs )
{
    struct mz_header_s mz_header;
    struct ne_header_s ne_header;
    int size;
    HMODULE hModule;
    NE_MODULE *pModule;
    BYTE *pData;
    char *buffer, *fastload = NULL;
    int fastload_offset = 0, fastload_length = 0;

  /* Read a block from either the file or the fast-load area. */
#define READ(offset,size,buffer) \
       ((fastload && ((offset) >= fastload_offset) && \
         ((offset)+(size) <= fastload_offset+fastload_length)) ? \
        (memcpy( buffer, fastload+(offset)-fastload_offset, (size) ), TRUE) : \
        (lseek( fd, mz_header.ne_offset+(offset), SEEK_SET), \
         read( fd, (buffer), (size) ) == (size)))

    lseek( fd, 0, SEEK_SET );
    if ((read( fd, &mz_header, sizeof(mz_header) ) != sizeof(mz_header)) ||
        (mz_header.mz_magic != MZ_SIGNATURE)) return (HMODULE)11;  /* invalid exe */

    lseek( fd, mz_header.ne_offset, SEEK_SET );
    if (read( fd, &ne_header, sizeof(ne_header) ) != sizeof(ne_header))
        return (HMODULE)11;  /* invalid exe */

    if (ne_header.ne_magic == PE_SIGNATURE) return (HMODULE)21;  /* win32 exe */
    if (ne_header.ne_magic != NE_SIGNATURE) return (HMODULE)11;  /* invalid exe */

    /* We now have a valid NE header */

    size = sizeof(NE_MODULE) +
             /* loaded file info */
           sizeof(LOADEDFILEINFO) + strlen(ofs->szPathName) +
             /* segment table */
           ne_header.n_segment_tab * sizeof(SEGTABLEENTRY) +
             /* resource table */
           ne_header.rname_tab_offset - ne_header.resource_tab_offset +
             /* resident names table */
           ne_header.moduleref_tab_offset - ne_header.rname_tab_offset +
             /* module ref table */
           ne_header.n_mod_ref_tab * sizeof(WORD) + 
             /* imported names table */
           ne_header.entry_tab_offset - ne_header.iname_tab_offset +
             /* entry table length */
           ne_header.entry_tab_length;

    hModule = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, size );
    if (!hModule) return (HMODULE)11;  /* invalid exe */
    FarSetOwner( hModule, (WORD)(DWORD)hModule );
    pModule = (NE_MODULE *)GlobalLock( hModule );
    memcpy( pModule, &ne_header, sizeof(NE_MODULE) );
    pModule->count = 0;
    pData = (BYTE *)(pModule + 1);

    /* Read the fast-load area */

    if (ne_header.additional_flags & NE_AFLAGS_FASTLOAD)
    {
        fastload_offset=ne_header.fastload_offset<<ne_header.align_shift_count;
        fastload_length=ne_header.fastload_length<<ne_header.align_shift_count;
        dprintf_module( stddeb, "Using fast-load area offset=%x len=%d\n",
                        fastload_offset, fastload_length );
        if ((fastload = (char *)malloc( fastload_length )) != NULL)
        {
            lseek( fd, mz_header.ne_offset + fastload_offset, SEEK_SET );
            if (read( fd, fastload, fastload_length ) != fastload_length)
            {
                free( fastload );
                fastload = NULL;
            }
        }
    }

    /* Store the filename information */

    pModule->fileinfo = (int)pData - (int)pModule;
    ((LOADEDFILEINFO*)pData)->length = sizeof(LOADEDFILEINFO)+strlen(ofs->szPathName);
    ((LOADEDFILEINFO*)pData)->fixed_media = TRUE;
    ((LOADEDFILEINFO*)pData)->error = 0;
    ((LOADEDFILEINFO*)pData)->date = 0;
    ((LOADEDFILEINFO*)pData)->time = 0;
    strcpy( ((LOADEDFILEINFO*)pData)->filename, ofs->szPathName );
    pData += ((LOADEDFILEINFO*)pData)->length--;

    /* Get the segment table */

    pModule->seg_table = (int)pData - (int)pModule;
    buffer = malloc( ne_header.n_segment_tab * sizeof(struct ne_segment_table_entry_s) );
    if (buffer)
    {
        int i;
        struct ne_segment_table_entry_s *pSeg;

        if (!READ( ne_header.segment_tab_offset,
             ne_header.n_segment_tab * sizeof(struct ne_segment_table_entry_s),
             buffer )) return (HMODULE)11;  /* invalid exe */
        pSeg = (struct ne_segment_table_entry_s *)buffer;
        for (i = ne_header.n_segment_tab; i > 0; i--, pSeg++)
        {
            memcpy( pData, pSeg, sizeof(*pSeg) );
            pData += sizeof(SEGTABLEENTRY);
        }
        free( buffer );
    }
    else return (HMODULE)11;  /* invalid exe */

    /* Get the resource table */

    if (ne_header.resource_tab_offset < ne_header.rname_tab_offset)
    {
        pModule->res_table = (int)pData - (int)pModule;
        if (!READ(ne_header.resource_tab_offset,
                  ne_header.rname_tab_offset - ne_header.resource_tab_offset,
                  pData )) return (HMODULE)11;  /* invalid exe */
        pData += ne_header.rname_tab_offset - ne_header.resource_tab_offset;
    }
    else pModule->res_table = 0;  /* No resource table */

    /* Get the resident names table */

    pModule->name_table = (int)pData - (int)pModule;
    if (!READ( ne_header.rname_tab_offset,
               ne_header.moduleref_tab_offset - ne_header.rname_tab_offset,
               pData )) return (HMODULE)11;  /* invalid exe */
    pData += ne_header.moduleref_tab_offset - ne_header.rname_tab_offset;

    /* Get the module references table */

    if (ne_header.n_mod_ref_tab > 0)
    {
        pModule->modref_table = (int)pData - (int)pModule;
        if (!READ( ne_header.moduleref_tab_offset,
                  ne_header.n_mod_ref_tab * sizeof(WORD),
                  pData )) return (HMODULE)11;  /* invalid exe */
        pData += ne_header.n_mod_ref_tab * sizeof(WORD);
    }
    else pModule->modref_table = 0;  /* No module references */

    /* Get the imported names table */

    pModule->import_table = (int)pData - (int)pModule;
    if (!READ( ne_header.iname_tab_offset, 
               ne_header.entry_tab_offset - ne_header.iname_tab_offset,
               pData )) return (HMODULE)11;  /* invalid exe */
    pData += ne_header.entry_tab_offset - ne_header.iname_tab_offset;

    /* Get the entry table */

    pModule->entry_table = (int)pData - (int)pModule;
    if (!READ( ne_header.entry_tab_offset,
               ne_header.entry_tab_length,
               pData )) return (HMODULE)11;  /* invalid exe */
    pData += ne_header.entry_tab_length;

    /* Get the non-resident names table */

    if (ne_header.nrname_tab_length)
    {
        pModule->nrname_handle = GLOBAL_Alloc( 0, ne_header.nrname_tab_length,
                                               hModule, FALSE, FALSE, FALSE );
        if (!pModule->nrname_handle) return (HMODULE)11;  /* invalid exe */
        buffer = GlobalLock( pModule->nrname_handle );
        lseek( fd, ne_header.nrname_tab_offset, SEEK_SET );
        if (read( fd, buffer, ne_header.nrname_tab_length )
              != ne_header.nrname_tab_length) return (HMODULE)11;  /* invalid exe */
    }
    else pModule->nrname_handle = 0;

    /* Allocate a segment for the implicitly-loaded DLLs */

    if (pModule->modref_count)
    {
        pModule->dlls_to_init = GLOBAL_Alloc(GMEM_ZEROINIT,
                                    (pModule->modref_count+1)*sizeof(HMODULE),
                                    hModule, FALSE, FALSE, FALSE );
        if (!pModule->dlls_to_init) return (HMODULE)11;  /* invalid exe */
    }
    else pModule->dlls_to_init = 0;

    if (debugging_module) MODULE_PrintModule( hModule );
    pModule->next = hFirstModule;
    hFirstModule = hModule;
    return hModule;
}


/***********************************************************************
 *           MODULE_GetOrdinal
 *
 * Lookup the ordinal for a given name.
 */
WORD MODULE_GetOrdinal( HMODULE hModule, char *name )
{
    char buffer[256], *cpnt;
    BYTE len;
    NE_MODULE *pModule;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;

    dprintf_module( stddeb, "MODULE_GetOrdinal("NPFMT",'%s')\n",
                    hModule, name );

      /* First handle names of the form '#xxxx' */

    if (name[0] == '#') return atoi( name + 1 );

      /* Now copy and uppercase the string */

    strcpy( buffer, name );
    AnsiUpper( buffer );
    len = strlen( buffer );

      /* First search the resident names */

    cpnt = (char *)pModule + pModule->name_table;

      /* Skip the first entry (module name) */
    cpnt += *cpnt + 1 + sizeof(WORD);
    while (*cpnt)
    {
        dprintf_module( stddeb, "  Checking '%*.*s'\n", *cpnt, *cpnt, cpnt+1 );
        if (((BYTE)*cpnt == len) && !memcmp( cpnt+1, buffer, len ))
        {
            dprintf_module( stddeb, "  Found: ordinal=%d\n",
                            *(WORD *)(cpnt + *cpnt + 1) );
            return *(WORD *)(cpnt + *cpnt + 1);
        }
        cpnt += *cpnt + 1 + sizeof(WORD);
    }

      /* Now search the non-resident names table */

    if (!pModule->nrname_handle) return 0;  /* No non-resident table */
    cpnt = (char *)GlobalLock( pModule->nrname_handle );

      /* Skip the first entry (module description string) */
    cpnt += *cpnt + 1 + sizeof(WORD);
    while (*cpnt)
    {
        dprintf_module( stddeb, "  Checking '%*.*s'\n", *cpnt, *cpnt, cpnt+1 );
        if (((BYTE)*cpnt == len) && !memcmp( cpnt+1, buffer, len ))
        {
            dprintf_module( stddeb, "  Found: ordinal=%d\n",
                            *(WORD *)(cpnt + *cpnt + 1) );
            return *(WORD *)(cpnt + *cpnt + 1);
        }
        cpnt += *cpnt + 1 + sizeof(WORD);
    }
    return 0;
}


/***********************************************************************
 *           MODULE_GetEntryPoint
 *
 * Return the entry point for a given ordinal.
 */
SEGPTR MODULE_GetEntryPoint( HMODULE hModule, WORD ordinal )
{
    NE_MODULE *pModule;
    WORD curOrdinal = 1;
    BYTE *p;
    WORD sel, offset;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;

    p = (BYTE *)pModule + pModule->entry_table;
    while (*p && (curOrdinal + *p <= ordinal))
    {
          /* Skipping this bundle */
        curOrdinal += *p;
        switch(p[1])
        {
            case 0:    p += 2; break;  /* unused */
            case 0xff: p += 2 + *p * 6; break;  /* moveable */
            default:   p += 2 + *p * 3; break;  /* fixed */
        }
    }
    if (!*p) return 0;

    switch(p[1])
    {
        case 0:  /* unused */
            return 0;
        case 0xff:  /* moveable */
            p += 2 + 6 * (ordinal - curOrdinal);
            sel = p[3];
            offset = *(WORD *)(p + 4);
            break;
        default:  /* fixed */
            sel = p[1];
            p += 2 + 3 * (ordinal - curOrdinal);
            offset = *(WORD *)(p + 1);
            break;
    }

    if (sel == 0xfe) sel = 0xffff;  /* constant entry */
    else sel = (WORD)(DWORD)NE_SEG_TABLE(pModule)[sel-1].selector;
    return (SEGPTR)MAKELONG( offset, sel );
}


/***********************************************************************
 *           MODULE_SetEntryPoint
 *
 * Change the value of an entry point. Use with caution!
 * It can only change the offset value, not the selector.
 */
BOOL MODULE_SetEntryPoint( HMODULE hModule, WORD ordinal, WORD offset )
{
    NE_MODULE *pModule;
    WORD curOrdinal = 1;
    BYTE *p;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return FALSE;

    p = (BYTE *)pModule + pModule->entry_table;
    while (*p && (curOrdinal + *p <= ordinal))
    {
          /* Skipping this bundle */
        curOrdinal += *p;
        switch(p[1])
        {
            case 0:    p += 2; break;  /* unused */
            case 0xff: p += 2 + *p * 6; break;  /* moveable */
            default:   p += 2 + *p * 3; break;  /* fixed */
        }
    }
    if (!*p) return FALSE;

    switch(p[1])
    {
        case 0:  /* unused */
            return FALSE;
        case 0xff:  /* moveable */
            p += 2 + 6 * (ordinal - curOrdinal);
            *(WORD *)(p + 4) = offset;
            break;
        default:  /* fixed */
            p += 2 + 3 * (ordinal - curOrdinal);
            *(WORD *)(p + 1) = offset;
            break;
    }
    return TRUE;
}


/***********************************************************************
 *           MODULE_GetEntryPointName
 *
 * Return the entry point name for a given ordinal.
 * Used only by relay debugging.
 * Warning: returned pointer is to a Pascal-type string.
 */
LPSTR MODULE_GetEntryPointName( HMODULE hModule, WORD ordinal )
{
    register char *cpnt;
    NE_MODULE *pModule;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;

      /* First search the resident names */

    cpnt = (char *)pModule + pModule->name_table;
    while (*cpnt)
    {
        cpnt += *cpnt + 1 + sizeof(WORD);
        if (*(WORD *)(cpnt + *cpnt + 1) == ordinal) return cpnt;
    }

      /* Now search the non-resident names table */

    if (!pModule->nrname_handle) return 0;  /* No non-resident table */
    cpnt = (char *)GlobalLock( pModule->nrname_handle );
    while (*cpnt)
    {
        cpnt += *cpnt + 1 + sizeof(WORD);
        if (*(WORD *)(cpnt + *cpnt + 1) == ordinal) return cpnt;
    }
    return NULL;
}


/***********************************************************************
 *           MODULE_GetModuleName
 */
LPSTR MODULE_GetModuleName( HMODULE hModule )
{
    NE_MODULE *pModule;
    BYTE *p, len;
    static char buffer[10];

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return NULL;
    p = (BYTE *)pModule + pModule->name_table;
    len = MIN( *p, 8 );
    memcpy( buffer, p + 1, len );
    buffer[len] = '\0';
    return buffer;
}


/**********************************************************************
 *           MODULE_RegisterModule
 */
void MODULE_RegisterModule( HMODULE hModule )
{
	NE_MODULE *pModule;
	pModule = (NE_MODULE *)GlobalLock( hModule );
	pModule->next = hFirstModule;
	hFirstModule = hModule;
}

/**********************************************************************
 *	    MODULE_FindModule
 *
 * Find a module from a path name.
 */
HMODULE MODULE_FindModule( LPCSTR path )
{
    HMODULE hModule = hFirstModule;
    LPCSTR filename, dotptr, modulepath, modulename;
    BYTE len, *name_table;

    if (!(filename = strrchr( path, '\\' ))) filename = path;
    else filename++;
    if ((dotptr = strrchr( filename, '.' )) != NULL)
        len = (BYTE)(dotptr - filename);
    else len = strlen( filename );

    while(hModule)
    {
        NE_MODULE *pModule = (NE_MODULE *)GlobalLock( hModule );
        if (!pModule) break;
        modulepath = ((LOADEDFILEINFO*)((char*)pModule + pModule->fileinfo))->filename;
        if (!(modulename = strrchr( modulepath, '\\' )))
            modulename = modulepath;
        else modulename++;
        if (!lstrcmpi( modulename, filename )) return hModule;

        name_table = (BYTE *)pModule + pModule->name_table;
        if ((*name_table == len) && !lstrncmpi(filename, name_table+1, len))
            return hModule;
        hModule = pModule->next;
    }
    return 0;
}


/**********************************************************************
 *	    MODULE_FreeModule
 *
 * Remove a module from memory.
 */
static void MODULE_FreeModule( HMODULE hModule )
{
    HMODULE *hPrevModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    HMODULE *pModRef;
    int i;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return;
    if (pModule->flags & NE_FFLAGS_BUILTIN)
        return;  /* Can't free built-in module */

    /* FIXME: should call the exit code for the library here */

      /* Remove it from the linked list */

    hPrevModule = &hFirstModule;
    while (*hPrevModule && (*hPrevModule != hModule))
    {
        hPrevModule = &((NE_MODULE *)GlobalLock( *hPrevModule ))->next;
    }
    if (*hPrevModule) *hPrevModule = pModule->next;

      /* Free all the segments */

    pSegment = NE_SEG_TABLE( pModule );
    for (i = 1; i <= pModule->seg_count; i++, pSegment++)
    {
        GlobalFree( pSegment->selector );
    }

      /* Free the referenced modules */

    pModRef = (HMODULE*)NE_MODULE_TABLE( pModule );
    for (i = 0; i < pModule->modref_count; i++, pModRef++)
    {
        FreeModule( *pModRef );
    }

      /* Free the module storage */

    if (pModule->nrname_handle) GlobalFree( pModule->nrname_handle );
    if (pModule->dlls_to_init) GlobalFree( pModule->dlls_to_init );
    GlobalFree( hModule );

      /* Remove module from cache */

    if (hCachedModule == hModule) hCachedModule = 0;
}


HINSTANCE PE_LoadModule(int fd, OFSTRUCT *ofs, LOADPARAMS* params);

/**********************************************************************
 *	    LoadModule    (KERNEL.45)
 */
HINSTANCE LoadModule( LPCSTR name, LPVOID paramBlock )
{
    HMODULE hModule;
    HANDLE hInstance, hPrevInstance;
    NE_MODULE *pModule;
    LOADPARAMS *params = (LOADPARAMS *)paramBlock;
#ifndef WINELIB
    WORD *pModRef, *pDLLs;
    int i, fd;

    hModule = MODULE_FindModule( name );

    if (!hModule)  /* We have to load the module */
    {
        OFSTRUCT ofs;

        /* Try to load the built-in first if not disabled */
        if ((hModule = MODULE_LoadBuiltin( name, FALSE ))) return hModule;

        if ((fd = FILE_OpenFile( name, &ofs, OF_READ )) == -1)
        {
            /* Now try the built-in even if disabled */
            if ((hModule = MODULE_LoadBuiltin( name, TRUE )))
            {
                fprintf( stderr, "Warning: could not load Windows DLL '%s', using built-in module.\n", name );
                return hModule;
            }
            return 2;  /* File not found */
        }

          /* Create the module structure */

        hModule = MODULE_LoadExeHeader( fd, &ofs );
        if (hModule == 21) hModule = PE_LoadModule( fd, &ofs, paramBlock );
        close( fd );
        if (hModule < 32)
        {
            fprintf( stderr, "LoadModule: can't load '%s', error=%d\n",
                     name, hModule );
            return hModule;
        }
        pModule = (NE_MODULE *)GlobalLock( hModule );

          /* Allocate the segments for this module */

        MODULE_CreateSegments( hModule );

        hPrevInstance = 0;
        hInstance = MODULE_CreateInstance( hModule, (LOADPARAMS*)paramBlock );

          /* Load the referenced DLLs */

        pModRef = (WORD *)((char *)pModule + pModule->modref_table);
        pDLLs = (WORD *)GlobalLock( pModule->dlls_to_init );
        for (i = 0; i < pModule->modref_count; i++, pModRef++)
        {
            char buffer[256];
            BYTE *pstr = (BYTE *)pModule + pModule->import_table + *pModRef;
            memcpy( buffer, pstr + 1, *pstr );
            strcpy( buffer + *pstr, ".dll" );
            dprintf_module( stddeb, "Loading '%s'\n", buffer );
            if (!(*pModRef = MODULE_FindModule( buffer )))
            {
                /* If the DLL is not loaded yet, load it and store */
                /* its handle in the list of DLLs to initialize.   */
                HMODULE hDLL;

                if ((hDLL = LoadModule( buffer, (LPVOID)-1 )) == 2)  /* file not found */
                {
                    char *p;

                    /* Try with prepending the path of the current module */
                    GetModuleFileName( hModule, buffer, 256 );
                    if (!(p = strrchr( buffer, '\\' ))) p = buffer;
                    memcpy( p + 1, pstr + 1, *pstr );
                    strcpy( p + 1 + *pstr, ".dll" );
                    hDLL = LoadModule( buffer, (LPVOID)-1 );
                }
                if (hDLL < 32)
                {
                    fprintf( stderr, "Could not load '%s' required by '%s', error = %d\n",
                             buffer, name, hDLL );
                    return 2;  /* file not found */
                }
                *pModRef = GetExePtr( hDLL );
                *pDLLs++ = *pModRef;
            }
            else  /* Increment the reference count of the DLL */
            {
                NE_MODULE *pOldDLL = (NE_MODULE *)GlobalLock( *pModRef );
                if (pOldDLL) pOldDLL->count++;
            }
        }

          /* Load the segments */

	if (pModule->flags & NE_FFLAGS_SELFLOAD)
	{
		/* Handle self loading modules */
		SEGTABLEENTRY * pSegTable = (SEGTABLEENTRY *) NE_SEG_TABLE(pModule);
		SELFLOADHEADER *selfloadheader;
		HMODULE hselfload = GetModuleHandle("WINPROCS");
		WORD oldss, oldsp, saved_dgroup = pSegTable[pModule->dgroup - 1].selector;
		fprintf (stderr, "Warning:  %*.*s is a self-loading module\n"
                                "Support for self-loading modules is very experimental\n",
                *((BYTE*)pModule + pModule->name_table),
                *((BYTE*)pModule + pModule->name_table),
                (char *)pModule + pModule->name_table + 1);
		NE_LoadSegment( hModule, 1 );
		selfloadheader = (SELFLOADHEADER *)
			PTR_SEG_OFF_TO_LIN(pSegTable->selector, 0);
		selfloadheader->EntryAddrProc = 
                                           MODULE_GetEntryPoint(hselfload,27);
		selfloadheader->MyAlloc  = MODULE_GetEntryPoint(hselfload,28);
		selfloadheader->SetOwner = MODULE_GetEntryPoint(GetModuleHandle("KERNEL"),403);
		pModule->self_loading_sel = GlobalHandleToSel(
					GLOBAL_Alloc (GMEM_ZEROINIT,
					0xFF00, hModule, FALSE, FALSE, FALSE)
					);
		oldss = IF1632_Saved16_ss;
		oldsp = IF1632_Saved16_sp;
		IF1632_Saved16_ss = pModule->self_loading_sel;
		IF1632_Saved16_sp = 0xFF00;
		if (!IF1632_Stack32_base) {
		  STACK32FRAME* frame32;
		  char *stack32Top;
		  /* Setup an initial 32 bit stack frame */
		  hInitialStack32 =  GLOBAL_Alloc( GMEM_FIXED, 0x10000,
						  hModule, FALSE, FALSE, 
						  FALSE );

		  /* Create the 32-bit stack frame */
		  
		  *(DWORD *)GlobalLock(hInitialStack32) = 0xDEADBEEF;
		  stack32Top = (char*)GlobalLock(hInitialStack32) + 
		    0x10000;
		  frame32 = (STACK32FRAME *)stack32Top - 1;
		  frame32->saved_esp = (DWORD)stack32Top;
		  frame32->edi = 0;
		  frame32->esi = 0;
		  frame32->edx = 0;
		  frame32->ecx = 0;
		  frame32->ebx = 0;
		  frame32->ebp = 0;
		  frame32->retaddr = 0;
		  frame32->codeselector = WINE_CODE_SELECTOR;
		  /* pTask->esp = (DWORD)frame32; */
		  IF1632_Stack32_base = WIN16_GlobalLock(hInitialStack32);

		}
		CallTo16_word_ww (selfloadheader->BootApp,
			pModule->self_loading_sel, hModule, fd);
		/* some BootApp procs overwrite the selector of dgroup */
		pSegTable[pModule->dgroup - 1].selector = saved_dgroup;
		IF1632_Saved16_ss = oldss;
		IF1632_Saved16_sp = oldsp;
		for (i = 2; i <= pModule->seg_count; i++) NE_LoadSegment( hModule, i );
		if (hInitialStack32){
		  GlobalUnlock (hInitialStack32);
		  GlobalFree (hInitialStack32);
		  IF1632_Stack32_base = hInitialStack32 = 0;
		}
	} 
	else
        {
            for (i = 1; i <= pModule->seg_count; i++)
                NE_LoadSegment( hModule, i );
        }

          /* Fixup the functions prologs */

        NE_FixupPrologs( hModule );

          /* Make sure the usage count is 1 on the first loading of  */
          /* the module, even if it contains circular DLL references */

        pModule->count = 1;

          /* Clear built-in flag in case it was set in the EXE file */

        pModule->flags &= ~NE_FFLAGS_BUILTIN;
    }
    else
    {
        pModule = (NE_MODULE *)GlobalLock( hModule );
        hPrevInstance = MODULE_GetInstance( hModule );
        hInstance = MODULE_CreateInstance( hModule, params );
        if (hInstance != hPrevInstance)  /* not a library */
            NE_LoadSegment( hModule, pModule->dgroup );
        pModule->count++;
    }
#else
    hModule = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(NE_MODULE) );
    pModule = (NE_MODULE *)GlobalLock( hModule );
    pModule->count = 1;
    pModule->magic = 0x454e;
    hPrevInstance = 0;
    hInstance = MODULE_CreateInstance( hModule, (LOADPARAMS*)paramBlock );
#endif /* WINELIB */

      /* Create a task for this instance */

    if (!(pModule->flags & NE_FFLAGS_LIBMODULE) && (paramBlock != (LPVOID)-1))
    {
        TASK_CreateTask( hModule, hInstance, hPrevInstance,
                         params->hEnvironment,
                         (LPSTR)PTR_SEG_TO_LIN( params->cmdLine ),
                         *((WORD *)PTR_SEG_TO_LIN(params->showCmd)+1) );
    }

    return hInstance;
}


/**********************************************************************
 *	    FreeModule    (KERNEL.46)
 */
BOOL FreeModule( HANDLE hModule )
{
    NE_MODULE *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return FALSE;

    dprintf_module( stddeb, "FreeModule: %s count %d\n", 
		    MODULE_GetModuleName(hModule), pModule->count );
    if (--pModule->count == 0) MODULE_FreeModule( hModule );
    return TRUE;
}


/**********************************************************************
 *	    GetModuleHandle    (KERNEL.47)
 */
HMODULE WIN16_GetModuleHandle( SEGPTR name )
{
#ifdef WINELIB32
    if (HIWORD(name) == 0) return GetExePtr( name );
#else
    if (HIWORD(name) == 0) return GetExePtr( LOWORD(name) );
#endif
    return MODULE_FindModule( PTR_SEG_TO_LIN(name) );
}

HMODULE GetModuleHandle( LPCSTR name )
{
    return MODULE_FindModule( name );
}


/**********************************************************************
 *	    GetModuleUsage    (KERNEL.48)
 */
int GetModuleUsage( HANDLE hModule )
{
    NE_MODULE *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;
    dprintf_module( stddeb, "GetModuleUsage("NPFMT"): returning %d\n",
                    hModule, pModule->count );
    return pModule->count;
}


/**********************************************************************
 *	    GetModuleFileName    (KERNEL.49)
 */
int GetModuleFileName( HANDLE hModule, LPSTR lpFileName, short nSize )
{
    NE_MODULE *pModule;
    char *name;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;
    name = ((LOADEDFILEINFO*)((char*)pModule + pModule->fileinfo))->filename;
    lstrcpyn( lpFileName, name, nSize );
    dprintf_module( stddeb, "GetModuleFilename: %s\n", lpFileName );
    return strlen(lpFileName);
}


/***********************************************************************
 *           LoadLibrary   (KERNEL.95)
 */
HANDLE LoadLibrary( LPCSTR libname )
{
#ifdef WINELIB
    dprintf_module( stddeb, "LoadLibrary: (%08x) %s\n", (int)libname, libname);
    WINELIB_UNIMP("LoadLibrary()");
    return (HANDLE)0;
#else
    HANDLE handle;

    dprintf_module( stddeb, "LoadLibrary: (%08x) %s\n", (int)libname, libname);

    /* This does not increment the module reference count, and will
     * therefore cause crashes on FreeLibrary calls.
    if ((handle = MODULE_FindModule( libname )) != 0) return handle;
     */
    handle = LoadModule( libname, (LPVOID)-1 );
    if (handle == (HANDLE)2)  /* file not found */
    {
        char buffer[256];
        lstrcpyn( buffer, libname, 252 );
        strcat( buffer, ".dll" );
        handle = LoadModule( buffer, (LPVOID)-1 );
    }
    if (handle >= (HANDLE)32) NE_InitializeDLLs( GetExePtr(handle) );
    return handle;
#endif
}


/***********************************************************************
 *           FreeLibrary   (KERNEL.96)
 */
void FreeLibrary( HANDLE handle )
{
    dprintf_module( stddeb,"FreeLibrary: "NPFMT"\n", handle );
    FreeModule( handle );
}


/***********************************************************************
 *           WinExec   (KERNEL.166)
 */
HANDLE WinExec( LPSTR lpCmdLine, WORD nCmdShow )
{
    LOADPARAMS params;
    HLOCAL cmdShowHandle, cmdLineHandle;
    HANDLE handle;
    WORD *cmdShowPtr;
    char *p, *cmdline, filename[256];

    if (!(cmdShowHandle = GlobalAlloc( 0, 2 * sizeof(WORD) ))) return 0;
    if (!(cmdLineHandle = GlobalAlloc( 0, 256 ))) return 0;

      /* Store nCmdShow */

    cmdShowPtr = (WORD *)GlobalLock( cmdShowHandle );
    cmdShowPtr[0] = 2;
    cmdShowPtr[1] = nCmdShow;

      /* Build the filename and command-line */

    cmdline = (char *)GlobalLock( cmdLineHandle );
    strncpy( filename, lpCmdLine, 256 );
    filename[255] = '\0';
    for (p = filename; *p && (*p != ' ') && (*p != '\t'); p++);
    if (*p)
    {
        strncpy( cmdline, p + 1, 128 );
        cmdline[127] = '\0';
    }
    else cmdline[0] = '\0';
    *p = '\0';

      /* Now load the executable file */

#ifdef WINELIB32
    params.hEnvironment = (HANDLE)GetDOSEnvironment();
#else
    params.hEnvironment = (HANDLE)SELECTOROF( GetDOSEnvironment() );
#endif
    params.cmdLine  = (SEGPTR)WIN16_GlobalLock( cmdLineHandle );
    params.showCmd  = (SEGPTR)WIN16_GlobalLock( cmdShowHandle );
    params.reserved = 0;
    handle = LoadModule( filename, &params );
    if (handle == (HANDLE)2)  /* file not found */
    {
	/* Check that the original file name did not have a suffix */
	p = strrchr(filename, '.');
        if (p && !(strchr(p, '/') || strchr(p, '\\')))
            return handle;  /* filename already includes a suffix! */
        strcat( filename, ".exe" );
        handle = LoadModule( filename, &params );
    }

    GlobalFree( cmdShowHandle );
    GlobalFree( cmdLineHandle );

#if 0
    if (handle < (HANDLE)32)	/* Error? */
	return handle;
    
/* FIXME: Yield never returns! 
   We may want to run more applications or start the debugger
   before calling Yield. If we don't Yield will be called immdiately
   after returning.  Why is it needed for Word anyway? */
    Yield();	/* program is executed immediately ....needed for word */

#endif
    return handle;
}


/***********************************************************************
 *           GetProcAddress   (KERNEL.50)
 */
FARPROC GetProcAddress( HANDLE hModule, SEGPTR name )
{
    WORD ordinal;
    SEGPTR ret;

    if (!hModule) hModule = GetCurrentTask();
    hModule = GetExePtr( hModule );

    if (HIWORD(name) != 0)
    {
        ordinal = MODULE_GetOrdinal( hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
        dprintf_module( stddeb, "GetProcAddress: "NPFMT" '%s'\n",
                        hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
    }
    else
    {
        ordinal = LOWORD(name);
        dprintf_module( stddeb, "GetProcAddress: "NPFMT" %04x\n",
                        hModule, ordinal );
    }
    if (!ordinal) return (FARPROC)0;

    ret = MODULE_GetEntryPoint( hModule, ordinal );

    dprintf_module( stddeb, "GetProcAddress: returning "SPFMT"\n", ret );
    return (FARPROC)ret;
}


/**********************************************************************
 *	    GetExpWinVer    (KERNEL.167)
 */
WORD GetExpWinVer( HMODULE hModule )
{
    NE_MODULE *pModule = (NE_MODULE *)GlobalLock( hModule );

    return pModule->expected_version;
}


/***********************************************************************
 *           GetWndProcEntry16 (not a Windows API function)
 *
 * Return an entry point from the WINPROCS dll.
 */
#ifndef WINELIB
WNDPROC GetWndProcEntry16( char *name )
{
    WORD ordinal;
    static HMODULE hModule = 0;

    if (!hModule) hModule = GetModuleHandle( "WINPROCS" );
    ordinal = MODULE_GetOrdinal( hModule, name );
    return MODULE_GetEntryPoint( hModule, ordinal );
}
#endif


/**********************************************************************
 *	    ModuleFirst    (TOOLHELP.59)
 */
BOOL ModuleFirst( MODULEENTRY *lpme )
{
    lpme->wNext = hFirstModule;
    return ModuleNext( lpme );
}


/**********************************************************************
 *	    ModuleNext    (TOOLHELP.60)
 */
BOOL ModuleNext( MODULEENTRY *lpme )
{
    NE_MODULE *pModule;

    if (!lpme->wNext) return FALSE;
    if (!(pModule = (NE_MODULE *)GlobalLock( lpme->wNext ))) return FALSE;
    strncpy( lpme->szModule, (char *)pModule + pModule->name_table,
             MAX_MODULE_NAME );
    lpme->szModule[MAX_MODULE_NAME] = '\0';
    lpme->hModule = lpme->wNext;
    lpme->wcUsage = pModule->count;
    strncpy( lpme->szExePath,
             ((LOADEDFILEINFO*)((char*)pModule + pModule->fileinfo))->filename,
             MAX_PATH );
    lpme->szExePath[MAX_PATH] = '\0';
    lpme->wNext = pModule->next;
    return TRUE;
}


/**********************************************************************
 *	    ModuleFindName    (TOOLHELP.61)
 */
BOOL ModuleFindName( MODULEENTRY *lpme, LPCSTR name )
{
    lpme->wNext = GetModuleHandle( name );
    return ModuleNext( lpme );
}


/**********************************************************************
 *	    ModuleFindHandle    (TOOLHELP.62)
 */
BOOL ModuleFindHandle( MODULEENTRY *lpme, HMODULE hModule )
{
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    lpme->wNext = hModule;
    return ModuleNext( lpme );
}
