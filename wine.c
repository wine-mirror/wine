/* $Id: exedump.c,v 1.1 1993/06/09 03:28:10 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/ldt.h>
#include <linux/segment.h>
#include <errno.h>
#include "neexe.h"
#include "segmem.h"
#include "prototypes.h"
#include "dlls.h"

extern int CallTo16(unsigned long csip, unsigned long sssp, unsigned short ds);
extern void CallTo32();

unsigned short WIN_StackSize;

char **Argv;
int Argc;

/**********************************************************************
 *					DebugPrintString
 */
int
DebugPrintString(char *str)
{
    fprintf(stderr, "%s", str);
    return 0;
}

/**********************************************************************
 *					myerror
 */
void
myerror(const char *s)
{
    char buffer[200];
    
    sprintf(buffer, "%s", Argv[0]);
    if (s == NULL)
	perror(buffer);
    else
	fprintf(stderr, "%s: %s\n", buffer, s);

    exit(1);
}

/**********************************************************************
 *					main
 */
main(int argc, char **argv)
{
    struct stat finfo;
    struct mz_header_s *mz_header;
    struct ne_header_s *ne_header;
    struct ne_segment_table_entry_s *seg_table;
    unsigned int status;
    unsigned int read_size;
    struct segment_descriptor_s *selector_table;
    int fd;
    int segment;
    int cs_reg, ds_reg, ss_reg, ip_reg, sp_reg;
    int rv;

    Argc = argc;
    Argv = argv;
    
    if (argc < 2)
    {
	fprintf(stderr, "usage: %s FILENAME\n", argv[0]);
	exit(1);
    }
    
    /*
     * Open file for reading.
     */
    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
	myerror(NULL);
    }

    /*
     * Allocate memory to hold entire executable.
     */
    if (fstat(fd, &finfo) < 0)
	myerror(NULL);
    
    /*
     * Establish header pointers.
     */
    mz_header = (struct mz_header_s *) malloc(sizeof(struct mz_header_s));;
    status = lseek(fd, 0, SEEK_SET);
    if (read(fd, mz_header, sizeof(struct mz_header_s)) !=
	sizeof(struct mz_header_s))
    {
	myerror("Unable to read MZ header from file");
    }
    if (mz_header->must_be_0x40 != 0x40)
	myerror("This is not a Windows program");
    
    ne_header = (struct ne_header_s *) malloc(sizeof(struct ne_header_s));
    status = lseek(fd, mz_header->ne_offset, SEEK_SET);
    if (read(fd, ne_header, sizeof(struct ne_header_s)) 
	!= sizeof(struct ne_header_s))
    {
	myerror("Unable to read NE header from file");
    }
    if (ne_header->header_type[0] != 'N' || ne_header->header_type[1] != 'E')
	myerror("This is not a Windows program");

    WIN_StackSize = ne_header->stack_length;
    

    /*
     * Create segment selectors.
     */
    status = lseek(fd, mz_header->ne_offset + ne_header->segment_tab_offset,
		   SEEK_SET);
    read_size  = ne_header->n_segment_tab *
	         sizeof(struct ne_segment_table_entry_s);
    seg_table = (struct ne_segment_table_entry_s *) malloc(read_size);
    if (read(fd, seg_table, read_size) != read_size)
	myerror("Unable to read segment table header from file");
    selector_table = CreateSelectors(fd, seg_table, ne_header);
    
    /*
     * Fixup references.
     */
    for (segment = 0; segment < ne_header->n_segment_tab; segment++)
    {
	if (FixupSegment(fd, mz_header, ne_header, seg_table,
			 selector_table, segment) < 0)
	{
	    myerror("fixup failed.");
	}
    }

    close(fd);
    /*
     * Fixup stack and jump to start.
     */
    ds_reg = selector_table[ne_header->auto_data_seg-1].selector;
    cs_reg = selector_table[ne_header->cs-1].selector;
    ip_reg = ne_header->ip;
    ss_reg = selector_table[ne_header->ss-1].selector;
    sp_reg = ne_header->sp;

    rv = CallTo16(cs_reg << 16 | ip_reg, ss_reg << 16 | sp_reg, ds_reg);
    printf ("rv = %x\n", rv);
}


/**********************************************************************
 *					GetImportedName
 */
char *
GetImportedName(int fd, struct mz_header_s *mz_header, 
		struct ne_header_s *ne_header, int name_offset, char *buffer)
{
    char *p;
    int length;
    int status;
    int i;
    
    status = lseek(fd, mz_header->ne_offset + ne_header->iname_tab_offset +
		   name_offset, SEEK_SET);
    length = 0;
    read(fd, &length, 1);  /* Get the length byte */
    read(fd, buffer, length);
    buffer[length] = 0;
    return buffer;
}

/**********************************************************************
 *					GetModuleName
 */
char *
GetModuleName(int fd, struct mz_header_s *mz_header, 
	      struct ne_header_s *ne_header, int index, char *buffer)
{
    char *p;
    int length;
    int name_offset, status;
    int i;
    
    status = lseek(fd, mz_header->ne_offset + ne_header->moduleref_tab_offset +
		   2*(index - 1), SEEK_SET);
    name_offset = 0;
    read(fd, &name_offset, 2);
    status = lseek(fd, mz_header->ne_offset + ne_header->iname_tab_offset +
		   name_offset, SEEK_SET);
    length = 0;
    read(fd, &length, 1);  /* Get the length byte */
    read(fd, buffer, length);
    buffer[length] = 0;
    return buffer;
}


/**********************************************************************
 *					FixupSegment
 */
int
FixupSegment(int fd, struct mz_header_s * mz_header,
	     struct ne_header_s *ne_header,
	     struct ne_segment_table_entry_s *seg_table, 
	     struct segment_descriptor_s *selector_table,
	     int segment_num)
{
    struct relocation_entry_s *rep, *rep1;
    struct ne_segment_table_entry_s *seg;
    struct segment_descriptor_s *sel;
    struct dll_table_entry_s *dll_table;
    unsigned short *sp;
    unsigned int selector, address;
    unsigned int next_addr;
    int ordinal;
    int status;
    char dll_name[257];
    char func_name[257];
    int i, n_entries;

    seg = &seg_table[segment_num];
    sel = &selector_table[segment_num];

    if (seg->seg_data_offset == 0)
	return 0;

    /*
     * Go through the relocation table on entry at a time.
     */
    i = seg->seg_data_length;
    if (i == 0)
	i = 0x10000;

    status = lseek(fd, seg->seg_data_offset * 512 + i, SEEK_SET);
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
	switch (rep->relocation_type)
	{
	  case NE_RELTYPE_ORDINAL:
	    if (GetModuleName(fd, mz_header, ne_header, rep->target1,
			      dll_name) == NULL)
	    {
		return -1;
	    }
	    
 	    dll_table = FindDLLTable(dll_name);
	    ordinal = rep->target2;
	    selector = dll_table[ordinal].selector;
	    address  = (unsigned int) dll_table[ordinal].address;
#ifdef DEBUG_FIXUP
	    printf("%d: %s.%d: %04.4x:%04.4x\n", i + 1, dll_name, ordinal,
		   selector, address);
#endif
	    break;
	    
	  case NE_RELTYPE_NAME:
	    if (GetModuleName(fd, mz_header, ne_header, rep->target1, dll_name)
		== NULL)
	    {
		return -1;
	    }
 	    dll_table = FindDLLTable(dll_name);

	    if (GetImportedName(fd, mz_header, ne_header, 
				rep->target2, func_name) == NULL)
	    {
		return -1;
	    }
	    ordinal = FindOrdinalFromName(dll_table, func_name);
	    selector = dll_table[ordinal].selector;
	    address  = (unsigned int) dll_table[ordinal].address;
#ifdef DEBUG_FIXUP
	    printf("%d: %s %s.%d: %04.4x:%04.4x\n", i + 1, func_name,
		   dll_name, ordinal, selector, address);
#endif
	    break;
	    
	  case NE_RELTYPE_INTERNAL:
	  default:
	    free(rep1);
	    return -1;
	}

	/*
	 * Stuff the right size result in.
	 */
	sp = (unsigned short *) ((char *) sel->base_addr + rep->offset);
	switch (rep->address_type)
	{
	  case NE_RADDR_OFFSET16:
	    do {
		next_addr = *sp;
		*sp = (unsigned short) address;
		sp = (unsigned short *) ((char *) sel->base_addr + next_addr);
	    } 
	    while (next_addr != 0xffff);

	    break;
	    
	  case NE_RADDR_POINTER32:
	    do {
		next_addr = *sp;
		*sp     = (unsigned short) address;
		*(sp+1) = (unsigned short) selector;
		sp = (unsigned short *) ((char *) sel->base_addr + next_addr);
	    } 
	    while (next_addr != 0xffff);

	    break;
	    
	  default:
	    free(rep1);
	    return -1;
	}
    }

    free(rep1);
    return 0;
}
