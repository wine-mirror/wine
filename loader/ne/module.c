/*
 * NE modules
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "wine/winbase16.h"
#include "winerror.h"
#include "module.h"
#include "neexe.h"
#include "peexe.h"
#include "toolhelp.h"
#include "file.h"
#include "ldt.h"
#include "callback.h"
#include "heap.h"
#include "task.h"
#include "global.h"
#include "process.h"
#include "toolhelp.h"
#include "snoop.h"
#include "stackframe.h"
#include "debug.h"
#include "file.h"
#include "loadorder.h"
#include "elfdll.h"

DEFAULT_DEBUG_CHANNEL(module)

FARPROC16 (*fnSNOOP16_GetProcAddress16)(HMODULE16,DWORD,FARPROC16) = NULL;
void (*fnSNOOP16_RegisterDLL)(NE_MODULE*,LPCSTR) = NULL;

#define hFirstModule (pThhook->hExeHead)

static NE_MODULE *pCachedModule = 0;  /* Module cached by NE_OpenFile */

static HMODULE16 NE_LoadBuiltin(LPCSTR name,BOOL force) { return 0; }
HMODULE16 (*fnBUILTIN_LoadModule)(LPCSTR name,BOOL force) = NE_LoadBuiltin;


/***********************************************************************
 *           NE_GetPtr
 */
NE_MODULE *NE_GetPtr( HMODULE16 hModule )
{
    return (NE_MODULE *)GlobalLock16( GetExePtr(hModule) );
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
        MSG( "**** %04x is not a module handle\n", hModule );
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
        DUMP( "%02x: pos=%d size=%d flags=%04x minsize=%d hSeg=%04x\n",
	      i + 1, pSeg->filepos, pSeg->size, pSeg->flags,
	      pSeg->minsize, pSeg->hSeg );

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
            char name[10];
            GetModuleName16( *pword, name, sizeof(name) );
	    DUMP( "%d: %04x -> '%s'\n", i, *pword, name );
        }
    }
    else DUMP( "None\n" );

      /* Dump the entry table */
    DUMP( "---\n" );
    DUMP( "Entry table:\n" );
    bundle = (ET_BUNDLE *)((BYTE *)pModule+pModule->entry_table);    
    do {
	entry = (ET_ENTRY *)((BYTE *)bundle+6);
	DUMP( "Bundle %d-%d: %02x\n", bundle->first, bundle->last, entry->type);
	ordinal = bundle->first;
	while (ordinal < bundle->last)
        {
	    if (entry->type == 0xff)
		DUMP("%d: %02x:%04x (moveable)\n", ordinal++, entry->segnum, entry->offs);
	    else
		DUMP("%d: %02x:%04x (fixed)\n", ordinal++, entry->segnum, entry->offs);
	    entry++;
    }
    } while ( (bundle->next)
	   && (bundle = ((ET_BUNDLE *)((BYTE *)pModule + bundle->next))) );

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
    MSG( "Module Flags Name\n" );
    while (hModule)
    {
        NE_MODULE *pModule = NE_GetPtr( hModule );
        if (!pModule)
        {
            MSG( "Bad module %04x in list\n", hModule );
            return;
        }
        MSG( " %04x  %04x  %.*s\n", hModule, pModule->flags,
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

    if (!(pModule = NE_GetPtr( hModule ))) return 0;
    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

    TRACE( module, "(%04x,'%s')\n", hModule, name );

      /* First handle names of the form '#xxxx' */

    if (name[0] == '#') return atoi( name + 1 );

      /* Now copy and uppercase the string */

    strcpy( buffer, name );
    CharUpperA( buffer );
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
    return NE_GetEntryPointEx( hModule, ordinal, TRUE );
}
FARPROC16 NE_GetEntryPointEx( HMODULE16 hModule, WORD ordinal, BOOL16 snoop )
{
    NE_MODULE *pModule;
    WORD sel, offset, i;

    ET_ENTRY *entry;
    ET_BUNDLE *bundle;

    if (!(pModule = NE_GetPtr( hModule ))) return 0;
    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

    bundle = (ET_BUNDLE *)((BYTE *)pModule + pModule->entry_table);
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
    offset = entry->offs;

    if (sel == 0xfe) sel = 0xffff;  /* constant entry */
    else sel = GlobalHandleToSel16(NE_SEG_TABLE(pModule)[sel-1].hSeg);
    if (sel==0xffff)
	return (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( sel, offset );
    if (!snoop || !fnSNOOP16_GetProcAddress16)
	return (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( sel, offset );
    else
	return (FARPROC16)fnSNOOP16_GetProcAddress16(hModule,ordinal,(FARPROC16)PTR_SEG_OFF_TO_SEGPTR( sel, offset ));
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
	if (!(bundle->next))
	    return 0;
    }

    entry = (ET_ENTRY *)((BYTE *)bundle+6);
    for (i=0; i < (ordinal - bundle->first - 1); i++)
	entry++;

    entry->offs = offset;
    return TRUE;
}


/***********************************************************************
 *           NE_OpenFile
 */
HANDLE NE_OpenFile( NE_MODULE *pModule )
{
    char *name;

    static HANDLE cachedfd = -1;

    TRACE( module, "(%p) cache: mod=%p fd=%d\n",
           pModule, pCachedModule, cachedfd );
    if (pCachedModule == pModule) return cachedfd;
    CloseHandle( cachedfd );
    pCachedModule = pModule;
    name = NE_MODULE_NAME( pModule );
    if ((cachedfd = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ,
                                   NULL, OPEN_EXISTING, 0, -1 )) == -1)
        MSG( "Can't open file '%s' for module %04x\n", name, pModule->self );
    else
        /* FIXME: should not be necessary */
        cachedfd = ConvertToGlobalHandle(cachedfd);
    TRACE(module, "opened '%s' -> %d\n",
                    name, cachedfd );
    return cachedfd;
}


/***********************************************************************
 *           NE_LoadExeHeader
 */
static HMODULE16 NE_LoadExeHeader( HFILE16 hFile, OFSTRUCT *ofs )
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

  /* Read a block from either the file or the fast-load area. */
#define READ(offset,size,buffer) \
       ((fastload && ((offset) >= fastload_offset) && \
         ((offset)+(size) <= fastload_offset+fastload_length)) ? \
        (memcpy( buffer, fastload+(offset)-fastload_offset, (size) ), TRUE) : \
        (_llseek16( hFile, (offset), SEEK_SET), \
         _hread16( hFile, (buffer), (size) ) == (size)))

    _llseek16( hFile, 0, SEEK_SET );
    if ((_hread16(hFile,&mz_header,sizeof(mz_header)) != sizeof(mz_header)) ||
        (mz_header.e_magic != IMAGE_DOS_SIGNATURE))
        return (HMODULE16)11;  /* invalid exe */

    _llseek16( hFile, mz_header.e_lfanew, SEEK_SET );
    if (_hread16( hFile, &ne_header, sizeof(ne_header) ) != sizeof(ne_header))
        return (HMODULE16)11;  /* invalid exe */

    if (ne_header.ne_magic == IMAGE_NT_SIGNATURE) return (HMODULE16)21;  /* win32 exe */
    if (ne_header.ne_magic != IMAGE_OS2_SIGNATURE) return (HMODULE16)11;  /* invalid exe */

    if (ne_header.ne_magic == IMAGE_OS2_SIGNATURE_LX) {
      MSG("Sorry, this is an OS/2 linear executable (LX) file !\n");
      return (HMODULE16)12;
    }

    /* We now have a valid NE header */

    size = sizeof(NE_MODULE) +
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
           ne_header.entry_tab_length +
	     /* entry table extra conversion space */
	   sizeof(ET_BUNDLE) +
	   2 * (ne_header.entry_tab_length - ne_header.n_mov_entry_points*6) +
             /* loaded file info */
           sizeof(OFSTRUCT)-sizeof(ofs->szPathName)+strlen(ofs->szPathName)+1;

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

    if (ne_header.additional_flags & NE_AFLAGS_FASTLOAD)
    {
        fastload_offset=ne_header.fastload_offset<<ne_header.align_shift_count;
        fastload_length=ne_header.fastload_length<<ne_header.align_shift_count;
        TRACE(module, "Using fast-load area offset=%x len=%d\n",
                        fastload_offset, fastload_length );
        if ((fastload = HeapAlloc( SystemHeap, 0, fastload_length )) != NULL)
        {
            _llseek16( hFile, fastload_offset, SEEK_SET);
            if (_hread16(hFile, fastload, fastload_length) != fastload_length)
            {
                HeapFree( SystemHeap, 0, fastload );
                WARN( module, "Error reading fast-load area!\n");
                fastload = NULL;
            }
        }
    }

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
            if (fastload)
		HeapFree( SystemHeap, 0, fastload );
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
        if (fastload)
	    HeapFree( SystemHeap, 0, fastload );
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
        if (fastload)
	    HeapFree( SystemHeap, 0, fastload );
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
            if (fastload)
		HeapFree( SystemHeap, 0, fastload );
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
        if (fastload)
	    HeapFree( SystemHeap, 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE16)11;  /* invalid exe */
    }
    pData += ne_header.entry_tab_offset - ne_header.iname_tab_offset;

    /* Load entry table, convert it to the optimized version used by Windows */

    if ((pTempEntryTable = HeapAlloc( SystemHeap, 0, ne_header.entry_tab_length)) != NULL)
    {
        BYTE nr_entries, type, *s;

	TRACE(module, "Converting entry table.\n");
    pModule->entry_table = (int)pData - (int)pModule;
    if (!READ( mz_header.e_lfanew + ne_header.entry_tab_offset,
                ne_header.entry_tab_length, pTempEntryTable ))
    {
            HeapFree( SystemHeap, 0, pTempEntryTable );
            if (fastload)
		HeapFree( SystemHeap, 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE16)11;  /* invalid exe */
    }

        s = pTempEntryTable;
        TRACE(module, "entry table: offs %04x, len %04x, entries %d\n", ne_header.entry_tab_offset, ne_header.entry_tab_length, *s);

	bundle = (ET_BUNDLE *)pData;
        TRACE(module, "first bundle: %p\n", bundle);
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
                    TRACE(module, "new bundle: %p\n", bundle);
                    bundle->first = bundle->last =
                        oldbundle->last + nr_entries;
                    bundle->next = 0;
                    (BYTE *)entry += sizeof(ET_BUNDLE);
                }
            }
        }
	HeapFree( SystemHeap, 0, pTempEntryTable );
    }
    else
    {
        if (fastload)
	    HeapFree( SystemHeap, 0, fastload );
        GlobalFree16( hModule );
        return (HMODULE16)11;  /* invalid exe */
    }

    pData += ne_header.entry_tab_length + sizeof(ET_BUNDLE) +
	     2 * (ne_header.entry_tab_length - ne_header.n_mov_entry_points*6);

    if ((DWORD)entry > (DWORD)pData)
       ERR(module, "converted entry table bigger than reserved space !!!\nentry: %p, pData: %p. Please report !\n", entry, pData);

    /* Store the filename information */

    pModule->fileinfo = (int)pData - (int)pModule;
    size = sizeof(OFSTRUCT)-sizeof(ofs->szPathName)+strlen(ofs->szPathName)+1;
    memcpy( pData, ofs, size );
    ((OFSTRUCT *)pData)->cBytes = size - 1;
    pData += size;

    /* Free the fast-load area */

#undef READ
    if (fastload)
	HeapFree( SystemHeap, 0, fastload );

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
        _llseek16( hFile, ne_header.nrname_tab_offset, SEEK_SET );
        if (_hread16( hFile, buffer, ne_header.nrname_tab_length )
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
    if (fnSNOOP16_RegisterDLL)
    	fnSNOOP16_RegisterDLL(pModule,ofs->szPathName);
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
        char buffer[260];
        BYTE *pstr = (BYTE *)pModule + pModule->import_table + *pModRef;
        memcpy( buffer, pstr + 1, *pstr );
       *(buffer + *pstr) = 0; /* terminate it */

        TRACE(module, "Loading '%s'\n", buffer );
        if (!(*pModRef = GetModuleHandle16( buffer )))
        {
            /* If the DLL is not loaded yet, load it and store */
            /* its handle in the list of DLLs to initialize.   */
            HMODULE16 hDLL;

            if ((hDLL = MODULE_LoadModule16( buffer, TRUE )) < 32)
            {
                /* FIXME: cleanup what was done */

                MSG( "Could not load '%s' required by '%.*s', error=%d\n",
                     buffer, *((BYTE*)pModule + pModule->name_table),
                     (char *)pModule + pModule->name_table + 1, hDLL );
                return FALSE;
            }
            *pModRef = GetExePtr( hDLL );
            *pDLLs++ = *pModRef;
        }
        else  /* Increment the reference count of the DLL */
        {
            NE_MODULE *pOldDLL;

	    pOldDLL = NE_GetPtr( *pModRef );
            if (pOldDLL) pOldDLL->count++;
        }
    }
    return TRUE;
}


/**********************************************************************
 *	    NE_LoadFileModule
 *
 * Load first instance of NE module from file.
 * (Note: caller is responsible for ensuring the module isn't
 *        already loaded!)
 */
static HINSTANCE16 NE_LoadFileModule( HFILE16 hFile, OFSTRUCT *ofs, 
                                      BOOL implicit )
{
    HINSTANCE16 hInstance;
    HMODULE16 hModule;
    NE_MODULE *pModule;

    /* Create the module structure */

    hModule = NE_LoadExeHeader( hFile, ofs );
    if (hModule < 32) return hModule;
    pModule = NE_GetPtr( hModule );

    /* Allocate the segments for this module */

    if (!NE_CreateSegments( pModule ) ||
        !(hInstance = NE_CreateInstance( pModule, NULL, FALSE )))
    {
        GlobalFreeAll16( hModule );
        return 8;  /* Insufficient memory */
    }

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

    return hInstance;
}

/**********************************************************************
 *	    NE_LoadModule
 *
 * Load first instance of NE module, deciding whether to use
 * built-in module or load module from file.
 * (Note: caller is responsible for ensuring the module isn't
 *        already loaded!)
 */
HINSTANCE16 NE_LoadModule( LPCSTR name, BOOL implicit )
{
    HINSTANCE16 hInstance;
    HFILE16 hFile;
    OFSTRUCT ofs;

    if ((hFile = OpenFile16( name, &ofs, OF_READ )) == HFILE_ERROR16)
    {
        char	buffer[260];

	if(implicit)
	{
	    /* 4 == strlen(".dll") */
	    strncpy(buffer, name, sizeof(buffer) - 1 - 4);
	    strcat(buffer, ".dll");
	    if ((hFile = OpenFile16( buffer, &ofs, OF_READ )) == HFILE_ERROR16)
		return 2;  /* File not found */
	}
    }

    hInstance = NE_LoadFileModule( hFile, &ofs, implicit );
    _lclose16( hFile );

    return hInstance;
}


/**********************************************************************
 *	    MODULE_LoadModule16
 *
 * Load a NE module in the order of the loadorder specification.
 * The caller is responsible that the module is not loaded already.
 *
 */
HINSTANCE16 MODULE_LoadModule16( LPCSTR libname, BOOL implicit )
{
	HINSTANCE16 hinst;
	int i;
	module_loadorder_t *plo;

	plo = MODULE_GetLoadOrder(libname);

	for(i = 0; i < MODULE_LOADORDER_NTYPES; i++)
	{
		switch(plo->loadorder[i])
		{
		case MODULE_LOADORDER_DLL:
			TRACE(module, "Trying native dll '%s'\n", libname);
			hinst = NE_LoadModule(libname, implicit);
			break;

		case MODULE_LOADORDER_ELFDLL:
			TRACE(module, "Trying elfdll '%s'\n", libname);
			hinst = ELFDLL_LoadModule16(libname, implicit);
			break;

		case MODULE_LOADORDER_BI:
			TRACE(module, "Trying built-in '%s'\n", libname);
			hinst = fnBUILTIN_LoadModule(libname, TRUE);
			break;

		default:
			ERR(module, "Got invalid loadorder type %d (%s index %d)\n", plo->loadorder[i], plo->modulename, i);
		/* Fall through */

		case MODULE_LOADORDER_SO:	/* This is not supported for NE modules */
		case MODULE_LOADORDER_INVALID:	/* We ignore this as it is an empty entry */
			hinst = 2;
			break;
		}

		if(hinst >= 32)
		{
			if(!implicit)
			{
				HMODULE16 hModule;
				NE_MODULE *pModule;

				hModule = GetModuleHandle16(libname);
				if(!hModule)
				{
					ERR(module, "Serious trouble. Just loaded module '%s' (hinst=0x%04x), but can't get module handle\n",
						libname, hinst);
					return 6;	/* ERROR_INVALID_HANDLE seems most appropriate */
				}

				pModule = NE_GetPtr(hModule);
				if(!pModule)
				{
					ERR(module, "Serious trouble. Just loaded module '%s' (hinst=0x%04x), but can't get NE_MODULE pointer\n",
						libname, hinst);
					return 6;	/* ERROR_INVALID_HANDLE seems most appropriate */
				}

				TRACE(module, "Loaded module '%s' at 0x%04x, \n", libname, hinst);

				/*
				 * Call initialization routines for all loaded DLLs. Note that
				 * when we load implicitly linked DLLs this will be done by InitTask().
				 */
				if(pModule->flags & NE_FFLAGS_LIBMODULE)
					NE_InitializeDLLs(hModule);
			}
			return hinst;
		}

		if(hinst != 2)
		{
			/* We quit searching when we get another error than 'File not found' */
			break;
		}
	}
	return hinst;	/* The last error that occured */
}


/**********************************************************************
 *          LoadModule16    (KERNEL.45)
 */
HINSTANCE16 WINAPI LoadModule16( LPCSTR name, LPVOID paramBlock )
{
    BOOL lib_only = !paramBlock || (paramBlock == (LPVOID)-1);
    LOADPARAMS16 *params;
    LPSTR cmd_line, new_cmd_line;
    LPCVOID env = NULL;
    STARTUPINFOA startup;
    PROCESS_INFORMATION info;
    HINSTANCE16 hInstance, hPrevInstance = 0;
    HMODULE16 hModule;
    NE_MODULE *pModule;
    PDB *pdb;

    /* Load module */

    if ( (hModule = GetModuleHandle16(name) ) != 0 )
    {
        /* Special case: second instance of an already loaded NE module */

        if ( !( pModule = NE_GetPtr( hModule ) ) ) return (HINSTANCE16)11;
        if ( pModule->module32 ) return (HINSTANCE16)21;

        hInstance = NE_CreateInstance( pModule, &hPrevInstance, lib_only );
        if ( hInstance != hPrevInstance )  /* not a library */
            NE_LoadSegment( pModule, pModule->dgroup );

        pModule->count++;
    }
    else
    {
        /* Main case: load first instance of NE module */

        if ( (hInstance = MODULE_LoadModule16( name, FALSE )) < 32 )
            return hInstance;

        if ( !(pModule = NE_GetPtr( hInstance )) )
            return (HINSTANCE16)11;
    }

    /* If library module, we're finished */

    if ( ( pModule->flags & NE_FFLAGS_LIBMODULE ) || lib_only )
        return hInstance;

    /* Create a task for this instance */

    pModule->flags |= NE_FFLAGS_GUI;  /* FIXME: is this necessary? */

    params = (LOADPARAMS16 *)paramBlock;
    cmd_line = (LPSTR)PTR_SEG_TO_LIN( params->cmdLine );
    if (!cmd_line) cmd_line = "";
    else if (*cmd_line) cmd_line++;  /* skip the length byte */

    if (!(new_cmd_line = HeapAlloc( GetProcessHeap(), 0,
                                    strlen(cmd_line)+strlen(name)+2 )))
        return 0;
    strcpy( new_cmd_line, name );
    strcat( new_cmd_line, " " );
    strcat( new_cmd_line, cmd_line );

    if (params->hEnvironment) env = GlobalLock16( params->hEnvironment );

    memset( &info, '\0', sizeof(info) );
    memset( &startup, '\0', sizeof(startup) );
    startup.cb = sizeof(startup);
    if (params->showCmd)
    {
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = ((UINT16 *)PTR_SEG_TO_LIN(params->showCmd))[1];
    }

    pdb = PROCESS_Create( pModule, new_cmd_line, env,
                          hInstance, hPrevInstance, 
                          NULL, NULL, TRUE, &startup, &info );

    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );

    if (params->hEnvironment) GlobalUnlock16( params->hEnvironment );
    HeapFree( GetProcessHeap(), 0, new_cmd_line );
    return hInstance;
}

/**********************************************************************
 *          NE_CreateProcess
 */
BOOL NE_CreateProcess( HFILE hFile, OFSTRUCT *ofs, LPCSTR cmd_line, LPCSTR env, 
                       LPSECURITY_ATTRIBUTES psa, LPSECURITY_ATTRIBUTES tsa,
                       BOOL inherit, LPSTARTUPINFOA startup,
                       LPPROCESS_INFORMATION info )
{
    HINSTANCE16 hInstance, hPrevInstance = 0;
    HMODULE16 hModule;
    NE_MODULE *pModule;
    HFILE16 hFile16;

    /* Special case: second instance of an already loaded NE module */

    if ( ( hModule = GetModuleHandle16( ofs->szPathName ) ) != 0 )
    {
        if (   !( pModule = NE_GetPtr( hModule) )
            ||  ( pModule->flags & NE_FFLAGS_LIBMODULE )
            ||  pModule->module32 )
        {
            SetLastError( ERROR_BAD_FORMAT );
            return FALSE;
        }

        hInstance = NE_CreateInstance( pModule, &hPrevInstance, FALSE );
        if ( hInstance != hPrevInstance )  /* not a library */
            NE_LoadSegment( pModule, pModule->dgroup );

        pModule->count++;
    }

    /* Main case: load first instance of NE module */
    else
    {
        /* If we didn't get a file handle, return */

        if ( hFile == HFILE_ERROR )
            return FALSE;

        /* Allocate temporary HFILE16 for NE_LoadFileModule */

        if (!DuplicateHandle( GetCurrentProcess(), hFile,
                              GetCurrentProcess(), &hFile,
                              0, FALSE, DUPLICATE_SAME_ACCESS ))
        {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }
        hFile16 = FILE_AllocDosHandle( hFile );

        /* Load module */

        hInstance = NE_LoadFileModule( hFile16, ofs, TRUE );
        _lclose16( hFile16 );

        if ( hInstance < 32 )
        {
            SetLastError( hInstance );
            return FALSE;
        }

        if (   !( pModule = NE_GetPtr( hInstance ) )
            ||  ( pModule->flags & NE_FFLAGS_LIBMODULE) )
        {
            /* FIXME: cleanup */
            SetLastError( ERROR_BAD_FORMAT );
            return FALSE;
        }
    }

    /* Create a task for this instance */

    pModule->flags |= NE_FFLAGS_GUI;  /* FIXME: is this necessary? */

    if ( !PROCESS_Create( pModule, cmd_line, env,
                          hInstance, hPrevInstance, 
                          psa, tsa, inherit, startup, info ) )
        return FALSE;

    return TRUE;
}

/***********************************************************************
 *           LoadLibrary16   (KERNEL.95)
 */
HINSTANCE16 WINAPI LoadLibrary16( LPCSTR libname )
{
    TRACE( module, "(%p) %s\n", libname, libname );

    /* Load library module */
    return LoadModule16( libname, (LPVOID)-1 );
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
    WORD ordinal = NE_GetOrdinal( hModule, "WEP" );

    if (ordinal) WEP = NE_GetEntryPoint( hModule, ordinal );
    if (!WEP)
    {
	WARN(module, "module %04x doesn't have a WEP\n", hModule );
	return FALSE;
    }
    return Callbacks->CallWindowsExitProc( WEP, WEP_FREE_DLL );
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

    TRACE( module, "%04x count %d\n", hModule, pModule->count );

    if (((INT16)(--pModule->count)) > 0 ) return TRUE;
    else pModule->count = 0;

    if (pModule->flags & NE_FFLAGS_BUILTIN)
        return FALSE;  /* Can't free built-in module */

    if (call_wep)
    {
        if (pModule->flags & NE_FFLAGS_LIBMODULE)
        {
            TDB *pTask = (TDB *)GlobalLock16( GetCurrentTask() );
            MODULE_CallWEP( hModule );

            /* Free the objects owned by the DLL module */

            if (pTask && pTask->userhandler)
                pTask->userhandler( hModule, USIG16_DLL_UNLOAD, 0,
                                    pTask->hInstance, pTask->hQueue );

            PROCESS_CallUserSignalProc( USIG_DLL_UNLOAD_WIN16, 0, hModule );
        }
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
 *	    FreeModule16    (KERNEL.46)
 */
BOOL16 WINAPI FreeModule16( HMODULE16 hModule )
{
    return NE_FreeModule( hModule, TRUE );
}


/***********************************************************************
 *           FreeLibrary16   (KERNEL.96)
 */
void WINAPI FreeLibrary16( HINSTANCE16 handle )
{
    TRACE(module,"%04x\n", handle );
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
    return pModule ? pModule->expected_version : 0;
}


/**********************************************************************
 *	    GetModuleFileName16    (KERNEL.49)
 */
INT16 WINAPI GetModuleFileName16( HINSTANCE16 hModule, LPSTR lpFileName,
                                  INT16 nSize )
{
    NE_MODULE *pModule;

    if (!hModule) hModule = GetCurrentTask();
    if (!(pModule = NE_GetPtr( hModule ))) return 0;
    lstrcpynA( lpFileName, NE_MODULE_NAME(pModule), nSize );
    TRACE(module, "%s\n", lpFileName );
    return strlen(lpFileName);
}


/**********************************************************************
 *	    GetModuleHandle16    (KERNEL.47)
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
    return MAKELONG(GetModuleHandle16( PTR_SEG_TO_LIN(name)), hFirstModule );
}

HMODULE16 WINAPI GetModuleHandle16( LPCSTR name )
{
    HMODULE16	hModule = hFirstModule;
    LPSTR	s;
    BYTE	len, *name_table;
    char	tmpstr[128];
    NE_MODULE *pModule;

    TRACE(module, "(%s)\n", name);

    if (!HIWORD(name))
    	return GetExePtr(LOWORD(name));

    len = strlen(name);
    if (!len)
    	return 0;

    strncpy(tmpstr, name, sizeof(tmpstr));
    tmpstr[sizeof(tmpstr)-1] = '\0';

    /* If 'name' matches exactly the module name of a module:
     * Return its handle.
     */
    for (hModule = hFirstModule; hModule ; hModule = pModule->next)
    {
	pModule = NE_GetPtr( hModule );
        if (!pModule) break;

        name_table = (BYTE *)pModule + pModule->name_table;
        if ((*name_table == len) && !strncmp(name, name_table+1, len))
            return hModule;
    }

    /* If uppercased 'name' matches exactly the module name of a module:
     * Return its handle
     */
    for (s = tmpstr; *s; s++)
    	*s = toupper(*s);

    for (hModule = hFirstModule; hModule ; hModule = pModule->next)
    {
	pModule = NE_GetPtr( hModule );
        if (!pModule) break;

        name_table = (BYTE *)pModule + pModule->name_table;
        if ((*name_table == len) && !strncmp(tmpstr, name_table+1, len))
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
	if (!lstrcmpiA(loadedfn, s))
	    return hModule;
    }

    /* If the extension of 'name' is '.EXE' and the base filename of 'name'
     * matches the base filename of the module filename of some 32-bit module:
     * Return the corresponding 16-bit dummy module handle. 
     */
    if (len >= 4 && !strcasecmp(name+len-4, ".EXE"))
    {
        HMODULE hModule = GetModuleHandleA( name );
        if ( hModule )
            return MapHModuleLS( hModule );
    }

    if (!strcmp(tmpstr,"MSDOS"))
	return 1;

    if (!strcmp(tmpstr,"TIMER"))
    {
	FIXME(module, "Eh... Should return caller's code segment, expect crash\n");
	return 0;
    }

    return 0;
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
 *		MapHModuleLS			(KERNEL32.520)
 */
HMODULE16 WINAPI MapHModuleLS(HMODULE hmod) {
	NE_MODULE	*pModule;

	if (!hmod)
		return ((TDB*)GlobalLock16(GetCurrentTask()))->hInstance;
	if (!HIWORD(hmod))
		return hmod; /* we already have a 16 bit module handle */
	pModule = (NE_MODULE*)GlobalLock16(hFirstModule);
	while (pModule)  {
		if (pModule->module32 == hmod)
			return pModule->self;
		pModule = (NE_MODULE*)GlobalLock16(pModule->next);
	}
	return 0;
}

/***************************************************************************
 *		MapHModuleSL			(KERNEL32.521)
 */
HMODULE WINAPI MapHModuleSL(HMODULE16 hmod) {
	NE_MODULE	*pModule;

	if (!hmod) {
		TDB *pTask = (TDB*)GlobalLock16(GetCurrentTask());

		hmod = pTask->hModule;
	}
	pModule = (NE_MODULE*)GlobalLock16(hmod);
	if (	(pModule->magic!=IMAGE_OS2_SIGNATURE)	||
		!(pModule->flags & NE_FFLAGS_WIN32)
	)
		return 0;
	return pModule->module32;
}

/***************************************************************************
 *		MapHInstLS			(KERNEL32.516)
 */
REGS_ENTRYPOINT(MapHInstLS) {
	EAX_reg(context) = MapHModuleLS(EAX_reg(context));
}

/***************************************************************************
 *		MapHInstSL			(KERNEL32.518)
 */
REGS_ENTRYPOINT(MapHInstSL) {
	EAX_reg(context) = MapHModuleSL(EAX_reg(context));
}

/***************************************************************************
 *		MapHInstLS_PN			(KERNEL32.517)
 */
REGS_ENTRYPOINT(MapHInstLS_PN) {
	if (EAX_reg(context))
	    EAX_reg(context) = MapHModuleLS(EAX_reg(context));
}

/***************************************************************************
 *		MapHInstSL_PN			(KERNEL32.519)
 */
REGS_ENTRYPOINT(MapHInstSL_PN) {
	if (EAX_reg(context))
	    EAX_reg(context) = MapHModuleSL(EAX_reg(context));
}

/***************************************************************************
 *		WIN16_MapHInstLS		(KERNEL.472)
 */
VOID WINAPI WIN16_MapHInstLS( CONTEXT *context ) {
	EAX_reg(context) = MapHModuleLS(EAX_reg(context));
}

/***************************************************************************
 *		WIN16_MapHInstSL		(KERNEL.473)
 */
VOID WINAPI WIN16_MapHInstSL( CONTEXT *context ) {
	EAX_reg(context) = MapHModuleSL(EAX_reg(context));
}
