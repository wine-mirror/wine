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
#include "global.h"
#include "ldt.h"
#include "module.h"
#include "neexe.h"
#include "stackframe.h"
#include "task.h"
#include "toolhelp.h"
#include "stddebug.h"
#include "debug.h"


static HMODULE hFirstModule = 0;
static HMODULE hCachedModule = 0;  /* Module cached by MODULE_OpenFile */


/***********************************************************************
 *           MODULE_Init
 *
 * Create the built-in modules.
 */
BOOL MODULE_Init(void)
{
    HMODULE hModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    struct dll_table_s *table;
    char *dosmem;
    int i;

      /* Create the built-in modules */

    for (i = 0, table = dll_builtin_table; i < N_BUILTINS; i++, table++)
    {
        if (!table->used) continue;

        hModule = GLOBAL_CreateBlock( GMEM_MOVEABLE, table->module_start,
                                      table->module_end - table->module_start,
                                      0, FALSE, FALSE, FALSE, NULL );
        if (!hModule) return FALSE;
        FarSetOwner( hModule, hModule );

        table->hModule = hModule;

        dprintf_module( stddeb, "Built-in %s: hmodule=%04x\n",
                        table->name, hModule );

          /* Allocate the code segment */

        pModule = (NE_MODULE *)GlobalLock( hModule );
        pSegTable = NE_SEG_TABLE( pModule );

        pSegTable->selector = GLOBAL_CreateBlock(GMEM_FIXED, table->code_start,
                                                 pSegTable->minsize, hModule,
                                                 TRUE, TRUE, FALSE, NULL );
        if (!pSegTable->selector) return FALSE;
        pSegTable++;

          /* Allocate the data segment */

        pSegTable->selector = GLOBAL_Alloc( GMEM_FIXED, pSegTable->minsize,
                                            hModule, FALSE, FALSE, FALSE );
        if (!pSegTable->selector) return FALSE;
        memcpy( GlobalLock( pSegTable->selector ), table->data_start,
                pSegTable->minsize );

        pModule->next = hFirstModule;
        hFirstModule = hModule;
    }

      /* Initialize some KERNEL exported values */

    if (!(hModule = GetModuleHandle( "KERNEL" ))) return TRUE;

      /* KERNEL.178: __WINFLAGS */
    MODULE_SetEntryPoint( hModule, 178, GetWinFlags() );

    /* Allocate 7 64k segments for 0000, A000, B000, C000, D000, E000, F000. */

    dosmem = malloc( 0x70000 );

    MODULE_SetEntryPoint( hModule, 183,  /* KERNEL.183: __0000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 193,  /* KERNEL.193: __0040H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x400,
                                  0x100, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 174,  /* KERNEL.174: __A000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x10000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 181,  /* KERNEL.181: __B000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x20000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 182,  /* KERNEL.182: __B800H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x28000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 195,  /* KERNEL.195: __C000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x30000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 179,  /* KERNEL.179: __D000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x40000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 190,  /* KERNEL.190: __E000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x50000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 173,  /* KERNEL.173: __ROMBIOS */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x60000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
    MODULE_SetEntryPoint( hModule, 194,  /* KERNEL.194: __F000H */
                          GLOBAL_CreateBlock( GMEM_FIXED, dosmem + 0x60000,
                                0x10000, hModule, FALSE, FALSE, FALSE, NULL ));
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

    printf( "Module %04x:\n", hmodule );
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
        printf( "%02x: pos=%d size=%d flags=%04x minsize=%d sel=%04x\n",
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

    static int cachedfd = -1;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_module( stddeb, "MODULE_OpenFile(%04x) cache: mod=%04x fd=%d\n",
                    hModule, hCachedModule, cachedfd );
    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return -1;
    if (hCachedModule == hModule) return cachedfd;
    close( cachedfd );
    hCachedModule = hModule;
    name = ((LOADEDFILEINFO*)((char*)pModule + pModule->fileinfo))->filename;
    cachedfd = open( DOS_GetUnixFileName( name ), O_RDONLY );
    dprintf_module( stddeb, "MODULE_OpenFile: opened '%s' -> %d\n",
                    name, cachedfd );
    return cachedfd;
}


/***********************************************************************
 *           MODULE_CreateSegments
 */
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
        pSegment->selector = GLOBAL_Alloc( GMEM_ZEROINIT | GMEM_FIXED,
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


/***********************************************************************
 *           MODULE_GetInstance
 */
static HINSTANCE MODULE_GetInstance( HMODULE hModule )
{
    SEGTABLEENTRY *pSegment;
    NE_MODULE *pModule;
    
    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return 0;
    if (pModule->dgroup == 0) return hModule;

    pSegment = NE_SEG_TABLE( pModule ) + pModule->dgroup - 1;
    
    return pSegment->selector;
}


/***********************************************************************
 *           MODULE_CreateInstance
 */
static HINSTANCE MODULE_CreateInstance( HMODULE hModule, LOADPARAMS *params )
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
        (mz_header.mz_magic != MZ_SIGNATURE)) return 11;  /* invalid exe */

    lseek( fd, mz_header.ne_offset, SEEK_SET );
    if (read( fd, &ne_header, sizeof(ne_header) ) != sizeof(ne_header))
        return 11;  /* invalid exe */

    if (ne_header.ne_magic == PE_SIGNATURE) return 21;  /* win32 exe */
    if (ne_header.ne_magic != NE_SIGNATURE) return 11;  /* invalid exe */

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
    if (!hModule) return 11;  /* invalid exe */
    FarSetOwner( hModule, hModule );
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
             buffer )) return 11;  /* invalid exe */
        pSeg = (struct ne_segment_table_entry_s *)buffer;
        for (i = ne_header.n_segment_tab; i > 0; i--, pSeg++)
        {
            memcpy( pData, pSeg, sizeof(*pSeg) );
            pData += sizeof(SEGTABLEENTRY);
        }
        free( buffer );
    }
    else return 11;  /* invalid exe */

    /* Get the resource table */

    if (ne_header.resource_tab_offset < ne_header.rname_tab_offset)
    {
        pModule->res_table = (int)pData - (int)pModule;
        if (!READ(ne_header.resource_tab_offset,
                  ne_header.rname_tab_offset - ne_header.resource_tab_offset,
                  pData )) return 11;  /* invalid exe */
        pData += ne_header.rname_tab_offset - ne_header.resource_tab_offset;
    }
    else pModule->res_table = 0;  /* No resource table */

    /* Get the resident names table */

    pModule->name_table = (int)pData - (int)pModule;
    if (!READ( ne_header.rname_tab_offset,
               ne_header.moduleref_tab_offset - ne_header.rname_tab_offset,
               pData )) return 11;  /* invalid exe */
    pData += ne_header.moduleref_tab_offset - ne_header.rname_tab_offset;

    /* Get the module references table */

    if (ne_header.n_mod_ref_tab > 0)
    {
        pModule->modref_table = (int)pData - (int)pModule;
        if (!READ( ne_header.moduleref_tab_offset,
                  ne_header.n_mod_ref_tab * sizeof(WORD),
                  pData )) return 11;  /* invalid exe */
        pData += ne_header.n_mod_ref_tab * sizeof(WORD);
    }
    else pModule->modref_table = 0;  /* No module references */

    /* Get the imported names table */

    pModule->import_table = (int)pData - (int)pModule;
    if (!READ( ne_header.iname_tab_offset, 
               ne_header.entry_tab_offset - ne_header.iname_tab_offset,
               pData )) return 11;  /* invalid exe */
    pData += ne_header.entry_tab_offset - ne_header.iname_tab_offset;

    /* Get the entry table */

    pModule->entry_table = (int)pData - (int)pModule;
    if (!READ( ne_header.entry_tab_offset,
               ne_header.entry_tab_length,
               pData )) return 11;  /* invalid exe */
    pData += ne_header.entry_tab_length;

    /* Get the non-resident names table */

    if (ne_header.nrname_tab_length)
    {
        pModule->nrname_handle = GLOBAL_Alloc( 0, ne_header.nrname_tab_length,
                                               hModule, FALSE, FALSE, FALSE );
        if (!pModule->nrname_handle) return 11;  /* invalid exe */
        buffer = GlobalLock( pModule->nrname_handle );
        lseek( fd, ne_header.nrname_tab_offset, SEEK_SET );
        if (read( fd, buffer, ne_header.nrname_tab_length )
              != ne_header.nrname_tab_length) return 11;  /* invalid exe */
    }
    else pModule->nrname_handle = 0;

    /* Allocate a segment for the implicitly-loaded DLLs */

    if (pModule->modref_count)
    {
        pModule->dlls_to_init = GLOBAL_Alloc(GMEM_ZEROINIT,
                                    (pModule->modref_count+1)*sizeof(HMODULE),
                                    hModule, FALSE, FALSE, FALSE );
        if (!pModule->dlls_to_init) return 11;  /* invalid exe */
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

    dprintf_module( stddeb, "MODULE_GetOrdinal(%04x,'%s')\n",
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
DWORD MODULE_GetEntryPoint( HMODULE hModule, WORD ordinal )
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
    else sel = NE_SEG_TABLE(pModule)[sel-1].selector;
    return MAKELONG( offset, sel );
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
    len = min( *p, 8 );
    memcpy( buffer, p + 1, len );
    buffer[len] = '\0';
    return buffer;
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
        if (!strcasecmp( modulename, filename )) return hModule;

        name_table = (BYTE *)pModule + pModule->name_table;
        if ((*name_table == len) && !strncasecmp(filename, name_table+1, len))
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
    WORD *pModRef;
    int i;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return;

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

    pModRef = NE_MODULE_TABLE( pModule );
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


/**********************************************************************
 *	    LoadModule    (KERNEL.45)
 */
HINSTANCE LoadModule( LPCSTR name, LPVOID paramBlock )
{
    HMODULE hModule;
    HANDLE hInstance, hPrevInstance;
    NE_MODULE *pModule;
    LOADPARAMS *params = (LOADPARAMS *)paramBlock;
    WORD *pModRef, *pDLLs;
    int i, fd;

    hModule = MODULE_FindModule( name );
    if (!hModule)  /* We have to load the module */
    {
        OFSTRUCT ofs;
        if (strchr( name, '/' )) name = DOS_GetDosFileName( name );
        if ((fd = OpenFile( name, &ofs, OF_READ )) == -1)
            return 2;  /* File not found */

          /* Create the module structure */

        if ((hModule = MODULE_LoadExeHeader( fd, &ofs )) < 32)
        {
            close( fd );
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

        for (i = 1; i <= pModule->seg_count; i++) NE_LoadSegment( hModule, i );

          /* Fixup the functions prologs */

        NE_FixupPrologs( hModule );

          /* Make sure the usage count is 1 on the first loading of  */
          /* the module, even if it contains circular DLL references */

        pModule->count = 1;
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
    if (HIWORD(name) == 0) return GetExePtr( LOWORD(name) );
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
    dprintf_module( stddeb, "GetModuleUsage(%04x): returning %d\n",
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
    strncpy( lpFileName, name, nSize );
    lpFileName[nSize-1] = '\0';
    dprintf_module( stddeb, "GetModuleFilename: %s\n", lpFileName );
    return strlen(lpFileName);
}


/***********************************************************************
 *           LoadLibrary   (KERNEL.95)
 */
HANDLE LoadLibrary( LPCSTR libname )
{
    HANDLE handle;

    dprintf_module( stddeb, "LoadLibrary: (%08x) %s\n", (int)libname, libname);
    /* This does not increment the module reference count, and will
     * therefore cause crashes on FreeLibrary calls.
    if ((handle = MODULE_FindModule( libname )) != 0) return handle;
     */
    handle = LoadModule( libname, (LPVOID)-1 );
    if (handle == 2)  /* file not found */
    {
        char buffer[256];
        strcpy( buffer, libname );
        strcat( buffer, ".dll" );
        handle = LoadModule( buffer, (LPVOID)-1 );
    }
    if (handle >= 32) NE_InitializeDLLs( GetExePtr(handle) );
    return handle;
}


/***********************************************************************
 *           FreeLibrary   (KERNEL.96)
 */
void FreeLibrary( HANDLE handle )
{
    dprintf_module( stddeb,"FreeLibrary: %04x\n", handle );
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

    params.hEnvironment = SELECTOROF( GetDOSEnvironment() );
    params.cmdLine  = WIN16_GlobalLock( cmdLineHandle );
    params.showCmd  = WIN16_GlobalLock( cmdShowHandle );
    params.reserved = 0;
    handle = LoadModule( filename, &params );
    if (handle == 2)  /* file not found */
    {
        strcat( filename, ".exe" );
        handle = LoadModule( filename, &params );
    }

    GlobalFree( cmdShowHandle );
    GlobalFree( cmdLineHandle );
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
        dprintf_module( stddeb, "GetProcAddress: %04x '%s'\n",
                        hModule, (LPSTR)PTR_SEG_TO_LIN(name) );
    }
    else
    {
        ordinal = LOWORD(name);
        dprintf_module( stddeb, "GetProcAddress: %04x %04x\n",
                        hModule, ordinal );
    }
    if (!ordinal) return (FARPROC)0;

    ret = MODULE_GetEntryPoint( hModule, ordinal );

    dprintf_module( stddeb, "GetProcAddress: returning %08lx\n", ret );
    return (FARPROC)ret;
}


/***********************************************************************
 *           GetWndProcEntry16 (not a Windows API function)
 *
 * Return an entry point from the WINPROCS dll.
 */
WNDPROC GetWndProcEntry16( char *name )
{
    WORD ordinal;
    static HMODULE hModule = 0;

    if (!hModule) hModule = GetModuleHandle( "WINPROCS" );
    ordinal = MODULE_GetOrdinal( hModule, name );
    return MODULE_GetEntryPoint( hModule, ordinal );
}


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
