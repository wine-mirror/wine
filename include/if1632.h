#ifndef __WINE_IF1632_H
#define __WINE_IF1632_H

#include <wintypes.h>

extern int CallToInit16(unsigned long csip, unsigned long sssp, 
			unsigned short ds);
extern int CallTo16cx(unsigned long csip, unsigned long dscx);
extern int CallToDllEntry(unsigned long csip, unsigned long dscx, unsigned short di);
extern void winestat(void);
extern struct dll_table_s *FindDLLTable(char *dll_name);
extern int FindOrdinalFromName(struct dll_table_entry_s *dll_table, char *func_name);
extern int ReturnArg(int arg);

extern BOOL RELAY_Init(void);

#endif /* __WINE_IF1632_H */
