/* $Id: prototypes.h,v 1.1 1993/06/29 15:55:18 root Exp $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef PROTOTYPES_H
#define PROTOTYPES_H

#include <sys/types.h>
#include "neexe.h"
#include "segmem.h"

extern struct segment_descriptor_s *
    CreateSelectors(int fd, struct ne_segment_table_entry_s *seg_table,
  		    struct ne_header_s *ne_header);

extern void PrintFileHeader(struct ne_header_s *ne_header);
extern void PrintSegmentTable(struct ne_segment_table_entry_s *seg_table, 
			      int nentries);
extern void PrintRelocationTable(char *exe_ptr, 
				 struct ne_segment_table_entry_s *seg_entry_p,
				 int segment);
extern int FixupSegment(int fd, struct mz_header_s * mz_header,
			struct ne_header_s *ne_header,
			struct ne_segment_table_entry_s *seg_table, 
			struct segment_descriptor_s *selecetor_table,
			int segment_num);
extern struct  dll_table_entry_s *FindDLLTable(char *dll_name);
extern unsigned int GetEntryPointFromOrdinal(int fd, 
					     struct mz_header_s *mz_header, 
					     struct ne_header_s *ne_header, 
					     int ordinal);

extern char WIN_CommandLine[];
extern struct mz_header_s *CurrentMZHeader;
extern struct ne_header_s *CurrentNEHeader;
extern int CurrentNEFile;

#endif /* PROTOTYPES_H */
