static char RCSId[] = "$Id: selector.c,v 1.3 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef __linux__
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/mman.h>
#include <linux/a.out.h>
#include <linux/ldt.h>
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/mman.h>
#endif
#include <errno.h>
#include "neexe.h"
#include "segmem.h"
#include "prototypes.h"
#include "wine.h"

/* #define DEBUG_SELECTORS /* */

#ifdef linux
#define DEV_ZERO
#define UTEXTSEL 0x23
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <machine/segments.h>
#define PAGE_SIZE getpagesize()
#define MODIFY_LDT_CONTENTS_DATA	0
#define MODIFY_LDT_CONTENTS_STACK	1
#define MODIFY_LDT_CONTENTS_CODE	2
#define UTEXTSEL 0x1f
#endif

static SEGDESC * EnvironmentSelector =  NULL;
static SEGDESC * PSP_Selector = NULL;
SEGDESC * MakeProcThunks = NULL;
unsigned short PSPSelector;
unsigned char ran_out = 0;
int LastUsedSelector = FIRST_SELECTOR - 1;

unsigned short SelectorMap[MAX_SELECTORS];
SEGDESC Segments[MAX_SELECTORS];

#ifdef DEV_ZERO
    static FILE *zfile = NULL;
#endif    

extern void KERNEL_Ordinal_102();
extern void UNIXLIB_Ordinal_0();

extern char **Argv;
extern int Argc;

/**********************************************************************
 *					FindUnusedSelectors
 */
int
FindUnusedSelectors(int n_selectors)
{
    int i;
    int n_found;

    n_found = 0;
    for (i = LastUsedSelector + 1; i != LastUsedSelector; i++)
    {
	if (i >= MAX_SELECTORS)
	{
	    n_found = 0;
	    i = FIRST_SELECTOR;
	}
	
	if (!SelectorMap[i] && ++n_found == n_selectors)
	    break;
    }
    
    if (i == LastUsedSelector)
	return 0;

    LastUsedSelector = i;
    return i - n_selectors + 1;
}

#ifdef HAVE_IPC
/**********************************************************************
 *					IPCCopySelector
 *
 * Created a shared memory copy of a segment:
 *
 *	- at a new selector location (if "new" is a 16-bit value)
 *	- at an arbitrary memory location (if "new" is a 32-bit value)
 */
int
IPCCopySelector(int i_old, unsigned long new, int swap_type)
{
    SEGDESC *s_new, *s_old;
    int i_new;
    void *base_addr;

    s_old  = &Segments[i_old];

    if (new & 0xffff0000)
    {
	/**************************************************************
	 * Let's set the address parameter for no segment.
	 */
	i_new = -1;
	s_new = NULL;
	base_addr = (void *) new;
    }
    else
    {
	/***************************************************************
	 * We need to fill in the segment descriptor for segment "new".
	 */
	i_new = new;
	s_new = &Segments[i_new];

	SelectorMap[i_new] = i_new;
    
	s_new->selector  = (i_new << 3) | 0x0007;
	s_new->base_addr = (void *) ((long) s_new->selector << 16);
	s_new->length    = s_old->length;
	s_new->flags     = s_old->flags;
	s_new->owner     = s_old->owner;
	if (swap_type)
	{
	    if (s_old->type == MODIFY_LDT_CONTENTS_DATA)
		s_new->type = MODIFY_LDT_CONTENTS_CODE;
	    else
		s_new->type = MODIFY_LDT_CONTENTS_DATA;
	}
	else
	    s_new->type = s_old->type;

	base_addr = s_new->base_addr;
    }

    /******************************************************************
     * If we don't have a shared memory key for s_old, then we need
     * to get one.  In this case, we'll also have to copy the data
     * to protect it.
     */
    if (s_old->shm_key == 0)
    {
	s_old->shm_key = shmget(IPC_PRIVATE, s_old->length, 0600);
	if (s_old->shm_key == 0)
	{
	    if (s_new)
		memset(s_new, 0, sizeof(*s_new));
	    return -1;
	}
	if (shmat(s_old->shm_key, base_addr, 0) == NULL)
	{
	    if (s_new)
		memset(s_new, 0, sizeof(*s_new));
	    shmctl(s_old->shm_key, IPC_RMID, NULL);
	    return -1;
	}
	memcpy(base_addr, s_old->base_addr, s_old->length);
	munmap(s_old->base_addr, 
	       ((s_old->length + PAGE_SIZE) & ~(PAGE_SIZE - 1)));
	shmat(s_old->shm_key, s_old->base_addr, 0);
    }
    /******************************************************************
     * If have shared memory key s_old, then just attach the new
     * address.
     */
    else
    {
	if (shmat(s_old->shm_key, base_addr, 0) == NULL)
	{
	    if (s_new)
		memset(s_new, 0, sizeof(*s_new));
	    return -1;
	}
    }

    /******************************************************************
     * If we are creating a new segment, then we also need to update
     * the LDT to include the new selector.  In this return the
     * new selector.
     */
    if (s_new)
    {
	s_new->shm_key = s_old->shm_key;

	if (set_ldt_entry(i_new, (unsigned long) base_addr, 
			  s_old->length - 1, 0, s_new->type, 0, 0) < 0)
	{
	    return -1;
	}

	return s_new->selector;
    }
    /******************************************************************
     * No new segment.  So, just return the shared memory key.
     */
    else
	return s_old->shm_key;
}
#endif

/**********************************************************************
 *					AllocSelector
 *
 * This is very bad!!!  This function is implemented for Windows
 * compatibility only.  Do not call this from the emulation library.
 */
unsigned int
AllocSelector(unsigned int old_selector)
{
    SEGDESC *s_new, *s_old;
    int i_new, i_old;
    int selector;
    
    i_new = FindUnusedSelectors(1);
    s_new = &Segments[i_new];
    
    if (old_selector)
    {
	i_old = (old_selector >> 3);
#ifdef HAVE_IPC
	selector = IPCCopySelector(i_old, i_new, 0);
	if (selector < 0)
	    return 0;
	else
	    return selector;
#else
	s_old = &Segments[i_old];
	s_new->selector = (i_new << 3) | 0x0007;
	*s_new = *s_old;
	SelectorMap[i_new] = SelectorMap[i_old];

	if (set_ldt_entry(i_new, s_new->base_addr, 
			  s_new->length - 1, 0, 
			  s_new->type, 0, 0) < 0)
	{
	    return 0;
	}
#endif
    }
    else
    {
	memset(s_new, 0, sizeof(*s_new));
	SelectorMap[i_new] = i_new;
    }

    return (i_new << 3) | 0x0007;
}

/**********************************************************************
 *					PrestoChangoSelector
 *
 * This is very bad!!!  This function is implemented for Windows
 * compatibility only.  Do not call this from the emulation library.
 */
unsigned int PrestoChangoSelector(unsigned src_selector, unsigned dst_selector)
{
#ifdef HAVE_IPC
    SEGDESC *src_s;
    int src_idx, dst_idx;

    src_idx = src_selector >> 3;
    dst_idx = dst_selector >> 3;

    if (src_idx == dst_idx)
    {
	src_s = &Segments[src_idx];
	
	if (src_s->type == MODIFY_LDT_CONTENTS_DATA)
	    src_s->type = MODIFY_LDT_CONTENTS_CODE;
	else
	    src_s->type = MODIFY_LDT_CONTENTS_DATA;

	if (set_ldt_entry(src_idx, (long) src_s->base_addr,
			  src_s->length - 1, 0, src_s->type, 0, 0) < 0)
	{
	    return 0;
	}

	return src_s->selector;
    }
    else
    {
	return IPCCopySelector(src_idx, dst_idx, 1);
    }
#else /* HAVE_IPC */
    SEGDESC *src_s, *dst_s;
    char *p;
    int src_idx, dst_idx;
    int alias_count;
    int i;

    src_idx = (SelectorMap[src_selector >> 3]);
    dst_idx = dst_selector >> 3;
    src_s = &Segments[src_idx];
    dst_s = &Segments[dst_idx];

    alias_count = 0;
    for (i = FIRST_SELECTOR; i < MAX_SELECTORS; i++)
	if (SelectorMap[i] == src_idx)
	    alias_count++;
    
    if (src_s->type == MODIFY_LDT_CONTENTS_DATA 
	|| alias_count > 1 || src_idx == dst_idx)
    {
	*dst_s = *src_s;
	
	if (src_s->type == MODIFY_LDT_CONTENTS_DATA)
	    dst_s->type = MODIFY_LDT_CONTENTS_CODE;
	else
	    dst_s->type = MODIFY_LDT_CONTENTS_DATA;

	SelectorMap[dst_idx] = SelectorMap[src_idx];
	if (set_ldt_entry(dst_idx, (long) dst_s->base_addr,
			  dst_s->length - 1, 0, dst_s->type, 0, 0) < 0)
	{
	    return 0;
	}
    }
    else
    {
	/*
	 * We're changing an unaliased code segment into a data
	 * segment.  The SAFEST (but ugliest) way to deal with 
	 * this is to map the new segment and copy all the contents.
	 */
	SelectorMap[dst_idx] = dst_idx;
	*dst_s = *src_s;
	dst_s->selector  = (dst_idx << 3) | 0x0007;
	dst_s->base_addr = (void *) ((unsigned int) dst_s->selector << 16);
	dst_s->type      = MODIFY_LDT_CONTENTS_DATA;
#ifdef DEV_ZERO
	if (zfile == NULL)
	    zfile = fopen("/dev/zero","r");
	p = (void *) mmap((char *) dst_s->base_addr,
			  ((dst_s->length + PAGE_SIZE) 
			   & ~(PAGE_SIZE - 1)),
			  PROT_EXEC | PROT_READ | PROT_WRITE,
			  MAP_FIXED | MAP_PRIVATE, fileno(zfile), 0);
#else
	p = (void *) mmap((char *) dst_s->base_addr,
			  ((dst_s->length + PAGE_SIZE) 
			   & ~(PAGE_SIZE - 1)),
			  PROT_EXEC | PROT_READ | PROT_WRITE,
			  MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
	if (p == NULL)
	    return 0;
	
	memcpy((void *) dst_s->base_addr, (void *) src_s->base_addr, 
	       dst_s->length);
	if (set_ldt_entry(src_idx, dst_s->base_addr,
			  dst_s->length - 1, 0, dst_s->type, 0, 0) < 0)
	{
	    return 0;
	}
	if (set_ldt_entry(dst_idx, dst_s->base_addr,
			  dst_s->length - 1, 0, dst_s->type, 0, 0) < 0)
	{
	    return 0;
	}

	munmap(src_s->base_addr,
	       (src_s->length + PAGE_SIZE) & ~(PAGE_SIZE - 1));
	SelectorMap[src_idx] = dst_idx;
	src_s->base_addr = dst_s->base_addr;
    }

    return dst_s->selector;
#endif /* HAVE_IPC */
}

/**********************************************************************
 *					AllocCStoDSAlias
 */
AllocDStoCSAlias(unsigned int ds_selector)
{
    unsigned int cs_selector;
    
    if (ds_selector == 0)
	return 0;
    
    cs_selector = AllocSelector(0);
    return PrestoChangoSelector(ds_selector, cs_selector);
}

/**********************************************************************
 *					FreeSelector
 */
unsigned int FreeSelector(unsigned int sel)
{
    SEGDESC *s;
    int sel_idx;
    int alias_count;
    int i;

#ifdef HAVE_IPC
    sel_idx = sel >> 3;

    if (sel_idx < FIRST_SELECTOR || sel_idx >= MAX_SELECTORS)
	return 0;
    
    s = &Segments[sel_idx];
    if (s->shm_key == 0)
    {
	munmap(s->base_addr, ((s->length + PAGE_SIZE) & ~(PAGE_SIZE - 1)));
	memcpy(s, 0, sizeof(*s));
	SelectorMap[sel_idx] = 0;
    }
    else
    {
	shmdt(s->base_addr);

	alias_count = 0;
	for (i = FIRST_SELECTOR; i < MAX_SELECTORS; i++)
	    if (SelectorMap[i] && Segments[i].shm_key == s->shm_key)
		alias_count++;
	
	if (alias_count == 1)
	    shmctl(s->shm_key, IPC_RMID, NULL);
	    
	memcpy(s, 0, sizeof(*s));
	SelectorMap[sel_idx] = 0;
    }
    
#else /* HAVE_IPC */
    sel_idx = SelectorMap[sel >> 3];

    if (sel_idx < FIRST_SELECTOR || sel_idx >= MAX_SELECTORS)
	return 0;
    
    if (sel_idx != (sel >> 3))
    {
	SelectorMap[sel >> 3] = 0;
	return 0;
    }
    
    alias_count = 0;
    for (i = FIRST_SELECTOR; i < MAX_SELECTORS; i++)
	if (SelectorMap[i] == sel_idx)
	    alias_count++;

    if (alias_count == 1)
    {
	s = &Segments[sel_idx];
	munmap(s->base_addr, ((s->length + PAGE_SIZE) & ~(PAGE_SIZE - 1)));
	memcpy(s, 0, sizeof(*s));
	SelectorMap[sel >> 3] = 0;
    }
#endif /* HAVE_IPC */

    return 0;
}

/**********************************************************************
 *					CreateNewSegments
 */
SEGDESC *
CreateNewSegments(int code_flag, int read_only, int length, int n_segments)
{
    SEGDESC *s, *first_segment;
    int contents;
    int i, last_i;
    
    i = FindUnusedSelectors(n_segments);

#ifdef DEBUG_SELECTORS    
    fprintf(stderr, 
	    "Using %d segments starting at index %d.\n", n_segments, i);
#endif

    /*
     * Fill in selector info.
     */
    first_segment = s = &Segments[i];
    for (last_i = i + n_segments; i < last_i; i++, s++)
    {
	if (code_flag)
	{
	    contents = MODIFY_LDT_CONTENTS_CODE;
	    s->flags = 0;
	}
	else
	{
	    contents = MODIFY_LDT_CONTENTS_DATA;
	    s->flags = NE_SEGFLAGS_DATA;
	}
	
	s->selector = (i << 3) | 0x0007;
	s->length = length;
#ifdef DEV_ZERO
	if (zfile == NULL)
	    zfile = fopen("/dev/zero","r");
	s->base_addr = (void *) mmap((char *) (s->selector << 16),
				     ((s->length + PAGE_SIZE - 1) & 
				      ~(PAGE_SIZE - 1)),
				     PROT_EXEC | PROT_READ | PROT_WRITE,
				     MAP_FIXED | MAP_PRIVATE, 
				     fileno(zfile), 0);
#else
	s->base_addr = (void *) mmap((char *) (s->selector << 16),
				     ((s->length + PAGE_SIZE - 1) & 
				      ~(PAGE_SIZE - 1)),
				     PROT_EXEC | PROT_READ | PROT_WRITE,
				     MAP_FIXED | MAP_PRIVATE | MAP_ANON, 
				     -1, 0);
#endif

	if (set_ldt_entry(i, (unsigned long) s->base_addr, 
			  (s->length - 1) & 0xffff, 0, 
			  contents, read_only, 0) < 0)
	{
	    memset(s, 0, sizeof(*s));
	    return NULL;
	}

	SelectorMap[i] = (unsigned short) i;
	s->type = contents;
    }

    return first_segment;
}

/**********************************************************************
 *					GetNextSegment
 */
SEGDESC *
GetNextSegment(unsigned int flags, unsigned int limit)
{
    return CreateNewSegments(0, 0, limit, 1);
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
#ifdef WINESTAT
		dll_table[ordinal].used++;
#endif
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
#ifdef WINESTAT
		dll_table[ordinal].used++;
#endif
	    return 0;
	};

	/* Not a builtin symbol, look to see what the file has for us */
	for(wpnt = wine_files; wpnt; wpnt = wpnt->next){
		if(strcmp(wpnt->name, dll_name)) continue;
		j = GetEntryPointFromOrdinal(wpnt, ordinal);
		*addr  = j & 0xffff;
		j = j >> 16;
#if 0
		/* This seems like it would never work */
		*sel = wpnt->selector_table[j].selector;
#else
		*sel = j;  /* Is there any reason this will ever fail?? */
#endif
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
static SEGDESC *
CreateEnvironment(void)
{
    char *p;
    SEGDESC * s;

    s = CreateNewSegments(0, 0, PAGE_SIZE, 1);
    if (s == NULL)
	return NULL;

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

    return  s;
}

/**********************************************************************
 *					CreatePSP
 */
static SEGDESC *
CreatePSP(void)
{
    struct dos_psp_s *psp;
    unsigned short *usp;
    SEGDESC * s;
    char *p1, *p2;
    int i;

    s = CreateNewSegments(0, 0, PAGE_SIZE, 1);

    /*
     * Fill PSP
     */
    PSPSelector = s->selector;
    psp = (struct dos_psp_s *) s->base_addr;
    psp->pspInt20 = 0x20cd;
    psp->pspDispatcher[0] = 0x9a;
    usp = (unsigned short *) &psp->pspDispatcher[1];
    *usp       = (unsigned short) KERNEL_Ordinal_102;
    *(usp + 1) = UTEXTSEL;
    psp->pspTerminateVector[0] = (unsigned short) UNIXLIB_Ordinal_0;
    psp->pspTerminateVector[1] = UTEXTSEL;
    psp->pspControlCVector[0] = (unsigned short) UNIXLIB_Ordinal_0;
    psp->pspControlCVector[1] = UTEXTSEL;
    psp->pspCritErrorVector[0] = (unsigned short) UNIXLIB_Ordinal_0;
    psp->pspCritErrorVector[1] = UTEXTSEL;
    psp->pspEnvironment = EnvironmentSelector->selector;

    p1 = psp->pspCommandTail;
    for (i = 1; i < Argc; i++)
    {
	if ((int) ((int) p1 - (int) psp->pspCommandTail) + 
	    strlen(Argv[i]) > 124)
	    break;
	
	for (p2 = Argv[i]; *p2 != '\0'; )
	    *p1++ = *p2++;
	
	*p1++ = ' ';
    }
    *p1++ = '\r';
    *p1 = '\0';
    psp->pspCommandTailCount = strlen(psp->pspCommandTail);

    return s;
}

/**********************************************************************
 *					CreateSelectors
 */
SEGDESC *
CreateSelectors(struct  w_files * wpnt)
{
    int fd = wpnt->fd;
    struct ne_segment_table_entry_s *seg_table = wpnt->seg_table;
    struct ne_header_s *ne_header = wpnt->ne_header;
    SEGDESC *selectors, *s, *stmp;
    unsigned short auto_data_sel;
    int contents, read_only;
    int SelectorTableLength;
    int i;
    int status;
    int old_length, file_image_length;
    int saved_old_length;

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
    for (i = 0; i < ne_header->n_segment_tab; i++, s++)
    {
	/*
	 * Store the flags in our table.
	 */
	s->flags = seg_table[i].seg_flags;

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
	    s->length         = seg_table[i].min_alloc;
	    file_image_length = seg_table[i].seg_data_length;
	    if (file_image_length == 0)	file_image_length = 0x10000;
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

	stmp = CreateNewSegments(!(s->flags & NE_SEGFLAGS_DATA), read_only,
				s->length, 1);
	s->base_addr = stmp->base_addr;
	s->selector = stmp->selector;
	
	if (seg_table[i].seg_data_offset != 0)
	{
	    /*
	     * Image in file.
	     */
	    status = lseek(fd, seg_table[i].seg_data_offset * 
			   (1 << ne_header->align_shift_count), SEEK_SET);
	    if(read(fd, s->base_addr, file_image_length) != file_image_length)
		myerror("Unable to read segment from file");
	}

	/*
	 * If this is the automatic data segment, then we must initialize
	 * the local heap.
	 */
	if (i + 1 == ne_header->auto_data_seg)
	{
	    auto_data_sel = s->selector;
	    saved_old_length = old_length;
	}
    }

    s = selectors;
    for (i = 0; i < ne_header->n_segment_tab; i++, s++)
    {
	Segments[s->selector >> 3].owner = auto_data_sel;
	if (s->selector == auto_data_sel)
	    HEAP_LocalInit(auto_data_sel, s->base_addr + saved_old_length, 
			   ne_header->local_heap_length);
    }

    if(!EnvironmentSelector) {
	    EnvironmentSelector = CreateEnvironment();
	    PSP_Selector = CreatePSP();
	    MakeProcThunks = CreateNewSegments(1, 0, 0x10000, 1);
    };

    return selectors;
}
