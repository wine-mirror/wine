#ifndef __WINE_ADVAPI32_H
#define __WINE_ADVAPI32_H
#include "shell.h"
#include "kernel32.h"
#define REGSAM long
BOOL WINAPI GetUserNameA (char * lpBuffer, DWORD  *nSize);
WINAPI LONG RegCreateKeyEx(HKEY key,
                            const char *subkey,
                            long dontuse,
                            const char *keyclass,
                            DWORD options,
                            REGSAM sam,
                            SECURITY_ATTRIBUTES *atts,
                            HKEY *res,
                            DWORD *disp);
WINAPI LONG RegSetValueExA (HKEY key,
                const char *name,
                DWORD dontuse,
                DWORD type,
                const void* data,
                DWORD len
                );
WINAPI LONG RegQueryValueExA(HKEY key,
                             const char *subkey,
                             DWORD dontuse,
                             DWORD *type,
                             void *ptr,
                             DWORD *len);


#endif  /* __WINE_ADVAPI32_H */
