#ifndef __WINE_SELECTORS_H
#define __WINE_SELECTORS_H

#include "dlls.h"
#include "segmem.h"
#include "windows.h"

extern int FindUnusedSelectors(int n_selectors);
extern int IPCCopySelector(int i_old, unsigned long new, int swap_type);
extern WORD AllocSelector(WORD old_selector);
extern unsigned int PrestoChangoSelector(unsigned src_selector, unsigned dst_selector);
extern WORD AllocDStoCSAlias(WORD ds_selector);
extern SEGDESC *CreateSelectors(struct  w_files * wpnt);
extern WORD FreeSelector(WORD sel);

extern SEGDESC *CreateNewSegments(int code_flag, int read_only, int length, 
					int n_segments);
extern SEGDESC *GetNextSegment(unsigned int flags, unsigned int limit);

extern unsigned int GetEntryDLLName(char *dll_name, char *function, int *sel,
					int *addr);
extern unsigned int GetEntryDLLOrdinal(char *dll_name, int ordinal, int *sel,
					int *addr);
extern unsigned int GetEntryPointFromOrdinal(struct w_files * wpnt, 
					int ordinal);
extern void InitSelectors(void);

#endif /* __WINE_SELECTORS_H */
