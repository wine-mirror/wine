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
#include "global.h"
#include "ldt.h"
#include "module.h"
#include "neexe.h"
#include "toolhelp.h"
#include "stddebug.h"
/* #define DEBUG_MODULE */
#include "debug.h"


extern BYTE KERNEL_Module_Start[], KERNEL_Module_End[];

extern struct dll_name_table_entry_s dll_builtin_table[];

static HMODULE hFirstModule = 0;


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
    int i;

      /* Create the built-in modules */

    for (i = 0; i < N_BUILTINS; i++)
    {
        if (!dll_builtin_table[i].dll_is_used) continue;
        table = dll_builtin_table[i].table;

        hModule = GLOBAL_CreateBlock( GMEM_MOVEABLE, table->module_start,
                                      table->module_end - table->module_start,
                                      0, FALSE, FALSE, FALSE );
        if (!hModule) return FALSE;
        FarSetOwner( hModule, hModule );

        table->hModule = hModule;

        dprintf_module( stddeb, "Built-in %s: hmodule=%04x\n",
                        dll_builtin_table[i].dll_name, hModule );

          /* Allocate the code segment */

        pModule = (NE_MODULE *)GlobalLock( hModule );
        pSegTable = NE_SEG_TABLE( pModule );

        pSegTable->selector = GLOBAL_CreateBlock(GMEM_FIXED, table->code_start,
                                                 pSegTable->minsize, hModule,
                                                 TRUE, TRUE, FALSE );
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

    static HMODULE hCachedModule = 0;
    static int cachedfd = -1;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_module( stddeb, "MODULE_OpenFile(%04x) cache: mod=%04x fd=%d\n",
                    hModule, hCachedModule, cachedfd );
    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return -1;
    if (hCachedModule == hModule) return cachedfd;
    close( cachedfd );
    hCachedModule = hModule;
    name = ((LOADEDFILEINFO*)((char*)pModule + pModule->fileinfo))->filename;
    cachedfd = open( name /* DOS_GetUnixFileName( name ) */, O_RDONLY );
    dprintf_module( stddeb, "MODULE_OpenFile: opened '%s' -> %d\n",
                    name, cachedfd );
    return cachedfd;
}


/***********************************************************************
 *           MODULE_CreateSegments
 */
BOOL MODULE_CreateSegments( HMODULE hModule )
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
        if (i == pModule->dgroup)
        {
            /* FIXME: this is needed because heap growing is not implemented */
            pModule->heap_size = 0x10000 - minsize;
            minsize = 0x10000;
        }
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
 *           MODULE_LoadExeHeader
 */
HMODULE MODULE_LoadExeHeader( int fd, char *filename )
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
           sizeof(LOADEDFILEINFO) + strlen(filename) +
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
    ((LOADEDFILEINFO*)pData)->length = sizeof(LOADEDFILEINFO)+strlen(filename);
    ((LOADEDFILEINFO*)pData)->fixed_media = TRUE;
    ((LOADEDFILEINFO*)pData)->error = 0;
    ((LOADEDFILEINFO*)pData)->date = 0;
    ((LOADEDFILEINFO*)pData)->time = 0;
    strcpy( ((LOADEDFILEINFO*)pData)->filename, filename );
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

    dprintf_module( stddeb, "MODULE_GetEntryPoint(%04x,%d)\n",
                    hModule, ordinal );
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
    if (!*p)
    {
        dprintf_module( stddeb, "  Not found (last=%d)\n", curOrdinal-1 );
        return 0;
    }

    switch(p[1])
    {
        case 0:  /* unused */
            dprintf_module( stddeb, "  Found, but entry is unused\n" );
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

    dprintf_module( stddeb, "  Found, logical addr = %04x:%04x\n",
                    sel, offset );
    if (sel == 0xfe) sel = 0xffff;  /* constant entry */
    else sel = NE_SEG_TABLE(pModule)[sel-1].selector;
    return MAKELONG( offset, sel );
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
 *	    LoadModule    (KERNEL.45)
 */
HINSTANCE MODULE_LoadModule( LPCSTR name, LPVOID paramBlock )
{
}


/**********************************************************************
 *	    GetModuleHandle    (KERNEL.47)
 */
HMODULE GetModuleHandle( LPCSTR name )
{
    char buffer[16];
    BYTE len;
    HMODULE hModule;

    strncpy( buffer, name, 15 );
    buffer[15] = '\0';
    len = strlen(buffer);
    AnsiUpper( buffer );

    hModule = hFirstModule;
    while( hModule )
    {
        NE_MODULE *pModule = (NE_MODULE *)GlobalLock( hModule );
        char *pname = (char *)pModule + pModule->name_table;
        if (((BYTE)*pname == len) && !memcmp( pname+1, buffer, len )) break;
        hModule = pModule->next;
    }
    dprintf_module( stddeb, "GetModuleHandle('%s'): returning %04x\n",
                    name, hModule );
    return hModule;
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
    return strlen(lpFileName);
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
