#ifndef WINELIB
/*
 * NE modules
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

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
#include "dlls.h"
#include "windows.h"
#include "arch.h"
#include "selectors.h"
#include "callback.h"
#include "module.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"


/***********************************************************************
 *           NE_LoadSegment
 */
BOOL NE_LoadSegment( HMODULE hModule, WORD segnum )
{
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable, *pSeg;
    WORD *pModuleTable;
    WORD count, i, offset;
    HMODULE module;
    DWORD address;
    int fd;
    struct relocation_entry_s *rep, *reloc_entries;
    BYTE *func_name;
    int size;
    char* mem;

    char buffer[100];
    int ordinal, additive;
    unsigned short *sp;

    if (!(pModule = (NE_MODULE *)GlobalLock( hModule ))) return FALSE;
    pSegTable = NE_SEG_TABLE( pModule );
    pSeg = pSegTable + segnum - 1;
    pModuleTable = NE_MODULE_TABLE( pModule );

    if (!pSeg->filepos) return TRUE;  /* No file image, just return */
	
    fd = MODULE_OpenFile( hModule );
    dprintf_module( stddeb, "Loading segment %d, selector="NPFMT"\n",
                    segnum, pSeg->selector );
    lseek( fd, pSeg->filepos << pModule->alignment, SEEK_SET );
    size = pSeg->size ? pSeg->size : 0x10000;
    mem = GlobalLock(pSeg->selector);
    if (pModule->flags & NE_FFLAGS_SELFLOAD && segnum > 1) {	
#ifndef WINELIB
 	/* Implement self loading segments */
 	SELFLOADHEADER *selfloadheader;
 	WORD oldss, oldsp, oldselector, newselector;
 	selfloadheader = (SELFLOADHEADER *)
 		PTR_SEG_OFF_TO_LIN(pSegTable->selector,0);
 	oldss = IF1632_Saved16_ss;
 	oldsp = IF1632_Saved16_sp;
 	oldselector = pSeg->selector;
 	IF1632_Saved16_ss = pModule->self_loading_sel;
 	IF1632_Saved16_sp = 0xFF00;
 	newselector =  CallTo16_word_www(selfloadheader->LoadAppSeg,
 		pModule->self_loading_sel, hModule, fd, segnum);
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
 	    fprintf(stderr, "A new selector was allocated for the dgroup segment\n"
 		    "Old selector is %d, new one is %d", oldselector, newselector);
 	  } else {
 	    FreeSelector(pSeg->selector);
 	    pSeg->selector = newselector;
 	  }
 	} 
 	
 	IF1632_Saved16_ss = oldss;
 	IF1632_Saved16_sp = oldsp;
#else
	fprintf(stderr,"JBP: Ignoring self loading segments in NE_LoadSegment.\n");
#endif
     }
    else if (!(pSeg->flags & NE_SEGFLAGS_ITERATED))
      read(fd, mem, size);
    else {
      /*
	 The following bit of code for "iterated segments" was written without
	 any documentation on the format of these segments. It seems to work,
	 but may be missing something. If you have any doco please either send
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

    dprintf_fixup( stddeb, "Fixups for %*.*s, segment %d, selector "NPFMT"\n",
                   *((BYTE *)pModule + pModule->name_table),
                   *((BYTE *)pModule + pModule->name_table),
                   (char *)pModule + pModule->name_table + 1,
                   segnum, pSeg->selector );

    reloc_entries = (struct relocation_entry_s *)xmalloc(count * sizeof(struct relocation_entry_s));
    if (read( fd, reloc_entries, count * sizeof(struct relocation_entry_s)) !=
            count * sizeof(struct relocation_entry_s))
    {
        dprintf_fixup( stddeb, "Unable to read relocation information\n" );
        return FALSE;
    }

    /*
     * Go through the relocation table on entry at a time.
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
            address = MODULE_GetEntryPoint( module, ordinal );
            if (!address)
            {
                NE_MODULE *pTarget = (NE_MODULE *)GlobalLock( module );
                if (!pTarget)
                    fprintf( stderr, "Module not found: "NPFMT", reference %d of module %*.*s\n",
                             module, rep->target1, 
                             *((BYTE *)pModule + pModule->name_table),
                             *((BYTE *)pModule + pModule->name_table),
                             (char *)pModule + pModule->name_table + 1 );
                else
                    fprintf( stderr, "Warning: no handler for %*.*s.%d, setting to 0:0\n",
                            *((BYTE *)pTarget + pTarget->name_table),
                            *((BYTE *)pTarget + pTarget->name_table),
                            (char *)pTarget + pTarget->name_table + 1,
                            ordinal );
            }
            if (debugging_fixup)
            {
                NE_MODULE *pTarget = (NE_MODULE *)GlobalLock( module );
                fprintf( stddeb,"%d: %*.*s.%d=%04x:%04x\n", i + 1, 
                         *((BYTE *)pTarget + pTarget->name_table),
                         *((BYTE *)pTarget + pTarget->name_table),
                         (char *)pTarget + pTarget->name_table + 1,
                         ordinal, HIWORD(address), LOWORD(address) );
            }
	    break;
	    
	  case NE_RELTYPE_NAME:
            module = pModuleTable[rep->target1-1];
            func_name = (char *)pModule + pModule->import_table + rep->target2;
            memcpy( buffer, func_name+1, *func_name );
            buffer[*func_name] = '\0';
            func_name = buffer;
            ordinal = MODULE_GetOrdinal( module, func_name );

            address = MODULE_GetEntryPoint( module, ordinal );

            if (!address)
            {
                NE_MODULE *pTarget = (NE_MODULE *)GlobalLock( module );
                fprintf( stderr, "Warning: no handler for %*.*s.%s, setting to 0:0\n",
                        *((BYTE *)pTarget + pTarget->name_table),
                        *((BYTE *)pTarget + pTarget->name_table),
                        (char *)pTarget + pTarget->name_table + 1, func_name );
            }
            if (debugging_fixup)
            {
                NE_MODULE *pTarget = (NE_MODULE *)GlobalLock( module );
                fprintf( stddeb,"%d: %*.*s.%s=%04x:%04x\n", i + 1, 
                         *((BYTE *)pTarget + pTarget->name_table),
                         *((BYTE *)pTarget + pTarget->name_table),
                         (char *)pTarget + pTarget->name_table + 1,
                         func_name, HIWORD(address), LOWORD(address) );
            }
	    break;
	    
	  case NE_RELTYPE_INTERNAL:
	    if (rep->target1 == 0x00ff)
	    {
		address  = MODULE_GetEntryPoint( hModule, rep->target2 );
	    }
	    else
	    {
                address = MAKELONG( rep->target2, pSegTable[rep->target1-1].selector );
	    }
	    
	    dprintf_fixup(stddeb,"%d: %04x:%04x\n", 
			  i + 1, HIWORD(address), LOWORD(address) );
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
	    dprintf_fixup(stddeb,
                   "%d: ADDR TYPE %d,  TYPE %d,  OFFSET %04x,  ",
		   i + 1, rep->address_type, rep->relocation_type, 
		   rep->offset);
	    dprintf_fixup(stddeb,"TARGET %04x %04x\n", 
		   rep->target1, rep->target2);
	    continue;
	    
	  default:
	    dprintf_fixup(stddeb,
		   "%d: ADDR TYPE %d,  TYPE %d,  OFFSET %04x,  ",
		   i + 1, rep->address_type, rep->relocation_type, 
		   rep->offset);
	    dprintf_fixup(stddeb,"TARGET %04x %04x\n", 
		    rep->target1, rep->target2);
	    free(reloc_entries);
	    return FALSE;
	}

	offset  = rep->offset;

	switch (rep->address_type)
	{
	  case NE_RADDR_LOWBYTE:
            do {
                sp = PTR_SEG_OFF_TO_LIN( pSeg->selector, offset );
                dprintf_fixup(stddeb,"    "NPFMT":%04x:%04x BYTE%s\n",
                              pSeg->selector, offset, *sp, additive ? " additive":"");
                offset = *sp;
		if(additive)
                    *(unsigned char*)sp = (unsigned char)((address+offset) & 0xFF);
		else
                    *(unsigned char*)sp = (unsigned char)(address & 0xFF);
            }
            while (offset != 0xffff && !additive);
            break;

	  case NE_RADDR_OFFSET16:
	    do {
                sp = PTR_SEG_OFF_TO_LIN( pSeg->selector, offset );
		dprintf_fixup(stddeb,"    "NPFMT":%04x:%04x OFFSET16%s\n",
                              pSeg->selector, offset, *sp, additive ? " additive" : "" );
		offset = *sp;
		*sp = LOWORD(address);
		if (additive) *sp += offset;
	    } 
	    while (offset != 0xffff && !additive);
	    break;
	    
	  case NE_RADDR_POINTER32:
	    do {
                sp = PTR_SEG_OFF_TO_LIN( pSeg->selector, offset );
		dprintf_fixup(stddeb,"    "NPFMT":%04x:%04x POINTER32%s\n",
                              pSeg->selector, offset, *sp, additive ? " additive" : "" );
		offset = *sp;
		*sp    = LOWORD(address);
		if (additive) *sp += offset;
		*(sp+1) = HIWORD(address);
	    } 
	    while (offset != 0xffff && !additive);
	    break;
	    
	  case NE_RADDR_SELECTOR:
	    do {
                sp = PTR_SEG_OFF_TO_LIN( pSeg->selector, offset );
		dprintf_fixup(stddeb,"    "NPFMT":%04x:%04x SELECTOR%s\n",
                              pSeg->selector, offset, *sp, additive ? " additive" : "" );
		offset = *sp;
		*sp    = HIWORD(address);
		/* Borland creates additive records with offset zero. Strange, but OK */
		if(additive && offset)
        	fprintf(stderr,"Additive selector to %4.4x.Please report\n",offset);
	    } 
            /* FIXME: Quicken 5 has a zero offset fixup.  This seems to work */
	    while (offset && offset != 0xffff && !additive);
	    break;
	    
	  default:
	    dprintf_fixup(stddeb,
		   "%d: ADDR TYPE %d,  TYPE %d,  OFFSET %04x,  ",
		   i + 1, rep->address_type, rep->relocation_type, 
		   rep->offset);
	    dprintf_fixup(stddeb,
		   "TARGET %04x %04x\n", rep->target1, rep->target2);
	    free(reloc_entries);
	    return FALSE;
	}
    }

    free(reloc_entries);
    return TRUE;
}


/***********************************************************************
 *           NE_FixupPrologs
 *
 * Fixup the exported functions prologs.
 */
void NE_FixupPrologs( HMODULE hModule )
{
#ifdef WINELIB
	fprintf(stderr,"NE_FixupPrologs should not be called for libwine\n");
#else
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;
    WORD dgroup = 0;
    WORD sel;
    BYTE *p, *fixup_ptr, count;

    pModule = (NE_MODULE *)GlobalLock( hModule );
    pSegTable = NE_SEG_TABLE(pModule);
    if (pModule->flags & NE_FFLAGS_SINGLEDATA)
        dgroup = pSegTable[pModule->dgroup-1].selector;

    dprintf_module( stddeb, "MODULE_FixupPrologs(%04x)\n", hModule );
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
            dprintf_module( stddeb,"Flags: %04x, sel %02x ", *p, sel);
            /* According to the output generated by TDUMP, the flags mean:
             * 0x0001 function is exported
	     * 0x0002 Single data (seems to occur only in DLLs)
	     */
	    if (sel == 0xff) { /* moveable */
		dprintf_module( stddeb, "(%02x) o %04x ", p[3], *(WORD *)(p+4) );
		fixup_ptr = (char *)GET_SEL_BASE(pSegTable[p[3]-1].selector) + *(WORD *)(p + 4);
	    } else { /* fixed */
		dprintf_module( stddeb, "offset %04x ", *(WORD *)(p+1) );
		fixup_ptr = (char *)GET_SEL_BASE(pSegTable[sel-1].selector) + *(WORD *)(p + 1);
	    }
	    dprintf_module( stddeb, "Signature: %02x %02x %02x,ff %x\n",
			   fixup_ptr[0], fixup_ptr[1], fixup_ptr[2],
			   pModule->flags );
            if (*p & 0x0001)
            {
                /* Verify the signature */
                if (((fixup_ptr[0] == 0x1e && fixup_ptr[1] == 0x58)
                     || (fixup_ptr[0] == 0x8c && fixup_ptr[1] == 0xd8))
                    && fixup_ptr[2] == 0x90)
                {
                    if (*p & 0x0002)
                    {
			if (pModule->flags & NE_FFLAGS_MULTIPLEDATA) {
			    /* can this happen? */
			    fprintf( stderr, "FixupPrologs got confused\n" );
			}
                        *fixup_ptr = 0xb8;	/* MOV AX, */
                        *(WORD *)(fixup_ptr+1) = dgroup;
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
		    dprintf_fixup( stddeb, "Unknown signature\n" );
		}
            }
	    else
	      dprintf_module( stddeb,"\n");
            p += (sel == 0xff) ? 6 : 3;  
        }
    }
#endif
}


/***********************************************************************
 *           NE_InitDLL
 *
 * Call the DLL initialization code
 */
static BOOL NE_InitDLL( HMODULE hModule )
{
#ifndef WINELIB
    int cs_reg, ds_reg, ip_reg, cx_reg, di_reg, bp_reg;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegTable;

    /* Registers at initialization must be:
     * cx     heap size
     * di     library instance
     * ds     data segment if any
     * es:si  command line (always 0)
     */

    pModule = (NE_MODULE *)GlobalLock( hModule );
    pSegTable = NE_SEG_TABLE( pModule );

    if (!(pModule->flags & NE_FFLAGS_LIBMODULE)) return TRUE; /*not a library*/
    if (!pModule->cs) return TRUE;  /* no initialization code */

    if (!(pModule->flags & NE_FFLAGS_SINGLEDATA))
    {
        if (pModule->flags & NE_FFLAGS_MULTIPLEDATA || pModule->dgroup)
        {
            /* Not SINGLEDATA */
            fprintf(stderr, "Library is not marked SINGLEDATA\n");
            exit(1);
        }
        else  /* DATA NONE DLL */
        {
            ds_reg = 0;
            cx_reg = 0;
        }
    }
    else  /* DATA SINGLE DLL */
    {
        ds_reg = pSegTable[pModule->dgroup-1].selector;
        cx_reg = pModule->heap_size;
    }

    cs_reg = pSegTable[pModule->cs-1].selector;
    ip_reg = pModule->ip;
    di_reg = ds_reg ? ds_reg : hModule;
    bp_reg = IF1632_Saved16_sp + ((WORD)&((STACK16FRAME*)1)->bp - 1);

    pModule->cs = 0;  /* Don't initialize it twice */
    dprintf_dll( stddeb, "Calling LibMain, cs:ip=%04x:%04x ds=%04x di=%04x cx=%04x\n", 
                 cs_reg, ip_reg, ds_reg, di_reg, cx_reg );
    return CallTo16_regs_( (FARPROC)(cs_reg << 16 | ip_reg), ds_reg,
                           0 /*es*/, 0 /*bp*/, 0 /*ax*/, 0 /*bx*/,
                           cx_reg, 0 /*dx*/, 0 /*si*/, di_reg );
#else
    fprintf( stderr,"JBP: Ignoring call to LibMain\n" );
    return FALSE;
#endif
}


/***********************************************************************
 *           NE_InitializeDLLs
 *
 * Initialize the loaded DLLs.
 */
void NE_InitializeDLLs( HMODULE hModule )
{
    NE_MODULE *pModule;
    HMODULE *pDLL;

    pModule = (NE_MODULE *)GlobalLock( hModule );
	if (pModule->magic == PE_SIGNATURE)
	{
		PE_InitializeDLLs(hModule);
		return;
	}
    if (pModule->dlls_to_init)
    {
	HANDLE to_init = pModule->dlls_to_init;
	pModule->dlls_to_init = 0;
        for (pDLL = (HMODULE *)GlobalLock( to_init ); *pDLL; pDLL++)
        {
            NE_InitializeDLLs( *pDLL );
            NE_InitDLL( *pDLL );
        }
        GlobalFree( to_init );
    }
    NE_InitDLL( hModule );
}


/***********************************************************************
 *           NE_PatchCodeHandle
 *
 * Needed for self-loading modules.
 */

/* It does nothing */
void PatchCodeHandle(HANDLE hSel)
{
}
#endif /* WINELIB */
