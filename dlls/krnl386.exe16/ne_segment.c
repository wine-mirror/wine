/*
 * NE segment loading
 *
 * Copyright 1993 Robert J. Amstadt
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

#include "wine/winbase16.h"
#include "wownt32.h"
#include "winternl.h"
#include "kernel16_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fixup);
WINE_DECLARE_DEBUG_CHANNEL(dll);
WINE_DECLARE_DEBUG_CHANNEL(module);

/*
 * Relocation table entry
 */
struct relocation_entry_s
{
    BYTE address_type;    /* Relocation address type */
    BYTE relocation_type; /* Relocation type */
    WORD offset;          /* Offset in segment to fixup */
    WORD target1;         /* Target specification */
    WORD target2;         /* Target specification */
};

/*
 * Relocation address types
 */
#define NE_RADDR_LOWBYTE      0
#define NE_RADDR_SELECTOR     2
#define NE_RADDR_POINTER32    3
#define NE_RADDR_OFFSET16     5
#define NE_RADDR_POINTER48    11
#define NE_RADDR_OFFSET32     13

/*
 * Relocation types
 */
#define NE_RELTYPE_INTERNAL  0
#define NE_RELTYPE_ORDINAL   1
#define NE_RELTYPE_NAME      2
#define NE_RELTYPE_OSFIXUP   3
#define NE_RELFLAG_ADDITIVE  4

/* Self-loading modules contain this structure in their first segment */
typedef struct
{
    WORD      version;       /* Must be "A0" (0x3041) */
    WORD      reserved;
    FARPROC16 BootApp;       /* startup procedure */
    FARPROC16 LoadAppSeg;    /* procedure to load a segment */
    FARPROC16 reserved2;
    FARPROC16 MyAlloc;       /* memory allocation procedure,
                              * wine must write this field */
    FARPROC16 EntryAddrProc;
    FARPROC16 ExitProc;      /* exit procedure */
    WORD      reserved3[4];
    FARPROC16 SetOwner;      /* Set Owner procedure, exported by wine */
} SELFLOADHEADER;

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
 *           NE_OpenFile
 */
static HFILE16 NE_OpenFile( NE_MODULE *pModule )
{
    char *name = NE_MODULE_NAME( pModule );
    HANDLE handle = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ,
                                 NULL, OPEN_EXISTING, 0, 0 );

    if (handle == INVALID_HANDLE_VALUE)
    {
        ERR( "Can't open file '%s' for module %04x\n", name, pModule->self );
        return HFILE_ERROR;
    }
    return Win32HandleToDosFileHandle( handle );
}


/***********************************************************************
 *           apply_relocations
 *
 * Apply relocations to a segment. Helper for NE_LoadSegment.
 */
static inline BOOL apply_relocations( NE_MODULE *pModule, const struct relocation_entry_s *rep,
                                      int count, int segnum )
{
    BYTE *func_name;
    char buffer[256];
    int i, ordinal;
    WORD offset, *sp;
    HMODULE16 module;
    FARPROC16 address = 0;
    HMODULE16 *pModuleTable = (HMODULE16 *)((char *)pModule + pModule->ne_modtab);
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );
    SEGTABLEENTRY *pSeg = pSegTable + segnum - 1;

    /*
     * Go through the relocation table one entry at a time.
     */
    for (i = 0; i < count; i++, rep++)
    {
        /*
         * Get the target address corresponding to this entry.
         */

        /* If additive, there is no target chain list. Instead, add source
           and target */
        int additive = rep->relocation_type & NE_RELFLAG_ADDITIVE;
        switch (rep->relocation_type & 3)
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
                             *((BYTE *)pModule + pModule->ne_restab),
                             *((BYTE *)pModule + pModule->ne_restab),
                             (char *)pModule + pModule->ne_restab + 1 );
                else
                {
                    ERR("No implementation for %.*s.%d, setting to 0xdeadbeef\n",
                            *((BYTE *)pTarget + pTarget->ne_restab),
                            (char *)pTarget + pTarget->ne_restab + 1,
                            ordinal );
                    address = (FARPROC16)0xdeadbeef;
                }
            }
            if (TRACE_ON(fixup))
            {
                NE_MODULE *pTarget = NE_GetPtr( module );
                TRACE("%d: %.*s.%d=%04x:%04x %s\n", i + 1,
                       *((BYTE *)pTarget + pTarget->ne_restab),
                       (char *)pTarget + pTarget->ne_restab + 1,
                       ordinal, HIWORD(address), LOWORD(address),
                       NE_GetRelocAddrName( rep->address_type, additive ) );
            }
            break;

        case NE_RELTYPE_NAME:
            module = pModuleTable[rep->target1-1];
            func_name = (BYTE *)pModule + pModule->ne_imptab + rep->target2;
            memcpy( buffer, func_name+1, *func_name );
            buffer[*func_name] = '\0';
            ordinal = NE_GetOrdinal( module, buffer );
            address = NE_GetEntryPoint( module, ordinal );

            if (ERR_ON(fixup) && !address)
            {
                NE_MODULE *pTarget = NE_GetPtr( module );
                ERR("No implementation for %.*s.%s, setting to 0xdeadbeef\n",
                    *((BYTE *)pTarget + pTarget->ne_restab),
                    (char *)pTarget + pTarget->ne_restab + 1, buffer );
            }
            if (!address) address = (FARPROC16) 0xdeadbeef;
            if (TRACE_ON(fixup))
            {
                NE_MODULE *pTarget = NE_GetPtr( module );
                TRACE("%d: %.*s.%s=%04x:%04x %s\n", i + 1,
                       *((BYTE *)pTarget + pTarget->ne_restab),
                       (char *)pTarget + pTarget->ne_restab + 1,
                       buffer, HIWORD(address), LOWORD(address),
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
                address = (FARPROC16)MAKESEGPTR( SEL(pSegTable[rep->target1-1].hSeg), rep->target2 );
            }

            TRACE("%d: %04x:%04x %s\n",
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
            TRACE("%d: TYPE %d, OFFSET %04x, TARGET %04x %04x %s\n",
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
            ERR("WARNING: module %s: unknown reloc addr type = 0x%02x. Please report.\n",
                 module, rep->address_type );
        }

        if (additive)
        {
            sp = MapSL( MAKESEGPTR( SEL(pSeg->hSeg), offset ) );
            TRACE("    %04x:%04x\n", offset, *sp );
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
                    ERR("Additive selector to %04x.Please report\n",*sp);
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
                WORD next_offset;

                sp = MapSL( MAKESEGPTR( SEL(pSeg->hSeg), offset ) );
                next_offset = *sp;
                TRACE("    %04x:%04x\n", offset, *sp );
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
    return TRUE;

unknown:
    WARN("WARNING: %d: unknown ADDR TYPE %d,  "
         "TYPE %d,  OFFSET %04x,  TARGET %04x %04x\n",
         i + 1, rep->address_type, rep->relocation_type,
         rep->offset, rep->target1, rep->target2);
    return FALSE;
}


/***********************************************************************
 *           NE_LoadSegment
 */
BOOL NE_LoadSegment( NE_MODULE *pModule, WORD segnum )
{
    WORD count;
    DWORD pos;
    const struct relocation_entry_s *rep;
    int size;
    SEGTABLEENTRY *pSegTable = NE_SEG_TABLE( pModule );
    SEGTABLEENTRY *pSeg = pSegTable + segnum - 1;

    if (pSeg->flags & NE_SEGFLAGS_LOADED)
    {
	/* self-loader ? -> already loaded it */
	if (pModule->ne_flags & NE_FFLAGS_SELFLOAD)
	    return TRUE;

	/* leave, except for DGROUP, as this may be the second instance */
	if (segnum != pModule->ne_autodata)
            return TRUE;
    }

    if (!pSeg->filepos) return TRUE;  /* No file image, just return */

    TRACE_(module)("Loading segment %d, hSeg=%04x, flags=%04x\n",
                    segnum, pSeg->hSeg, pSeg->flags );
    pos = pSeg->filepos << pModule->ne_align;
    if (pSeg->size) size = pSeg->size;
    else size = pSeg->minsize ? pSeg->minsize : 0x10000;

    if (pModule->ne_flags & NE_FFLAGS_SELFLOAD && segnum > 1)
    {
 	/* Implement self-loading segments */
 	SELFLOADHEADER *selfloadheader;
        WORD old_ss = CURRENT_SS, old_sp = CURRENT_SP;
        HFILE16 hFile16;
        WORD args[3];
        DWORD ret;

 	selfloadheader = MapSL( MAKESEGPTR(SEL(pSegTable->hSeg),0) );
        CURRENT_SS = pModule->self_loading_sel;
        CURRENT_SP = 0xff00 - sizeof(STACK16FRAME);

        hFile16 = NE_OpenFile( pModule );
        TRACE_(dll)("Call LoadAppSegProc(hmodule=0x%04x,hf=%x,segnum=%d)\n",
                    pModule->self,hFile16,segnum );
        args[2] = pModule->self;
        args[1] = hFile16;
        args[0] = segnum;
        WOWCallback16Ex( (DWORD)selfloadheader->LoadAppSeg, WCB16_PASCAL, sizeof(args), args, &ret );
        pSeg->hSeg = LOWORD(ret);
        TRACE_(dll)("Ret LoadAppSegProc: hSeg=0x%04x\n", pSeg->hSeg);
        _lclose16( hFile16 );
        CURRENT_SS = old_ss;
        CURRENT_SP = old_sp;

        pSeg->flags |= NE_SEGFLAGS_LOADED;
        return TRUE;
    }
    else if (!(pSeg->flags & NE_SEGFLAGS_ITERATED))
    {
        void *mem = GlobalLock16(pSeg->hSeg);
        if (!NE_READ_DATA( pModule, mem, pos, size ))
            return FALSE;
        pos += size;
    }
    else
    {
        /*
          The following bit of code for "iterated segments" was written without
          any documentation on the format of these segments. It seems to work,
          but may be missing something.
        */
        const char *buff = NE_GET_DATA( pModule, pos, size );
        const char* curr = buff;
        char *mem = GlobalLock16(pSeg->hSeg);

        pos += size;
        if (buff == NULL) return FALSE;

        while(curr < buff + size) {
            unsigned int rept = ((const short *)curr)[0];
            unsigned int len =  ((const short *)curr)[1];

            curr += 2*sizeof(short);
            while (rept--)
            {
                memcpy( mem, curr, len );
                mem += len;
            }
            curr += len;
        }
    }

    pSeg->flags |= NE_SEGFLAGS_LOADED;

    /* Perform exported function prolog fixups */
    NE_FixupSegmentPrologs( pModule, segnum );

    if (!(pSeg->flags & NE_SEGFLAGS_RELOC_DATA))
        return TRUE;  /* No relocation data, we are done */

    if (!NE_READ_DATA( pModule, &count, pos, sizeof(count) ) || !count) return TRUE;
    pos += sizeof(count);

    TRACE("Fixups for %.*s, segment %d, hSeg %04x\n",
          *((BYTE *)pModule + pModule->ne_restab),
          (char *)pModule + pModule->ne_restab + 1,
          segnum, pSeg->hSeg );

    if (!(rep = NE_GET_DATA( pModule, pos, count * sizeof(struct relocation_entry_s) )))
        return FALSE;

    return apply_relocations( pModule, rep, count, segnum );
}


/***********************************************************************
 *           NE_LoadAllSegments
 */
BOOL NE_LoadAllSegments( NE_MODULE *pModule )
{
    int i;
    SEGTABLEENTRY * pSegTable = NE_SEG_TABLE(pModule);

    if (pModule->ne_flags & NE_FFLAGS_SELFLOAD)
    {
        HFILE16 hFile16;
        HGLOBAL16 sel;
        /* Handle self-loading modules */
        SELFLOADHEADER *selfloadheader;
        HMODULE16 mod = GetModuleHandle16("KERNEL");
        WORD old_ss = CURRENT_SS, old_sp = CURRENT_SP;
        WORD args[2];

        TRACE_(module)("%.*s is a self-loading module!\n",
                       *((BYTE*)pModule + pModule->ne_restab),
                       (char *)pModule + pModule->ne_restab + 1);
        if (!NE_LoadSegment( pModule, 1 )) return FALSE;
        selfloadheader = MapSL( MAKESEGPTR(SEL(pSegTable->hSeg), 0) );
        selfloadheader->EntryAddrProc = GetProcAddress16(mod,"EntryAddrProc");
        selfloadheader->MyAlloc       = GetProcAddress16(mod,"MyAlloc");
        selfloadheader->SetOwner      = GetProcAddress16(mod,"FarSetOwner");
        sel = GlobalAlloc16( GMEM_ZEROINIT, 0xFF00 );
        pModule->self_loading_sel = SEL(sel);
        FarSetOwner16( sel, pModule->self );
        CURRENT_SS = pModule->self_loading_sel;
        CURRENT_SP = 0xff00 - sizeof(STACK16FRAME);

        hFile16 = NE_OpenFile(pModule);
        TRACE_(dll)("CallBootAppProc(hModule=0x%04x,hf=0x%04x)\n",
              pModule->self,hFile16);
        args[1] = pModule->self;
        args[0] = hFile16;
        WOWCallback16Ex( (DWORD)selfloadheader->BootApp, WCB16_PASCAL, sizeof(args), args, NULL );
	TRACE_(dll)("Return from CallBootAppProc\n");
        _lclose16(hFile16);
        CURRENT_SS = old_ss;
        CURRENT_SP = old_sp;

        for (i = 2; i <= pModule->ne_cseg; i++)
            if (!NE_LoadSegment( pModule, i )) return FALSE;
    }
    else
    {
        for (i = 1; i <= pModule->ne_cseg; i++)
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

    TRACE("(%d);\n", segnum);

    if (pSegTable[segnum-1].flags & NE_SEGFLAGS_DATA)
    {
	pSegTable[segnum-1].flags |= NE_SEGFLAGS_LOADED;
	return;
    }

    if (!pModule->ne_autodata) return;

    if (!pSegTable[pModule->ne_autodata-1].hSeg) return;
    dgroup = SEL(pSegTable[pModule->ne_autodata-1].hSeg);

    pSeg = MapSL( MAKESEGPTR(sel, 0) );

    bundle = (ET_BUNDLE *)((BYTE *)pModule+pModule->ne_enttab);

    do {
        TRACE("num_entries: %d, bundle: %p, next: %04x, pSeg: %p\n", bundle->last - bundle->first, bundle, bundle->next, pSeg);
        if (!(num_entries = bundle->last - bundle->first))
            return;
        entry = (ET_ENTRY *)((BYTE *)bundle+6);
        while (num_entries--)
        {
            /*TRACE("entry: %p, entry->segnum: %d, entry->offs: %04x\n", entry, entry->segnum, entry->offs);*/
            if (entry->segnum == segnum)
            {
                pFunc = pSeg+entry->offs;
                TRACE("pFunc: %p, *(DWORD *)pFunc: %08lx, num_entries: %d\n", pFunc, *(DWORD *)pFunc, num_entries);
                if (*(pFunc+2) == 0x90)
                {
                    if (*(WORD *)pFunc == 0x581e) /* push ds, pop ax */
                    {
                        TRACE("patch %04x:%04x -> mov ax, ds\n", sel, entry->offs);
                        *(WORD *)pFunc = 0xd88c; /* mov ax, ds */
                    }

                    if (*(WORD *)pFunc == 0xd88c)
                    {
                        if ((entry->flags & 2)) /* public data ? */
                        {
                            TRACE("patch %04x:%04x -> mov ax, dgroup [%04x]\n", sel, entry->offs, dgroup);
                            *pFunc = 0xb8; /* mov ax, */
                            *(WORD *)(pFunc+1) = dgroup;
                        }
                        else if ((pModule->ne_flags & NE_FFLAGS_MULTIPLEDATA)
                                 && (entry->flags & 1)) /* exported ? */
                        {
                            TRACE("patch %04x:%04x -> nop, nop\n", sel, entry->offs);
                            *(WORD *)pFunc = 0x9090; /* nop, nop */
                        }
                    }
                }
            }
            entry++;
        }
    } while ( (bundle->next) && (bundle = ((ET_BUNDLE *)((BYTE *)pModule + bundle->next))) );
}


/***********************************************************************
 *           PatchCodeHandle (KERNEL.110)
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
    for (segnum = 1; segnum <= pModule->ne_cseg; segnum++)
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

    if (!(pModule->ne_flags & NE_FFLAGS_SINGLEDATA))
    {
        if (pModule->ne_flags & NE_FFLAGS_MULTIPLEDATA || pModule->ne_autodata)
        {
            /* Not SINGLEDATA */
            ERR_(dll)("Library is not marked SINGLEDATA\n");
        }
        /* DATA NONE DLL */
        *ds = 0;
        *heap = 0;
    }
    else  /* DATA SINGLE DLL */
    {
	if (pModule->ne_autodata) {
            *ds   = SEL(pSegTable[pModule->ne_autodata-1].hSeg);
            *heap = pModule->ne_heap;
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
static BOOL NE_InitDLL( NE_MODULE *pModule )
{
    SEGTABLEENTRY *pSegTable;
    WORD hInst, ds, heap;
    CONTEXT context;

    pSegTable = NE_SEG_TABLE( pModule );

    if (!(pModule->ne_flags & NE_FFLAGS_LIBMODULE) ||
        (pModule->ne_flags & NE_FFLAGS_WIN32)) return TRUE; /*not a library*/

    /* Call USER signal handler for Win3.1 compatibility. */
    NE_CallUserSignalProc( pModule->self, USIG16_DLL_LOAD );

    if (!SELECTOROF(pModule->ne_csip)) return TRUE;  /* no initialization code */


    /* Registers at initialization must be:
     * cx     heap size
     * di     library instance
     * ds     data segment if any
     * es:si  command line (always 0)
     */

    memset( &context, 0, sizeof(context) );

    NE_GetDLLInitParams( pModule, &hInst, &ds, &heap );

    context.Ecx = heap;
    context.Edi = hInst;
    context.SegDs = ds;
    context.SegEs = ds;   /* who knows ... */
    context.SegCs = SEL(pSegTable[SELECTOROF(pModule->ne_csip)-1].hSeg);
    context.Eip   = OFFSETOF(pModule->ne_csip);
    context.Ebp   = CURRENT_SP + FIELD_OFFSET(STACK16FRAME,bp);

    pModule->ne_csip = 0;  /* Don't initialize it twice */
    TRACE_(dll)("Calling LibMain for %.*s, cs:ip=%04lx:%04lx ds=%04lx di=%04x cx=%04x\n",
                *((BYTE*)pModule + pModule->ne_restab),
                (char *)pModule + pModule->ne_restab + 1,
                context.SegCs, context.Eip, context.SegDs,
                LOWORD(context.Edi), LOWORD(context.Ecx) );
    WOWCallback16Ex( 0, WCB16_REGS, 0, NULL, (DWORD *)&context );
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
    NE_MODULE *pModule;
    HMODULE16 *pDLL;

    if (!(pModule = NE_GetPtr( hModule ))) return;

    if (pModule->dlls_to_init)
    {
	HGLOBAL16 to_init = pModule->dlls_to_init;
	pModule->dlls_to_init = 0;
        for (pDLL = GlobalLock16( to_init ); *pDLL; pDLL++)
        {
            NE_InitializeDLLs( *pDLL );
        }
        GlobalFree16( to_init );
    }
    NE_InitDLL( pModule );
}


/**********************************************************************
 *	    NE_CallUserSignalProc
 *
 * According to "Undocumented Windows", the task signal proc is
 * bypassed for module load/unload notifications, and the USER signal
 * proc is called directly instead. This is what this function does.
 */
typedef DWORD (WINAPI *pSignalProc)( HANDLE16 module, UINT16 code, UINT16 exit,
                                     HINSTANCE16 inst, HQUEUE16 queue );

void NE_CallUserSignalProc( HMODULE16 hModule, UINT16 code )
{
    FARPROC16 proc;
    HMODULE16 user = GetModuleHandle16("user.exe");

    if (!user) return;
    if ((proc = GetProcAddress16( user, "SignalProc" )))
    {
        /* USER is always a builtin dll */
        pSignalProc sigproc = (pSignalProc)((ENTRYPOINT16 *)MapSL( (SEGPTR)proc ))->target;
        sigproc( hModule, code, 0, 0, 0 );
    }
}


/***********************************************************************
 *           NE_CallDllEntryPoint
 *
 * Call the DllEntryPoint of DLLs with subsystem >= 4.0
 */
typedef DWORD (WINAPI *WinNEEntryProc)(DWORD,WORD,WORD,WORD,DWORD,WORD);

static void NE_CallDllEntryPoint( NE_MODULE *pModule, DWORD dwReason )
{
    WORD hInst, ds, heap;
    FARPROC16 entryPoint;

    if (!(pModule->ne_flags & NE_FFLAGS_LIBMODULE)) return;
    if (!(pModule->ne_flags & NE_FFLAGS_BUILTIN) && pModule->ne_expver < 0x0400) return;
    if (!(entryPoint = GetProcAddress16( pModule->self, "DllEntryPoint" ))) return;

    NE_GetDLLInitParams( pModule, &hInst, &ds, &heap );

    TRACE_(dll)( "Calling %s DllEntryPoint, cs:ip=%04x:%04x\n",
                 NE_MODULE_NAME( pModule ),
                 SELECTOROF(entryPoint), OFFSETOF(entryPoint) );

    if ( pModule->ne_flags & NE_FFLAGS_BUILTIN )
    {
        WinNEEntryProc entryProc = (WinNEEntryProc)((ENTRYPOINT16 *)MapSL( (SEGPTR)entryPoint ))->target;

        entryProc( dwReason, hInst, ds, heap, 0, 0 );
    }
    else
    {
        CONTEXT context;
        WORD args[8];

        memset( &context, 0, sizeof(context) );
        context.SegDs = ds;
        context.SegEs = ds;   /* who knows ... */
        context.SegCs = HIWORD(entryPoint);
        context.Eip   = LOWORD(entryPoint);
        context.Ebp   = CURRENT_SP + FIELD_OFFSET(STACK16FRAME,bp);

        args[7] = HIWORD(dwReason);
        args[6] = LOWORD(dwReason);
        args[5] = hInst;
        args[4] = ds;
        args[3] = heap;
        args[2] = 0;     /* HIWORD(dwReserved1) */
        args[1] = 0;     /* LOWORD(dwReserved1) */
        args[0] = 0;     /* wReserved2 */
        WOWCallback16Ex( 0, WCB16_REGS, sizeof(args), args, (DWORD *)&context );
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

struct ne_init_list
{
    int count;
    int size;
    NE_MODULE **module;
};

static void add_to_init_list( struct ne_init_list *list, NE_MODULE *hModule )
{
    NE_MODULE **newModule = NULL;
    if ( list->count == list->size )
    {
        int newSize = list->size + 128;

	if (list->module) 
            newModule = HeapReAlloc( GetProcessHeap(), 0,
                                             list->module, newSize*sizeof(NE_MODULE *) );
	else
            newModule = HeapAlloc( GetProcessHeap(), 0,
                                             newSize*sizeof(NE_MODULE *) );
        if ( !newModule )
        {
            FIXME_(dll)("Out of memory!\n");
            return;
        }

        list->module = newModule;
        list->size   = newSize;
    }

    list->module[list->count++] = hModule;
}

static void free_init_list( struct ne_init_list *list )
{
    if ( list->module )
    {
        HeapFree( GetProcessHeap(), 0, list->module );
        memset( list, 0, sizeof(*list) );
    }
}

static void fill_init_list( struct ne_init_list *list, HMODULE16 hModule )
{
    NE_MODULE *pModule;
    HMODULE16 *pModRef;
    int i;

    if (!(pModule = NE_GetPtr( hModule ))) return;

    /* Never add a module twice */
    for ( i = 0; i < list->count; i++ )
        if ( list->module[i] == pModule )
            return;

    /* Check for recursive call */
    if ( pModule->ne_flagsothers & 0x80 ) return;

    TRACE_(dll)("(%s) - START\n", NE_MODULE_NAME(pModule) );

    /* Tag current module to prevent recursive loop */
    pModule->ne_flagsothers |= 0x80;

    /* Recursively attach all DLLs this one depends on */
    pModRef = (HMODULE16 *)((char *)pModule + pModule->ne_modtab);
    for ( i = 0; i < pModule->ne_cmod; i++ )
        if ( pModRef[i] ) fill_init_list( list, pModRef[i] );

    /* Add current module */
    add_to_init_list( list, pModule );

    /* Remove recursion flag */
    pModule->ne_flagsothers &= ~0x80;

    TRACE_(dll)("(%s) - END\n", NE_MODULE_NAME(pModule) );
}

static void call_init_list( struct ne_init_list *list )
{
    int i;
    for ( i = 0; i < list->count; i++ )
        NE_CallDllEntryPoint( list->module[i], DLL_PROCESS_ATTACH );
}

void NE_DllProcessAttach( HMODULE16 hModule )
{
    struct ne_init_list list;
    memset( &list, 0, sizeof(list) );

    fill_init_list( &list, hModule );
    call_init_list( &list );
    free_init_list( &list );
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
 *           MyAlloc   (KERNEL.668) Wine-specific export
 *
 * MyAlloc() function for self-loading apps.
 */
DWORD WINAPI MyAlloc16( WORD wFlags, WORD wSize, WORD wElem )
{
    DWORD size = wSize << wElem;
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
    if ( !pModule->ne_autodata )
        return pModule->self;
    else
    {
        SEGTABLEENTRY *pSeg;
        pSeg = NE_SEG_TABLE( pModule ) + pModule->ne_autodata - 1;
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
    struct ldt_bits bits;

    if ( segnum < 1 || segnum > pModule->ne_cseg )
        return FALSE;

    if ( (pModule->ne_flags & NE_FFLAGS_SELFLOAD) && segnum != 1 )
        return TRUE;    /* selfloader allocates segment itself */

    if ( (pSeg->flags & NE_SEGFLAGS_ALLOCATED) && segnum != pModule->ne_autodata )
        return TRUE;    /* all but DGROUP only allocated once */

    minsize = pSeg->minsize ? pSeg->minsize : 0x10000;
    if ( segnum == SELECTOROF(pModule->ne_sssp) ) minsize += pModule->ne_stack;
    if ( segnum == pModule->ne_autodata ) minsize += pModule->ne_heap;

    bits = (pSeg->flags & NE_SEGFLAGS_DATA) ? data_segment : code16_segment;
    bits.default_big = !!(pSeg->flags & NE_SEGFLAGS_32BIT);
    pSeg->hSeg = GLOBAL_Alloc( NE_Ne2MemFlags(pSeg->flags), minsize, pModule->self, bits );
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
    for ( i = 1; i <= pModule->ne_cseg; i++ )
        if ( !NE_CreateSegment( pModule, i ) )
            return FALSE;

    pModule->dgroup_entry = pModule->ne_autodata ? pModule->ne_segtab +
                            (pModule->ne_autodata - 1) * sizeof(SEGTABLEENTRY) : 0;
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
    return (pModule->ne_flags & NE_FFLAGS_LIBMODULE) != 0;
}
