/*
 * String definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STRING_H
#define __WINE_STRING_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif

#if defined(__x86_64__) && !defined(_WIN64)
#define _WIN64
#endif

#if !defined(_MSC_VER) && !defined(__int64)
# ifdef _WIN64
#   define __int64 long
# else
#   define __int64 long long
# endif
#endif

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _NLSCMP_DEFINED
#define _NLSCMPERROR               ((unsigned int)0x7fffffff)
#define _NLSCMP_DEFINED
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

void*       _memccpy(void*,const void*,int,unsigned int);
int         _memicmp(const void*,const void*,unsigned int);
int         _strcmpi(const char*,const char*);
char*       _strdup(const char*);
char*       _strerror(const char*);
int         _stricmp(const char*,const char*);
int         _stricoll(const char*,const char*);
char*       _strlwr(char*);
int         _strnicmp(const char*,const char*,size_t);
char*       _strnset(char*,int,size_t);
char*       _strrev(char*);
char*       _strset(char*,int);
char*       _strupr(char*);

void*       memchr(const void*,int,size_t);
int         memcmp(const void*,const void*,size_t);
void*       memcpy(void*,const void*,size_t);
void*       memmove(void*,const void*,size_t);
void*       memset(void*,int,size_t);
char*       strcat(char*,const char*);
char*       strchr(const char*,int);
int         strcmp(const char*,const char*);
int         strcoll(const char*,const char*);
char*       strcpy(char*,const char*);
size_t strcspn(const char*,const char*);
char*       strerror(int);
size_t strlen(const char*);
char*       strncat(char*,const char*,size_t);
int         strncmp(const char*,const char*,size_t);
char*       strncpy(char*,const char*,size_t);
char*       strpbrk(const char*,const char*);
char*       strrchr(const char*,int);
size_t strspn(const char*,const char*);
char*       strstr(const char*,const char*);
char*       strtok(char*,const char*);
size_t strxfrm(char*,const char*,size_t);

#ifndef _WSTRING_DEFINED
#define _WSTRING_DEFINED
wchar_t*_wcsdup(const wchar_t*);
int             _wcsicmp(const wchar_t*,const wchar_t*);
int             _wcsicoll(const wchar_t*,const wchar_t*);
wchar_t*_wcslwr(wchar_t*);
int             _wcsnicmp(const wchar_t*,const wchar_t*,size_t);
wchar_t*_wcsnset(wchar_t*,wchar_t,size_t);
wchar_t*_wcsrev(wchar_t*);
wchar_t*_wcsset(wchar_t*,wchar_t);
wchar_t*_wcsupr(wchar_t*);

wchar_t*wcscat(wchar_t*,const wchar_t*);
wchar_t*wcschr(const wchar_t*,wchar_t);
int             wcscmp(const wchar_t*,const wchar_t*);
int             wcscoll(const wchar_t*,const wchar_t*);
wchar_t*wcscpy(wchar_t*,const wchar_t*);
size_t  wcscspn(const wchar_t*,const wchar_t*);
size_t  wcslen(const wchar_t*);
wchar_t*wcsncat(wchar_t*,const wchar_t*,size_t);
int             wcsncmp(const wchar_t*,const wchar_t*,size_t);
wchar_t*wcsncpy(wchar_t*,const wchar_t*,size_t);
wchar_t*wcspbrk(const wchar_t*,const wchar_t*);
wchar_t*wcsrchr(const wchar_t*,wchar_t wcFor);
size_t  wcsspn(const wchar_t*,const wchar_t*);
wchar_t*wcsstr(const wchar_t*,const wchar_t*);
wchar_t*wcstok(wchar_t*,const wchar_t*);
size_t  wcsxfrm(wchar_t*,const wchar_t*,size_t);
#endif /* _WSTRING_DEFINED */

#ifdef __cplusplus
}
#endif


static inline void* memccpy(void *s1, const void *s2, int c, size_t n) { return _memccpy(s1, s2, c, n); }
static inline int memicmp(const void* s1, const void* s2, size_t len) { return _memicmp(s1, s2, len); }
static inline int strcasecmp(const char* s1, const char* s2) { return _stricmp(s1, s2); }
static inline int strcmpi(const char* s1, const char* s2) { return _strcmpi(s1, s2); }
static inline char* strdup(const char* buf) { return _strdup(buf); }
static inline int stricmp(const char* s1, const char* s2) { return _stricmp(s1, s2); }
static inline int stricoll(const char* s1, const char* s2) { return _stricoll(s1, s2); }
static inline char* strlwr(char* str) { return _strlwr(str); }
static inline int strncasecmp(const char *str1, const char *str2, size_t n) { return _strnicmp(str1, str2, n); }
static inline int strnicmp(const char* s1, const char* s2, size_t n) { return _strnicmp(s1, s2, n); }
static inline char* strnset(char* str, int value, unsigned int len) { return _strnset(str, value, len); }
static inline char* strrev(char* str) { return _strrev(str); }
static inline char* strset(char* str, int value) { return _strset(str, value); }
static inline char* strupr(char* str) { return _strupr(str); }

static inline wchar_t* wcsdup(const wchar_t* str) { return _wcsdup(str); }
static inline int wcsicoll(const wchar_t* str1, const wchar_t* str2) { return _wcsicoll(str1, str2); }
static inline wchar_t* wcslwr(wchar_t* str) { return _wcslwr(str); }
static inline int wcsnicmp(const wchar_t* str1, const wchar_t* str2, size_t n) { return _wcsnicmp(str1, str2, n); }
static inline wchar_t* wcsnset(wchar_t* str, wchar_t c, size_t n) { return _wcsnset(str, c, n); }
static inline wchar_t* wcsrev(wchar_t* str) { return _wcsrev(str); }
static inline wchar_t* wcsset(wchar_t* str, wchar_t c) { return _wcsset(str, c); }
static inline wchar_t* wcsupr(wchar_t* str) { return _wcsupr(str); }

#endif /* __WINE_STRING_H */
