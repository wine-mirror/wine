#ifndef __WINE_NTDLL_MISC_H
#define __WINE_NTDLL_MISC_H

#include "ntdef.h"
#include "winnt.h"

/* debug helper */
extern LPCSTR debugstr_as( const STRING *str );
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
extern void dump_ObjectAttributes (POBJECT_ATTRIBUTES ObjectAttributes);
extern void dump_AnsiString(PANSI_STRING as, BOOLEAN showstring);
extern void dump_UnicodeString(PUNICODE_STRING us, BOOLEAN showstring);

#endif
