/*
 * String definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STRING_H
#define __WINE_STRING_H
#define __WINE_USE_MSVCRT

#include "winnt.h"

#ifdef USE_MSVCRT_PREFIX
#define MSVCRT(x)    MSVCRT_##x
#else
#define MSVCRT(x)    x
#endif


#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif


#ifdef __cplusplus
extern "C" {
#endif

void*       _memccpy(void*,const void*,int,MSVCRT(size_t));
int         _memicmp(const void*,const void*,MSVCRT(size_t));
int         _strcmpi(const char*,const char*);
char*       _strdup(const char*);
char*       _strerror(const char*);
int         _stricmp(const char*,const char*);
int         _stricoll(const char*,const char*);
char*       _strlwr(char*);
int         _strnicmp(const char*,const char*,MSVCRT(size_t));
char*       _strnset(char*,int,MSVCRT(size_t));
char*       _strrev(char*);
char*       _strset(char*,int);
char*       _strupr(char*);

void*       MSVCRT(memchr)(const void*,int,MSVCRT(size_t));
int         MSVCRT(memcmp)(const void*,const void*,MSVCRT(size_t));
void*       MSVCRT(memcpy)(void*,const void*,MSVCRT(size_t));
void*       MSVCRT(memmove)(void*,const void*,MSVCRT(size_t));
void*       MSVCRT(memset)(void*,int,MSVCRT(size_t));
char*       MSVCRT(strcat)(char*,const char*);
char*       MSVCRT(strchr)(const char*,int);
int         MSVCRT(strcmp)(const char*,const char*);
int         MSVCRT(strcoll)(const char*,const char*);
char*       MSVCRT(strcpy)(char*,const char*);
MSVCRT(size_t) MSVCRT(strcspn)(const char*,const char*);
char*       MSVCRT(strerror)(int);
MSVCRT(size_t) MSVCRT(strlen)(const char*);
char*       MSVCRT(strncat)(char*,const char*,MSVCRT(size_t));
int         MSVCRT(strncmp)(const char*,const char*,MSVCRT(size_t));
char*       MSVCRT(strncpy)(char*,const char*,MSVCRT(size_t));
char*       MSVCRT(strpbrk)(const char*,const char*);
char*       MSVCRT(strrchr)(const char*,int);
MSVCRT(size_t) MSVCRT(strspn)(const char*,const char*);
char*       MSVCRT(strstr)(const char*,const char*);
char*       MSVCRT(strtok)(char*,const char*);
MSVCRT(size_t) MSVCRT(strxfrm)(char*,const char*,MSVCRT(size_t));

WCHAR*      _wcsdup(const WCHAR*);
int         _wcsicmp(const WCHAR*,const WCHAR*);
int         _wcsicoll(const WCHAR*,const WCHAR*);
WCHAR*      _wcslwr(WCHAR*);
int         _wcsnicmp(const WCHAR*,const WCHAR*,MSVCRT(size_t));
WCHAR*      _wcsnset(WCHAR*,WCHAR,MSVCRT(size_t));
WCHAR*      _wcsrev(WCHAR*);
WCHAR*      _wcsset(WCHAR*,WCHAR);
WCHAR*      _wcsupr(WCHAR*);

WCHAR*      MSVCRT(wcscat)(WCHAR*,const WCHAR*);
WCHAR*      MSVCRT(wcschr)(const WCHAR*,WCHAR);
int         MSVCRT(wcscmp)(const WCHAR*,const WCHAR*);
int         MSVCRT(wcscoll)(const WCHAR*,const WCHAR*);
WCHAR*      MSVCRT(wcscpy)(WCHAR*,const WCHAR*);
MSVCRT(size_t) MSVCRT(wcscspn)(const WCHAR*,const WCHAR*);
MSVCRT(size_t) MSVCRT(wcslen)(const WCHAR*);
WCHAR*      MSVCRT(wcsncat)(WCHAR*,const WCHAR*,MSVCRT(size_t));
int         MSVCRT(wcsncmp)(const WCHAR*,const WCHAR*,MSVCRT(size_t));
WCHAR*      MSVCRT(wcsncpy)(WCHAR*,const WCHAR*,MSVCRT(size_t));
WCHAR*      MSVCRT(wcspbrk)(const WCHAR*,const WCHAR*);
WCHAR*      MSVCRT(wcsrchr)(const WCHAR*,WCHAR wcFor);
MSVCRT(size_t) MSVCRT(wcsspn)(const WCHAR*,const WCHAR*);
WCHAR*      MSVCRT(wcsstr)(const WCHAR*,const WCHAR*);
WCHAR*      MSVCRT(wcstok)(WCHAR*,const WCHAR*);
MSVCRT(size_t) MSVCRT(wcsxfrm)(WCHAR*,const WCHAR*,MSVCRT(size_t));

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define memccpy _memccpy
#define memicmp _memicmp
#define strcasecmp _strcasecmp
#define strcmpi _strcmpi
#define strdup _strdup
#define stricmp _stricmp
#define stricoll _stricoll
#define strlwr _strlwr
#define strncasecmp _strncasecmp
#define strnicmp _strnicmp
#define strnset _strnset
#define strrev _strrev
#define strset _strset
#define strupr _strupr

#define wcsdup _wcsdup
#define wcsicoll _wcsicoll
#define wcslwr _wcslwr
#define wcsnicmp _wcsnicmp
#define wcsnset _wcsnset
#define wcsrev _wcsrev
#define wcsset _wcsset
#define wcsupr _wcsupr
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_STRING_H */
