#ifndef __WINE_DEBUGSTR_H
#define __WINE_DEBUGSTR_H

#include "windef.h"

/* These function return a printable version of a string, including
   quotes.  The string will be valid for some time, but not indefinitely
   as strings are re-used.  */
struct _GUID;

extern LPSTR debugstr_an (LPCSTR s, int n);
extern LPSTR debugstr_a (LPCSTR s);
extern LPSTR debugstr_wn (LPCWSTR s, int n);
extern LPSTR debugstr_w (LPCWSTR s);
extern LPSTR debugres_a (LPCSTR res);
extern LPSTR debugres_w (LPCWSTR res);
extern LPSTR debugstr_guid( const struct _GUID *id );
extern void debug_dumpstr (LPCSTR s);
extern LPSTR debugstr_hex_dump (const void *ptr, int len);

#ifdef __GNUC__
extern int dbg_printf(const char *format, ...) __attribute__((format (printf,1,2)));
#else
extern int dbg_printf(const char *format, ...);
#endif


#endif /* __WINE_DEBUGSTR_H */
