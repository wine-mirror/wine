static char RCSId[] = "$Id: wine.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#ifdef linux
#include <linux/head.h>
#include <linux/ldt.h>
#include <linux/segment.h>
#endif
#include <string.h>
#include <errno.h>
#include "neexe.h"
#include "segmem.h"
#include "prototypes.h"
#include "dlls.h"
#include "wine.h"
#include "windows.h"
#include "wineopts.h"
#include "arch.h"
#include "options.h"
#include "stddebug.h"
/* #define DEBUG_FIXUP /* */
/* #undef DEBUG_FIXUP  /* */
#include "debug.h"

extern HANDLE CreateNewTask(HINSTANCE hInst);
extern void InitializeLoadedDLLs(struct w_files *wpnt);
extern int CallToInit16(unsigned long csip, unsigned long sssp, 
			unsigned short ds);
extern void CallTo32();
extern char WindowsPath[256];
extern unsigned short WIN_StackSize;
extern unsigned short WIN_HeapSize;

int FixupSegment(struct w_files *, int);
void FixupFunctionPrologs(struct w_files *);
char *GetModuleName(struct w_files * wpnt, int index, char *buffer);

#ifndef WINELIB

/**********************************************************************/

void load_ne_header (int fd, struct ne_header_s *ne_header)
{
    if (read(fd, ne_header, sizeof(struct ne_header_s)) 
	!= sizeof(struct ne_header_s))
    {
	myerror("Unable to read NE header from file");
    }
}
#endif

/**********************************************************************
 *			LoadNEImage
 * Load one NE format executable into memory
 */
HINSTANCE LoadNEImage(struct w_files *wpnt)
{
    unsigned int read_size, status, segment;
    int i;

    wpnt->ne = malloc(sizeof(struct ne_data));
    wpnt->ne->resnamtab = NULL;
    wpnt->ne->ne_header = malloc(sizeof(struct ne_header_s));
    lseek(wpnt->fd, wpnt->mz_header->ne_offset, SEEK_SET);
    load_ne_header(wpnt->fd, wpnt->ne->ne_header);

#ifndef WINELIB
    /*
     * Create segment selectors.
     */
    status = lseek(wpnt->fd, wpnt->mz_header->ne_offset + 
		   wpnt->ne->ne_header->segment_tab_offset,
		   SEEK_SET);
    read_size  = wpnt->ne->ne_header->n_segment_tab *
	sizeof(struct ne_segment_table_entry_s);
    wpnt->ne->seg_table = (struct ne_segment_table_entry_s *) malloc(read_size);
    if (read(wpnt->fd, wpnt->ne->seg_table, read_size) != read_size)
	myerror("Unable to read segment table header from file");
    wpnt->ne->selector_table = CreateSelectors(wpnt);
    wpnt->hinstance = (wpnt->ne->
		       selector_table[wpnt->ne->ne_header->auto_data_seg-1].
		       selector);
#endif
    /* Get the lookup  table.  This is used for looking up the addresses
       of functions that are exported */

    read_size  = wpnt->ne->ne_header->entry_tab_length;
    wpnt->ne->lookup_table = (char *) malloc(read_size);
    lseek(wpnt->fd, wpnt->mz_header->ne_offset + 
	  wpnt->ne->ne_header->entry_tab_offset, SEEK_SET);
    if (read(wpnt->fd, wpnt->ne->lookup_table, read_size) != read_size)
	myerror("Unable to read lookup table header from file");

    /* Get the iname table.  This is used for looking up the names
       of functions that are exported */

    status = lseek(wpnt->fd, wpnt->ne->ne_header->nrname_tab_offset,  SEEK_SET);
    read_size  = wpnt->ne->ne_header->nrname_tab_length;
    wpnt->ne->nrname_table = (char *) malloc(read_size);
    if (read(wpnt->fd, wpnt->ne->nrname_table, read_size) != read_size)
	myerror("Unable to read nrname table header from file");

    status = lseek(wpnt->fd, wpnt->mz_header->ne_offset + 
		   wpnt->ne->ne_header->rname_tab_offset,  SEEK_SET);
    read_size  = wpnt->ne->ne_header->moduleref_tab_offset - 
	    wpnt->ne->ne_header->rname_tab_offset;
    wpnt->ne->rname_table = (char *) malloc(read_size);
    if (read(wpnt->fd, wpnt->ne->rname_table, read_size) != read_size)
	myerror("Unable to read rname table header from file");

    /*
     * Now load any DLLs that  this module refers to.
     */
    for(i=0; i<wpnt->ne->ne_header->n_mod_ref_tab; i++)
    {
      char buff[14];
      GetModuleName(wpnt, i + 1, buff);

      if (strcasecmp(buff, wpnt->name) != 0 )
	LoadImage(buff, DLL, 0);
    }
#ifndef WINELIB
    /* fixup references */

    for (segment = 0; segment < wpnt->ne->ne_header->n_segment_tab; segment++)
	if (FixupSegment(wpnt, segment) < 0)
		myerror("fixup failed.");

    FixupFunctionPrologs(wpnt);
    InitializeLoadedDLLs(wpnt);
#endif
    return(wpnt->hinstance);
}

/**********************************************************************
 *					GetImportedName
 */
char *
GetImportedName(int fd, struct mz_header_s *mz_header, 
		struct ne_header_s *ne_header, int name_offset, char *buffer)
{
    int length;
    int status;
    
    status = lseek(fd, mz_header->ne_offset + ne_header->iname_tab_offset +
		   name_offset, SEEK_SET);
    length = 0;
    read(fd, &length, 1);  /* Get the length byte */
    length = CONV_CHAR_TO_LONG (length);
    read(fd, buffer, length);
    buffer[length] = 0;
    return buffer;
}

/**********************************************************************
 *					GetModuleName
 */
char *
GetModuleName(struct w_files * wpnt, int index, char *buffer)
{
    int fd = wpnt->fd;
    struct mz_header_s *mz_header = wpnt->mz_header; 
    struct ne_header_s *ne_header = wpnt->ne->ne_header;
    int length;
    WORD name_offset, status;
    int i;
    
    status = lseek(fd, mz_header->ne_offset + ne_header->moduleref_tab_offset +
		   2*(index - 1), SEEK_SET);
    name_offset = 0;
    read(fd, &name_offset, 2);
    name_offset = CONV_SHORT (name_offset);
    status = lseek(fd, mz_header->ne_offset + ne_header->iname_tab_offset +
		   name_offset, SEEK_SET);
    length = 0;
    read(fd, &length, 1);  /* Get the length byte */
    length = CONV_CHAR_TO_LONG (length);
    read(fd, buffer, length);
    buffer[length] = 0;

    /* Module names  are always upper case */
    for(i=0; i<length; i++)
    	if (islower(buffer[i]))
    		buffer[i] = toupper(buffer[i]);

    return buffer;
}


#ifndef WINELIB
/**********************************************************************
 *					FixupSegment
 */
int
FixupSegment(struct w_files * wpnt, int segment_num)
{
    int fd =  wpnt->fd;
    struct mz_header_s *mz_header = wpnt->mz_header;
    struct ne_header_s *ne_header =  wpnt->ne->ne_header;
    struct ne_segment_table_entry_s *seg_table = wpnt->ne->seg_table;
    struct segment_descriptor_s *selector_table = wpnt->ne->selector_table;
    struct relocation_entry_s *rep, *rep1;
    struct ne_segment_table_entry_s *seg;
    struct segment_descriptor_s *sel;
    int status;
    unsigned short *sp;
    unsigned int selector, address;
    unsigned int next_addr;
    int ordinal;
    char dll_name[257];
    char func_name[257];
    int i, n_entries;
    int additive;

    seg = &seg_table[segment_num];
    sel = &selector_table[segment_num];

    dprintf_fixup(stddeb, "Segment fixups for %s, segment %d, selector %x\n", 
                  wpnt->name, segment_num, (int) sel->base_addr >> 16);

    if ((seg->seg_data_offset == 0) ||
	!(seg->seg_flags & NE_SEGFLAGS_RELOC_DATA))
	return 0;

    /*
     * Go through the relocation table on entry at a time.
     */
    i = seg->seg_data_length;
    if (i == 0)
	i = 0x10000;

    status = lseek(fd, seg->seg_data_offset * 
		       (1 << ne_header->align_shift_count) + i, SEEK_SET);
    n_entries = 0;
    read(fd, &n_entries, sizeof(short int));
    rep = (struct relocation_entry_s *)
	  malloc(n_entries * sizeof(struct relocation_entry_s));

    if (read(fd,rep, n_entries * sizeof(struct relocation_entry_s)) !=
        n_entries * sizeof(struct relocation_entry_s))
    {
	myerror("Unable to read relocation information");
    }
    
    rep1 = rep;

    for (i = 0; i < n_entries; i++, rep++)
    {
	/*
	 * Get the target address corresponding to this entry.
	 */
	additive = 0;
	
	switch (rep->relocation_type)
	{
	  case NE_RELTYPE_ORDINALADD:
	    additive = 1;
	    
	  case NE_RELTYPE_ORDINAL:
	    if (GetModuleName(wpnt, rep->target1,
			      dll_name) == NULL)
	    {
	      fprintf(stderr, "NE_RELTYPE_ORDINAL failed");
		return -1;
	    }
	    
	    ordinal = rep->target2;

  	    status = GetEntryDLLOrdinal(dll_name, ordinal, &selector,
					&address);
	    if (status)
	    {
		char s[80];
		
		sprintf(s, "Bad DLL name '%s.%d'", dll_name, ordinal);
		myerror(s);
		return -1;
	    }

	    dprintf_fixup(stddeb,"%d: %s.%d: %04.4x:%04.4x\n", i + 1, 
		   dll_name, ordinal, selector, address);
	    break;
	    
	  case NE_RELTYPE_NAMEADD:
	    additive = 1;
	    
	  case NE_RELTYPE_NAME:
	    if (GetModuleName(wpnt, rep->target1, dll_name)
		== NULL)
	    {
	      fprintf(stderr,"NE_RELTYPE_NAME failed");
		return -1;
	    }

	    if (GetImportedName(fd, mz_header, ne_header, 
				rep->target2, func_name) == NULL)
	    {
	      fprintf(stderr,"getimportedname failed");
		return -1;
	    }

  	    status = GetEntryDLLName(dll_name, func_name, &selector, 
					   &address);
	    if (status)
	    {
		char s[80];
		
		sprintf(s, "Bad DLL name '%s (%s)'", dll_name,func_name);
		myerror(s);
		return -1;
	    }

	    dprintf_fixup(stddeb,"%d: %s %s.%d: %04.4x:%04.4x\n", i + 1, 
                   func_name, dll_name, ordinal, selector, address);
	    break;
	    
	  case NE_RELTYPE_INTERNAL:
    	  case NE_RELTYPE_INT1:
	    if (rep->target1 == 0x00ff)
	    {
		address  = GetEntryPointFromOrdinal(wpnt, rep->target2);
		selector = (address >> 16) & 0xffff;
		address &= 0xffff;
	    }
	    else
	    {
		selector = selector_table[rep->target1-1].selector;
		address  = rep->target2;
	    }
	    
	    dprintf_fixup(stddeb,"%d: %04.4x:%04.4x\n", 
			  i + 1, selector, address);
	    break;

	  case 7:
	    /* Relocation type 7:
	     *
	     *    These appear to be used as fixups for the Windows
	     * floating point emulator.  Let's just ignore them and
	     * try to use the hardware floating point.  Linux should
	     * successfully emulate the coprocessor if it doesn't
	     * exist.
	     */
	    dprintf_fixup(stddeb,
                   "%d: ADDR TYPE %d,  TYPE %d,  OFFSET %04.4x,  ",
		   i + 1, rep->address_type, rep->relocation_type, 
		   rep->offset);
	    dprintf_fixup(stddeb,"TARGET %04.4x %04.4x\n", 
		   rep->target1, rep->target2);
	    continue;
	    
	  default:
	    dprintf_fixup(stddeb,
		   "%d: ADDR TYPE %d,  TYPE %d,  OFFSET %04.4x,  ",
		   i + 1, rep->address_type, rep->relocation_type, 
		   rep->offset);
	    dprintf_fixup(stddeb,"TARGET %04.4x %04.4x\n", 
		    rep->target1, rep->target2);
	    free(rep1);
	    return -1;
	}

	/*
	 * Stuff the right size result in.
	 */
	sp = (unsigned short *) ((char *) sel->base_addr + rep->offset);
	if (additive)
	{
	    if (FindDLLTable(dll_name) == NULL)
		additive = 2;
	    dprintf_fixup(stddeb,
		   "%d: ADDR TYPE %d,  TYPE %d,  OFFSET %04.4x,  ",
		   i + 1, rep->address_type, rep->relocation_type, 
		   rep->offset);
	    dprintf_fixup(stddeb,"TARGET %04.4x %04.4x\n", 
		    rep->target1, rep->target2);
	    dprintf_fixup(stddeb, "    Additive = %d\n", additive);
	}
	
	switch (rep->address_type)
	{
	  case NE_RADDR_OFFSET16:
	    do {
		dprintf_fixup(stddeb,"    %04.4x:%04.4x:%04.4x OFFSET16\n",
		       (unsigned long) sp >> 16, (int) sp & 0xFFFF, *sp);
		next_addr = *sp;
		*sp = (unsigned short) address;
		if (additive == 2)
		    *sp += next_addr;
		sp = (unsigned short *) ((char *) sel->base_addr + next_addr);
	    } 
	    while (next_addr != 0xffff && !additive);

	    break;
	    
	  case NE_RADDR_POINTER32:
	    do {
		dprintf_fixup(stddeb,"    %04.4x:%04.4x:%04.4x POINTER32\n",
		       (unsigned long) sp >> 16, (int) sp & 0xFFFF, *sp);
		next_addr = *sp;
		*sp     = (unsigned short) address;
		if (additive == 2)
		    *sp += next_addr;
		*(sp+1) = (unsigned short) selector;
		sp = (unsigned short *) ((char *) sel->base_addr + next_addr);
	    } 
	    while (next_addr != 0xffff && !additive);

	    break;
	    
	  case NE_RADDR_SELECTOR:
	    do {
		dprintf_fixup(stddeb,"    %04.4x:%04.4x:%04.4x SELECTOR\n",
		       (unsigned long) sp >> 16, (int) sp & 0xFFFF, *sp);
		next_addr = *sp;
		*sp     = (unsigned short) selector;
		sp = (unsigned short *) ((char *) sel->base_addr + next_addr);
		if (rep->relocation_type == NE_RELTYPE_INT1) 
		    break;
	    } 
	    while (next_addr != 0xffff && !additive);

	    break;
	    
	  default:
	    dprintf_fixup(stddeb,
		   "%d: ADDR TYPE %d,  TYPE %d,  OFFSET %04.4x,  ",
		   i + 1, rep->address_type, rep->relocation_type, 
		   rep->offset);
	    dprintf_fixup(stddeb,
		   "TARGET %04.4x %04.4x\n", rep->target1, rep->target2);
	    free(rep1);
	    return -1;
	}
    }

    free(rep1);
    return 0;
}

int NEunloadImage(struct w_files *wpnt)
{
	printf("NEunloadImage() called!\n");
	/* free resources, image */
	return 1;
}

int StartNEprogram(struct w_files *wpnt)
{
    int cs_reg, ds_reg, ss_reg, ip_reg, sp_reg;
    /*
     * Fixup stack and jump to start. 
     */
    WIN_StackSize = wpnt->ne->ne_header->stack_length;
    WIN_HeapSize = wpnt->ne->ne_header->local_heap_length;

    ds_reg = wpnt->ne->selector_table[wpnt->ne->ne_header->auto_data_seg-1].selector;
    cs_reg = wpnt->ne->selector_table[wpnt->ne->ne_header->cs-1].selector;
    ip_reg = wpnt->ne->ne_header->ip;
    ss_reg = wpnt->ne->selector_table[wpnt->ne->ne_header->ss-1].selector;
    sp_reg = wpnt->ne->ne_header->sp;

    return CallToInit16(cs_reg << 16 | ip_reg, ss_reg << 16 | sp_reg, ds_reg);
}

void InitNEDLL(struct w_files *wpnt)
{
	int cs_reg, ds_reg, ip_reg, rv;
	/* 
	 * Is this a library? 
	 */
	if (wpnt->ne->ne_header->format_flags & 0x8000)
	{
	    if (!(wpnt->ne->ne_header->format_flags & 0x0001))
	    {
		/* Not SINGLEDATA */
		fprintf(stderr, "Library is not marked SINGLEDATA\n");
		exit(1);
	    }

	    ds_reg = wpnt->ne->selector_table[wpnt->ne->
					  ne_header->auto_data_seg-1].selector;
	    cs_reg = wpnt->ne->selector_table[wpnt->ne->ne_header->cs-1].selector;
	    ip_reg = wpnt->ne->ne_header->ip;

	    if (cs_reg) {
		fprintf(stderr, "Initializing %s, cs:ip %04x:%04x, ds %04x\n", 
		    wpnt->name, cs_reg, ip_reg, ds_reg);
	    	    
		rv = CallTo16(cs_reg << 16 | ip_reg, ds_reg);
		printf ("rv = %x\n", rv);
	    } else
		printf("%s skipped\n", wpnt->name);
	}
}

#endif /* !WINELIB */
