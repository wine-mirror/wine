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

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

#ifndef MSVCRT_NLSCMP_DEFINED
#define _NLSCMPERROR               ((unsigned int)0x7fffffff)
#define MSVCRT_NLSCMP_DEFINED
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
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

#ifndef MSVCRT_WSTRING_DEFINED
#define MSVCRT_WSTRING_DEFINED
MSVCRT(wchar_t)*_wcsdup(const MSVCRT(wchar_t)*);
int             _wcsicmp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
int             _wcsicoll(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wcslwr(MSVCRT(wchar_t)*);
int             _wcsnicmp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*_wcsnset(MSVCRT(wchar_t)*,MSVCRT(wchar_t),MSVCRT(size_t));
MSVCRT(wchar_t)*_wcsrev(MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wcsset(MSVCRT(wchar_t)*,MSVCRT(wchar_t));
MSVCRT(wchar_t)*_wcsupr(MSVCRT(wchar_t)*);

MSVCRT(wchar_t)*MSVCRT(wcscat)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcschr)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t));
int             MSVCRT(wcscmp)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
int             MSVCRT(wcscoll)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcscpy)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(size_t)  MSVCRT(wcscspn)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(size_t)  MSVCRT(wcslen)(const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcsncat)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
int             MSVCRT(wcsncmp)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*MSVCRT(wcsncpy)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*MSVCRT(wcspbrk)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcsrchr)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t) wcFor);
MSVCRT(size_t)  MSVCRT(wcsspn)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcsstr)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcstok)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(size_t)  MSVCRT(wcsxfrm)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
#endif /* MSVCRT_WSTRING_DEFINED */

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
