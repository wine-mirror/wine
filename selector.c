static char RCSId[] = "$Id: selector.c,v 1.3 1993/07/04 04:04:21 root Exp root $";
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
#include "wine.h"

#define MAX_SELECTORS	512

static struct segment_descriptor_s * EnvironmentSelector =  NULL;
static struct segment_descriptor_s * PSP_Selector = NULL;
struct segment_descriptor_s * MakeProcThunks = NULL;
unsigned short PSPSelector;
unsigned char ran_out = 0;
unsigned short SelectorOwners[MAX_SELECTORS];

static int next_unused_selector = 0;
extern void KERNEL_Ordinal_102();
extern void UNIXLIB_Ordinal_0();

/**********************************************************************
 *					GetNextSegment
 */
struct segment_descriptor_s *
GetNextSegment(unsigned int flags, unsigned int limit)
{
    struct segment_descriptor_s *selectors, *s;
    int sel_idx;
    FILE *zfile;

    sel_idx = next_unused_selector++;

    /*
     * Fill in selector info.
     */
    zfile = fopen("/dev/zero","r");

    s = malloc(sizeof(*s));
    s->flags = NE_SEGFLAGS_DATA;
    s->selector = (sel_idx << 3) | 0x0007;
    s->length = limit;
    s->base_addr = (void *) mmap((char *) (s->selector << 16),
				 ((s->length + PAGE_SIZE - 1) & 
				  ~(PAGE_SIZE - 1)),
				 PROT_EXEC | PROT_READ | PROT_WRITE,
				 MAP_FIXED | MAP_PRIVATE, fileno(zfile), 0);

    fclose(zfile);

    if (set_ldt_entry(sel_idx, (unsigned long) s->base_addr, 
		      (s->length - 1) & 0xffff, 0, 
		      MODIFY_LDT_CONTENTS_DATA, 0, 0) < 0)
    {
	next_unused_selector--;
	free(s);
	return NULL;
    }

    return s;
}

/**********************************************************************
 *					GetEntryPointFromOrdinal
 */
union lookup{
    struct entry_tab_header_s *eth;
    struct entry_tab_movable_s *etm;
    struct entry_tab_fixed_s *etf;
    char  * cpnt;
};

unsigned int GetEntryDLLName(char * dll_name, char * function, int * sel, 
				int  * addr)
{
	struct dll_table_entry_s *dll_table;
	struct w_files * wpnt;
	char * cpnt;
	int ordinal, j, len;

	dll_table = FindDLLTable(dll_name);

	if(dll_table) {
		ordinal = FindOrdinalFromName(dll_table, function);
		*sel = dll_table[ordinal].selector;
		*addr  = (unsigned int) dll_table[ordinal].address;
		return 0;
	};

	/* We need a means  of determining the ordinal for the function. */
	/* Not a builtin symbol, look to see what the file has for us */
	for(wpnt = wine_files; wpnt; wpnt = wpnt->next){
		if(strcmp(wpnt->name, dll_name)) continue;
		cpnt  = wpnt->nrname_table;
		while(1==1){
			if( ((int) cpnt)  - ((int)wpnt->nrname_table) >  
			   wpnt->ne_header->nrname_tab_length)  return 1;
			len = *cpnt++;
			if(strncmp(cpnt, function, len) ==  0) break;
			cpnt += len + 2;
		};
		ordinal =  *((unsigned short *)  (cpnt +  len));
		j = GetEntryPointFromOrdinal(wpnt, ordinal);		
		*addr  = j & 0xffff;
		j = j >> 16;
		*sel = wpnt->selector_table[j].selector;
		return 0;
	};
	return 1;
}

unsigned int GetEntryDLLOrdinal(char * dll_name, int ordinal, int * sel, 
				int  * addr)
{
	struct dll_table_entry_s *dll_table;
	struct w_files * wpnt;
	int j;

	dll_table = FindDLLTable(dll_name);

	if(dll_table) {
	    *sel = dll_table[ordinal].selector;
	    *addr  = (unsigned int) dll_table[ordinal].address;
	    return 0;
	};

	/* Not a builtin symbol, look to see what the file has for us */
	for(wpnt = wine_files; wpnt; wpnt = wpnt->next){
		if(strcmp(wpnt->name, dll_name)) continue;
		j = GetEntryPointFromOrdinal(wpnt, ordinal);
		*addr  = j & 0xffff;
		j = j >> 16;
		*sel = wpnt->selector_table[j].selector;
		return 0;
	};
	return 1;
}

unsigned int 
GetEntryPointFromOrdinal(struct w_files * wpnt, int ordinal)
{
   int fd =  wpnt->fd;
   struct mz_header_s *mz_header = wpnt->mz_header;   
   struct ne_header_s *ne_header = wpnt->ne_header;   

   
    union lookup entry_tab_pointer;
    struct entry_tab_header_s *eth;
    struct entry_tab_movable_s *etm;
    struct entry_tab_fixed_s *etf;
    int current_ordinal;
    int i;
    

   entry_tab_pointer.cpnt = wpnt->lookup_table;
    /*
     * Let's walk through the table until we get to our entry.
     */
    current_ordinal = 1;
    while (1)
    {
	/*
	 * Read header for this bundle.
	 */
	eth = entry_tab_pointer.eth++;
	
	if (eth->n_entries == 0)
	    return 0xffffffff;  /* Yikes - we went off the end of the table */

	if (eth->seg_number == 0)
	{
	    current_ordinal += eth->n_entries;
	    if(current_ordinal > ordinal) return 0;
	    continue;
	}

	/*
	 * Read each of the bundle entries.
	 */
	for (i = 0; i < eth->n_entries; i++, current_ordinal++)
	{
	    if (eth->seg_number >= 0xfe)
	    {
		    etm = entry_tab_pointer.etm++;

		if (current_ordinal == ordinal)
		{
		    return ((unsigned int) 
			    (wpnt->selector_table[etm->seg_number - 1].base_addr + 
			     etm->offset));
		}
	    }
	    else
	    {
		    etf = entry_tab_pointer.etf++;

		if (current_ordinal == ordinal)
		{
		    return ((unsigned int) 
			    (wpnt->selector_table[eth->seg_number - 1].base_addr + 
			     (int) etf->offset[0] + 
			     ((int) etf->offset[1] << 8)));
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
    return EnvironmentSelector->base_addr;
}

/**********************************************************************
 *					CreateEnvironment
 */
static struct segment_descriptor_s *
CreateEnvironment(FILE *zfile)
{
    char *p;
    int sel_idx;
    struct segment_descriptor_s * s;

    s = (struct segment_descriptor_s *) 
	    malloc(sizeof(struct segment_descriptor_s));

    sel_idx =  next_unused_selector;
    /*
     * Create memory to hold environment.
     */
    s->flags = NE_SEGFLAGS_DATA;
    s->selector = (next_unused_selector++ << 3) | 0x0007;
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
    if (set_ldt_entry(sel_idx, (unsigned long) s->base_addr, 
		      (s->length - 1) & 0xffff, 0, 
		      MODIFY_LDT_CONTENTS_DATA, 0, 0) < 0)
    {
	myerror("Could not create LDT entry for environment");
    }
    return  s;
}

/**********************************************************************
 *					CreateThunks
 */
static struct segment_descriptor_s *
CreateThunks(FILE *zfile)
{
    int sel_idx;
    struct segment_descriptor_s * s;

    s = (struct segment_descriptor_s *) 
	    malloc(sizeof(struct segment_descriptor_s));

    sel_idx =  next_unused_selector;
    /*
     * Create memory to hold environment.
     */
    s->flags = 0;
    s->selector = (next_unused_selector++ << 3) | 0x0007;
    s->length = 0x10000;
    s->base_addr = (void *) mmap((char *) (s->selector << 16),
				 s->length,
				 PROT_EXEC | PROT_READ | PROT_WRITE,
				 MAP_FIXED | MAP_PRIVATE, fileno(zfile), 0);


    /*
     * Create entry in LDT for this segment.
     */
    if (set_ldt_entry(sel_idx, (unsigned long) s->base_addr, 
		      (s->length - 1) & 0xffff, 0, 
		      MODIFY_LDT_CONTENTS_CODE, 0, 0) < 0)
    {
	myerror("Could not create LDT entry for thunks");
    }
    return  s;
}

/**********************************************************************
 *					CreatePSP
 */
static struct segment_descriptor_s *
CreatePSP(FILE *zfile)
{
    struct dos_psp_s *psp;
    unsigned short *usp;
    int sel_idx;
    struct segment_descriptor_s * s;

    s = (struct segment_descriptor_s *) 
	    malloc(sizeof(struct segment_descriptor_s));
    
    sel_idx =  next_unused_selector;
    /*
     * Create memory to hold PSP.
     */
    s->flags = NE_SEGFLAGS_DATA;
    s->selector = (next_unused_selector++ << 3) | 0x0007;
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
    psp->pspEnvironment = EnvironmentSelector->selector;
    psp->pspCommandTailCount = 1;
    strcpy(psp->pspCommandTail, "\r");
    

    /*
     * Create entry in LDT for this segment.
     */
    if (set_ldt_entry(sel_idx, (unsigned long) s->base_addr, 
		      (s->length - 1) & 0xffff, 0, 
		      MODIFY_LDT_CONTENTS_DATA, 0, 0) < 0)
    {
	myerror("Could not create LDT entry for PSP");
    }
    return s;
}

/**********************************************************************
 *					CreateSelectors
 */
struct segment_descriptor_s *
CreateSelectors(struct  w_files * wpnt)
{
    int fd = wpnt->fd;
    struct ne_segment_table_entry_s *seg_table = wpnt->seg_table;
    struct ne_header_s *ne_header = wpnt->ne_header;
    struct segment_descriptor_s *selectors, *s;
    unsigned short *sp;
    int contents, read_only;
    int SelectorTableLength;
    int i;
    int status;
    FILE * zfile;
    int old_length;

    /*
     * Allocate memory for the table to keep track of all selectors.
     */
    SelectorTableLength = ne_header->n_segment_tab;
    selectors = malloc(SelectorTableLength * sizeof(*selectors));
    if (selectors == NULL)
	return NULL;

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
	s->selector = ((next_unused_selector + i) << 3) | 0x0007;

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
	if (set_ldt_entry(i, (unsigned long) s->base_addr, 
			  (s->length - 1) & 0xffff, 0, 
			  contents, read_only, 0) < 0)
	{
	    free(selectors);
	    fprintf(stderr,"Ran out of ldt  entries.\n");
	    ran_out++;
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

    sp = &SelectorOwners[next_unused_selector];
    for (i = 0; i < ne_header->n_segment_tab; i++)
	*sp++ = (((next_unused_selector + ne_header->auto_data_seg - 1) << 3)
		 | 0x0007);

    next_unused_selector += ne_header->n_segment_tab;

    if(!EnvironmentSelector) {
	    EnvironmentSelector = CreateEnvironment(zfile);
	    PSP_Selector = CreatePSP(zfile);
	    MakeProcThunks = CreateThunks(zfile);
    };

    fclose(zfile);

    return selectors;
}
