#ifndef __WINE_NTDLL_MISC_H
#define __WINE_NTDLL_MISC_H

#include "ntdef.h"
#include "winnt.h"

/* debug helper */
extern LPCSTR debugstr_as (PANSI_STRING us);
extern LPCSTR debugstr_us (PUNICODE_STRING us);
extern void dump_ObjectAttributes (POBJECT_ATTRIBUTES ObjectAttributes);
extern void dump_AnsiString(PANSI_STRING as, BOOLEAN showstring);
extern void dump_UnicodeString(PUNICODE_STRING us, BOOLEAN showstring);

#endif
