/*
 * Standard I/O definitions.
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STDIO_H
#define __WINE_STDIO_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#include <pshpack8.h>

#ifndef RC_INVOKED
#include <stdarg.h>
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

/* file._flag flags */
#define _IOREAD          0x0001
#define _IOWRT           0x0002
#define _IOMYBUF         0x0008
#define _IOEOF           0x0010
#define _IOERR           0x0020
#define _IOSTRG          0x0040
#define _IORW            0x0080

#ifndef NULL
#ifdef  __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
#endif

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* more file._flag flags, but these conflict with Unix */
#define _IOFBF    0x0000
#define _IONBF    0x0004
#define _IOLBF    0x0040

#define EOF       (-1)
#define FILENAME_MAX 260
#define TMP_MAX   0x7fff
#define FOPEN_MAX 20
#define L_tmpnam  260

#define BUFSIZ    512

#ifndef SEEK_SET
#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2
#endif

#ifndef _FILE_DEFINED
#define _FILE_DEFINED
typedef struct _iobuf
{
  char* _ptr;
  int   _cnt;
  char* _base;
  int   _flag;
  int   _file;
  int   _charbuf;
  int   _bufsiz;
  char* _tmpfname;
} FILE;
#endif  /* _FILE_DEFINED */

#ifndef _FPOS_T_DEFINED
typedef __int64 fpos_t;
#define _FPOS_T_DEFINED
#endif

#ifndef _SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif

#ifndef _WCTYPE_T_DEFINED
typedef unsigned short  wint_t;
typedef unsigned short  wctype_t;
#define _WCTYPE_T_DEFINED
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _STDIO_DEFINED
FILE*        __p__iob(void);
#define _iob               (__p__iob())
#endif /* _STDIO_DEFINED */

#define stdin              (_iob+STDIN_FILENO)
#define stdout             (_iob+STDOUT_FILENO)
#define stderr             (_iob+STDERR_FILENO)

#ifndef _STDIO_DEFINED
#define _STDIO_DEFINED
int         _fcloseall(void);
FILE* _fdopen(int,const char*);
int         _fgetchar(void);
int         _filbuf(FILE*);
int         _fileno(FILE*);
int         _flsbuf(int,FILE*);
int         _flushall(void);
int         _fputchar(int);
FILE* _fsopen(const char*,const char*,int);
int         _getmaxstdio(void);
int         _getw(FILE*);
int         _pclose(FILE*);
FILE* _popen(const char*,const char*);
int         _putw(int,FILE*);
int         _rmtmp(void);
int         _setmaxstdio(int);
int         _snprintf(char*,size_t,const char*,...);
char*       _tempnam(const char*,const char*);
int         _unlink(const char*);
int         _vsnprintf(char*,size_t,const char*,va_list);

void        clearerr(FILE*);
int         fclose(FILE*);
int         feof(FILE*);
int         ferror(FILE*);
int         fflush(FILE*);
int         fgetc(FILE*);
int         fgetpos(FILE*,fpos_t*);
char*       fgets(char*,int,FILE*);
FILE* fopen(const char*,const char*);
int         fprintf(FILE*,const char*,...);
int         fputc(int,FILE*);
int         fputs(const char*,FILE*);
size_t fread(void*,size_t,size_t,FILE*);
FILE* freopen(const char*,const char*,FILE*);
int         fscanf(FILE*,const char*,...);
int         fseek(FILE*,long,int);
int         fsetpos(FILE*,fpos_t*);
long        ftell(FILE*);
size_t fwrite(const void*,size_t,size_t,FILE*);
int         getc(FILE*);
int         getchar(void);
char*       gets(char*);
void        perror(const char*);
int         printf(const char*,...);
int         putc(int,FILE*);
int         putchar(int);
int         puts(const char*);
int         remove(const char*);
int         rename(const char*,const char*);
void        rewind(FILE*);
int         scanf(const char*,...);
void        setbuf(FILE*,char*);
int         setvbuf(FILE*,char*,int,size_t);
int         sprintf(char*,const char*,...);
int         sscanf(const char*,const char*,...);
FILE* tmpfile(void);
char*       tmpnam(char*);
int         ungetc(int,FILE*);
int         vfprintf(FILE*,const char*,va_list);
int         vprintf(const char*,va_list);
int         vsprintf(char*,const char*,va_list);

#ifndef _WSTDIO_DEFINED
#define _WSTDIO_DEFINED
wint_t  _fgetwchar(void);
wint_t  _fputwchar(wint_t);
wchar_t*_getws(wchar_t*);
int             _putws(const wchar_t*);
int             _snwprintf(wchar_t*,size_t,const wchar_t*,...);
int             _vsnwprintf(wchar_t*,size_t,const wchar_t*,va_list);
FILE*   _wfdopen(int,const wchar_t*);
FILE*   _wfopen(const wchar_t*,const wchar_t*);
FILE*   _wfreopen(const wchar_t*,const wchar_t*,FILE*);
FILE*   _wfsopen(const wchar_t*,const wchar_t*,int);
void            _wperror(const wchar_t*);
FILE*   _wpopen(const wchar_t*,const wchar_t*);
int             _wremove(const wchar_t*);
wchar_t*_wtempnam(const wchar_t*,const wchar_t*);
wchar_t*_wtmpnam(wchar_t*);

wint_t  fgetwc(FILE*);
wchar_t*fgetws(wchar_t*,int,FILE*);
wint_t  fputwc(wint_t,FILE*);
int             fputws(const wchar_t*,FILE*);
int             fwprintf(FILE*,const wchar_t*,...);
int             fputws(const wchar_t*,FILE*);
int             fwscanf(FILE*,const wchar_t*,...);
wint_t  getwc(FILE*);
wint_t  getwchar(void);
wchar_t*getws(wchar_t*);
wint_t  putwc(wint_t,FILE*);
wint_t  putwchar(wint_t);
int             putws(const wchar_t*);
int             swprintf(wchar_t*,const wchar_t*,...);
int             swscanf(const wchar_t*,const wchar_t*,...);
wint_t  ungetwc(wint_t,FILE*);
int             vfwprintf(FILE*,const wchar_t*,va_list);
int             vswprintf(wchar_t*,const wchar_t*,va_list);
int             vwprintf(const wchar_t*,va_list);
int             wprintf(const wchar_t*,...);
int             wscanf(const wchar_t*,...);
#endif /* _WSTDIO_DEFINED */

#endif /* _STDIO_DEFINED */

#ifdef __cplusplus
}
#endif


static inline FILE* fdopen(int fd, const char *mode) { return _fdopen(fd, mode); }
static inline int fgetchar(void) { return _fgetchar(); }
static inline int fileno(FILE* file) { return _fileno(file); }
static inline int fputchar(int c) { return _fputchar(c); }
static inline int pclose(FILE* file) { return _pclose(file); }
static inline FILE* popen(const char* command, const char* mode) { return _popen(command, mode); }
static inline char* tempnam(const char *dir, const char *prefix) { return _tempnam(dir, prefix); }
#ifndef _UNLINK_DEFINED
static inline int unlink(const char* path) { return _unlink(path); }
#define _UNLINK_DEFINED
#endif
static inline int vsnprintf(char *buffer, size_t size, const char *format, va_list args) { return _vsnprintf(buffer,size,format,args); }

static inline wint_t fgetwchar(void) { return _fgetwchar(); }
static inline wint_t fputwchar(wint_t wc) { return _fputwchar(wc); }
static inline int getw(FILE* file) { return _getw(file); }
static inline int putw(int val, FILE* file) { return _putw(val, file); }
static inline FILE* wpopen(const wchar_t* command,const wchar_t* mode) { return _wpopen(command, mode); }

#include <poppack.h>

#endif /* __WINE_STDIO_H */
