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
#include "windows.h"
#include "class.h"
#include "file.h"
#include "global.h"
#include "heap.h"
#include "hook.h"
#include "module.h"
#include "neexe.h"
#include "process.h"
#include "resource.h"
#include "selectors.h"
#include "stackframe.h"
#include "task.h"
#include "toolhelp.h"
#include "stddebug.h"
#include "debug.h"
#include "callback.h"

extern HINSTANCE16 PE_LoadModule( HFILE32 hf, OFSTRUCT *ofs, LOADPARAMS* params );

static HMODULE16 hFirstModule = 0;
static HMODULE16 hCachedModule = 0;  /* Module cached by MODULE_OpenFile */


/***********************************************************************
 *           MODULE_GetPtr
 */
NE_MODULE *MODULE_GetPtr( HMODULE32 hModule )
{
    HMODULE16 hnd =MODULE_HANDLEtoHMODULE16(hModule);

    if (!hnd)
    	return NULL;
    return (NE_MODULE*)GlobalLock16(hnd);
}

/***********************************************************************
 *           MODULE_HANDLEtoHMODULE16
 */
HMODULE16
MODULE_HANDLEtoHMODULE16(HANDLE32 handle) {
    NE_MODULE	*pModule;

    if (HIWORD(handle))
    {
	/* this is a HMODULE32 */

        /* walk the list looking for the correct startaddress */
    	pModule = (NE_MODULE *)GlobalLock16( hFirstModule );
	while (pModule)
        {
            if (pModule->module32 == handle) return pModule->self;
            pModule = (NE_MODULE*)GlobalLock16(pModule->next);
	}
	return 0;
    }
    return GetExePtr(handle);
}

/***********************************************************************
 *           MODULE_HANDLEtoHMODULE32
 * return HMODULE32, if possible, HMODULE16 otherwise
 */
HMODULE32
MODULE_HANDLEtoHMODULE32(HANDLE32 handle) {
    NE_MODULE *pModule;

    if (HIWORD(handle))
    	return (HMODULE32)handle;
    else {
    	handle = GetExePtr(handle);
	if (!handle)
	    return 0;
    	pModule = (NE_MODULE *)GlobalLock16( handle );
	if (!pModule)
	    return 0;
	
	if (pModule->module32) return pModule->module32;
	return handle;
    }
}

/***********************************************************************
 *           MODULE_DumpModule
 */
void MODULE_DumpModule( HMODULE32 hModule )
{
    int i, ordinal;
    SEGTABLEENTRY *pSeg;
    BYTE *pstr;
    WORD *pword;
    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr( hModule )))
    {
        fprintf( stderr, "**** %04x is not a module handle\n", hModule );
        return;
    }

      /* Dump the module info */

    printf( "Module %04x:\n", hModule );
    printf( "count=%d flags=%04x heap=%d stack=%d\n",
            pModule->count, pModule->flags,
            pModule->heap_size, pModule->stack_size );
    printf( "cs:ip=%04x:%04x ss:sp=%04x:%04x ds=%04x nb seg=%d modrefs=%d\n",
           pModule->cs, pModule->ip, pModule->ss, pModule->sp, pModule->dgroup,
           pModule->seg_count, pModule->modref_count );
    printf( "os_flags=%d swap_area=%d version=%04x\n",
            pModule->os_flags, pModule->min_swap_area,
            pModule->expected_version );
    if (pModule->flags & NE_FFLAGS_WIN32)
        printf( "PE module=%08x\n", pModule->module32 );

      /* Dump the file info */

    printf( "Filename: '%s'\n", NE_MODULE_NAME(pModule) );

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
	    printf( "%d: %04x -> '%s'\n", i, *pword,
		    MODULE_GetModuleName(*pword));
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
        pstr = (char *)GlobalLock16( pModule->nrname_handle );
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
 *           MODULE_WalkModules
 *
 * Walk the module list and print the modules.
 */
void MODULE_WalkModules(void)
{
    HMODULE16 hModule = hFirstModule;
    fprintf( stderr, "Module Flags Name\n" );
    while (hModule)
    {
        NE_MODULE *pModule = MODULE_GetPtr( hModule );
        if (!pModule)
        {
            fprintf( stderr, "**** Bad module %04x in list\n", hModule );
            return;
        }
        fprintf( stderr, " %04x  %04x  %.*s\n", hModule, pModule->flags,
                 *((char *)pModule + pModule->name_table),
                 (char *)pModule + pModule->name_table + 1 );
        hModule = pModule->next;
    }
}


/***********************************************************************
 *           MODULE_OpenFile
 */
int MODULE_OpenFile( HMODULE32 hModule )
{
    NE_MODULE *pModule;
    DOS_FULL_NAME full_name;
    char *name;

    static int cachedfd = -1;

    hModule = MODULE_HANDLEtoHMODULE16(hModule);
    dprintf_module( stddeb, "MODULE_OpenFile(%04x) cache: mod=%04x fd=%d\n",
                    hModule, hCachedModule, cachedfd );
    if (!(pModule = MODULE_GetPtr( hModule ))) return -1;
    if (hCachedModule == hModule) return cachedfd;
    close( cachedfd );
    hCachedModule = hModule;
    name = NE_MODULE_NAME( pModule );
    if (!DOSFS_GetFullName( name, TRUE, &full_name ) ||
        (cachedfd = open( full_name.long_name, O_RDONLY )) == -1)
        fprintf( stderr, "MODULE_OpenFile: can't open file '%s' for module %04x\n",
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
 *           MODULE_AllocateSegment (WPROCS.26)
 */

DWORD WINAPI MODULE_AllocateSegment(WORD wFlags, WORD wSize, WORD wElem)
{
    WORD size = wSize << wElem;
    HANDLE16 hMem = GlobalAlloc16( MODULE_Ne2MemFlags(wFlags), size);
    return MAKELONG( hMem, GlobalHandleToSel(hMem) );
}

/***********************************************************************
 *           MODULE_CreateSegments
 */
static BOOL32 MODULE_CreateSegments( HMODULE32 hModule )
{
    SEGTABLEENTRY *pSegment;
    NE_MODULE *pModule;
    int i, minsize;

    if (!(pModule = MODULE_GetPtr( hModule ))) return FALSE;
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


/***********************************************************************
 *           MODULE_GetInstance
 */
HINSTANCE16 MODULE_GetInstance( HMODULE32 hModule )
{
    SEGTABLEENTRY *pSegment;
    NE_MODULE *pModule;
    
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (pModule->dgroup == 0) return hModule;

    pSegment = NE_SEG_TABLE( pModule ) + pModule->dgroup - 1;
    
    return pSegment->selector;
}


/***********************************************************************
 *           MODULE_CreateInstance
 */
HINSTANCE16 MODULE_CreateInstance( HMODULE16 hModule, LOADPARAMS *params )
{
    SEGTABLEENTRY *pSegment;
    NE_MODULE *pModule;
    int minsize;
    HINSTANCE16 hNewInstance, hPrevInstance;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
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
 *           MODULE_CreateDummyModule
 *
 * Create a dummy NE module for Win32 or Winelib.
 */
HMODULE32 MODULE_CreateDummyModule( const OFSTRUCT *ofs )
{
    HMODULE32 hModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    char *pStr,*s;
    int len;
    const char* basename;

    INT32 of_size = sizeof(OFSTRUCT) - sizeof(ofs->szPathName)
                    + strlen(ofs->szPathName) + 1;
    INT32 size = sizeof(NE_MODULE) +
                 /* loaded file info */
                 of_size +
                 /* segment table: DS,CS */
                 2 * sizeof(SEGTABLEENTRY) +
                 /* name table */
                 9 +
                 /* several empty tables */
                 8;

    hModule = GlobalAlloc16( GMEM_MOVEABLE | GMEM_ZEROINIT, size );
    if (!hModule) return (HMODULE32)11;  /* invalid exe */

    FarSetOwner( hModule, hModule );
    pModule = (NE_MODULE *)GlobalLock16( hModule );

    /* Set all used entries */
    pModule->magic            = IMAGE_OS2_SIGNATURE;
    pModule->count            = 1;
    pModule->next             = 0;
    pModule->flags            = 0;
    pModule->dgroup           = 1;
    pModule->ss               = 1;
    pModule->cs               = 2;
    pModule->heap_size        = 0xe000;
    pModule->stack_size       = 0x1000;
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
    pModule->seg_table = pModule->dgroup_entry = (int)pSegment - (int)pModule;
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
    basename = strrchr(ofs->szPathName,'\\');
    if (!basename) basename = ofs->szPathName;
    else basename++;
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

    MODULE_RegisterModule( pModule );
    return hModule;
}


/***********************************************************************
 *           MODULE_LoadExeHeader
 */
static HMODULE32 MODULE_LoadExeHeader( HFILE32 hFile, OFSTRUCT *ofs )
{
    IMAGE_DOS_HEADER mz_header;
    IMAGE_OS2_HEADER ne_header;
    int size;
    HMODULE32 hModule;
    NE_MODULE *pModule;
    BYTE *pData;
    char *buffer, *fastload = NULL;
    int fastload_offset = 0, fastload_length = 0;

  /* Read a block from either the file or the fast-load area. */
#define READ(offset,size,buffer) \
       ((fastload && ((offset) >= fastload_offset) && \
         ((offset)+(size) <= fastload_offset+fastload_length)) ? \
        (memcpy( buffer, fastload+(offset)-fastload_offset, (size) ), TRUE) : \
        (_llseek32( hFile, (offset), SEEK_SET), \
         _lread32( hFile, (buffer), (size) ) == (size)))

    _llseek32( hFile, 0, SEEK_SET );
    if ((_lread32(hFile,&mz_header,sizeof(mz_header)) != sizeof(mz_header)) ||
        (mz_header.e_magic != IMAGE_DOS_SIGNATURE))
        return (HMODULE32)11;  /* invalid exe */

    _llseek32( hFile, mz_header.e_lfanew, SEEK_SET );
    if (_lread32( hFile, &ne_header, sizeof(ne_header) ) != sizeof(ne_header))
        return (HMODULE32)11;  /* invalid exe */

    if (ne_header.ne_magic == IMAGE_NT_SIGNATURE) return (HMODULE32)21;  /* win32 exe */
    if (ne_header.ne_magic != IMAGE_OS2_SIGNATURE) return (HMODULE32)11;  /* invalid exe */

    if (ne_header.ne_magic == IMAGE_OS2_SIGNATURE_LX) {
      fprintf(stderr, "Sorry, this is an OS/2 linear executable (LX) file !\n");
      return (HMODULE32)12;
    }
    /* We now have a valid NE header */

    size = sizeof(NE_MODULE) +
             /* loaded file info */
           sizeof(OFSTRUCT)-sizeof(ofs->szPathName)+strlen(ofs->szPathName)+1+
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

    hModule = GlobalAlloc16( GMEM_MOVEABLE | GMEM_ZEROINIT, size );
    if (!hModule) return (HMODULE32)11;  /* invalid exe */
    FarSetOwner( hModule, hModule );
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    memcpy( pModule, &ne_header, sizeof(ne_header) );
    pModule->count = 0;
    pModule->module32 = 0;
    pModule->self = hModule;
    pModule->self_loading_sel = 0;
    pData = (BYTE *)(pModule + 1);

    /* Clear internal Wine flags in case they are set in the EXE file */

    pModule->flags &= ~(NE_FFLAGS_BUILTIN | NE_FFLAGS_WIN32);

    /* Read the fast-load area */

    if (ne_header.additional_flags & NE_AFLAGS_FASTLOAD)
    {
        fastload_offset=ne_header.fastload_offset<<ne_header.align_shift_count;
        fastload_length=ne_header.fastload_length<<ne_header.align_shift_count;
        dprintf_module( stddeb, "Using fast-load area offset=%x len=%d\n",
                        fastload_offset, fastload_length );
        if ((fastload = HeapAlloc( SystemHeap, 0, fastload_length )) != NULL)
        {
            _llseek32( hFile, fastload_offset, SEEK_SET);
            if (_lread32(hFile, fastload, fastload_length) != fastload_length)
            {
                HeapFree( SystemHeap, 0, fastload );
                fprintf(stderr, "Error reading fast-load area !\n");
                fastload = NULL;
            }
        }
    }

    /* Store the filename information */

    pModule->fileinfo = (int)pData - (int)pModule;
    size = sizeof(OFSTRUCT)-sizeof(ofs->szPathName)+strlen(ofs->szPathName)+1;
    memcpy( pData, ofs, size );
    ((OFSTRUCT *)pData)->cBytes = size - 1;
    pData += size;

    /* Get the segment table */

    pModule->seg_table = (int)pData - (int)pModule;
    buffer = HeapAlloc( SystemHeap, 0, ne_header.n_segment_tab *
                                      sizeof(struct ne_segment_table_entry_s));
    if (buffer)
    {
        int i;
        struct ne_segment_table_entry_s *pSeg;

        if (!READ( mz_header.e_lfanew + ne_header.segment_tab_offset,
             ne_header.n_segment_tab * sizeof(struct ne_segment_table_entry_s),
             buffer ))
        {
            HeapFree( SystemHeap, 0, buffer );
            if (fastload) HeapFree( SystemHeap, 0, fastload );
            GlobalFree16( hModule );
            return (HMODULE32)11;  /* invalid exe */
        }
        pSeg = (struct ne_segment_table_entry_s *)buffer;
        for (i = ne_header.n_segment_tab; i > 0; i--, pSeg++)
        {
            memcpy( pData, pSeg, sizeof(*pSeg) );
            pData += sizeof(SEGTABLEENTRY);
        }
        HeapFree( SystemHeap, 0, buffer );
    }
    else
    {
        if (fastload) HeapFree( SystemHeap, 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE32)11;  /* invalid exe */
    }

    /* Get the resource table */

    if (ne_header.resource_tab_offset < ne_header.rname_tab_offset)
    {
        pModule->res_table = (int)pData - (int)pModule;
        if (!READ(mz_header.e_lfanew + ne_header.resource_tab_offset,
                  ne_header.rname_tab_offset - ne_header.resource_tab_offset,
                  pData )) return (HMODULE32)11;  /* invalid exe */
        pData += ne_header.rname_tab_offset - ne_header.resource_tab_offset;
	NE_InitResourceHandler( hModule );
    }
    else pModule->res_table = 0;  /* No resource table */

    /* Get the resident names table */

    pModule->name_table = (int)pData - (int)pModule;
    if (!READ( mz_header.e_lfanew + ne_header.rname_tab_offset,
               ne_header.moduleref_tab_offset - ne_header.rname_tab_offset,
               pData ))
    {
        if (fastload) HeapFree( SystemHeap, 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE32)11;  /* invalid exe */
    }
    pData += ne_header.moduleref_tab_offset - ne_header.rname_tab_offset;

    /* Get the module references table */

    if (ne_header.n_mod_ref_tab > 0)
    {
        pModule->modref_table = (int)pData - (int)pModule;
        if (!READ( mz_header.e_lfanew + ne_header.moduleref_tab_offset,
                  ne_header.n_mod_ref_tab * sizeof(WORD),
                  pData ))
        {
            if (fastload) HeapFree( SystemHeap, 0, fastload );
            GlobalFree16( hModule );
            return (HMODULE32)11;  /* invalid exe */
        }
        pData += ne_header.n_mod_ref_tab * sizeof(WORD);
    }
    else pModule->modref_table = 0;  /* No module references */

    /* Get the imported names table */

    pModule->import_table = (int)pData - (int)pModule;
    if (!READ( mz_header.e_lfanew + ne_header.iname_tab_offset, 
               ne_header.entry_tab_offset - ne_header.iname_tab_offset,
               pData ))
    {
        if (fastload) HeapFree( SystemHeap, 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE32)11;  /* invalid exe */
    }
    pData += ne_header.entry_tab_offset - ne_header.iname_tab_offset;

    /* Get the entry table */

    pModule->entry_table = (int)pData - (int)pModule;
    if (!READ( mz_header.e_lfanew + ne_header.entry_tab_offset,
               ne_header.entry_tab_length,
               pData ))
    {
        if (fastload) HeapFree( SystemHeap, 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE32)11;  /* invalid exe */
    }
    pData += ne_header.entry_tab_length;

    /* Free the fast-load area */

#undef READ
    if (fastload) HeapFree( SystemHeap, 0, fastload );

    /* Get the non-resident names table */

    if (ne_header.nrname_tab_length)
    {
        pModule->nrname_handle = GLOBAL_Alloc( 0, ne_header.nrname_tab_length,
                                               hModule, FALSE, FALSE, FALSE );
        if (!pModule->nrname_handle)
        {
            GlobalFree16( hModule );
            return (HMODULE32)11;  /* invalid exe */
        }
        buffer = GlobalLock16( pModule->nrname_handle );
        _llseek32( hFile, ne_header.nrname_tab_offset, SEEK_SET );
        if (_lread32( hFile, buffer, ne_header.nrname_tab_length )
              != ne_header.nrname_tab_length)
        {
            GlobalFree16( pModule->nrname_handle );
            GlobalFree16( hModule );
            return (HMODULE32)11;  /* invalid exe */
        }
    }
    else pModule->nrname_handle = 0;

    /* Allocate a segment for the implicitly-loaded DLLs */

    if (pModule->modref_count)
    {
        pModule->dlls_to_init = GLOBAL_Alloc(GMEM_ZEROINIT,
                                    (pModule->modref_count+1)*sizeof(HMODULE32),
                                    hModule, FALSE, FALSE, FALSE );
        if (!pModule->dlls_to_init)
        {
            if (pModule->nrname_handle) GlobalFree16( pModule->nrname_handle );
            GlobalFree16( hModule );
            return (HMODULE32)11;  /* invalid exe */
        }
    }
    else pModule->dlls_to_init = 0;

    MODULE_RegisterModule( pModule );
    return hModule;
}


/***********************************************************************
 *           MODULE_GetOrdinal
 *
 * Lookup the ordinal for a given name.
 */
WORD MODULE_GetOrdinal( HMODULE32 hModule, const char *name )
{
    unsigned char buffer[256], *cpnt;
    BYTE len;
    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;

    dprintf_module( stddeb, "MODULE_GetOrdinal(%04x,'%s')\n",
                    hModule, name );

      /* First handle names of the form '#xxxx' */

    if (name[0] == '#') return atoi( name + 1 );

      /* Now copy and uppercase the string */

    strcpy( buffer, name );
    CharUpper32A( buffer );
    len = strlen( buffer );

      /* First search the resident names */

    cpnt = (char *)pModule + pModule->name_table;

      /* Skip the first entry (module name) */
    cpnt += *cpnt + 1 + sizeof(WORD);
    while (*cpnt)
    {
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
    cpnt = (char *)GlobalLock16( pModule->nrname_handle );

      /* Skip the first entry (module description string) */
    cpnt += *cpnt + 1 + sizeof(WORD);
    while (*cpnt)
    {
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
FARPROC16 MODULE_GetEntryPoint( HMODULE32 hModule, WORD ordinal )
{
    NE_MODULE *pModule;
    WORD curOrdinal = 1;
    BYTE *p;
    WORD sel, offset;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;

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
    return (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( sel, offset );
}


/***********************************************************************
 *           EntryAddrProc   (WPROCS.27)
 */
FARPROC16 WINAPI EntryAddrProc( HMODULE16 hModule, WORD ordinal )
{
    return MODULE_GetEntryPoint( hModule, ordinal );
}


/***********************************************************************
 *           MODULE_SetEntryPoint
 *
 * Change the value of an entry point. Use with caution!
 * It can only change the offset value, not the selector.
 */
BOOL16 MODULE_SetEntryPoint( HMODULE32 hModule, WORD ordinal, WORD offset )
{
    NE_MODULE *pModule;
    WORD curOrdinal = 1;
    BYTE *p;

    if (!(pModule = MODULE_GetPtr( hModule ))) return FALSE;

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
        extern LRESULT ColorDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT FileOpenDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT FileSaveDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT FindTextDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT PrintDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT PrintSetupDlgProc(HWND16,UINT16,WPARAM16,LPARAM);
        extern LRESULT ReplaceTextDlgProc(HWND16,UINT16,WPARAM16,LPARAM);

        if (!strcmp(name,"ColorDlgProc"))
            return (FARPROC16)ColorDlgProc;
        if (!strcmp(name,"FileOpenDlgProc"))
            return (FARPROC16)FileOpenDlgProc;
        if (!strcmp(name,"FileSaveDlgProc"))
            return (FARPROC16)FileSaveDlgProc;
        if (!strcmp(name,"FindTextDlgProc"))
            return (FARPROC16)FindTextDlgProc;
        if (!strcmp(name,"PrintDlgProc"))
            return (FARPROC16)PrintDlgProc;
        if (!strcmp(name,"PrintSetupDlgProc"))
            return (FARPROC16)PrintSetupDlgProc;
        if (!strcmp(name,"ReplaceTextDlgProc"))
            return (FARPROC16)ReplaceTextDlgProc;
        fprintf(stderr,"warning: No mapping for %s(), add one in library/miscstubs.c\n",name);
        assert( FALSE );
        return NULL;
    }
    else
    {
        WORD ordinal;
        static HMODULE32 hModule = 0;

        if (!hModule) hModule = GetModuleHandle16( "WPROCS" );
        ordinal = MODULE_GetOrdinal( hModule, name );
        if (!(ret = MODULE_GetEntryPoint( hModule, ordinal )))
        {            
            fprintf( stderr, "GetWndProc16: %s not found\n", name );
            assert( FALSE );
        }
    }
    return ret;
}


/***********************************************************************
 *           MODULE_GetModuleName
 */
LPSTR MODULE_GetModuleName( HMODULE32 hModule )
{
    NE_MODULE *pModule;
    BYTE *p, len;
    static char buffer[10];

    if (!(pModule = MODULE_GetPtr( hModule ))) return NULL;
    p = (BYTE *)pModule + pModule->name_table;
    len = MIN( *p, 8 );
    memcpy( buffer, p + 1, len );
    buffer[len] = '\0';
    return buffer;
}


/**********************************************************************
 *           MODULE_RegisterModule
 */
void MODULE_RegisterModule( NE_MODULE *pModule )
{
    pModule->next = hFirstModule;
    hFirstModule = pModule->self;
}


/**********************************************************************
 *	    MODULE_FindModule
 *
 * Find a module from a path name.
 */
HMODULE32 MODULE_FindModule( LPCSTR path )
{
    HMODULE32 hModule = hFirstModule;
    LPCSTR filename, dotptr, modulepath, modulename;
    BYTE len, *name_table;

    if (!(filename = strrchr( path, '\\' ))) filename = path;
    else filename++;
    if ((dotptr = strrchr( filename, '.' )) != NULL)
        len = (BYTE)(dotptr - filename);
    else len = strlen( filename );

    while(hModule)
    {
        NE_MODULE *pModule = MODULE_GetPtr( hModule );
        if (!pModule) break;
        modulepath = NE_MODULE_NAME(pModule);
        if (!(modulename = strrchr( modulepath, '\\' )))
            modulename = modulepath;
        else modulename++;
        if (!lstrcmpi32A( modulename, filename )) return hModule;

        name_table = (BYTE *)pModule + pModule->name_table;
        if ((*name_table == len) && !lstrncmpi32A(filename, name_table+1, len))
            return hModule;
        hModule = pModule->next;
    }
    return 0;
}


/**********************************************************************
 *	    MODULE_CallWEP
 *
 * Call a DLL's WEP, allowing it to shut down.
 * FIXME: we always pass the WEP WEP_FREE_DLL, never WEP_SYSTEM_EXIT
 */
static BOOL16 MODULE_CallWEP( HMODULE16 hModule )
{
    FARPROC16 WEP = (FARPROC16)0;
    WORD ordinal = MODULE_GetOrdinal( hModule, "WEP" );

    if (ordinal) WEP = MODULE_GetEntryPoint( hModule, ordinal );
    if (!WEP)
    {
	dprintf_module( stddeb, "module %04x doesn't have a WEP\n", hModule );
	return FALSE;
    }
    return Callbacks->CallWindowsExitProc( WEP, WEP_FREE_DLL );
}


/**********************************************************************
 *	    MODULE_FreeModule
 *
 * Remove a module from memory.
 */
BOOL16 MODULE_FreeModule( HMODULE32 hModule, TDB* pTaskContext )
{
    HMODULE16 *hPrevModule;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    HMODULE16 *pModRef;
    int i;

    if (!(pModule = MODULE_GetPtr( hModule ))) return FALSE;
    hModule = pModule->self;

    if (((INT16)(--pModule->count)) > 0 ) return TRUE;
    else pModule->count = 0;

    if (pModule->flags & NE_FFLAGS_BUILTIN)
        return FALSE;  /* Can't free built-in module */

    if (pModule->flags & NE_FFLAGS_LIBMODULE) 
    {
	MODULE_CallWEP( hModule );

	/* Free the objects owned by the DLL module */

	if( pTaskContext && pTaskContext->userhandler )
	{
            pTaskContext->userhandler( hModule, USIG_DLL_UNLOAD, 0,
                                       pTaskContext->hInstance,
                                       pTaskContext->hQueue );
	}
    }
    /* Clear magic number just in case */

    pModule->magic = pModule->self = 0;

      /* Remove it from the linked list */

    hPrevModule = &hFirstModule;
    while (*hPrevModule && (*hPrevModule != hModule))
    {
        hPrevModule = &(MODULE_GetPtr( *hPrevModule ))->next;
    }
    if (*hPrevModule) *hPrevModule = pModule->next;

      /* Free all the segments */

    pSegment = NE_SEG_TABLE( pModule );
    for (i = 1; i <= pModule->seg_count; i++, pSegment++)
    {
        GlobalFree16( pSegment->selector );
    }

      /* Free the referenced modules */

    pModRef = (HMODULE16*)NE_MODULE_TABLE( pModule );
    for (i = 0; i < pModule->modref_count; i++, pModRef++)
    {
        FreeModule16( *pModRef );
    }

      /* Free the module storage */

    if (pModule->nrname_handle) GlobalFree16( pModule->nrname_handle );
    if (pModule->dlls_to_init) GlobalFree16( pModule->dlls_to_init );
    GlobalFree16( hModule );

      /* Remove module from cache */

    if (hCachedModule == hModule) hCachedModule = 0;

    return TRUE;
}


/**********************************************************************
 *	    MODULE_Load
 *
 * Implementation of LoadModule()
 */
HINSTANCE16 MODULE_Load( LPCSTR name, LPVOID paramBlock, UINT16 uFlags)
{
    HMODULE32 hModule;
    HINSTANCE16 hInstance, hPrevInstance;
    NE_MODULE *pModule;
    LOADPARAMS *params = (LOADPARAMS *)paramBlock;
    OFSTRUCT ofs;
    HFILE32 hFile;

    if (__winelib)
    {
        lstrcpyn32A( ofs.szPathName, name, sizeof(ofs.szPathName) );
        if ((hModule = MODULE_CreateDummyModule( &ofs )) < 32) return hModule;
        pModule = (NE_MODULE *)GlobalLock16( hModule );
        hPrevInstance = 0;
        hInstance = MODULE_CreateInstance( hModule, params );
    }
    else
    {
        hModule = MODULE_FindModule( name );

        if (!hModule)  /* We have to load the module */
        {
            /* Try to load the built-in first if not disabled */
            if ((hModule = BUILTIN_LoadModule( name, FALSE )))
                return MODULE_HANDLEtoHMODULE16( hModule );
            
            if ((hFile = OpenFile32( name, &ofs, OF_READ )) == HFILE_ERROR32)
            {
                /* Now try the built-in even if disabled */
                if ((hModule = BUILTIN_LoadModule( name, TRUE )))
                {
                    fprintf( stderr, "Warning: could not load Windows DLL '%s', using built-in module.\n", name );
                    return MODULE_HANDLEtoHMODULE16( hModule );
                }
                return 2;  /* File not found */
            }

            /* Create the module structure */

            hModule = MODULE_LoadExeHeader( hFile, &ofs );
            if (hModule < 32)
            {
                if (hModule == 21)
                    hModule = PE_LoadModule( hFile, &ofs, paramBlock );
                else _lclose32( hFile );

                if (hModule < 32)
                    fprintf( stderr, "LoadModule: can't load '%s', error=%d\n",
                             name, hModule );
                return hModule;
            }
            _lclose32( hFile );
            pModule = MODULE_GetPtr( hModule );
            pModule->flags |= uFlags; /* stamp implicitly loaded modules */

            /* Allocate the segments for this module */

            MODULE_CreateSegments( hModule );
            hPrevInstance = 0;
            hInstance = MODULE_CreateInstance(hModule,(LOADPARAMS*)paramBlock);

            /* Load the referenced DLLs */

            if (!NE_LoadDLLs( pModule )) return 2;  /* File not found */

            /* Load the segments */

            NE_LoadAllSegments( pModule );

            /* Fixup the functions prologs */

            NE_FixupPrologs( pModule );

            /* Make sure the usage count is 1 on the first loading of  */
            /* the module, even if it contains circular DLL references */

            pModule->count = 1;

            /* Call initialization rountines for all loaded DLLs. Note that
             * when we load implicitly linked DLLs this will be done by InitTask().
             */

            if ((pModule->flags & (NE_FFLAGS_LIBMODULE | NE_FFLAGS_IMPLICIT)) ==
                                       NE_FFLAGS_LIBMODULE )
                NE_InitializeDLLs( hModule );
        }
        else /* module is already loaded, just create a new data segment if it's a task */
        {
            pModule = MODULE_GetPtr( hModule );
            hPrevInstance = MODULE_GetInstance( hModule );
            hInstance = MODULE_CreateInstance( hModule, params );
            if (hInstance != hPrevInstance)  /* not a library */
                NE_LoadSegment( pModule, pModule->dgroup );
            pModule->count++;
        }
    } /* !winelib */

    /* Create a task for this instance */

    if (!(pModule->flags & NE_FFLAGS_LIBMODULE) && (paramBlock != (LPVOID)-1))
    {
	HTASK16 hTask;
        WORD	showcmd;

	pModule->flags |= NE_FFLAGS_GUI;

	/* PowerPoint passes NULL as showCmd */
	if (params->showCmd)
		showcmd = *((WORD *)PTR_SEG_TO_LIN(params->showCmd)+1);
	else
		showcmd = 0; /* FIXME: correct */

        hTask = TASK_CreateTask( hModule, hInstance, hPrevInstance,
                                 params->hEnvironment,
                                (LPSTR)PTR_SEG_TO_LIN( params->cmdLine ),
                                 showcmd );

	if( hTask && TASK_GetNextTask(hTask)) Yield16();
    }

    return hInstance;
}


/**********************************************************************
 *	    LoadModule16    (KERNEL.45)
 */
HINSTANCE16 LoadModule16( LPCSTR name, LPVOID paramBlock )
{
    return MODULE_Load( name, paramBlock, 0 );
}

/**********************************************************************
 *	    LoadModule32    (KERNEL32)
 * FIXME: check this function
 */
DWORD LoadModule32( LPCSTR name, LPVOID paramBlock ) 
{
    return MODULE_Load( name, paramBlock, 0 );
}


/**********************************************************************
 *	    FreeModule16    (KERNEL.46)
 */
BOOL16 WINAPI FreeModule16( HMODULE16 hModule )
{
    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr( hModule ))) return FALSE;
    dprintf_module( stddeb, "FreeModule16: %s count %d\n", 
		    MODULE_GetModuleName(hModule), pModule->count );

    return MODULE_FreeModule( hModule, GlobalLock16(GetCurrentTask()) );
}


/**********************************************************************
 *	    GetModuleHandle16    (KERNEL.47)
 */
HMODULE16 WINAPI WIN16_GetModuleHandle( SEGPTR name )
{
    if (HIWORD(name) == 0) return MODULE_HANDLEtoHMODULE16( (HINSTANCE16)name );
    return MODULE_FindModule( PTR_SEG_TO_LIN(name) );
}

HMODULE16 WINAPI GetModuleHandle16( LPCSTR name )
{
    return MODULE_FindModule( name );
}

/***********************************************************************
 *              GetModuleHandle         (KERNEL32.237)
 */
HMODULE32 WINAPI GetModuleHandle32A(LPCSTR module)
{
    HMODULE32	hModule;

    dprintf_win32(stddeb, "GetModuleHandleA: %s\n", module ? module : "NULL");
/* Freecell uses the result of GetModuleHandleA(0) as the hInstance in
all calls to e.g. CreateWindowEx. */
    if (module == NULL) {
	TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
	hModule = pTask->hInstance;
    } else
	hModule = MODULE_FindModule(module);
    return MODULE_HANDLEtoHMODULE32(hModule);
}

HMODULE32 WINAPI GetModuleHandle32W(LPCWSTR module)
{
    HMODULE32 hModule;
    LPSTR modulea = HEAP_strdupWtoA( GetProcessHeap(), 0, module );
    hModule = GetModuleHandle32A( modulea );
    HeapFree( GetProcessHeap(), 0, modulea );
    return hModule;
}


/**********************************************************************
 *	    GetModuleUsage    (KERNEL.48)
 */
INT16 WINAPI GetModuleUsage( HINSTANCE16 hModule )
{
    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    dprintf_module( stddeb, "GetModuleUsage(%04x): returning %d\n",
                    hModule, pModule->count );
    return pModule->count;
}


/**********************************************************************
 *	    GetModuleFileName16    (KERNEL.49)
 */
INT16 WINAPI GetModuleFileName16( HINSTANCE16 hModule, LPSTR lpFileName,
                                  INT16 nSize )
{
    NE_MODULE *pModule;

    if (!hModule) hModule = GetCurrentTask();
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    lstrcpyn32A( lpFileName, NE_MODULE_NAME(pModule), nSize );
    dprintf_module( stddeb, "GetModuleFileName16: %s\n", lpFileName );
    return strlen(lpFileName);
}


/***********************************************************************
 *              GetModuleFileName32A      (KERNEL32.235)
 */
DWORD WINAPI GetModuleFileName32A( HMODULE32 hModule, LPSTR lpFileName,
                                   DWORD size )
{                   
    NE_MODULE *pModule;
           
    if (!hModule)
    {
        TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
        hModule = pTask->hInstance;
    }
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    lstrcpyn32A( lpFileName, NE_MODULE_NAME(pModule), size );
    dprintf_module( stddeb, "GetModuleFileName32A: %s\n", lpFileName );
    return strlen(lpFileName);
}                   
 

/***********************************************************************
 *              GetModuleFileName32W      (KERNEL32.236)
 */
DWORD WINAPI GetModuleFileName32W( HMODULE32 hModule, LPWSTR lpFileName,
                                   DWORD size )
{
    LPSTR fnA = (char*)HeapAlloc( GetProcessHeap(), 0, size );
    DWORD res = GetModuleFileName32A( hModule, fnA, size );
    lstrcpynAtoW( lpFileName, fnA, size );
    HeapFree( GetProcessHeap(), 0, fnA );
    return res;
}


/**********************************************************************
 *	    GetModuleName    (KERNEL.27)
 */
BOOL16 WINAPI GetModuleName( HINSTANCE16 hinst, LPSTR buf, INT16 nSize )
{
    LPSTR name = MODULE_GetModuleName(hinst);

    if (!name) return FALSE;
    lstrcpyn32A( buf, name, nSize );
    return TRUE;
}


/***********************************************************************
 *           LoadLibraryEx32W   (KERNEL.513)
 * FIXME
 */
HMODULE32 WINAPI LoadLibraryEx32W16( LPCSTR libname, HANDLE16 hf,
                                       DWORD flags )
{
    fprintf(stderr,"LoadLibraryEx32W(%s,%d,%08lx)\n",libname,hf,flags);
    return LoadLibraryEx32A(libname,hf,flags);
}

/***********************************************************************
 *           LoadLibraryEx32A   (KERNEL32)
 */
HMODULE32 WINAPI LoadLibraryEx32A(LPCSTR libname,HFILE32 hfile,DWORD flags)
{
    HMODULE32 hmod;
    
    hmod = PE_LoadLibraryEx32A(libname,hfile,flags);
    if (hmod <= 32) {
	char buffer[256];

	strcpy( buffer, libname );
	strcat( buffer, ".dll" );
	hmod = PE_LoadLibraryEx32A(buffer,hfile,flags);
    }
    /* initialize all DLLs, which haven't been initialized yet. */
    PE_InitializeDLLs( PROCESS_Current(), DLL_PROCESS_ATTACH, NULL);
    return hmod;
}

/***********************************************************************
 *           LoadLibraryA         (KERNEL32)
 */
HMODULE32 WINAPI LoadLibrary32A(LPCSTR libname) {
	return LoadLibraryEx32A(libname,0,0);
}

/***********************************************************************
 *           LoadLibraryW         (KERNEL32)
 */
HMODULE32 WINAPI LoadLibrary32W(LPCWSTR libnameW)
{
    return LoadLibraryEx32W(libnameW,0,0);
}

/***********************************************************************
 *           LoadLibraryExW       (KERNEL32)
 */
HMODULE32 WINAPI LoadLibraryEx32W(LPCWSTR libnameW,HFILE32 hfile,DWORD flags)
{
    LPSTR libnameA = HEAP_strdupWtoA( GetProcessHeap(), 0, libnameW );
    HMODULE32 ret = LoadLibraryEx32A( libnameA , hfile, flags );

    HeapFree( GetProcessHeap(), 0, libnameA );
    return ret;
}

/***********************************************************************
 *           FreeLibrary
 */
BOOL32 WINAPI FreeLibrary32(HINSTANCE32 hLibModule)
{
	dprintf_module(stddeb,"FreeLibrary: hLibModule: %08x\n", hLibModule);
	return MODULE_FreeModule(hLibModule, 
	                         GlobalLock16(GetCurrentTask()) );
}


/***********************************************************************
 *           LoadLibrary   (KERNEL.95)
 */
HINSTANCE16 WINAPI LoadLibrary16( LPCSTR libname )
{
    HINSTANCE16 handle;

    if (__winelib)
    {
        fprintf( stderr, "LoadLibrary not supported in Winelib\n" );
        return 0;
    }
    dprintf_module( stddeb, "LoadLibrary: (%08x) %s\n", (int)libname, libname);

    handle = MODULE_Load( libname, (LPVOID)-1, 0 );
    if (handle == (HINSTANCE16)2)  /* file not found */
    {
        char buffer[256];
        lstrcpyn32A( buffer, libname, 252 );
        strcat( buffer, ".dll" );
        handle = MODULE_Load( buffer, (LPVOID)-1, 0 );
    }
    return handle;
}


/***********************************************************************
 *           PrivateLoadLibrary       (KERNEL32)
 *
 * FIXME: rough guesswork, don't know what "Private" means
 */
HINSTANCE32 WINAPI PrivateLoadLibrary(LPCSTR libname)
{
        return (HINSTANCE32)LoadLibrary16(libname);
}


/***********************************************************************
 *           FreeLibrary16   (KERNEL.96)
 */
void WINAPI FreeLibrary16( HINSTANCE16 handle )
{
    dprintf_module( stddeb,"FreeLibrary: %04x\n", handle );
    FreeModule16( handle );
}


/***********************************************************************
 *           PrivateFreeLibrary       (KERNEL32)
 *
 * FIXME: rough guesswork, don't know what "Private" means
 */
void WINAPI PrivateFreeLibrary(HINSTANCE32 handle)
{
	FreeLibrary16((HINSTANCE16)handle);
}


/***********************************************************************
 *           WinExec16   (KERNEL.166)
 */
HINSTANCE16 WINAPI WinExec16( LPCSTR lpCmdLine, UINT16 nCmdShow )
{
    return WinExec32( lpCmdLine, nCmdShow );
}


/***********************************************************************
 *           WinExec32   (KERNEL32.566)
 */
HINSTANCE32 WINAPI WinExec32( LPCSTR lpCmdLine, UINT32 nCmdShow )
{
    LOADPARAMS params;
    HGLOBAL16 cmdShowHandle, cmdLineHandle;
    HINSTANCE16 handle = 2;
    WORD *cmdShowPtr;
    char *p, *cmdline, filename[256];
    static int use_load_module = 1;
    int  spacelimit = 0, exhausted = 0;

    if (!lpCmdLine)
        return 2;  /* File not found */
    if (!(cmdShowHandle = GlobalAlloc16( 0, 2 * sizeof(WORD) )))
        return 8;  /* Out of memory */
    if (!(cmdLineHandle = GlobalAlloc16( 0, 2048 )))
    {
        GlobalFree16( cmdShowHandle );
        return 8;  /* Out of memory */
    }

    /* Keep trying to load a file by trying different filenames; e.g.,
       for the cmdline "abcd efg hij", try "abcd" with args "efg hij",
       then "abcd efg" with arg "hij", and finally "abcd efg hij" with
       no args */

    while(!exhausted && handle == 2) {
	int spacecount = 0;

	/* Store nCmdShow */

	cmdShowPtr = (WORD *)GlobalLock16( cmdShowHandle );
	cmdShowPtr[0] = 2;
	cmdShowPtr[1] = nCmdShow;

	/* Build the filename and command-line */

	cmdline = (char *)GlobalLock16( cmdLineHandle );
	lstrcpyn32A(filename, lpCmdLine,
		    sizeof(filename) - 4 /* for extension */);

	/* Keep grabbing characters until end-of-string, tab, or until the
	   number of spaces is greater than the spacelimit */

	for (p = filename; ; p++) {
	    if(*p == ' ') {
		++spacecount;
		if(spacecount > spacelimit) {
		    ++spacelimit;
		    break;
		}
	    }

	    if(*p == '\0' || *p == '\t') {
		exhausted = 1;
		break;
	    }
	}

	if (*p)
	    lstrcpyn32A( cmdline + 1, p + 1, 255 );
	else
	    cmdline[1] = '\0';

	cmdline[0] = strlen( cmdline + 1 );
	*p = '\0';
	/* this is a (hopefully acceptable hack to get the whole
	   commandline for PROCESS_Create
	   we put it after the processed one */
	lstrcpyn32A(cmdline + (unsigned char)cmdline[0] +2,
		    lpCmdLine, 2048 - 256);

	/* Now load the executable file */

	if (use_load_module)
	{
	    /* Winelib: Use LoadModule() only for the program itself */
	    if (__winelib) use_load_module = 0;
	    params.hEnvironment = (HGLOBAL16)SELECTOROF( GetDOSEnvironment() );
	    params.cmdLine  = (SEGPTR)WIN16_GlobalLock16( cmdLineHandle );
	    params.showCmd  = (SEGPTR)WIN16_GlobalLock16( cmdShowHandle );
	    params.reserved = 0;
	    handle = LoadModule16( filename, &params );
	    if (handle == 2)  /* file not found */
	    {
		/* Check that the original file name did not have a suffix */
		p = strrchr(filename, '.');
		/* if there is a '.', check if either \ OR / follow */
		if (!p || strchr(p, '/') || strchr(p, '\\'))
		{
		    p = filename + strlen(filename);
		    strcpy( p, ".exe" );
		    handle = LoadModule16( filename, &params );
		    *p = '\0';  /* Remove extension */
		}
	    }
	}
	else
	    handle = 2; /* file not found */

	if (handle < 32)
	{
	    /* Try to start it as a unix program */
	    if (!fork())
	    {
		/* Child process */
		DOS_FULL_NAME full_name;
		const char *unixfilename = NULL;
		const char *argv[256], **argptr;
		int iconic = (nCmdShow == SW_SHOWMINIMIZED ||
			      nCmdShow == SW_SHOWMINNOACTIVE);

		/* get unixfilename */
		if (strchr(filename, '/') ||
		    strchr(filename, ':') ||
		    strchr(filename, '\\'))
		{
		    if (DOSFS_GetFullName( filename, TRUE, &full_name ))
			unixfilename = full_name.long_name;
		}
		else unixfilename = filename;

		if (unixfilename)
		{
		    /* build argv */
		    argptr = argv;
		    if (iconic) *argptr++ = "-iconic";
		    *argptr++ = unixfilename;
		    p = cmdline + 1;
		    while (1)
		    {
			while (*p && (*p == ' ' || *p == '\t')) *p++ = '\0';
			if (!*p) break;
			*argptr++ = p;
			while (*p && *p != ' ' && *p != '\t') p++;
		    }
		    *argptr++ = 0;

		    /* Execute */
		    execvp(argv[0], (char**)argv);
		}

		/* Failed ! */

		if (__winelib)
		{
		    /* build argv */
		    argptr = argv;
		    *argptr++ = "wine";
		    if (iconic) *argptr++ = "-iconic";
		    *argptr++ = lpCmdLine;
		    *argptr++ = 0;

		    /* Execute */
		    execvp(argv[0] , (char**)argv);

		    /* Failed ! */
		    fprintf(stderr, "WinExec: can't exec 'wine %s'\n",
			    lpCmdLine);
		}
		exit(1);
	    }
	}
    } /* while (!exhausted && handle < 32) */

    GlobalFree16( cmdShowHandle );
    GlobalFree16( cmdLineHandle );
    return handle;
}


/***********************************************************************
 *           WIN32_GetProcAddress16   (KERNEL32.36)
 * Get procaddress in 16bit module from win32... (kernel32 undoc. ordinal func)
 */
FARPROC16 WINAPI WIN32_GetProcAddress16( HMODULE32 hModule, LPSTR name )
{
    WORD	ordinal;
    FARPROC16	ret;

    if (!hModule) {
    	fprintf(stderr,"WIN32_GetProcAddress16: hModule may not be 0!\n");
	return (FARPROC16)0;
    }
    hModule = MODULE_HANDLEtoHMODULE16(hModule);
    if (HIWORD(name)) {
        ordinal = MODULE_GetOrdinal( hModule, name );
        dprintf_module( stddeb, "WIN32_GetProcAddress16: %04x '%s'\n",
                        hModule, name );
    } else {
        ordinal = LOWORD(name);
        dprintf_module( stddeb, "GetProcAddress: %04x %04x\n",
                        hModule, ordinal );
    }
    if (!ordinal) return (FARPROC16)0;
    ret = MODULE_GetEntryPoint( hModule, ordinal );
    dprintf_module(stddeb,"WIN32_GetProcAddress16: returning %08x\n",(UINT32)ret);
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
    hModule = MODULE_HANDLEtoHMODULE16( hModule );

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
    if (!ordinal) return (FARPROC16)0;

    ret = MODULE_GetEntryPoint( hModule, ordinal );

    dprintf_module( stddeb, "GetProcAddress: returning %08x\n", (UINT32)ret );
    return ret;
}


/***********************************************************************
 *           GetProcAddress32   (KERNEL32.257)
 */
FARPROC32 WINAPI GetProcAddress32( HMODULE32 hModule, LPCSTR function )
{
    NE_MODULE *pModule;

    if (HIWORD(function))
	dprintf_win32(stddeb,"GetProcAddress32(%08lx,%s)\n",(DWORD)hModule,function);
    else
	dprintf_win32(stddeb,"GetProcAddress32(%08lx,%p)\n",(DWORD)hModule,function);
    if (!(pModule = MODULE_GetPtr( hModule )))
        return (FARPROC32)0;
    if (!pModule->module32)
    {
    	fprintf(stderr,"Oops, Module 0x%08lx has got no module32?\n",
		(DWORD)MODULE_HANDLEtoHMODULE32(hModule)
	);
	return (FARPROC32)0;
    }
    return PE_FindExportedFunction( pModule->module32, function );
}

/***********************************************************************
 *           RtlImageNtHeaders   (NTDLL)
 */
LPIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE32 hModule)
{
    /* basically:
     * return  hModule+(((IMAGE_DOS_HEADER*)hModule)->e_lfanew); 
     * but we could get HMODULE16 or the like (think builtin modules)
     */

    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr( hModule )))
        return (LPIMAGE_NT_HEADERS)0;
    if (!(pModule->flags & NE_FFLAGS_WIN32) || !pModule->module32)
        return (LPIMAGE_NT_HEADERS)0;
    return PE_HEADER(pModule->module32);
}


/**********************************************************************
 *	    GetExpWinVer    (KERNEL.167)
 */
WORD WINAPI GetExpWinVer( HMODULE16 hModule )
{
    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    return pModule ? pModule->expected_version : 0;
}


/**********************************************************************
 *	    IsSharedSelector    (KERNEL.345)
 */
BOOL16 WINAPI IsSharedSelector( HANDLE16 selector )
{
    /* Check whether the selector belongs to a DLL */
    NE_MODULE *pModule = MODULE_GetPtr( selector );
    if (!pModule) return FALSE;
    return (pModule->flags & NE_FFLAGS_LIBMODULE) != 0;
}


/**********************************************************************
 *	    ModuleFirst    (TOOLHELP.59)
 */
BOOL16 WINAPI ModuleFirst( MODULEENTRY *lpme )
{
    lpme->wNext = hFirstModule;
    return ModuleNext( lpme );
}


/**********************************************************************
 *	    ModuleNext    (TOOLHELP.60)
 */
BOOL16 WINAPI ModuleNext( MODULEENTRY *lpme )
{
    NE_MODULE *pModule;
    char *name;

    if (!lpme->wNext) return FALSE;
    if (!(pModule = MODULE_GetPtr( lpme->wNext ))) return FALSE;
    name = (char *)pModule + pModule->name_table;
    memcpy( lpme->szModule, name + 1, *name );
    lpme->szModule[(BYTE)*name] = '\0';
    lpme->hModule = lpme->wNext;
    lpme->wcUsage = pModule->count;
    strncpy( lpme->szExePath, NE_MODULE_NAME(pModule), MAX_PATH );
    lpme->szExePath[MAX_PATH] = '\0';
    lpme->wNext = pModule->next;
    return TRUE;
}


/**********************************************************************
 *	    ModuleFindName    (TOOLHELP.61)
 */
BOOL16 WINAPI ModuleFindName( MODULEENTRY *lpme, LPCSTR name )
{
    lpme->wNext = GetModuleHandle16( name );
    return ModuleNext( lpme );
}


/**********************************************************************
 *	    ModuleFindHandle    (TOOLHELP.62)
 */
BOOL16 WINAPI ModuleFindHandle( MODULEENTRY *lpme, HMODULE16 hModule )
{
    hModule = MODULE_HANDLEtoHMODULE16( hModule );
    lpme->wNext = hModule;
    return ModuleNext( lpme );
}
