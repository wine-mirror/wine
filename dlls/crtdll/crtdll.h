#ifndef __WINE_CRTDLL_H
#define __WINE_CRTDLL_H

#include "config.h"
#include "windef.h"
#include "wine/windef16.h"
#include "debugtools.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include <time.h>
#include <stdarg.h>
#include <setjmp.h>


#define CRTDLL_LC_ALL		0
#define CRTDLL_LC_COLLATE	1
#define CRTDLL_LC_CTYPE		2
#define CRTDLL_LC_MONETARY	3
#define CRTDLL_LC_NUMERIC	4
#define CRTDLL_LC_TIME		5
#define CRTDLL_LC_MIN		LC_ALL
#define CRTDLL_LC_MAX		LC_TIME

/* ctype defines */
#define CRTDLL_UPPER		0x1
#define CRTDLL_LOWER		0x2
#define CRTDLL_DIGIT		0x4
#define CRTDLL_SPACE		0x8
#define CRTDLL_PUNCT		0x10
#define CRTDLL_CONTROL		0x20
#define CRTDLL_BLANK		0x40
#define CRTDLL_HEX		0x80
#define CRTDLL_LEADBYTE		0x8000
#define CRTDLL_ALPHA		(0x0100|CRTDLL_UPPER|CRTDLL_LOWER)

/* stat() mode bits */
#define _S_IFMT                  0170000
#define _S_IFREG                 0100000
#define _S_IFDIR                 0040000
#define _S_IFCHR                 0020000
#define _S_IFIFO                 0010000
#define _S_IREAD                 0000400
#define _S_IWRITE                0000200
#define _S_IEXEC                 0000100

/* _open modes */
#define _O_RDONLY 0x0000
#define _O_WRONLY 0x0001
#define _O_RDWR   0x0002
#define _O_APPEND 0x0008
#define _O_CREAT  0x0100
#define _O_TRUNC  0x0200
#define _O_EXCL   0x0400
#define _O_TEXT   0x4000
#define _O_BINARY 0x8000

/* _access() bit flags FIXME: incomplete */
#define W_OK      2

/* windows.h RAND_MAX is smaller than normal RAND_MAX */
#define CRTDLL_RAND_MAX         0x7fff

/* heap function constants */
#define _HEAPEMPTY    -1
#define _HEAPOK       -2
#define _HEAPBADBEGIN -3
#define _HEAPBADNODE  -4
#define _HEAPEND      -5
#define _HEAPBADPTR   -6
#define _FREEENTRY    0
#define _USEDENTRY    1

/* fpclass constants */
#define _FPCLASS_SNAN 1
#define _FPCLASS_QNAN 2
#define _FPCLASS_NINF 4
#define _FPCLASS_NN   8
#define _FPCLASS_ND   16
#define _FPCLASS_NZ   32
#define _FPCLASS_PZ   64
#define _FPCLASS_PD   128
#define _FPCLASS_PN   256
#define _FPCLASS_PINF 512

/* _statusfp bit flags */
#define _SW_INEXACT    0x1
#define _SW_UNDERFLOW  0x2
#define _SW_OVERFLOW   0x4
#define _SW_ZERODIVIDE 0x8
#define _SW_INVALID    0x10
#define _SW_DENORMAL   0x80000

/* _controlfp masks and bitflags - x86 only so far*/
#ifdef __i386__
#define _MCW_EM        0x8001f
#define _EM_INEXACT    0x1
#define _EM_UNDERFLOW  0x2
#define _EM_OVERFLOW   0x4
#define _EM_ZERODIVIDE 0x8
#define _EM_INVALID    0x10

#define _MCW_RC        0x300
#define _RC_NEAR       0x0
#define _RC_DOWN       0x100
#define _RC_UP         0x200
#define _RC_CHOP       0x300

#define _MCW_PC        0x30000
#define _PC_64         0x0
#define _PC_53         0x10000
#define _PC_24         0x20000

#define _MCW_IC        0x40000
#define _IC_AFFINE     0x40000
#define _IC_PROJECTIVE 0x0

#define _EM_DENORMAL   0x80000

#endif

/* CRTDLL Globals */
extern INT  CRTDLL_doserrno;
extern INT  CRTDLL_errno;


/* Binary compatible structures, types and defines used
 * by CRTDLL. These should move to external header files,
 * and in some cases need to be renamed (CRTDLL_FILE!) to defs
 * as used by lcc/bcc/watcom/vc++ etc, in order to get source
 * compatibility for winelib.
 */

typedef struct _crtfile
{
  CHAR* _ptr;
  INT   _cnt;
  CHAR* _base;
  INT   _flag;
  INT   _file; /* fd */
  int   _charbuf;
  int   _bufsiz;
  char *_tmpfname;
} CRTDLL_FILE;

/* file._flag flags */
#define _IOREAD   0x0001
#define _IOWRT    0x0002
#define _IOEOF    0x0010
#define _IOERR    0x0020
#define _IORW     0x0080
#define _IOAPPEND 0x0200

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define EOF         -1

extern CRTDLL_FILE __CRTDLL_iob[3];

#define CRTDLL_stdin  (&__CRTDLL_iob[0])
#define CRTDLL_stdout (&__CRTDLL_iob[1])
#define CRTDLL_stderr (&__CRTDLL_iob[2])

typedef struct _find_t
{
  unsigned      attrib;
  time_t        time_create; /* -1 when N/A */
  time_t        time_access; /* -1 when N/A */
  time_t        time_write;
  unsigned long size;        /* FIXME: 64 bit ??*/
  char          name[MAX_PATH];
} find_t;

typedef struct _diskfree_t {
  unsigned num_clusters;
  unsigned available;
  unsigned cluster_sectors;
  unsigned sector_bytes;
} diskfree_t;

struct _stat
{
    UINT16 st_dev;
    UINT16 st_ino;
    UINT16 st_mode;
    INT16  st_nlink;
    INT16  st_uid;
    INT16  st_gid;
    UINT   st_rdev;
    INT    st_size;
    INT    st_atime;
    INT    st_mtime;
    INT    st_ctime;
};

struct _timeb
{
  time_t  time;
  UINT16  millitm;
  INT16   timezone;
  INT16   dstflag;
};

typedef long fpos_t;

struct complex
{
  double real;
  double imaginary;
};

typedef struct _heapinfo
{
  int * _pentry;
  size_t _size;
  int    _useflag;
} _HEAPINFO;

struct _utimbuf
{
  time_t actime;
  time_t modtime;
};

/* _matherr exception type */
struct _exception
{
  int type;
  char *name;
  double arg1;
  double arg2;
  double retval;
};

typedef VOID (*sig_handler_type)(VOID);

typedef VOID (*new_handler_type)(VOID);

typedef VOID (*_INITTERMFUN)();

typedef VOID (*atexit_function)(VOID);

typedef INT (__cdecl *comp_func)(LPCVOID, LPCVOID );

/* CRTDLL functions */

/* CRTDLL_dir.c */
INT    __cdecl CRTDLL__chdir( LPCSTR newdir );
BOOL   __cdecl CRTDLL__chdrive( INT newdrive );
INT    __cdecl CRTDLL__findclose( DWORD hand );
DWORD  __cdecl CRTDLL__findfirst( LPCSTR fspec, find_t* ft );
INT    __cdecl CRTDLL__findnext( DWORD hand, find_t * ft );
CHAR*  __cdecl CRTDLL__getcwd( LPSTR buf, INT size );
CHAR*  __cdecl CRTDLL__getdcwd( INT drive,LPSTR buf, INT size );
UINT   __cdecl CRTDLL__getdiskfree( UINT disk, diskfree_t* d );
INT    __cdecl CRTDLL__getdrive( VOID );
INT    __cdecl CRTDLL__mkdir( LPCSTR newdir );
INT    __cdecl CRTDLL__rmdir( LPSTR dir );

/* CRTDLL_exit.c */
INT    __cdecl CRTDLL__abnormal_termination( VOID );
VOID   __cdecl CRTDLL__amsg_exit( INT err );
VOID   __cdecl CRTDLL__assert( LPVOID str, LPVOID file, UINT line );
VOID   __cdecl CRTDLL__c_exit( VOID );
VOID   __cdecl CRTDLL__cexit( VOID );
void   __cdecl CRTDLL_exit( DWORD ret );
VOID   __cdecl CRTDLL__exit( LONG ret );
VOID   __cdecl CRTDLL_abort( VOID );
INT    __cdecl CRTDLL_atexit( atexit_function x );
atexit_function __cdecl CRTDLL__onexit( atexit_function func);

/* CRTDLL_file.c */
CRTDLL_FILE* __cdecl CRTDLL__iob( VOID );
CRTDLL_FILE* __cdecl CRTDLL__fsopen( LPCSTR path, LPCSTR mode, INT share );
CRTDLL_FILE* __cdecl CRTDLL__fdopen( INT fd, LPCSTR mode );
LPSTR        __cdecl CRTDLL__mktemp( LPSTR pattern );
CRTDLL_FILE* __cdecl CRTDLL_fopen( LPCSTR path, LPCSTR mode );
CRTDLL_FILE* __cdecl CRTDLL_freopen( LPCSTR path,LPCSTR mode,CRTDLL_FILE* f );
INT    __cdecl CRTDLL__fgetchar( VOID );
DWORD  __cdecl CRTDLL_fread( LPVOID ptr,INT size,INT nmemb,CRTDLL_FILE* file );
INT    __cdecl CRTDLL_fscanf( CRTDLL_FILE* stream, LPSTR format, ... );
INT    __cdecl CRTDLL__filbuf( CRTDLL_FILE* file );
LONG   __cdecl CRTDLL__filelength( INT fd );
INT    __cdecl CRTDLL__fileno( CRTDLL_FILE* file );
INT    __cdecl CRTDLL__flsbuf( INT c, CRTDLL_FILE* file );
INT    __cdecl CRTDLL__fputchar( INT c );
INT    __cdecl CRTDLL__flushall( VOID );
INT    __cdecl CRTDLL__fcloseall( VOID );
LONG   __cdecl CRTDLL__lseek( INT fd, LONG offset, INT whence );
LONG   __cdecl CRTDLL_fseek( CRTDLL_FILE* file, LONG offset, INT whence );
VOID   __cdecl CRTDLL_rewind( CRTDLL_FILE* file );
INT    __cdecl CRTDLL_fsetpos( CRTDLL_FILE* file, fpos_t *pos );
LONG   __cdecl CRTDLL_ftell( CRTDLL_FILE* file );
UINT   __cdecl CRTDLL_fwrite( LPCVOID ptr,INT size,INT nmemb,CRTDLL_FILE*file);
INT    __cdecl CRTDLL_setbuf( CRTDLL_FILE* file, LPSTR buf );
INT    __cdecl CRTDLL__open_osfhandle( HANDLE osfhandle, INT flags );
INT    __cdecl CRTDLL_vfprintf( CRTDLL_FILE* file, LPCSTR format,va_list args);
INT    __cdecl CRTDLL_fprintf( CRTDLL_FILE* file, LPCSTR format, ... );
INT    __cdecl CRTDLL__putw( INT val, CRTDLL_FILE* file );
INT    __cdecl CRTDLL__read( INT fd, LPVOID buf, UINT count );
UINT   __cdecl CRTDLL__write( INT fd,LPCVOID buf,UINT count );
INT    __cdecl CRTDLL__access( LPCSTR filename, INT mode );
INT    __cdecl CRTDLL_fflush( CRTDLL_FILE* file );
INT    __cdecl CRTDLL_fputc( INT c, CRTDLL_FILE* file );
VOID   __cdecl CRTDLL_putchar( INT x );
INT    __cdecl CRTDLL_fputs( LPCSTR s, CRTDLL_FILE* file );
INT    __cdecl CRTDLL_puts( LPCSTR s );
INT    __cdecl CRTDLL_putc( INT c, CRTDLL_FILE* file );
INT    __cdecl CRTDLL_fgetc( CRTDLL_FILE* file );
INT    __cdecl CRTDLL_getchar( VOID );
INT    __cdecl CRTDLL_getc( CRTDLL_FILE* file );
CHAR*  __cdecl CRTDLL_fgets( LPSTR s, INT size, CRTDLL_FILE* file );
LPSTR  __cdecl CRTDLL_gets( LPSTR buf );
INT    __cdecl CRTDLL_fclose( CRTDLL_FILE* file );
INT    __cdecl CTRDLL__creat( LPCSTR path, INT flags );
INT    __cdecl CRTDLL__eof( INT fd );
LONG   __cdecl CRTDLL__tell(INT fd);
INT    __cdecl CRTDLL__umask(INT umask);
INT    __cdecl CRTDLL__utime( LPCSTR path, struct _utimbuf *t );
INT    __cdecl CRTDLL__unlink( LPCSTR pathname );
INT    __cdecl CRTDLL_rename( LPCSTR oldpath,LPCSTR newpath );
int    __cdecl CRTDLL__stat(  LPCSTR filename, struct _stat * buf );
INT    __cdecl CRTDLL__open( LPCSTR path,INT flags );
INT    __cdecl CRTDLL__chmod( LPCSTR path, INT flags );
INT    __cdecl CRTDLL__close( INT fd );
INT    __cdecl CRTDLL_feof( CRTDLL_FILE* file );
INT    __cdecl CRTDLL__setmode( INT fh,INT mode );
INT    __cdecl CRTDLL_remove( LPCSTR path );
INT    __cdecl CRTDLL__commit( INT fd );
INT    __cdecl CRTDLL__fstat( int file, struct _stat* buf );
INT    __cdecl CRTDLL__futime( INT fd, struct _utimbuf *t );
HANDLE __cdecl CRTDLL__get_osfhandle( INT fd );

/* CRTDLL_main.c */
DWORD  __cdecl CRTDLL__initterm( _INITTERMFUN *start,_INITTERMFUN *end );
VOID   __cdecl CRTDLL__global_unwind2( PEXCEPTION_FRAME frame );
VOID   __cdecl CRTDLL__local_unwind2( PEXCEPTION_FRAME endframe, DWORD nr );
INT    __cdecl CRTDLL__setjmp( LPDWORD *jmpbuf );
VOID   __cdecl CRTDLL_srand( DWORD seed );
INT    __cdecl CRTDLL__getw( CRTDLL_FILE* file );
INT    __cdecl CRTDLL__isatty(INT fd);
VOID   __cdecl CRTDLL__beep( UINT freq, UINT duration );
INT    __cdecl CRTDLL_rand( VOID );
UINT   __cdecl CRTDLL__rotl( UINT x,INT shift );
double __cdecl CRTDLL__logb( double x );
DWORD  __cdecl CRTDLL__lrotl( DWORD x,INT shift );
DWORD  __cdecl CRTDLL__lrotr( DWORD x,INT shift );
DWORD  __cdecl CRTDLL__rotr( UINT x,INT shift );
double __cdecl CRTDLL__scalb( double x, LONG y );
INT    __cdecl CRTDLL__mbsicmp( unsigned char *x,unsigned char *y );
INT    __cdecl CRTDLL_vswprintf( LPWSTR buffer, LPCWSTR spec, va_list args );
VOID   __cdecl CRTDLL_longjmp( jmp_buf env, int val );
LPSTR  __cdecl CRTDLL_setlocale( INT category,LPCSTR locale );
INT    __cdecl CRTDLL__isctype( INT c, UINT type );
LPSTR  __cdecl CRTDLL__fullpath( LPSTR buf, LPCSTR name, INT size );
VOID   __cdecl CRTDLL__splitpath( LPCSTR path, LPSTR drive, LPSTR directory,
                                  LPSTR filename, LPSTR extension );
INT    __cdecl CRTDLL__matherr( struct _exception *e );
VOID   __cdecl CRTDLL__makepath( LPSTR path, LPCSTR drive,
                                 LPCSTR directory, LPCSTR filename,
                                 LPCSTR extension );
LPINT  __cdecl CRTDLL__errno( VOID );
LPINT  __cdecl CRTDLL___doserrno( VOID );
LPCSTR**__cdecl CRTDLL__sys_errlist( VOID );
VOID   __cdecl CRTDLL_perror( LPCSTR err );
UINT   __cdecl CRTDLL__statusfp( VOID );
LPSTR  __cdecl CRTDLL__strerror( LPCSTR err );
LPSTR  __cdecl CRTDLL_strerror( INT err );
LPSTR  __cdecl CRTDLL__tempnam( LPCSTR dir, LPCSTR prefix );
LPSTR  __cdecl CRTDLL_tmpnam( LPSTR s );
LPVOID __cdecl CRTDLL_signal( INT sig, sig_handler_type ptr );
VOID   __cdecl CRTDLL__sleep( ULONG timeout );
LPSTR  __cdecl CRTDLL_getenv( LPCSTR name );
INT    __cdecl CRTDLL_isalnum( INT c );
INT    __cdecl CRTDLL_isalpha( INT c );
INT    __cdecl CRTDLL_iscntrl( INT c );
INT    __cdecl CRTDLL_isdigit( INT c );
INT    __cdecl CRTDLL_isgraph( INT c );
INT    __cdecl CRTDLL_islower( INT c);
INT    __cdecl CRTDLL_isprint( INT c);
INT    __cdecl CRTDLL_ispunct( INT c);
INT    __cdecl CRTDLL_isspace( INT c);
INT    __cdecl CRTDLL_isupper( INT c);
INT    __cdecl CRTDLL_isxdigit( INT c );
double __cdecl CRTDLL_ldexp( double x, LONG y );
LPSTR  __cdecl CRTDLL__mbsrchr( LPSTR s,CHAR x );
VOID   __cdecl CRTDLL___dllonexit ( VOID );
VOID   __cdecl CRTDLL__mbccpy( LPSTR dest, LPSTR src );
INT    __cdecl CRTDLL___isascii( INT c );
INT    __cdecl CRTDLL___toascii( INT c );
INT    __cdecl CRTDLL_iswascii( LONG c );
INT    __cdecl CRTDLL___iscsym( UCHAR c );
INT    __cdecl CRTDLL___iscsymf( UCHAR c );
INT    __cdecl CRTDLL__loaddll( LPSTR dllname );
INT    __cdecl CRTDLL__unloaddll( HANDLE dll );
WCHAR* __cdecl CRTDLL__itow( INT value,WCHAR* out,INT base );
WCHAR* __cdecl CRTDLL__ltow( LONG value,WCHAR* out,INT base );
WCHAR* __cdecl CRTDLL__ultow( ULONG value,WCHAR* out,INT base );
CHAR   __cdecl CRTDLL__toupper( CHAR c );
CHAR   __cdecl CRTDLL__tolower( CHAR c );
double __cdecl CRTDLL__cabs( struct complex c );
double __cdecl CRTDLL__chgsign( double d );
UINT   __cdecl CRTDLL__control87( UINT newVal, UINT mask);
UINT   __cdecl CRTDLL__controlfp( UINT newVal, UINT mask);
double __cdecl CRTDLL__copysign( double x, double sign );
INT    __cdecl CRTDLL__finite( double d );
UINT   __cdecl CRTDLL__clearfp( VOID );
INT    __cdecl CRTDLL__fpclass( double d );
VOID   __cdecl CRTDLL__fpreset( void );
INT    __cdecl CRTDLL__isnan( double d );
LPVOID __cdecl CRTDLL__lsearch( LPVOID match, LPVOID start, LPUINT array_size,
                                UINT elem_size, comp_func cf );
VOID   __cdecl CRTDLL__purecall( VOID );
double __cdecl CRTDLL__y0( double x );
double __cdecl CRTDLL__y1( double x );
double __cdecl CRTDLL__yn( INT x, double y );

/* CRTDLL_mem.c */
LPVOID  __cdecl CRTDLL_new( DWORD size );
VOID   __cdecl CRTDLL_delete( LPVOID ptr );
new_handler_type __cdecl CRTDLL_set_new_handler( new_handler_type func );
INT    __cdecl CRTDLL__heapchk( VOID );
INT    __cdecl CRTDLL__heapmin( VOID );
INT    __cdecl CRTDLL__heapset( UINT value );
INT    __cdecl CRTDLL__heapwalk( struct _heapinfo *next );
LPVOID __cdecl CRTDLL__expand( LPVOID ptr, INT size );
LONG   __cdecl CRTDLL__msize( LPVOID mem );
LPVOID __cdecl CRTDLL_calloc( DWORD size, DWORD count );
VOID   __cdecl CRTDLL_free( LPVOID ptr );
LPVOID __cdecl CRTDLL_malloc( DWORD size );
LPVOID __cdecl CRTDLL_realloc( VOID *ptr, DWORD size );

/* CRTDLL_spawn.c */
HANDLE __cdecl CRTDLL__spawnve( INT flags, LPSTR name, LPSTR *argv, LPSTR *envv);
INT    __cdecl CRTDLL_system( LPSTR x );

/* CRTDLL_str.c */
LPSTR  __cdecl CRTDLL__strdec( LPSTR str1, LPSTR str2 );
LPSTR  __cdecl CRTDLL__strdup( LPCSTR ptr );
LPSTR  __cdecl CRTDLL__strinc( LPSTR str );
UINT   __cdecl CRTDLL__strnextc( LPCSTR str );
LPSTR  __cdecl CRTDLL__strninc( LPSTR str, INT n );
LPSTR  __cdecl CRTDLL__strnset( LPSTR str, INT c, INT len );
LPSTR  __cdecl CRTDLL__strrev ( LPSTR str );
LPSTR  __cdecl CRTDLL__strset ( LPSTR str, INT set );
LONG   __cdecl CRTDLL__strncnt( LPSTR str, LONG max );
LPSTR  __cdecl CRTDLL__strspnp( LPSTR str1, LPSTR str2 );
VOID   __cdecl CRTDLL__swab( LPSTR src, LPSTR dst, INT len );

/* CRTDLL_time.c */
LPSTR   __cdecl CRTDLL__strdate ( LPSTR date );
LPSTR   __cdecl CRTDLL__strtime ( LPSTR date );
clock_t __cdecl CRTDLL_clock ( void );
double  __cdecl CRTDLL_difftime ( time_t time1, time_t time2 );
time_t  __cdecl CRTDLL_time ( time_t *timeptr );

/* mbstring.c */
LPSTR  __cdecl CRTDLL__mbsinc( LPCSTR str );
INT    __cdecl CRTDLL__mbslen( LPCSTR str );
INT    __cdecl CRTDLL_mbtowc( WCHAR *dst, LPCSTR str, INT n );
LPWSTR __cdecl CRTDLL__wcsdup( LPCWSTR str );
INT    __cdecl CRTDLL__wcsicoll( LPCWSTR str1, LPCWSTR str2 );
LPWSTR __cdecl CRTDLL__wcsnset( LPWSTR str, WCHAR c, INT n );
LPWSTR __cdecl CRTDLL__wcsrev( LPWSTR str );
LPWSTR __cdecl CRTDLL__wcsset( LPWSTR str, WCHAR c );
DWORD  __cdecl CRTDLL_wcscoll( LPCWSTR str1, LPCWSTR str2 );
LPWSTR __cdecl CRTDLL_wcspbrk( LPCWSTR str, LPCWSTR accept );
INT    __cdecl CRTDLL_wctomb( LPSTR dst, WCHAR ch );

/* wcstring.c */
INT    __cdecl CRTDLL_iswalnum( WCHAR wc );
INT    __cdecl CRTDLL_iswalpha( WCHAR wc );
INT    __cdecl CRTDLL_iswcntrl( WCHAR wc );
INT    __cdecl CRTDLL_iswdigit( WCHAR wc );
INT    __cdecl CRTDLL_iswgraph( WCHAR wc );
INT    __cdecl CRTDLL_iswlower( WCHAR wc );
INT    __cdecl CRTDLL_iswprint( WCHAR wc );
INT    __cdecl CRTDLL_iswpunct( WCHAR wc );
INT    __cdecl CRTDLL_iswspace( WCHAR wc );
INT    __cdecl CRTDLL_iswupper( WCHAR wc );
INT    __cdecl CRTDLL_iswxdigit( WCHAR wc );

/* INTERNAL: Shared internal functions */
void   __CRTDLL__set_errno(ULONG err);
LPSTR  __CRTDLL__strndup(LPSTR buf, INT size);
VOID   __CRTDLL__init_io(VOID);

#endif /* __WINE_CRTDLL_H */
