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
extern char *WineIniFileName(void);
extern char *WinIniFileName(void);

#define MAX_DOS_DRIVES	26

#define WINE_INI WineIniFileName()
#define WIN_INI WinIniFileName()

#ifdef linux
struct sigcontext_struct {
	unsigned short sc_gs, __gsh;
	unsigned short sc_fs, __fsh;
	unsigned short sc_es, __esh;
	unsigned short sc_ds, __dsh;
	unsigned long sc_edi;
	unsigned long sc_esi;
	unsigned long sc_ebp;
	unsigned long sc_esp;
	unsigned long sc_ebx;
	unsigned long sc_edx;
	unsigned long sc_ecx;
	unsigned long sc_eax;
	unsigned long sc_trapno;
	unsigned long sc_err;
	unsigned long sc_eip;
	unsigned short sc_cs, __csh;
	unsigned long sc_efl;
	unsigned long esp_at_signal;
	unsigned short sc_ss, __ssh;
	unsigned long i387;
	unsigned long oldmask;
	unsigned long cr2;
};
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <signal.h>
#define sigcontext_struct sigcontext
#define HZ 100
#endif

void load_mz_header (int, struct mz_header_s *);
void load_ne_header (int, struct ne_header_s *);
int  load_typeinfo  (int, struct resource_typeinfo_s *);
int  load_nameinfo  (int, struct resource_nameinfo_s *);

#endif /* WINE_H */
