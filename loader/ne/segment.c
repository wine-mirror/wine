/*
 * NE segment loading
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "neexe.h"
#include "windows.h"
#include "global.h"
#include "task.h"
#include "selectors.h"
#include "callback.h"
#include "file.h"
#include "module.h"
#include "stackframe.h"
#include "debug.h"
#include "xmalloc.h"


/***********************************************************************
 *           NE_GetRelocAddrName
 */
static const char *NE_GetRelocAddrName( BYTE addr_type, int additive )
{
    switch(addr_type & 0x7f)
    {
    case NE_RADDR_LOWBYTE:   return additive ? "BYTE add" : "BYTE";
    case NE_RADDR_OFFSET16:  return additive ? "OFFSET16 add" : "OFFSET16";
    case NE_RADDR_POINTER32: return additive ? "POINTER32 add" : "POINTER32";
    case NE_RADDR_SELECTOR:  return additive ? "SELECTOR add" : "SELECTOR";
    case NE_RADDR_POINTER48: return additive ? "POINTER48 add" : "POINTER48";
    case NE_RADDR_OFFSET32:  return additive ? "OFFSET32 add" : "OFFSET32";
    }
    return "???";
}


/***********************************************************************
 *           NE_LoadSegment
 */
BOOL32 NE_LoadSegment( NE_MODULE *pModule, WORD segnum )
{
    SEGTABLEENTRY *pSegTable, *pSeg;
    WORD *pModuleTable;
    WORD count, i, offset, next_offset;
    HMODULE16 module;
    FARPROC16 address = 0;
    int fd;
    struct relocation_entry_s *rep, *reloc_entries;
    BYTE *func_name;
    int size;
    char* mem;

    char buffer[256];
    int ordinal, additive;
    unsigned short *sp;

    pSegTable = NE_SEG_TABLE( pModule );
    pSeg = pSegTable + segnum - 1;
    pModuleTable = NE_MODULE_TABLE( pModule );

    if (!pSeg->filepos) return TRUE;  /* No file image, just return */
	
    fd = NE_OpenFile( pModule );
    TRACE(module, "Loading segment %d, selector=%04x, flags=%04x\n",
                    segnum, pSeg->selector, pSeg->flags );
    lseek( fd, pSeg->filepos << pModule->alignment, SEEK_SET );
    if (pSeg->size) size = pSeg->size;
    else if (pSeg->minsize) size = pSeg->minsize;
    else size = 0x10000;
    mem = GlobalLock16(pSeg->selector);
    if (pModule->flags & NE_FFLAGS_SELFLOAD && segnum > 1)
    {
 	/* Implement self loading segments */
 	SELFLOADHEADER *selfloadheader;
        STACK16FRAME *stack16Top;
        DWORD oldstack;
 	WORD oldselector, newselector;
        THDB *thdb = THREAD_Current();
        HFILE32 hf = FILE_DupUnixHandle( fd );

 	selfloadheader = (SELFLOADHEADER *)
 		PTR_SEG_OFF_TO_LIN(pSegTable->selector,0);
 	oldstack = thdb->cur_stack;
 	oldselector = pSeg->selector;
 	thdb->cur_stack = PTR_SEG_OFF_TO_SEGPTR(pModule->self_loading_sel,
                                                 0xff00 - sizeof(*stack16Top));
        stack16Top = (STACK16FRAME *)PTR_SEG_TO_LIN(thdb->cur_stack);
        stack16Top->frame32 = 0;
        stack16Top->ds = stack16Top->es = pModule->self_loading_sel;
        stack16Top->entry_point = 0;
        stack16Top->entry_ip = 0;
        stack16Top->entry_cs = 0;
        stack16Top->bp = 0;
        stack16Top->ip = 0;
        stack16Top->cs = 0;
 	newselector = Callbacks->CallLoadAppSegProc(selfloadheader->LoadAppSeg,
                                                   pModule->self, hf, segnum );
        _lclose32( hf );
 	if (newselector != oldselector) {
 	  /* Self loaders like creating their own selectors; 
 	   * they love asking for trouble to Wine developers
 	   */
 	  if (segnum == pModule->dgroup) {
 	    memcpy(PTR_SEG_OFF_TO_LIN(oldselector,0),
 		   PTR_SEG_OFF_TO_LIN(newselector,0), 
 		   pSeg->minsize ? pSeg->minsize : 0x10000);
 	    FreeSelector(newselector);
 	    pSeg->selector = oldselector;
 	    TRACE(module, "New selector allocated for dgroup segment:Old=%d,New=%d\n", 
                oldselector, newselector);
 	  } else {
 	    FreeSelector(pSeg->selector);
 	    pSeg->selector = newselector;
 	  }
 	} 
 	
 	thdb->cur_stack = oldstack;
    }
    else if (!(pSeg->flags & NE_SEGFLAGS_ITERATED))
      read(fd, mem, size);
    else {
      /*
	 The following bit of code for "iterated segments" was written without
	 any documentation on the format of these segments. It seems to work,
	 but may be missing something. If you have any doc please either send
	 it to me or fix the code yourself. gfm@werple.mira.net.au
      */
      char* buff = xmalloc(size);
      char* curr = buff;
      read(fd, buff, size);
      while(curr < buff + size) {
	unsigned int rept = *((short*) curr)++;
	unsigned int len = *((short*) curr)++;
	for(; rept > 0; rept--) {
	  char* bytes = curr;
	  unsigned int byte;
	  for(byte = 0; byte < len; byte++)
	    *mem++ = *bytes++;
	}
	curr += len;
      }
      free(buff);
    }

    pSeg->flags |= NE_SEGFLAGS_LOADED;
    if (!(pSeg->flags & NE_SEGFLAGS_RELOC_DATA))
        return TRUE;  /* No relocation data, we are done */

    read( fd, &count, sizeof(count) );
    if (!count) return TRUE;

    TRACE(fixup, "Fixups for %.*s, segment %d, selector %04x\n",
                   *((BYTE *)pModule + pModule->name_table),
                   (char *)pModule + pModule->name_table + 1,
                   segnum, pSeg->selector );
    TRACE(segment, "Fixups for %.*s, segment %d, selector %04x\n",
                   *((BYTE *)pModule + pModule->name_table),
                   (char *)pModule + pModule->name_table + 1,
                   segnum, pSeg->selector );

    reloc_entries = (struct relocation_entry_s *)xmalloc(count * sizeof(struct relocation_entry_s));
    if (read( fd, reloc_entries, count * sizeof(struct relocation_entry_s)) !=
            count * sizeof(struct relocation_entry_s))
    {
        WARN(fixup, "Unable to read relocation information\n" );
        return FALSE;
    }

    /*
     * Go through the relocation table one entry at a time.
     */
    rep = reloc_entries;
    for (i = 0; i < count; i++, rep++)
    {
	/*
	 * Get the target address corresponding to this entry.
	 */

	/* If additive, there is no target chain list. Instead, add source
	   and target */
	additive = rep->relocation_type & NE_RELFLAG_ADDITIVE;
	rep->relocation_type &= 0x3;

	switch (rep->relocation_type)
	{
	  case NE_RELTYPE_ORDINAL:
            module = pModuleTable[rep->target1-1];
	    ordinal = rep->target2;
            address = NE_GetEntryPoint( module, ordinal );
            if (!address)
            {
                NE_MODULE *pTarget = NE_GetPtr( module );
                if (!pTarget)
                    WARN(module, "Module not found: %04x, reference %d of module %*.*s\n",
                             module, rep->target1, 
                             *((BYTE *)pModule + pModule->name_table),
                             *((BYTE *)pModule + pModule->name_table),
                             (char *)pModule + pModule->name_table + 1 );
                else
                    WARN(module, "No handler for %.*s.%d, setting to 0:0\n",
                            *((BYTE *)pTarget + pTarget->name_table),
                            (char *)pTarget + pTarget->name_table + 1,
                            ordinal );
            }
            if (TRACE_ON(fixup))
            {
                NE_MODULE *pTarget = NE_GetPtr( module );
                TRACE( fixup, "%d: %.*s.%d=%04x:%04x %s\n", i + 1, 
                       *((BYTE *)pTarget + pTarget->name_table),
                       (char *)pTarget + pTarget->name_table + 1,
                       ordinal, HIWORD(address), LOWORD(address),
                       NE_GetRelocAddrName( rep->address_type, additive ) );
            }
	    break;
	    
	  case NE_RELTYPE_NAME:
            module = pModuleTable[rep->target1-1];
            func_name = (char *)pModule + pModule->import_table + rep->target2;
            memcpy( buffer, func_name+1, *func_name );
            buffer[*func_name] = '\0';
            func_name = buffer;
            ordinal = NE_GetOrdinal( module, func_name );
            address = NE_GetEntryPoint( module, ordinal );

            if (ERR_ON(fixup) && !address)
            {
                NE_MODULE *pTarget = NE_GetPtr( module );
                ERR(fixup, "Warning: no handler for %.*s.%s, setting to 0:0\n",
                    *((BYTE *)pTarget + pTarget->name_table),
                    (char *)pTarget + pTarget->name_table + 1, func_name );
            }
            if (TRACE_ON(fixup))
            {
	        NE_MODULE *pTarget = NE_GetPtr( module );
                TRACE( fixup, "%d: %.*s.%s=%04x:%04x %s\n", i + 1, 
                       *((BYTE *)pTarget + pTarget->name_table),
                       (char *)pTarget + pTarget->name_table + 1,
                       func_name, HIWORD(address), LOWORD(address),
                       NE_GetRelocAddrName( rep->address_type, additive ) );
            }
	    break;
	    
	  case NE_RELTYPE_INTERNAL:
	    if ((rep->target1 & 0xff) == 0xff)
	    {
		address  = NE_GetEntryPoint( pModule->self, rep->target2 );
	    }
	    else
	    {
                address = (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( pSegTable[rep->target1-1].selector, rep->target2 );
	    }
	    
	    TRACE( fixup,"%d: %04x:%04x %s\n", 
                   i + 1, HIWORD(address), LOWORD(address),
                   NE_GetRelocAddrName( rep->address_type, additive ) );
	    break;

	  case NE_RELTYPE_OSFIXUP:
	    /* Relocation type 7:
	     *
	     *    These appear to be used as fixups for the Windows
	     * floating point emulator.  Let's just ignore them and
	     * try to use the hardware floating point.  Linux should
	     * successfully emulate the coprocessor if it doesn't
	     * exist.
	     */
	    TRACE( fixup, "%d: TYPE %d, OFFSET %04x, TARGET %04x %04x %s\n",
                   i + 1, rep->relocation_type, rep->offset,
                   rep->target1, rep->target2,
                   NE_GetRelocAddrName( rep->address_type, additive ) );
	    continue;
	}

	offset  = rep->offset;

        /* Apparently, high bit of address_type is sometimes set; */
        /* we ignore it for now */
	if (rep->address_type > NE_RADDR_OFFSET32)
        {
            char module[10];
            GetModuleName( pModule->self, module, sizeof(module) );
            ERR( fixup, "WARNING: module %s: unknown reloc addr type = 0x%02x. Please report.\n",
                 module, rep->address_type );
        }

        if (additive)
        {
            sp = PTR_SEG_OFF_TO_LIN( pSeg->selector, offset );
            TRACE( fixup,"    %04x:%04x\n", offset, *sp );
            switch (rep->address_type & 0x7f)
            {
            case NE_RADDR_LOWBYTE:
                *(BYTE *)sp += LOBYTE((int)address);
                break;
            case NE_RADDR_OFFSET16:
		*sp += LOWORD(address);
                break;
            case NE_RADDR_POINTER32:
		*sp += LOWORD(address);
		*(sp+1) = HIWORD(address);
                break;
            case NE_RADDR_SELECTOR:
		/* Borland creates additive records with offset zero. Strange, but OK */
                if (*sp)
                    ERR(fixup,"Additive selector to %04x.Please report\n",*sp);
		else
                    *sp = HIWORD(address);
                break;
            default:
                goto unknown;
            }
        }
        else  /* non-additive fixup */
        {
            do
            {
                sp = PTR_SEG_OFF_TO_LIN( pSeg->selector, offset );
                next_offset = *sp;
                TRACE( fixup,"    %04x:%04x\n", offset, *sp );
                switch (rep->address_type & 0x7f)
                {
                case NE_RADDR_LOWBYTE:
                    *(BYTE *)sp = LOBYTE((int)address);
                    break;
                case NE_RADDR_OFFSET16:
                    *sp = LOWORD(address);
                    break;
                case NE_RADDR_POINTER32:
                    *(FARPROC16 *)sp = address;
                    break;
                case NE_RADDR_SELECTOR:
                    *sp = SELECTOROF(address);
                    break;
                default:
                    goto unknown;
                }
                if (next_offset == offset) break;  /* avoid infinite loop */
                if (next_offset >= GlobalSize16(pSeg->selector)) break;
                offset = next_offset;
            } while (offset && (offset != 0xffff));
        }
    }

    free(reloc_entries);
    return TRUE;

unknown:
    WARN(fixup, "WARNING: %d: unknown ADDR TYPE %d,  "
         "TYPE %d,  OFFSET %04x,  TARGET %04x %04x\n",
         i + 1, rep->address_type, rep->relocation_type, 
         rep->offset, rep->target1, rep->target2);
    free(reloc_entries);
    return FALSE;
}


/***********************************************************************
 *           NE_LoadAllSegments
 */
BOOL32 NE_LoadAllSegments( NE_MODULE *pModule )
{
    int i;

    if (pModule->flags & NE_FFLAGS_SELFLOAD)
    {
        HFILE32 hf;
        /* Handle self loading modules */
        SEGTABLEENTRY * pSegTable = (SEGTABLEENTRY *) NE_SEG_TABLE(pModule);
        SELFLOADHEADER *selfloadheader;
        STACK16FRAME *stack16Top;
        THDB *thdb = THREAD_Current();
        HMODULE16 hselfload = GetModuleHandle16("WPROCS");
        DWORD oldstack;
        WORD saved_dgroup = pSegTable[pModule->dgroup - 1].selector;

        TRACE(module, "%.*s is a self-loading module!\n",
		     *((BYTE*)pModule + pModule->name_table),
		     (char *)pModule + pModule->name_table + 1);
        if (!NE_LoadSegment( pModule, 1 )) return FALSE;
        selfloadheader = (SELFLOADHEADER *)
                          PTR_SEG_OFF_TO_LIN(pSegTable->selector, 0);
        selfloadheader->EntryAddrProc = NE_GetEntryPoint(hselfload,27);
        selfloadheader->MyAlloc  = NE_GetEntryPoint(hselfload,28);
        selfloadheader->SetOwner = NE_GetEntryPoint(GetModuleHandle16("KERNEL"),403);
        pModule->self_loading_sel = GlobalHandleToSel(GLOBAL_Alloc(GMEM_ZEROINIT, 0xFF00, pModule->self, FALSE, FALSE, FALSE));
        oldstack = thdb->cur_stack;
        thdb->cur_stack = PTR_SEG_OFF_TO_SEGPTR(pModule->self_loading_sel,
                                                0xff00 - sizeof(*stack16Top) );
        stack16Top = (STACK16FRAME *)PTR_SEG_TO_LIN(thdb->cur_stack);
        stack16Top->frame32 = 0;
        stack16Top->ebp = 0;
        stack16Top->ds = stack16Top->es = pModule->self_loading_sel;
        stack16Top->entry_point = 0;
        stack16Top->entry_ip = 0;
        stack16Top->entry_cs = 0;
        stack16Top->bp = 0;
        stack16Top->ip = 0;
        stack16Top->cs = 0;

        hf = FILE_DupUnixHandle( NE_OpenFile( pModule ) );
        Callbacks->CallBootAppProc(selfloadheader->BootApp, pModule->self, hf);
        _lclose32(hf);
        /* some BootApp procs overwrite the selector of dgroup */
        pSegTable[pModule->dgroup - 1].selector = saved_dgroup;
        thdb->cur_stack = oldstack;
        for (i = 2; i <= pModule->seg_count; i++)
            if (!NE_LoadSegment( pModule, i )) return FALSE;
    }
    else
    {
        for (i = 1; i <= pModule->seg_count; i++)
            if (!NE_LoadSegment( pModule, i )) return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           NE_FixupPrologs
 *
 * Fixup the exported functions prologs.
 */
void NE_FixupPrologs( NE_MODULE *pModule )
{
    SEGTABLEENTRY *pSegTable;
    WORD dgroup = 0;
    WORD sel;
    BYTE *p, *fixup_ptr, count;
    dbg_decl_str(module, 512);

    pSegTable = NE_SEG_TABLE(pModule);
    if (pModule->flags & NE_FFLAGS_SINGLEDATA)
        dgroup = pSegTable[pModule->dgroup-1].selector;

    TRACE(module, "(%04x)\n", pModule->self );
    p = (BYTE *)pModule + pModule->entry_table;
    while (*p)
    {
        if (p[1] == 0)  /* Unused entry */
        {
            p += 2;  /* Skip it */
            continue;
        }
        if (p[1] == 0xfe)  /* Constant entry */
        {
            p += 2 + *p * 3;  /* Skip it */
            continue;
        }

        /* Now fixup the entries of this bundle */
        count = *p;
        sel = p[1];
        p += 2;
        while (count-- > 0)
        {
	    dbg_reset_str(module);
            dsprintf(module,"Flags: %04x, sel %02x ", *p, sel);
            /* According to the output generated by TDUMP, the flags mean:
             * 0x0001 function is exported
	     * 0x0002 Single data (seems to occur only in DLLs)
	     */
	    if (sel == 0xff) { /* moveable */
		dsprintf(module, "(%02x) o %04x", p[3], *(WORD *)(p+4) );
		fixup_ptr = (char *)GET_SEL_BASE(pSegTable[p[3]-1].selector) + *(WORD *)(p + 4);
	    } else { /* fixed */
		dsprintf(module, "offset %04x", *(WORD *)(p+1) );
		fixup_ptr = (char *)GET_SEL_BASE(pSegTable[sel-1].selector) + 
		  *(WORD *)(p + 1);
	    }
	    TRACE(module, "%s Signature: %02x %02x %02x,ff %x\n",
			    dbg_str(module), fixup_ptr[0], fixup_ptr[1], 
			    fixup_ptr[2], pModule->flags );
            if (*p & 0x0001)
            {
                /* Verify the signature */
                if (((fixup_ptr[0] == 0x1e && fixup_ptr[1] == 0x58)
                     || (fixup_ptr[0] == 0x8c && fixup_ptr[1] == 0xd8))
                    && fixup_ptr[2] == 0x90)
                {
                    if (*p & 0x0002)
                    {
			if (pModule->flags & NE_FFLAGS_MULTIPLEDATA)
                        {
			    /* can this happen? */
			    ERR(fixup, "FixupPrologs got confused\n" );
			}
                        else if (pModule->flags & NE_FFLAGS_SINGLEDATA)
                        {
                            *fixup_ptr = 0xb8;	/* MOV AX, */
                            *(WORD *)(fixup_ptr+1) = dgroup;
                        }
                    }
                    else
                    {
			if (pModule->flags & NE_FFLAGS_MULTIPLEDATA) {
			    fixup_ptr[0] = 0x90; /* non-library: NOPs */
			    fixup_ptr[1] = 0x90;
			    fixup_ptr[2] = 0x90;
			}
                    }
                } else {
		    WARN(fixup, "Unknown signature\n" );
		}
            }
	    else
	      TRACE(module,"\n");
            p += (sel == 0xff) ? 6 : 3;  
        }
    }
}

/***********************************************************************
 *           NE_GetDLLInitParams
 */
static VOID NE_GetDLLInitParams( NE_MODULE *pModule, 
				 WORD *hInst, WORD *ds, WORD *heap )
{
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );

    if (!(pModule->flags & NE_FFLAGS_SINGLEDATA))
    {
        if (pModule->flags & NE_FFLAGS_MULTIPLEDATA || pModule->dgroup)
        {
            /* Not SINGLEDATA */
            ERR(dll, "Library is not marked SINGLEDATA\n");
            exit(1);
        }
        else  /* DATA NONE DLL */
        {
            *ds = 0;
            *heap = 0;
        }
    }
    else  /* DATA SINGLE DLL */
    {
	if (pModule->dgroup) {
            *ds   = pSegTable[pModule->dgroup-1].selector;
            *heap = pModule->heap_size;
	}
	else /* hmm, DLL has no dgroup,
		but why has it NE_FFLAGS_SINGLEDATA set ?
		Buggy DLL compiler ? */
	{
            *ds   = 0;
            *heap = 0;
	}
    }

    *hInst = *ds ? *ds : pModule->self;
}


/***********************************************************************
 *           NE_InitDLL
 *
 * Call the DLL initialization code
 */
static BOOL32 NE_InitDLL( TDB* pTask, NE_MODULE *pModule )
{
    SEGTABLEENTRY *pSegTable;
    WORD hInst, ds, heap, fs;
    CONTEXT context;

    pSegTable = NE_SEG_TABLE( pModule );

    if (!(pModule->flags & NE_FFLAGS_LIBMODULE) ||
        (pModule->flags & NE_FFLAGS_WIN32)) return TRUE; /*not a library*/

    /* Call USER signal handler. This is necessary to install a
     * proper loader for HICON and HCURSOR resources that this DLL 
     * may contain. InitApp() does this for task modules. */

    if (pTask && pTask->userhandler)
    {
        pTask->userhandler( pModule->self, USIG_DLL_LOAD, 0, pTask->hInstance,
                            pTask->hQueue );
    }

    if (!pModule->cs) return TRUE;  /* no initialization code */


    /* Registers at initialization must be:
     * cx     heap size
     * di     library instance
     * ds     data segment if any
     * es:si  command line (always 0)
     */

    memset( &context, 0, sizeof(context) );

    NE_GetDLLInitParams( pModule, &hInst, &ds, &heap );
    GET_FS( fs );

    ECX_reg(&context) = heap;
    EDI_reg(&context) = hInst;
    DS_reg(&context)  = ds;
    ES_reg(&context)  = ds;   /* who knows ... */
    FS_reg(&context)  = fs;

    CS_reg(&context)  = pSegTable[pModule->cs-1].selector;
    EIP_reg(&context) = pModule->ip;
    EBP_reg(&context) = OFFSETOF(THREAD_Current()->cur_stack)
                          + (WORD)&((STACK16FRAME*)0)->bp;


    pModule->cs = 0;  /* Don't initialize it twice */
    TRACE(dll, "Calling LibMain, cs:ip=%04lx:%04x ds=%04lx di=%04x cx=%04x\n", 
                 CS_reg(&context), IP_reg(&context), DS_reg(&context),
                 DI_reg(&context), CX_reg(&context) );
    Callbacks->CallRegisterShortProc( &context, 0 );
    return TRUE;
}

/***********************************************************************
 *           NE_CallDllEntryPoint
 *
 * Call the DllEntryPoint of DLLs with subsystem >= 4.0 
 */

static void NE_CallDllEntryPoint( NE_MODULE *pModule, DWORD dwReason )
{
    WORD hInst, ds, heap, fs;
    FARPROC16 entryPoint;
    WORD ordinal;
    CONTEXT context;
    THDB *thdb = THREAD_Current();
    LPBYTE stack = (LPBYTE)THREAD_STACK16(thdb);

    if (pModule->expected_version < 0x0400) return;
    if (!(ordinal = NE_GetOrdinal( pModule->self, "DllEntryPoint" ))) return;
    if (!(entryPoint = NE_GetEntryPoint( pModule->self, ordinal ))) return;

    memset( &context, 0, sizeof(context) );

    NE_GetDLLInitParams( pModule, &hInst, &ds, &heap );
    GET_FS( fs );

    DS_reg(&context) = ds;
    ES_reg(&context) = ds;   /* who knows ... */
    FS_reg(&context) = fs;

    CS_reg(&context) = HIWORD(entryPoint);
    IP_reg(&context) = LOWORD(entryPoint);
    EBP_reg(&context) =  OFFSETOF( thdb->cur_stack )
                         + (WORD)&((STACK16FRAME*)0)->bp;

    *(DWORD *)(stack -  4) = dwReason;      /* dwReason */
    *(WORD *) (stack -  6) = hInst;         /* hInst */
    *(WORD *) (stack -  8) = ds;            /* wDS */
    *(WORD *) (stack - 10) = heap;          /* wHeapSize */
    *(DWORD *)(stack - 14) = 0;             /* dwReserved1 */
    *(WORD *) (stack - 16) = 0;             /* wReserved2 */

    TRACE(dll, "Calling DllEntryPoint, cs:ip=%04lx:%04x\n",
          CS_reg(&context), IP_reg(&context));

    Callbacks->CallRegisterShortProc( &context, 16 );
}


/***********************************************************************
 *           NE_InitializeDLLs
 *
 * Recursively initialize all DLLs (according to the order in which 
 * they where loaded).
 */
void NE_InitializeDLLs( HMODULE16 hModule )
{
    TDB* pTask = (TDB*)GlobalLock16(GetCurrentTask());
    NE_MODULE *pModule;
    HMODULE16 *pDLL;

    if (!(pModule = NE_GetPtr( hModule ))) return;
    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

    if (pModule->dlls_to_init)
    {
	HGLOBAL16 to_init = pModule->dlls_to_init;
	pModule->dlls_to_init = 0;
        for (pDLL = (HMODULE16 *)GlobalLock16( to_init ); *pDLL; pDLL++)
        {
            NE_InitializeDLLs( *pDLL );
        }
        GlobalFree16( to_init );
    }
    NE_InitDLL( pTask, pModule );
    NE_CallDllEntryPoint( pModule, DLL_PROCESS_ATTACH );
}


/***********************************************************************
 *           NE_CreateInstance
 *
 * If lib_only is TRUE, handle the module like a library even if it is a .EXE
 */
HINSTANCE16 NE_CreateInstance( NE_MODULE *pModule, HINSTANCE16 *prev,
                               BOOL32 lib_only )
{
    SEGTABLEENTRY *pSegment;
    int minsize;
    HINSTANCE16 hNewInstance;

    if (pModule->dgroup == 0)
    {
        if (prev) *prev = pModule->self;
        return pModule->self;
    }

    pSegment = NE_SEG_TABLE( pModule ) + pModule->dgroup - 1;
    if (prev) *prev = pSegment->selector;

      /* if it's a library, create a new instance only the first time */
    if (pSegment->selector)
    {
        if (pModule->flags & NE_FFLAGS_LIBMODULE) return pSegment->selector;
        if (lib_only) return pSegment->selector;
    }

    minsize = pSegment->minsize ? pSegment->minsize : 0x10000;
    if (pModule->ss == pModule->dgroup) minsize += pModule->stack_size;
    minsize += pModule->heap_size;
    hNewInstance = GLOBAL_Alloc( GMEM_ZEROINIT | GMEM_FIXED, minsize,
                                 pModule->self, FALSE, FALSE, FALSE );
    if (!hNewInstance) return 0;
    pSegment->selector = hNewInstance;
    return hNewInstance;
}


/***********************************************************************
 *           PatchCodeHandle
 *
 * Needed for self-loading modules.
 */

/* It does nothing */
void WINAPI PatchCodeHandle(HANDLE16 hSel)
{
    FIXME(module,"(%04x): stub.\n",hSel);
}


/***********************************************************************
 *           NE_Ne2MemFlags
 *
 * This function translates NE segment flags to GlobalAlloc flags
 */
static WORD NE_Ne2MemFlags(WORD flags)
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
 *           NE_AllocateSegment (WPROCS.26)
 */
DWORD WINAPI NE_AllocateSegment( WORD wFlags, WORD wSize, WORD wElem )
{
    WORD size = wSize << wElem;
    HANDLE16 hMem = GlobalAlloc16( NE_Ne2MemFlags(wFlags), size);
    return MAKELONG( hMem, GlobalHandleToSel(hMem) );
}


/***********************************************************************
 *           NE_CreateSegments
 */
BOOL32 NE_CreateSegments( NE_MODULE *pModule )
{
    SEGTABLEENTRY *pSegment;
    int i, minsize;

    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

    pSegment = NE_SEG_TABLE( pModule );
    for (i = 1; i <= pModule->seg_count; i++, pSegment++)
    {
        minsize = pSegment->minsize ? pSegment->minsize : 0x10000;
        if (i == pModule->ss) minsize += pModule->stack_size;
	/* The DGROUP is allocated by NE_CreateInstance */
        if (i == pModule->dgroup) continue;
        pSegment->selector = GLOBAL_Alloc( NE_Ne2MemFlags(pSegment->flags),
                                      minsize, pModule->self,
                                      !(pSegment->flags & NE_SEGFLAGS_DATA),
                                      FALSE,
                            FALSE /*pSegment->flags & NE_SEGFLAGS_READONLY*/ );
        if (!pSegment->selector) return FALSE;
    }

    pModule->dgroup_entry = pModule->dgroup ? pModule->seg_table +
                            (pModule->dgroup - 1) * sizeof(SEGTABLEENTRY) : 0;
    return TRUE;
}


/**********************************************************************
 *	    IsSharedSelector    (KERNEL.345)
 */
BOOL16 WINAPI IsSharedSelector( HANDLE16 selector )
{
    /* Check whether the selector belongs to a DLL */
    NE_MODULE *pModule = NE_GetPtr( selector );
    if (!pModule) return FALSE;
    return (pModule->flags & NE_FFLAGS_LIBMODULE) != 0;
}
