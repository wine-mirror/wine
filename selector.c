static char RCSId[] = "$Id: selector.c,v 1.1 1993/06/29 15:55:18 root Exp $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/mman.h>
#include <linux/a.out.h>
#include <linux/ldt.h>
#include <errno.h>
#include "neexe.h"
#include "segmem.h"
#include "prototypes.h"

struct segment_descriptor_s *SelectorTable;
int SelectorTableLength;
int EnvironmentSelectorIdx;
int PSPSelectorIdx;
unsigned short PSPSelector;

extern void KERNEL_Ordinal_102();
extern void UNIXLIB_Ordinal_0();

/**********************************************************************
 *					GetEntryPointFromOrdinal
 */
unsigned int 
GetEntryPointFromOrdinal(int fd, struct mz_header_s *mz_header, 
			 struct ne_header_s *ne_header, int ordinal)
{
    struct entry_tab_header_s eth;
    struct entry_tab_movable_s etm;
    struct entry_tab_fixed_s etf;
    int current_ordinal;
    int i;
    
    /*
     * Move to the beginning of the entry table.
     */
    lseek(fd, mz_header->ne_offset + ne_header->entry_tab_offset, SEEK_SET);

    /*
     * Let's walk through the table until we get to our entry.
     */
    current_ordinal = 1;
    while (1)
    {
	/*
	 * Read header for this bundle.
	 */
	if (read(fd, &eth, sizeof(eth)) != sizeof(eth))
	    myerror("Error reading entry table");
	
	if (eth.n_entries == 0)
	    return 0;

	if (eth.seg_number == 0)
	{
	    current_ordinal++;
	    continue;
	}

	/*
	 * Read each of the bundle entries.
	 */
	for (i = 0; i < eth.n_entries; i++, current_ordinal++)
	{
	    if (eth.seg_number >= 0xfe)
	    {
		if (read(fd, &etm, sizeof(etm)) != sizeof(etm))
		    myerror("Error reading entry table");

		if (current_ordinal == ordinal)
		{
		    return ((unsigned int) 
			    (SelectorTable[etm.seg_number - 1].base_addr + 
			     etm.offset));
		}
	    }
	    else
	    {
		if (read(fd, &etf, sizeof(etf)) != sizeof(etf))
		    myerror("Error reading entry table");

		if (current_ordinal == ordinal)
		{
		    return ((unsigned int) 
			    (SelectorTable[eth.seg_number - 1].base_addr + 
			     (int) etf.offset[0] + 
			     ((int) etf.offset[1] << 8)));
		}
	    }
	}
    }
}

/**********************************************************************
 *					GetDOSEnvironment
 */
void *
GetDOSEnvironment()
{
    return SelectorTable[EnvironmentSelectorIdx].base_addr;
}

/**********************************************************************
 *					CreateEnvironment
 */
void
CreateEnvironment(int sel_idx, struct segment_descriptor_s *s, FILE *zfile)
{
    char *p;

    EnvironmentSelectorIdx = sel_idx;

    /*
     * Create memory to hold environment.
     */
    s->flags = NE_SEGFLAGS_DATA;
    s->selector = (sel_idx << 3) | 0x0007;
    s->length = PAGE_SIZE;
    s->base_addr = (void *) mmap((char *) (s->selector << 16),
				 PAGE_SIZE,
				 PROT_EXEC | PROT_READ | PROT_WRITE,
				 MAP_FIXED | MAP_PRIVATE, fileno(zfile), 0);

    /*
     * Fill environment with meaningless babble.
     */
    p = (char *) s->base_addr;
    strcpy(p, "PATH=C:\\WINDOWS");
    p += strlen(p) + 1;
    *p++ = '\0';
    *p++ = 11;
    *p++ = 0;
    strcpy(p, "C:\\TEST.EXE");

    /*
     * Create entry in LDT for this segment.
     */
    if (set_ldt_entry(sel_idx, (unsigned long) s->base_addr, s->length, 0, 
		      MODIFY_LDT_CONTENTS_DATA, 0, 0) < 0)
    {
	myerror("Could not create LDT entry for environment");
    }
}

/**********************************************************************
 *					CreatePSP
 */
void
CreatePSP(int sel_idx, struct segment_descriptor_s *s, FILE *zfile)
{
    struct dos_psp_s *psp;
    unsigned short *usp;
    
    PSPSelectorIdx = sel_idx;

    /*
     * Create memory to hold PSP.
     */
    s->flags = NE_SEGFLAGS_DATA;
    s->selector = (sel_idx << 3) | 0x0007;
    s->length = PAGE_SIZE;
    s->base_addr = (void *) mmap((char *) (s->selector << 16),
				 PAGE_SIZE,
				 PROT_EXEC | PROT_READ | PROT_WRITE,
				 MAP_FIXED | MAP_PRIVATE, fileno(zfile), 0);

    /*
     * Fill PSP
     */
    PSPSelector = s->selector;
    psp = (struct dos_psp_s *) s->base_addr;
    psp->pspInt20 = 0x20cd;
    psp->pspDispatcher[0] = 0x9a;
    usp = (unsigned short *) &psp->pspDispatcher[1];
    *usp       = (unsigned short) KERNEL_Ordinal_102;
    *(usp + 1) = 0x23;
    psp->pspTerminateVector[0] = (unsigned short) UNIXLIB_Ordinal_0;
    psp->pspTerminateVector[1] = 0x0023;
    psp->pspControlCVector[0] = (unsigned short) UNIXLIB_Ordinal_0;
    psp->pspControlCVector[1] = 0x0023;
    psp->pspCritErrorVector[0] = (unsigned short) UNIXLIB_Ordinal_0;
    psp->pspCritErrorVector[1] = 0x0023;
    psp->pspEnvironment = SelectorTable[EnvironmentSelectorIdx].selector;
    psp->pspCommandTailCount = 1;
    strcpy(psp->pspCommandTail, "\r");
    

    /*
     * Create entry in LDT for this segment.
     */
    if (set_ldt_entry(sel_idx, (unsigned long) s->base_addr, s->length, 0, 
		      MODIFY_LDT_CONTENTS_DATA, 0, 0) < 0)
    {
	myerror("Could not create LDT entry for PSP");
    }
}

/**********************************************************************
 *					CreateSelectors
 */
struct segment_descriptor_s *
CreateSelectors(int fd, struct ne_segment_table_entry_s *seg_table,
		struct ne_header_s *ne_header)
{
    struct segment_descriptor_s *selectors, *s;
    int contents, read_only;
    int i;
    int status;
    FILE * zfile;
    int old_length;

    /*
     * Allocate memory for the table to keep track of all selectors.
     */
    SelectorTableLength = ne_header->n_segment_tab + 2;
    selectors = malloc(SelectorTableLength * sizeof(*selectors));
    if (selectors == NULL)
	return NULL;
    SelectorTable = selectors;

    /*
     * Step through the segment table in the exe header.
     */
    s = selectors;
    zfile = fopen("/dev/zero","r");
    for (i = 0; i < ne_header->n_segment_tab; i++, s++)
    {
#ifdef DEBUG_SEGMENT
	printf("  %2d: OFFSET %04.4x, LENGTH %04.4x, ",
	       i + 1, seg_table[i].seg_data_offset, 
	       seg_table[i].seg_data_length);
	printf("FLAGS %04.4x, MIN ALLOC %04.4x\n",
	       seg_table[i].seg_flags, seg_table[i].min_alloc);
#endif

	/*
	 * Store the flags in our table.
	 */
	s->flags = seg_table[i].seg_flags;
	s->selector = (i << 3) | 0x0007;

	/*
	 * Is there an image for this segment in the file?
	 */
	if (seg_table[i].seg_data_offset == 0)
	{
	    /*
	     * No image in exe file, let's allocate some memory for it.
	     */
	    s->length = seg_table[i].min_alloc;
	}
	else
	{
	    /*
	     * Image in file, let's just point to the image in memory.
	     */
	    s->length    = seg_table[i].seg_data_length;
	}

	if (s->length == 0)
	    s->length = 0x10000;
	old_length = s->length;

	/*
	 * If this is the automatic data segment, its size must be adjusted.
	 * First we need to check for local heap.  Second we nee to see if
	 * this is also the stack segment.
	 */
	if (i + 1 == ne_header->auto_data_seg)
	{
	    s->length += ne_header->local_heap_length;

	    if (i + 1 == ne_header->ss)
	    {
		s->length += ne_header->stack_length;
		ne_header->sp = s->length;
	    }
	}

	/*
	 * Is this a DATA or CODE segment?
	 */
	read_only = 0;
	if (s->flags & NE_SEGFLAGS_DATA)
	{
	    contents = MODIFY_LDT_CONTENTS_DATA;
	    if (s->flags & NE_SEGFLAGS_READONLY)
		read_only = 1;
	}
	else
	{
	    contents = MODIFY_LDT_CONTENTS_CODE;
	    if (s->flags & NE_SEGFLAGS_EXECUTEONLY)
		read_only = 1;
	}
	s->base_addr =
	  (void *) mmap((char *) (s->selector << 16),
			(s->length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1),
			PROT_EXEC | PROT_READ | PROT_WRITE,
			MAP_FIXED | MAP_PRIVATE, fileno(zfile), 0);
	if (seg_table[i].seg_data_offset != 0)
	{
	    /*
	     * Image in file.
	     */
	    status = lseek(fd, seg_table[i].seg_data_offset * 
			   (1 << ne_header->align_shift_count), SEEK_SET);
	    if(read(fd, s->base_addr, old_length) != old_length)
		myerror("Unable to read segment from file");
	}
	/*
	 * Create entry in LDT for this segment.
	 */
	if (set_ldt_entry(i, (unsigned long) s->base_addr, s->length, 0, 
			  contents, read_only, 0) < 0)
	{
	    free(selectors);
	    return NULL;
	}
	/*
	 * If this is the automatic data segment, then we must initialize
	 * the local heap.
	 */
	if (i + 1 == ne_header->auto_data_seg)
	{
	    HEAP_LocalInit(s->base_addr + old_length, 
			   ne_header->local_heap_length);
	}
    }

    CreateEnvironment(i++, s++, zfile);
    CreatePSP(i++, s++, zfile);

    fclose(zfile);

    return selectors;
}
