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

#include "wine/winbase16.h"
#include "neexe.h"
#include "global.h"
#include "task.h"
#include "selectors.h"
#include "callback.h"
#include "file.h"
#include "module.h"
#include "stackframe.h"
#include "builtin16.h"
#include "debugtools.h"
#include "toolhelp.h"

DECLARE_DEBUG_CHANNEL(dll)
DECLARE_DEBUG_CHANNEL(fixup)
DECLARE_DEBUG_CHANNEL(module)
DECLARE_DEBUG_CHANNEL(segment)

#define SEL(x) ((x)|1)

static void NE_FixupSegmentPrologs(NE_MODULE *pModule, WORD segnum);

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
BOOL NE_LoadSegment( NE_MODULE *pModule, WORD segnum )
{
    SEGTABLEENTRY *pSegTable, *pSeg;
    WORD *pModuleTable;
    WORD count, i, offset, next_offset;
    HMODULE16 module;
    FARPROC16 address = 0;
    HFILE hf;
    DWORD res;
    struct relocation_entry_s *rep, *reloc_entries;
    BYTE *func_name;
    int size;
    char* mem;

    char buffer[256];
    int ordinal, additive;
    unsigned short *sp;

    pSegTable = NE_SEG_TABLE( pModule );
    pSeg = pSegTable + segnum - 1;

    if (pSeg->flags & NE_SEGFLAGS_LOADED)
    {
	/* self-loader ? -> already loaded it */
	if (pModule->flags & NE_FFLAGS_SELFLOAD)
	    return TRUE;

	/* leave, except for DGROUP, as this may be the second instance */
	if (segnum != pModule->dgroup)
            return TRUE;
    }

    if (!pSeg->filepos) return TRUE;  /* No file image, just return */
	
    pModuleTable = NE_MODULE_TABLE( pModule );

    hf = NE_OpenFile( pModule );
    TRACE_(module)("Loading segment %d, hSeg=%04x, flags=%04x\n",
                    segnum, pSeg->hSeg, pSeg->flags );
    SetFilePointer( hf, pSeg->filepos << pModule->alignment, NULL, SEEK_SET );
    if (pSeg->size) size = pSeg->size;
    else size = pSeg->minsize ? pSeg->minsize : 0x10000;
    mem = GlobalLock16(pSeg->hSeg);
    if (pModule->flags & NE_FFLAGS_SELFLOAD && segnum > 1)
    {
 	/* Implement self-loading segments */
 	SELFLOADHEADER *selfloadheader;
        DWORD oldstack;
        HFILE hFile32;
        HFILE16 hFile16;

 	selfloadheader = (SELFLOADHEADER *)
 		PTR_SEG_OFF_TO_LIN(SEL(pSegTable->hSeg),0);
 	oldstack = NtCurrentTeb()->cur_stack;
 	NtCurrentTeb()->cur_stack = PTR_SEG_OFF_TO_SEGPTR(pModule->self_loading_sel,
                                                          0xff00 - sizeof(STACK16FRAME));

	TRACE_(dll)("CallLoadAppSegProc(hmodule=0x%04x,hf=0x%04x,segnum=%d\n",
		pModule->self,hf,segnum );
        DuplicateHandle( GetCurrentProcess(), hf, GetCurrentProcess(), &hFile32,
                         0, FALSE, DUPLICATE_SAME_ACCESS );
        hFile16 = FILE_AllocDosHandle( hFile32 );
 	pSeg->hSeg = Callbacks->CallLoadAppSegProc( selfloadheader->LoadAppSeg,
                                                    pModule->self, hFile16,
                                                    segnum );
	TRACE_(dll)("Ret CallLoadAppSegProc: hSeg = 0x%04x\n", pSeg->hSeg);
        _lclose16( hFile16 );
 	NtCurrentTeb()->cur_stack = oldstack;
    }
    else if (!(pSeg->flags & NE_SEGFLAGS_ITERATED))
        ReadFile(hf, mem, size, &res, NULL);
    else {
      /*
	 The following bit of code for "iterated segments" was written without
	 any documentation on the format of these segments. It seems to work,
	 but may be missing something. If you have any doc please either send
	 it to me or fix the code yourself. gfm@werple.mira.net.au
      */
      char* buff = HeapAlloc(GetProcessHeap(), 0, size);
      char* curr = buff;

      if(buff == NULL) {
          WARN_(dll)("Memory exausted!");
          return FALSE;
      }

      ReadFile(hf, buff, size, &res, NULL);
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
      HeapFree(GetProcessHeap(), 0, buff);
    }

    pSeg->flags |= NE_SEGFLAGS_LOADED;

    /* Perform exported function prolog fixups */
    NE_FixupSegmentPrologs( pModule, segnum );

    if (!(pSeg->flags & NE_SEGFLAGS_RELOC_DATA))
        return TRUE;  /* No relocation data, we are done */

    ReadFile(hf, &count, sizeof(count), &res, NULL);
    if (!count) return TRUE;

    TRACE_(fixup)("Fixups for %.*s, segment %d, hSeg %04x\n",
                   *((BYTE *)pModule + pModule->name_table),
                   (char *)pModule + pModule->name_table + 1,
                   segnum, pSeg->hSeg );
    TRACE_(segment)("Fixups for %.*s, segment %d, hSeg %04x\n",
                   *((BYTE *)pModule + pModule->name_table),
                   (char *)pModule + pModule->name_table + 1,
                   segnum, pSeg->hSeg );

    reloc_entries = (struct relocation_entry_s *)HeapAlloc(GetProcessHeap(), 0, count * sizeof(struct relocation_entry_s));
    if(reloc_entries == NULL) {
        WARN_(fixup)("Not enough memory for relocation entries!");
        return FALSE;
    }
    if (!ReadFile( hf, reloc_entries, count * sizeof(struct relocation_entry_s), &res, NULL) ||
        (res != count * sizeof(struct relocation_entry_s)))
    {
        WARN_(fixup)("Unable to read relocation information\n" );
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
                    WARN_(module)("Module not found: %04x, reference %d of module %*.*s\n",
                             module, rep->target1, 
                             *((BYTE *)pModule + pModule->name_table),
                             *((BYTE *)pModule + pModule->name_table),
                             (char *)pModule + pModule->name_table + 1 );
                else
                {
                    ERR_(fixup)("No implementation for %.*s.%d, setting to 0xdeadbeef\n",
                            *((BYTE *)pTarget + pTarget->name_table),
                            (char *)pTarget + pTarget->name_table + 1,
                            ordinal );
                    address = (FARPROC16)0xdeadbeef;
                }
            }
            if (TRACE_ON(fixup))
            {
                NE_MODULE *pTarget = NE_GetPtr( module );
                TRACE_(fixup)("%d: %.*s.%d=%04x:%04x %s\n", i + 1, 
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
                ERR_(fixup)("No implementation for %.*s.%s, setting to 0xdeadbeef\n",
                    *((BYTE *)pTarget + pTarget->name_table),
                    (char *)pTarget + pTarget->name_table + 1, func_name );
            }
            if (!address) address = (FARPROC16) 0xdeadbeef;
            if (TRACE_ON(fixup))
            {
	        NE_MODULE *pTarget = NE_GetPtr( module );
                TRACE_(fixup)("%d: %.*s.%s=%04x:%04x %s\n", i + 1, 
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
                address = (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( SEL(pSegTable[rep->target1-1].hSeg), rep->target2 );
	    }
	    
	    TRACE_(fixup)("%d: %04x:%04x %s\n", 
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
	    TRACE_(fixup)("%d: TYPE %d, OFFSET %04x, TARGET %04x %04x %s\n",
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
            GetModuleName16( pModule->self, module, sizeof(module) );
            ERR_(fixup)("WARNING: module %s: unknown reloc addr type = 0x%02x. Please report.\n",
                 module, rep->address_type );
        }

        if (additive)
        {
            sp = PTR_SEG_OFF_TO_LIN( SEL(pSeg->hSeg), offset );
            TRACE_(fixup)("    %04x:%04x\n", offset, *sp );
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
                    ERR_(fixup)("Additive selector to %04x.Please report\n",*sp);
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
                sp = PTR_SEG_OFF_TO_LIN( SEL(pSeg->hSeg), offset );
                next_offset = *sp;
                TRACE_(fixup)("    %04x:%04x\n", offset, *sp );
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
                if (next_offset >= GlobalSize16(pSeg->hSeg)) break;
                offset = next_offset;
            } while (offset != 0xffff);
        }
    }

    HeapFree(GetProcessHeap(), 0, reloc_entries);
    return TRUE;

unknown:
    WARN_(fixup)("WARNING: %d: unknown ADDR TYPE %d,  "
         "TYPE %d,  OFFSET %04x,  TARGET %04x %04x\n",
         i + 1, rep->address_type, rep->relocation_type, 
         rep->offset, rep->target1, rep->target2);
    HeapFree(GetProcessHeap(), 0, reloc_entries);
    return FALSE;
}


/***********************************************************************
 *           NE_LoadAllSegments
 */
BOOL NE_LoadAllSegments( NE_MODULE *pModule )
{
    int i;
    SEGTABLEENTRY * pSegTable = (SEGTABLEENTRY *) NE_SEG_TABLE(pModule);

    if (pModule->flags & NE_FFLAGS_SELFLOAD)
    {
        HFILE hf;
        HFILE16 hFile16;
        /* Handle self-loading modules */
        SELFLOADHEADER *selfloadheader;
        HMODULE16 hselfload = GetModuleHandle16("WPROCS");
        DWORD oldstack;

        TRACE_(module)("%.*s is a self-loading module!\n",
		     *((BYTE*)pModule + pModule->name_table),
		     (char *)pModule + pModule->name_table + 1);
        if (!NE_LoadSegment( pModule, 1 )) return FALSE;
        selfloadheader = (SELFLOADHEADER *)
                          PTR_SEG_OFF_TO_LIN(SEL(pSegTable->hSeg), 0);
        selfloadheader->EntryAddrProc = NE_GetEntryPoint(hselfload,27);
        selfloadheader->MyAlloc  = NE_GetEntryPoint(hselfload,28);
        selfloadheader->SetOwner = NE_GetEntryPoint(GetModuleHandle16("KERNEL"),403);
        pModule->self_loading_sel = SEL(GLOBAL_Alloc(GMEM_ZEROINIT, 0xFF00, pModule->self, FALSE, FALSE, FALSE));
        oldstack = NtCurrentTeb()->cur_stack;
        NtCurrentTeb()->cur_stack = PTR_SEG_OFF_TO_SEGPTR(pModule->self_loading_sel,
                                                          0xff00 - sizeof(STACK16FRAME) );

        DuplicateHandle( GetCurrentProcess(), NE_OpenFile(pModule),
                         GetCurrentProcess(), &hf, 0, FALSE, DUPLICATE_SAME_ACCESS );
        hFile16 = FILE_AllocDosHandle( hf );
        TRACE_(dll)("CallBootAppProc(hModule=0x%04x,hf=0x%04x)\n",
              pModule->self,hFile16);
        Callbacks->CallBootAppProc(selfloadheader->BootApp, pModule->self,hFile16);
	TRACE_(dll)("Return from CallBootAppProc\n");
        _lclose16(hf);
        NtCurrentTeb()->cur_stack = oldstack;

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
 *           NE_FixupSegmentPrologs
 *
 * Fixup exported functions prologs of one segment
 */
static void NE_FixupSegmentPrologs(NE_MODULE *pModule, WORD segnum)
{
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );
    ET_BUNDLE *bundle;
    ET_ENTRY *entry;
    WORD dgroup, num_entries, sel = SEL(pSegTable[segnum-1].hSeg);
    BYTE *pSeg, *pFunc;

    TRACE_(module)("(%d);\n", segnum);

    if (pSegTable[segnum-1].flags & NE_SEGFLAGS_DATA)
    {
	pSegTable[segnum-1].flags |= NE_SEGFLAGS_LOADED;
	return;
    }

    if (!pModule->dgroup) return;

    if (!(dgroup = SEL(pSegTable[pModule->dgroup-1].hSeg))) return;
    
    pSeg = PTR_SEG_OFF_TO_LIN(sel, 0);

    bundle = (ET_BUNDLE *)((BYTE *)pModule+pModule->entry_table);

    do {
	TRACE_(module)("num_entries: %d, bundle: %p, next: %04x, pSeg: %p\n", bundle->last - bundle->first, bundle, bundle->next, pSeg);
	if (!(num_entries = bundle->last - bundle->first))
	    return;
	entry = (ET_ENTRY *)((BYTE *)bundle+6);
	while (num_entries--)
    {
	    /*TRACE(module, "entry: %p, entry->segnum: %d, entry->offs: %04x\n", entry, entry->segnum, entry->offs);*/
	    if (entry->segnum == segnum)
        {
		pFunc = ((BYTE *)pSeg+entry->offs);
		TRACE_(module)("pFunc: %p, *(DWORD *)pFunc: %08lx, num_entries: %d\n", pFunc, *(DWORD *)pFunc, num_entries);
		if (*(pFunc+2) == 0x90)
        {
		    if (*(WORD *)pFunc == 0x581e) /* push ds, pop ax */
		    {
			TRACE_(module)("patch %04x:%04x -> mov ax, ds\n", sel, entry->offs);
			*(WORD *)pFunc = 0xd88c; /* mov ax, ds */
        }

		    if (*(WORD *)pFunc == 0xd88c)
                        {
			if ((entry->flags & 2)) /* public data ? */
                        {
			    TRACE_(module)("patch %04x:%04x -> mov ax, dgroup [%04x]\n", sel, entry->offs, dgroup);
			    *pFunc = 0xb8; /* mov ax, */
			    *(WORD *)(pFunc+1) = dgroup;
                    }
                    else
			if ((pModule->flags & NE_FFLAGS_MULTIPLEDATA)
			&& (entry->flags & 1)) /* exported ? */
                    {
			    TRACE_(module)("patch %04x:%04x -> nop, nop\n", sel, entry->offs);
			    *(WORD *)pFunc = 0x9090; /* nop, nop */
			}
                    }
		}
            }
	    entry++;
	}
    } while ( (bundle->next)
	 && (bundle = ((ET_BUNDLE *)((BYTE *)pModule + bundle->next))) );
}


/***********************************************************************
 *           PatchCodeHandle
 *
 * Needed for self-loading modules.
 */
DWORD WINAPI PatchCodeHandle16(HANDLE16 hSeg)
{
    WORD segnum;
    WORD sel = SEL(hSeg);
    NE_MODULE *pModule = NE_GetPtr(FarGetOwner16(sel));
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE(pModule);

    TRACE_(module)("(%04x);\n", hSeg);

    /* find the segment number of the module that belongs to hSeg */
    for (segnum = 1; segnum <= pModule->seg_count; segnum++)
    {
	if (SEL(pSegTable[segnum-1].hSeg) == sel)
	{
	    NE_FixupSegmentPrologs(pModule, segnum);
	    break;
        }
    }

    return MAKELONG(hSeg, sel);
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
            ERR_(dll)("Library is not marked SINGLEDATA\n");
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
            *ds   = SEL(pSegTable[pModule->dgroup-1].hSeg);
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

    *hInst = *ds ? GlobalHandle16(*ds) : pModule->self;
}


/***********************************************************************
 *           NE_InitDLL
 *
 * Call the DLL initialization code
 */
static BOOL NE_InitDLL( TDB* pTask, NE_MODULE *pModule )
{
    SEGTABLEENTRY *pSegTable;
    WORD hInst, ds, heap;
    CONTEXT86 context;

    pSegTable = NE_SEG_TABLE( pModule );

    if (!(pModule->flags & NE_FFLAGS_LIBMODULE) ||
        (pModule->flags & NE_FFLAGS_WIN32)) return TRUE; /*not a library*/

    /* Call USER signal handler for Win3.1 compatibility. */
    TASK_CallTaskSignalProc( USIG16_DLL_LOAD, pModule->self );

    if (!pModule->cs) return TRUE;  /* no initialization code */


    /* Registers at initialization must be:
     * cx     heap size
     * di     library instance
     * ds     data segment if any
     * es:si  command line (always 0)
     */

    memset( &context, 0, sizeof(context) );

    NE_GetDLLInitParams( pModule, &hInst, &ds, &heap );

    ECX_reg(&context) = heap;
    EDI_reg(&context) = hInst;
    DS_reg(&context)  = ds;
    ES_reg(&context)  = ds;   /* who knows ... */

    CS_reg(&context)  = SEL(pSegTable[pModule->cs-1].hSeg);
    EIP_reg(&context) = pModule->ip;
    EBP_reg(&context) = OFFSETOF(NtCurrentTeb()->cur_stack) + (WORD)&((STACK16FRAME*)0)->bp;


    pModule->cs = 0;  /* Don't initialize it twice */
    TRACE_(dll)("Calling LibMain, cs:ip=%04lx:%04lx ds=%04lx di=%04x cx=%04x\n", 
                 CS_reg(&context), EIP_reg(&context), DS_reg(&context),
                 DI_reg(&context), CX_reg(&context) );
    Callbacks->CallRegisterShortProc( &context, 0 );
    return TRUE;
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
}


/***********************************************************************
 *           NE_CallDllEntryPoint
 *
 * Call the DllEntryPoint of DLLs with subsystem >= 4.0 
 */
typedef DWORD WINAPI (*WinNEEntryProc)(DWORD,WORD,WORD,WORD,DWORD,WORD);

static void NE_CallDllEntryPoint( NE_MODULE *pModule, DWORD dwReason )
{
    WORD hInst, ds, heap;
    FARPROC16 entryPoint;
    WORD ordinal;

    if (!(pModule->flags & NE_FFLAGS_LIBMODULE)) return;
    if (!(pModule->flags & NE_FFLAGS_BUILTIN) && pModule->expected_version < 0x0400) return;
    if (!(ordinal = NE_GetOrdinal( pModule->self, "DllEntryPoint" ))) return;
    if (!(entryPoint = NE_GetEntryPoint( pModule->self, ordinal ))) return;

    NE_GetDLLInitParams( pModule, &hInst, &ds, &heap );

    TRACE_(dll)( "Calling %s DllEntryPoint, cs:ip=%04x:%04x\n",
                 NE_MODULE_NAME( pModule ),
                 SELECTOROF(entryPoint), OFFSETOF(entryPoint) );

    if ( pModule->flags & NE_FFLAGS_BUILTIN )
    {
        WinNEEntryProc entryProc = (WinNEEntryProc)
			  ((ENTRYPOINT16 *)PTR_SEG_TO_LIN( entryPoint ))->target;

        entryProc( dwReason, hInst, ds, heap, 0, 0 );
    }
    else
    {
        LPBYTE stack = (LPBYTE)CURRENT_STACK16;
        CONTEXT86 context;

        memset( &context, 0, sizeof(context) );
        DS_reg(&context) = ds;
        ES_reg(&context) = ds;   /* who knows ... */

        CS_reg(&context) = HIWORD(entryPoint);
        EIP_reg(&context) = LOWORD(entryPoint);
        EBP_reg(&context) =  OFFSETOF( NtCurrentTeb()->cur_stack )
                             + (WORD)&((STACK16FRAME*)0)->bp;

        *(DWORD *)(stack -  4) = dwReason;      /* dwReason */
        *(WORD *) (stack -  6) = hInst;         /* hInst */
        *(WORD *) (stack -  8) = ds;            /* wDS */
        *(WORD *) (stack - 10) = heap;          /* wHeapSize */
        *(DWORD *)(stack - 14) = 0;             /* dwReserved1 */
        *(WORD *) (stack - 16) = 0;             /* wReserved2 */

        Callbacks->CallRegisterShortProc( &context, 16 );
    }
}

/***********************************************************************
 *           NE_DllProcessAttach
 * 
 * Call the DllEntryPoint of all modules this one (recursively)
 * depends on, according to the order in which they were loaded.
 *
 * Note that --as opposed to the PE module case-- there is no notion
 * of 'module loaded into a process' for NE modules, and hence we 
 * have no place to store the fact that the DllEntryPoint of a
 * given module was already called on behalf of this process (e.g.
 * due to some earlier LoadLibrary16 call).
 *
 * Thus, we just call the DllEntryPoint twice in that case.  Win9x
 * appears to behave this way as well ...
 *
 * This routine must only be called with the Win16Lock held.
 * 
 * FIXME:  We should actually abort loading in case the DllEntryPoint
 *         returns FALSE ...
 *
 */
void NE_DllProcessAttach( HMODULE16 hModule )
{
    NE_MODULE *pModule;
    WORD *pModRef;
    int i;

    if (!(pModule = NE_GetPtr( hModule ))) return;
    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

    /* Check for recursive call */
    if ( pModule->misc_flags & 0x80 ) return;

    TRACE_(dll)("(%s) - START\n", NE_MODULE_NAME(pModule) );

    /* Tag current module to prevent recursive loop */
    pModule->misc_flags |= 0x80;

    /* Recursively attach all DLLs this one depends on */
    pModRef = NE_MODULE_TABLE( pModule );
    for ( i = 0; i < pModule->modref_count; i++ )
        if ( pModRef[i] )
            NE_DllProcessAttach( (HMODULE16)pModRef[i] );

    /* Call DLL entry point */
    NE_CallDllEntryPoint( pModule, DLL_PROCESS_ATTACH );

    /* Remove recursion flag */
    pModule->misc_flags &= ~0x80;

    TRACE_(dll)("(%s) - END\n", NE_MODULE_NAME(pModule) );
}


/***********************************************************************
 *           NE_Ne2MemFlags
 *
 * This function translates NE segment flags to GlobalAlloc flags
 */
static WORD NE_Ne2MemFlags(WORD flags)
{ 
    WORD memflags = 0;
#if 1
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
#endif
    return memflags;
}

/***********************************************************************
 *           NE_AllocateSegment (WPROCS.26)
 *
 * MyAlloc() function for self-loading apps.
 */
DWORD WINAPI NE_AllocateSegment( WORD wFlags, WORD wSize, WORD wElem )
{
    WORD size = wSize << wElem;
    HANDLE16 hMem = 0;

    if (wSize || (wFlags & NE_SEGFLAGS_MOVEABLE))
        hMem = GlobalAlloc16( NE_Ne2MemFlags(wFlags), size);

    if ( ((wFlags & 0x7) != 0x1) && /* DATA */
         ((wFlags & 0x7) != 0x7) ) /* DATA|ALLOCATED|LOADED */
    {
        WORD hSel = SEL(hMem);
        WORD access = SelectorAccessRights16(hSel,0,0);

	access |= 2<<2; /* SEGMENT_CODE */
	SelectorAccessRights16(hSel,1,access);
    }
    if (size)
	return MAKELONG( hMem, SEL(hMem) );
    else
	return MAKELONG( 0, hMem );
}

/***********************************************************************
 *           NE_GetInstance
 */
HINSTANCE16 NE_GetInstance( NE_MODULE *pModule )
{
    if ( !pModule->dgroup )
        return pModule->self;
    else
    {
        SEGTABLEENTRY *pSeg;
        pSeg = NE_SEG_TABLE( pModule ) + pModule->dgroup - 1;
        return pSeg->hSeg;
    }
}    

/***********************************************************************
 *           NE_CreateSegment
 */
BOOL NE_CreateSegment( NE_MODULE *pModule, int segnum )
{
    SEGTABLEENTRY *pSeg = NE_SEG_TABLE( pModule ) + segnum - 1;
    int minsize;

    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

    if ( segnum < 1 || segnum > pModule->seg_count )
        return FALSE;

    if ( (pModule->flags & NE_FFLAGS_SELFLOAD) && segnum != 1 )
        return TRUE;    /* selfloader allocates segment itself */

    if ( (pSeg->flags & NE_SEGFLAGS_ALLOCATED) && segnum != pModule->dgroup )
        return TRUE;    /* all but DGROUP only allocated once */

    minsize = pSeg->minsize ? pSeg->minsize : 0x10000;
    if ( segnum == pModule->ss )     minsize += pModule->stack_size;
    if ( segnum == pModule->dgroup ) minsize += pModule->heap_size;

    pSeg->hSeg = GLOBAL_Alloc( NE_Ne2MemFlags(pSeg->flags),
                                   minsize, pModule->self,
                                   !(pSeg->flags & NE_SEGFLAGS_DATA),
                                    (pSeg->flags & NE_SEGFLAGS_32BIT) != 0,
                            FALSE /*pSeg->flags & NE_SEGFLAGS_READONLY*/ );
    if (!pSeg->hSeg) return FALSE;

    pSeg->flags |= NE_SEGFLAGS_ALLOCATED;
    return TRUE;
}

/***********************************************************************
 *           NE_CreateAllSegments
 */
BOOL NE_CreateAllSegments( NE_MODULE *pModule )
{
    int i;
    for ( i = 1; i <= pModule->seg_count; i++ )
        if ( !NE_CreateSegment( pModule, i ) )
            return FALSE;

    pModule->dgroup_entry = pModule->dgroup ? pModule->seg_table +
                            (pModule->dgroup - 1) * sizeof(SEGTABLEENTRY) : 0;
    return TRUE;
}


/**********************************************************************
 *	    IsSharedSelector    (KERNEL.345)
 */
BOOL16 WINAPI IsSharedSelector16( HANDLE16 selector )
{
    /* Check whether the selector belongs to a DLL */
    NE_MODULE *pModule = NE_GetPtr( selector );
    if (!pModule) return FALSE;
    return (pModule->flags & NE_FFLAGS_LIBMODULE) != 0;
}
