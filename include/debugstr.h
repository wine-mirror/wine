#ifndef __WINE_DEBUGSTR_H
#define __WINE_DEBUGSTR_H

#include "wintypes.h"

/* These function return a printable version of a string, including
   quotes.  The string will be valid for some time, but not indefinitely
   as strings are re-used.  */

LPSTR debugstr_an (LPCSTR s, int n);
LPSTR debugstr_a (LPCSTR s);
LPSTR debugstr_wn (LPCWSTR s, int n);
LPSTR debugstr_w (LPCWSTR s);
LPSTR debugres (const void *res);

#endif /* __WINE_DEBUGSTR_H */


