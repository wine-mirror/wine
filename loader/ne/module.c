/*
 * NE modules
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>
#include "module.h"
#include "ldt.h"
#include "heap.h"
#include "global.h"
#include "process.h"
#include "debug.h"

HMODULE16 hFirstModule = 0;

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

    if (!(pModule = MODULE_GetPtr16( hModule )))
    {
        fprintf( stderr, "**** %04x is not a module handle\n", hModule );
        return;
    }

      /* Dump the module info */
    DUMP( "---\n" );
    DUMP( "Module %04x:\n", hModule );
    DUMP( "count=%d flags=%04x heap=%d stack=%d\n",
	  pModule->count, pModule->flags,
	  pModule->heap_size, pModule->stack_size );
    DUMP( "cs:ip=%04x:%04x ss:sp=%04x:%04x ds=%04x nb seg=%d modrefs=%d\n",
	  pModule->cs, pModule->ip, pModule->ss, pModule->sp, pModule->dgroup,
	  pModule->seg_count, pModule->modref_count );
    DUMP( "os_flags=%d swap_area=%d version=%04x\n",
	  pModule->os_flags, pModule->min_swap_area,
	  pModule->expected_version );
    if (pModule->flags & NE_FFLAGS_WIN32)
        DUMP( "PE module=%08x\n", pModule->module32 );

      /* Dump the file info */
    DUMP( "---\n" );
    DUMP( "Filename: '%s'\n", NE_MODULE_NAME(pModule) );

      /* Dump the segment table */
    DUMP( "---\n" );
    DUMP( "Segment table:\n" );
    pSeg = NE_SEG_TABLE( pModule );
    for (i = 0; i < pModule->seg_count; i++, pSeg++)
        DUMP( "%02x: pos=%d size=%d flags=%04x minsize=%d sel=%04x\n",
	      i + 1, pSeg->filepos, pSeg->size, pSeg->flags,
	      pSeg->minsize, pSeg->selector );

      /* Dump the resource table */
    DUMP( "---\n" );
    DUMP( "Resource table:\n" );
    if (pModule->res_table)
    {
        pword = (WORD *)((BYTE *)pModule + pModule->res_table);
        DUMP( "Alignment: %d\n", *pword++ );
        while (*pword)
        {
            struct resource_typeinfo_s *ptr = (struct resource_typeinfo_s *)pword;
            struct resource_nameinfo_s *pname = (struct resource_nameinfo_s *)(ptr + 1);
            DUMP( "id=%04x count=%d\n", ptr->type_id, ptr->count );
            for (i = 0; i < ptr->count; i++, pname++)
                DUMP( "offset=%d len=%d id=%04x\n",
		      pname->offset, pname->length, pname->id );
            pword = (WORD *)pname;
        }
    }
    else DUMP( "None\n" );

      /* Dump the resident name table */
    DUMP( "---\n" );
    DUMP( "Resident-name table:\n" );
    pstr = (char *)pModule + pModule->name_table;
    while (*pstr)
    {
        DUMP( "%*.*s: %d\n", *pstr, *pstr, pstr + 1,
	      *(WORD *)(pstr + *pstr + 1) );
        pstr += *pstr + 1 + sizeof(WORD);
    }

      /* Dump the module reference table */
    DUMP( "---\n" );
    DUMP( "Module ref table:\n" );
    if (pModule->modref_table)
    {
        pword = (WORD *)((BYTE *)pModule + pModule->modref_table);
        for (i = 0; i < pModule->modref_count; i++, pword++)
        {
	    DUMP( "%d: %04x -> '%s'\n", i, *pword,
		    MODULE_GetModuleName(*pword));
        }
    }
    else DUMP( "None\n" );

      /* Dump the entry table */
    DUMP( "---\n" );
    DUMP( "Entry table:\n" );
    pstr = (char *)pModule + pModule->entry_table;
    ordinal = 1;
    while (*pstr)
    {
        DUMP( "Bundle %d-%d: %02x\n", ordinal, ordinal + *pstr - 1, pstr[1]);
        if (!pstr[1])
        {
            ordinal += *pstr;
            pstr += 2;
        }
        else if ((BYTE)pstr[1] == 0xff)  /* moveable */
        {
            i = *pstr;
            pstr += 2;
            while (i--)
            {
                DUMP( "%d: %02x:%04x (moveable)\n",
		      ordinal++, pstr[3], *(WORD *)(pstr + 4) );
                pstr += 6;
            }
        }
        else  /* fixed */
        {
            i = *pstr;
            pstr += 2;
            while (i--)
            {
                DUMP( "%d: %04x (fixed)\n",
		      ordinal++, *(WORD *)(pstr + 1) );
                pstr += 3;
            }
        }
    }

    /* Dump the non-resident names table */
    DUMP( "---\n" );
    DUMP( "Non-resident names table:\n" );
    if (pModule->nrname_handle)
    {
        pstr = (char *)GlobalLock16( pModule->nrname_handle );
        while (*pstr)
        {
            DUMP( "%*.*s: %d\n", *pstr, *pstr, pstr + 1,
                   *(WORD *)(pstr + *pstr + 1) );
            pstr += *pstr + 1 + sizeof(WORD);
        }
    }
    DUMP( "\n" );
}


/***********************************************************************
 *           NE_WalkModules
 *
 * Walk the module list and print the modules.
 */
void NE_WalkModules(void)
{
    HMODULE16 hModule = hFirstModule;
    fprintf( stderr, "Module Flags Name\n" );
    while (hModule)
    {
        NE_MODULE *pModule = MODULE_GetPtr16( hModule );
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


/**********************************************************************
 *           NE_RegisterModule
 */
void NE_RegisterModule( NE_MODULE *pModule )
{
    pModule->next = hFirstModule;
    hFirstModule = pModule->self;
}


/***********************************************************************
 *           NE_GetOrdinal
 *
 * Lookup the ordinal for a given name.
 */
WORD NE_GetOrdinal( HMODULE16 hModule, const char *name )
{
    unsigned char buffer[256], *cpnt;
    BYTE len;
    NE_MODULE *pModule;

    if (!(pModule = MODULE_GetPtr16( hModule ))) return 0;
    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

    TRACE( module, "(%04x,'%s')\n", hModule, name );

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
            TRACE(module, "  Found: ordinal=%d\n",
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
            TRACE(module, "  Found: ordinal=%d\n",
                            *(WORD *)(cpnt + *cpnt + 1) );
            return *(WORD *)(cpnt + *cpnt + 1);
        }
        cpnt += *cpnt + 1 + sizeof(WORD);
    }
    return 0;
}


/***********************************************************************
 *           NE_GetEntryPoint   (WPROCS.27)
 *
 * Return the entry point for a given ordinal.
 */
FARPROC16 NE_GetEntryPoint( HMODULE16 hModule, WORD ordinal )
{
    NE_MODULE *pModule;
    WORD curOrdinal = 1;
    BYTE *p;
    WORD sel, offset;

    if (!(pModule = MODULE_GetPtr16( hModule ))) return 0;
    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

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
 *           NE_SetEntryPoint
 *
 * Change the value of an entry point. Use with caution!
 * It can only change the offset value, not the selector.
 */
BOOL16 NE_SetEntryPoint( HMODULE16 hModule, WORD ordinal, WORD offset )
{
    NE_MODULE *pModule;
    WORD curOrdinal = 1;
    BYTE *p;

    if (!(pModule = MODULE_GetPtr16( hModule ))) return FALSE;
    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

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
 *           NE_LoadExeHeader
 */
static HMODULE16 NE_LoadExeHeader( HFILE32 hFile, OFSTRUCT *ofs )
{
    IMAGE_DOS_HEADER mz_header;
    IMAGE_OS2_HEADER ne_header;
    int size;
    HMODULE16 hModule;
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
        return (HMODULE16)11;  /* invalid exe */

    _llseek32( hFile, mz_header.e_lfanew, SEEK_SET );
    if (_lread32( hFile, &ne_header, sizeof(ne_header) ) != sizeof(ne_header))
        return (HMODULE16)11;  /* invalid exe */

    if (ne_header.ne_magic == IMAGE_NT_SIGNATURE) return (HMODULE16)21;  /* win32 exe */
    if (ne_header.ne_magic != IMAGE_OS2_SIGNATURE) return (HMODULE16)11;  /* invalid exe */

    if (ne_header.ne_magic == IMAGE_OS2_SIGNATURE_LX) {
      fprintf(stderr, "Sorry, this is an OS/2 linear executable (LX) file !\n");
      return (HMODULE16)12;
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
    if (!hModule) return (HMODULE16)11;  /* invalid exe */
    FarSetOwner( hModule, hModule );
    pModule = (NE_MODULE *)GlobalLock16( hModule );
    memcpy( pModule, &ne_header, sizeof(ne_header) );
    pModule->count = 0;
    pModule->module32 = 0;
    pModule->self = hModule;
    pModule->self_loading_sel = 0;
    pData = (BYTE *)(pModule + 1);

    /* Clear internal Wine flags in case they are set in the EXE file */

    pModule->flags &= ~(NE_FFLAGS_BUILTIN|NE_FFLAGS_WIN32|NE_FFLAGS_IMPLICIT);

    /* Read the fast-load area */

    if (ne_header.additional_flags & NE_AFLAGS_FASTLOAD)
    {
        fastload_offset=ne_header.fastload_offset<<ne_header.align_shift_count;
        fastload_length=ne_header.fastload_length<<ne_header.align_shift_count;
        TRACE(module, "Using fast-load area offset=%x len=%d\n",
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
            return (HMODULE16)11;  /* invalid exe */
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
        return (HMODULE16)11;  /* invalid exe */
    }

    /* Get the resource table */

    if (ne_header.resource_tab_offset < ne_header.rname_tab_offset)
    {
        pModule->res_table = (int)pData - (int)pModule;
        if (!READ(mz_header.e_lfanew + ne_header.resource_tab_offset,
                  ne_header.rname_tab_offset - ne_header.resource_tab_offset,
                  pData )) return (HMODULE16)11;  /* invalid exe */
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
        return (HMODULE16)11;  /* invalid exe */
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
            return (HMODULE16)11;  /* invalid exe */
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
        return (HMODULE16)11;  /* invalid exe */
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
        return (HMODULE16)11;  /* invalid exe */
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
            return (HMODULE16)11;  /* invalid exe */
        }
        buffer = GlobalLock16( pModule->nrname_handle );
        _llseek32( hFile, ne_header.nrname_tab_offset, SEEK_SET );
        if (_lread32( hFile, buffer, ne_header.nrname_tab_length )
              != ne_header.nrname_tab_length)
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
        pModule->dlls_to_init = GLOBAL_Alloc(GMEM_ZEROINIT,
                                    (pModule->modref_count+1)*sizeof(HMODULE16),
                                    hModule, FALSE, FALSE, FALSE );
        if (!pModule->dlls_to_init)
        {
            if (pModule->nrname_handle) GlobalFree16( pModule->nrname_handle );
            GlobalFree16( hModule );
            return (HMODULE16)11;  /* invalid exe */
        }
    }
    else pModule->dlls_to_init = 0;

    NE_RegisterModule( pModule );
    return hModule;
}


/***********************************************************************
 *           NE_LoadDLLs
 *
 * Load all DLLs implicitly linked to a module.
 */
static BOOL32 NE_LoadDLLs( NE_MODULE *pModule )
{
    int i;
    WORD *pModRef = (WORD *)((char *)pModule + pModule->modref_table);
    WORD *pDLLs = (WORD *)GlobalLock16( pModule->dlls_to_init );

    for (i = 0; i < pModule->modref_count; i++, pModRef++)
    {
        char buffer[256];
        BYTE *pstr = (BYTE *)pModule + pModule->import_table + *pModRef;
        memcpy( buffer, pstr + 1, *pstr );
        strcpy( buffer + *pstr, ".dll" );
        TRACE(module, "Loading '%s'\n", buffer );
        if (!(*pModRef = MODULE_FindModule16( buffer )))
        {
            /* If the DLL is not loaded yet, load it and store */
            /* its handle in the list of DLLs to initialize.   */
            HMODULE16 hDLL;

            if ((hDLL = MODULE_Load( buffer, NE_FFLAGS_IMPLICIT,
                                     NULL, NULL, 0 )) == 2)
            {
                /* file not found */
                char *p;

                /* Try with prepending the path of the current module */
                GetModuleFileName16( pModule->self, buffer, sizeof(buffer) );
                if (!(p = strrchr( buffer, '\\' ))) p = buffer;
                memcpy( p + 1, pstr + 1, *pstr );
                strcpy( p + 1 + *pstr, ".dll" );
                hDLL = MODULE_Load( buffer, NE_FFLAGS_IMPLICIT, NULL, NULL, 0);
            }
            if (hDLL < 32)
            {
                /* FIXME: cleanup what was done */

                fprintf( stderr, "Could not load '%s' required by '%.*s', error = %d\n",
                         buffer, *((BYTE*)pModule + pModule->name_table),
                         (char *)pModule + pModule->name_table + 1, hDLL );
                return FALSE;
            }
            *pModRef = GetExePtr( hDLL );
            *pDLLs++ = *pModRef;
        }
        else  /* Increment the reference count of the DLL */
        {
            NE_MODULE *pOldDLL = MODULE_GetPtr16( *pModRef );
            if (pOldDLL) pOldDLL->count++;
        }
    }
    return TRUE;
}


/**********************************************************************
 *	    NE_LoadModule
 *
 * Implementation of LoadModule().
 *
 * cmd_line must contain the whole command-line, including argv[0] (and
 * without a preceding length byte).
 * If cmd_line is NULL, the module is loaded as a library even if it is a .exe
 */
HINSTANCE16 NE_LoadModule( HFILE32 hFile, OFSTRUCT *ofs, UINT16 flags,
                           LPCSTR cmd_line, LPCSTR env, UINT32 show_cmd )
{
    HMODULE16 hModule;
    HINSTANCE16 hInstance;
    NE_MODULE *pModule;

    /* Create the module structure */

    if ((hModule = NE_LoadExeHeader( hFile, ofs )) < 32) return hModule;

    pModule = MODULE_GetPtr16( hModule );
    pModule->flags |= flags; /* stamp implicitly loaded modules */

    /* Allocate the segments for this module */

    NE_CreateSegments( hModule );
    hInstance = MODULE_CreateInstance( hModule, NULL, (cmd_line == NULL) );

    /* Load the referenced DLLs */

    if (!NE_LoadDLLs( pModule ))
        return 2;  /* File not found (FIXME: free everything) */

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
                                                           NE_FFLAGS_LIBMODULE)
        NE_InitializeDLLs( hModule );

    /* Create a task for this instance */

    if (cmd_line && !(pModule->flags & NE_FFLAGS_LIBMODULE))
    {
        PDB32 *pdb;

	pModule->flags |= NE_FFLAGS_GUI;

        pdb = PROCESS_Create( pModule, cmd_line, env, hInstance, 0, show_cmd );
        if (pdb && (GetNumTasks() > 1)) Yield16();
    }

    return hInstance;
}
