/* $Id: prototypes.h,v 1.3 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef PROTOTYPES_H
#define PROTOTYPES_H

#include <sys/types.h>
#include "neexe.h"
#include "segmem.h"
#include "wine.h"

extern struct segment_descriptor_s *
    CreateSelectors(struct w_files *);

extern void PrintFileHeader(struct ne_header_s *ne_header);
extern void PrintSegmentTable(struct ne_segment_table_entry_s *seg_table, 
			      int nentries);
extern void PrintRelocationTable(char *exe_ptr, 
				 struct ne_segment_table_entry_s *seg_entry_p,
				 int segment);
extern int FixupSegment(struct w_files * wpnt, int segment_num);
extern struct  dll_table_entry_s *FindDLLTable(char *dll_name);
extern unsigned int GetEntryPointFromOrdinal(struct w_files * wpnt, 
					     int ordinal);

extern struct segment_descriptor_s *GetNextSegment(unsigned int flags,
						   unsigned int limit);
extern struct mz_header_s *CurrentMZHeader;
extern struct ne_header_s *CurrentNEHeader;
extern int CurrentNEFile;
extern do_int1A(struct sigcontext_struct * context);
extern do_int21(struct sigcontext_struct * context);

#endif /* PROTOTYPES_H */

