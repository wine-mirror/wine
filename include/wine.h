#ifndef  WINE_H
#define  WINE_H

#include "dlls.h"

struct w_files{
  struct w_files  * next;
  char * name;   /* Name, as it appears in the windows binaries */
  char * filename;  /* Actual name of the unix file that satisfies this */
  int fd;
  struct mz_header_s *mz_header;
  struct ne_header_s *ne_header;
  struct ne_segment_table_entry_s *seg_table;
  struct segment_descriptor_s *selector_table;
  char * lookup_table;
  char * nrname_table;
  char * rname_table;
  unsigned short hinstance;
};

extern struct  w_files * wine_files;

extern char *GetFilenameFromInstance(unsigned short instance);
extern struct w_files *GetFileInfo(unsigned short instance);

#endif
