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

#define SEL(x) GlobalHandleToSel(x)

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
    HFILE32 hf;
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

    if (pSeg->flags & NE_SEGFLAGS_LOADED) /* already loaded ? */
        return TRUE;

    if (!pSeg->filepos) return TRUE;  /* No file image, just return */
	
    pModuleTable = NE_MODULE_TABLE( pModule );

    hf = NE_OpenFile( pModule );
    TRACE(module, "Loading segment %d, hSeg=%04x, flags=%04x\n",
                    segnum, pSeg->hSeg, pSeg->flags );
    SetFilePointer( hf, pSeg->filepos << pModule->alignment, NULL, SEEK_SET );
    if (pSeg->size) size = pSeg->size;
    else size = pSeg->minsize ? pSeg->minsize : 0x10000;
    mem = GlobalLock16(pSeg->hSeg);
    if (pModule->flags & NE_FFLAGS_SELFLOAD && segnum > 1)
    {
 	/* Implement self loading segments */
 	SELFLOADHEADER *selfloadheader;
        STACK16FRAME *stack16Top;
        DWORD oldstack;
 	WORD old_hSeg, new_hSeg;
        THDB *thdb = THREAD_Current();
        HFILE32 hFile32;
        HFILE16 hFile16;

 	selfloadheader = (SELFLOADHEADER *)
 		PTR_SEG_OFF_TO_LIN(SEL(pSegTable->hSeg),0);
 	oldstack = thdb->cur_stack;
 	old_hSeg = pSeg->hSeg;
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
	TRACE(dll,"CallLoadAppSegProc(hmodule=0x%04x,hf=0x%04x,segnum=%d\n",
		pModule->self,hf,segnum );
        DuplicateHandle( GetCurrentProcess(), hf, GetCurrentProcess(), &hFile32,
                         0, FALSE, DUPLICATE_SAME_ACCESS );
        hFile16 = FILE_AllocDosHandle( hFile32 );
 	new_hSeg = Callbacks->CallLoadAppSegProc(selfloadheader->LoadAppSeg,
                                                    pModule->self, hFile16,
                                                    segnum );
	TRACE(dll,"Ret CallLoadAppSegProc: hSeg = 0x%04x\n",new_hSeg);
        _lclose16( hFile16 );
 	if (SEL(new_hSeg) != SEL(old_hSeg)) {
 	  /* Self loaders like creating their own selectors; 
 	   * they love asking for trouble to Wine developers
 	   */
 	  if (segnum == pModule->dgroup) {
 	    memcpy(PTR_SEG_OFF_TO_LIN(SEL(old_hSeg),0),
 		   PTR_SEG_OFF_TO_LIN(SEL(new_hSeg),0), 
 		   pSeg->minsize ? pSeg->minsize : 0x10000);
 	    FreeSelector(SEL(new_hSeg));
 	    pSeg->hSeg = old_hSeg;
 	    TRACE(module, "New hSeg allocated for dgroup segment:Old=%d,New=%d\n", 
                old_hSeg, new_hSeg);
 	  } else {
 	    FreeSelector(SEL(pSeg->hSeg));
 	    pSeg->hSeg = new_hSeg;
 	  }
 	} 
 	
 	thdb->cur_stack = oldstack;
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
      char* buff = xmalloc(size);
      char* curr = buff;
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
      free(buff);
    }

    pSeg->flags |= NE_SEGFLAGS_LOADED;
    if (!(pSeg->flags & NE_SEGFLAGS_RELOC_DATA))
        return TRUE;  /* No relocation data, we are done */

    ReadFile(hf, &count, sizeof(count), &res, NULL);
    if (!count) return TRUE;

    TRACE(fixup, "Fixups for %.*s, segment %d, hSeg %04x\n",
                   *((BYTE *)pModule + pModule->name_table),
                   (char *)pModule + pModule->name_table + 1,
                   segnum, pSeg->hSeg );
    TRACE(segment, "Fixups for %.*s, segment %d, hSeg %04x\n",
                   *((BYTE *)pModule + pModule->name_table),
                   (char *)pModule + pModule->name_table + 1,
                   segnum, pSeg->hSeg );

    reloc_entries = (struct relocation_entry_s *)xmalloc(count * sizeof(struct relocation_entry_s));
    if (!ReadFile( hf, reloc_entries, count * sizeof(struct relocation_entry_s), &res, NULL) ||
        (res != count * sizeof(struct relocation_entry_s)))
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
                {
                    ERR(fixup, "No implementation for %.*s.%d, setting to 0xdeadbeef\n",
                            *((BYTE *)pTarget + pTarget->name_table),
                            (char *)pTarget + pTarget->name_table + 1,
                            ordinal );
                    address = (FARPROC16)0xdeadbeef;
                }
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
                ERR(fixup, "No implementation for %.*s.%s, setting to 0xdeadbeef\n",
                    *((BYTE *)pTarget + pTarget->name_table),
                    (char *)pTarget + pTarget->name_table + 1, func_name );
            }
            if (!address) address = (FARPROC16) 0xdeadbeef;
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
                address = (FARPROC16)PTR_SEG_OFF_TO_SEGPTR( SEL(pSegTable[rep->target1-1].hSeg), rep->target2 );
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
            sp = PTR_SEG_OFF_TO_LIN( SEL(pSeg->hSeg), offset );
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
                sp = PTR_SEG_OFF_TO_LIN( SEL(pSeg->hSeg), offset );
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
                if (next_offset >= GlobalSize16(pSeg->hSeg)) break;
                offset = next_offset;
            } while (offset != 0xffff);
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
    SEGTABLEENTRY * pSegTable = (SEGTABLEENTRY *) NE_SEG_TABLE(pModule);

    if (pModule->flags & NE_FFLAGS_SELFLOAD)
    {
        HFILE32 hf;
        HFILE16 hFile16;
        /* Handle self loading modules */
        SELFLOADHEADER *selfloadheader;
        STACK16FRAME *stack16Top;
        THDB *thdb = THREAD_Current();
        HMODULE16 hselfload = GetModuleHandle16("WPROCS");
        DWORD oldstack;
        WORD saved_hSeg = pSegTable[pModule->dgroup - 1].hSeg;

        TRACE(module, "%.*s is a self-loading module!\n",
		     *((BYTE*)pModule + pModule->name_table),
		     (char *)pModule + pModule->name_table + 1);
        if (!NE_LoadSegment( pModule, 1 )) return FALSE;
        selfloadheader = (SELFLOADHEADER *)
                          PTR_SEG_OFF_TO_LIN(SEL(pSegTable->hSeg), 0);
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

        DuplicateHandle( GetCurrentProcess(), NE_OpenFile(pModule),
                         GetCurrentProcess(), &hf, 0, FALSE, DUPLICATE_SAME_ACCESS );
        hFile16 = FILE_AllocDosHandle( hf );
        TRACE(dll,"CallBootAppProc(hModule=0x%04x,hf=0x%04x)\n",
              pModule->self,hFile16);
        Callbacks->CallBootAppProc(selfloadheader->BootApp, pModule->self,hFile16);
	TRACE(dll,"Return from CallBootAppProc\n");
        _lclose16(hf);
        /* some BootApp procs overwrite the segment handle of dgroup */
        pSegTable[pModule->dgroup - 1].hSeg = saved_hSeg;
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
 *           PatchCodeHandle
 *
 * Needed for self-loading modules.
 */

/* It does nothing */
DWORD WINAPI PatchCodeHandle(HANDLE16 hSel)
{
    FIXME(module,"(%04x): stub.\n",hSel);
    return (DWORD)NULL;
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
        dgroup = SEL(pSegTable[pModule->dgroup-1].hSeg);

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
		fixup_ptr = (char *)GET_SEL_BASE(SEL(pSegTable[p[3]-1].hSeg)) + *(WORD *)(p + 4);
	    } else { /* fixed */
		dsprintf(module, "offset %04x", *(WORD *)(p+1) );
		fixup_ptr = (char *)GET_SEL_BASE(SEL(pSegTable[sel-1].hSeg)) + 
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
    WORD hInst, ds, heap;
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

    ECX_reg(&context) = heap;
    EDI_reg(&context) = hInst;
    DS_reg(&context)  = ds;
    ES_reg(&context)  = ds;   /* who knows ... */

    CS_reg(&context)  = SEL(pSegTable[pModule->cs-1].hSeg);
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
    WORD hInst, ds, heap;
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

    DS_reg(&context) = ds;
    ES_reg(&context) = ds;   /* who knows ... */

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
    if (prev) *prev = SEL(pSegment->hSeg);

      /* if it's a library, create a new instance only the first time */
    if (pSegment->hSeg)
    {
        if (pModule->flags & NE_FFLAGS_LIBMODULE) return SEL(pSegment->hSeg);
        if (lib_only) return SEL(pSegment->hSeg);
    }

    minsize = pSegment->minsize ? pSegment->minsize : 0x10000;
    if (pModule->ss == pModule->dgroup) minsize += pModule->stack_size;
    minsize += pModule->heap_size;
    hNewInstance = GLOBAL_Alloc( GMEM_ZEROINIT | GMEM_FIXED, minsize,
                                 pModule->self, FALSE, FALSE, FALSE );
    if (!hNewInstance) return 0;
    pSegment->hSeg = hNewInstance;
    pSegment->flags |= NE_SEGFLAGS_ALLOCATED;
    return hNewInstance;
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
 */
DWORD WINAPI NE_AllocateSegment( WORD wFlags, WORD wSize, WORD wElem )
{
    WORD size = wSize << wElem;
    HANDLE16 hMem = GlobalAlloc16( NE_Ne2MemFlags(wFlags), size);

    /* not data == code */
    if (	(wFlags & NE_SEGFLAGS_EXECUTEONLY) ||
    		!(wFlags & NE_SEGFLAGS_DATA)
    ) {
        WORD hSel = GlobalHandleToSel(hMem);
        WORD access = SelectorAccessRights(hSel,0,0);

	access |= 2<<2; /* SEGMENT_CODE */
	SelectorAccessRights(hSel,1,access);
    }
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
        pSegment->hSeg = GLOBAL_Alloc( NE_Ne2MemFlags(pSegment->flags),
                                      minsize, pModule->self,
                                      !(pSegment->flags & NE_SEGFLAGS_DATA),
                                      FALSE,
                            FALSE /*pSegment->flags & NE_SEGFLAGS_READONLY*/ );
        if (!pSegment->hSeg) return FALSE;
	pSegment->flags |= NE_SEGFLAGS_ALLOCATED;
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
