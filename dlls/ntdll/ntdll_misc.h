#ifndef __WINE_NTDLL_MISC_H
#define __WINE_NTDLL_MISC_H

#include "ntdef.h"
#include "winnt.h"

/* debug helper */
extern LPCSTR debugstr_as( const STRING *str );
extern LPCSTR debugstr_us( const UNICODE_STRING *str );
extern void dump_ObjectAttributes (const OBJECT_ATTRIBUTES *ObjectAttributes);
extern void dump_UnicodeString(const UNICODE_STRING *us, BOOLEAN showstring);

#endif
