#ifndef __WINE_IF1632_H
#define __WINE_IF1632_H

#include <wintypes.h>

extern int CallToInit16(unsigned long csip, unsigned long sssp, 
			unsigned short ds);
extern int CallTo16cx(unsigned long csip, unsigned long dscx);
extern int CallToDllEntry(unsigned long csip, unsigned long dscx, unsigned short di);
extern int CallBack16(void *func, int n_args, ...);
extern void *CALLBACK_MakeProcInstance(void *func, int instance);
extern void CallLineDDAProc(FARPROC func, short xPos, short yPos, long lParam);
extern void winestat(void);
extern int DLLRelay(unsigned int func_num, unsigned int seg_off);
extern struct dll_table_entry_s *FindDLLTable(char *dll_name);
extern int FindOrdinalFromName(struct dll_table_entry_s *dll_table, char *func_name);
extern int ReturnArg(int arg);

#endif /* __WINE_IF1632_H */
