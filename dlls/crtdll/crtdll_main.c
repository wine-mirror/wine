/*
 * The C RunTime DLL
 * 
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 */

/*
Unresolved issues Uwe Bonnes 970904:
- Handling of Binary/Text Files is crude. If in doubt, use fromdos or recode
- Arguments in crtdll.spec for functions with double argument
- system-call calls another wine process, but without debugging arguments
              and uses the first wine executable in the path
- tested with ftp://ftp.remcomp.com/pub/remcomp/lcc-win32.zip, a C-Compiler
 		for Win32, based on lcc, from Jacob Navia
AJ 990101:
- needs a proper stdio emulation based on Win32 file handles
- should set CRTDLL errno from GetLastError() in every function
UB 000416:
- probably not thread safe
*/

/* NOTE: This file also implements the wcs* functions. They _ARE_ in 
 * the newer Linux libcs, but use 4 byte wide characters, so are unusable,
 * since we need 2 byte wide characters. - Marcus Meissner, 981031
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <setjmp.h>
#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "ntddk.h"
#include "debugtools.h"
#include "module.h"
#include "heap.h"
#include "crtdll.h"
#include "drive.h"
#include "file.h"
#include "options.h"
#include "winnls.h"

DEFAULT_DEBUG_CHANNEL(crtdll);

/* windows.h RAND_MAX is smaller than normal RAND_MAX */
#define CRTDLL_RAND_MAX         0x7fff 

static DOS_FULL_NAME CRTDLL_tmpname;

UINT CRTDLL_argc_dll;         /* CRTDLL.23 */
LPSTR *CRTDLL_argv_dll;         /* CRTDLL.24 */
LPSTR  CRTDLL_acmdln_dll;       /* CRTDLL.38 */
UINT CRTDLL_basemajor_dll;    /* CRTDLL.42 */
UINT CRTDLL_baseminor_dll;    /* CRTDLL.43 */
UINT CRTDLL_baseversion_dll;  /* CRTDLL.44 */
UINT CRTDLL_commode_dll;      /* CRTDLL.59 */
LPSTR  CRTDLL_environ_dll;      /* CRTDLL.75 */
UINT CRTDLL_fmode_dll;        /* CRTDLL.104 */
UINT CRTDLL_osmajor_dll;      /* CRTDLL.241 */
UINT CRTDLL_osminor_dll;      /* CRTDLL.242 */
UINT CRTDLL_osmode_dll;       /* CRTDLL.243 */
UINT CRTDLL_osver_dll;        /* CRTDLL.244 */
UINT CRTDLL_osversion_dll;    /* CRTDLL.245 */
UINT CRTDLL_winmajor_dll;     /* CRTDLL.329 */
UINT CRTDLL_winminor_dll;     /* CRTDLL.330 */
UINT CRTDLL_winver_dll;       /* CRTDLL.331 */

/* FIXME: structure layout is obviously not correct */
typedef struct
{
    HANDLE handle;
    int      pad[7];
} CRTDLL_FILE;

CRTDLL_FILE CRTDLL_iob[3];

static CRTDLL_FILE * const CRTDLL_stdin  = &CRTDLL_iob[0];
static CRTDLL_FILE * const CRTDLL_stdout = &CRTDLL_iob[1];
static CRTDLL_FILE * const CRTDLL_stderr = &CRTDLL_iob[2];

typedef VOID (*new_handler_type)(VOID);

static new_handler_type new_handler;

#if defined(__GNUC__) && defined(__i386__)
#define USING_REAL_FPU
#define DO_FPU(x,y) __asm__ __volatile__( x " %0;fwait" : "=m" (y) : )
#define POP_FPU(x) DO_FPU("fstpl",x)
#endif

CRTDLL_FILE * __cdecl CRTDLL__fdopen(INT handle, LPCSTR mode);

/*********************************************************************
 *                  CRTDLL_MainInit  (CRTDLL.init)
 */
BOOL WINAPI CRTDLL_Init(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	TRACE("(0x%08x,%ld,%p)\n",hinstDLL,fdwReason,lpvReserved);

	if (fdwReason == DLL_PROCESS_ATTACH) {
		CRTDLL__fdopen(0,"r");
		CRTDLL__fdopen(1,"w");
		CRTDLL__fdopen(2,"w");
	}
	return TRUE;
}

/*********************************************************************
 *                  _GetMainArgs  (CRTDLL.022)
 */
LPSTR * __cdecl CRTDLL__GetMainArgs(LPDWORD argc,LPSTR **argv,
                                LPSTR *environ,DWORD flag)
{
        char *cmdline;
        char  **xargv;
	int	xargc,end,last_arg,afterlastspace;
	DWORD	version;

	TRACE("(%p,%p,%p,%ld).\n",
		argc,argv,environ,flag
	);

	if (CRTDLL_acmdln_dll != NULL)
		HeapFree(GetProcessHeap(), 0, CRTDLL_acmdln_dll);

	CRTDLL_acmdln_dll = cmdline = HEAP_strdupA( GetProcessHeap(), 0,
                                                    GetCommandLineA() );
 	TRACE("got '%s'\n", cmdline);

	version	= GetVersion();
	CRTDLL_osver_dll       = version >> 16;
	CRTDLL_winminor_dll    = version & 0xFF;
	CRTDLL_winmajor_dll    = (version>>8) & 0xFF;
	CRTDLL_baseversion_dll = version >> 16;
	CRTDLL_winver_dll      = ((version >> 8) & 0xFF) + ((version & 0xFF) << 8);
	CRTDLL_baseminor_dll   = (version >> 16) & 0xFF;
	CRTDLL_basemajor_dll   = (version >> 24) & 0xFF;
	CRTDLL_osversion_dll   = version & 0xFFFF;
	CRTDLL_osminor_dll     = version & 0xFF;
	CRTDLL_osmajor_dll     = (version>>8) & 0xFF;

	/* missing threading init */

	end=0;last_arg=0;xargv=NULL;xargc=0;afterlastspace=0;
	while (1)
	{
	    if ((cmdline[end]==' ') || (cmdline[end]=='\0'))
	    {
		if (cmdline[end]=='\0')
		    last_arg=1;
		else
		    cmdline[end]='\0';
		/* alloc xargc + NULL entry */
			xargv=(char**)HeapReAlloc( GetProcessHeap(), 0, xargv,
		                             sizeof(char*)*(xargc+1));
		if (strlen(cmdline+afterlastspace))
		{
		    xargv[xargc] = HEAP_strdupA( GetProcessHeap(), 0,
                                                       cmdline+afterlastspace);
		    xargc++;
                    if (!last_arg) /* need to seek to the next arg ? */
		    {
			end++;
			while (cmdline[end]==' ')
			    end++;
	}
		    afterlastspace=end;
		}
		else
		{
		    xargv[xargc] = NULL; /* the last entry is NULL */
		    break;
		}
	    }
	    else
		end++;
	}
	CRTDLL_argc_dll	= xargc;
	*argc		= xargc;
	CRTDLL_argv_dll	= xargv;
	*argv		= xargv;

	TRACE("found %d arguments\n",
		CRTDLL_argc_dll);
	CRTDLL_environ_dll = *environ = GetEnvironmentStringsA();
	return environ;
}


typedef void (*_INITTERMFUN)();

/* fixme: move to header */
struct find_t 
{   unsigned	attrib;
    time_t	time_create;	/* -1 when not avaiable */
    time_t	time_access;	/* -1 when not avaiable */
    time_t	time_write;
    unsigned long	size;	/* fixme: 64 bit ??*/
    char	name[260];
};
 /*********************************************************************
 *                  _findfirst    (CRTDLL.099)
 * 
 * BUGS
 *   Unimplemented
 */
DWORD __cdecl CRTDLL__findfirst(LPCSTR fname,  struct find_t * x2)
{
  FIXME(":(%s,%p): stub\n",fname,x2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*********************************************************************
 *                  _findnext     (CRTDLL.100)
 * 
 * BUGS
 *   Unimplemented
 */
INT __cdecl CRTDLL__findnext(DWORD hand, struct find_t * x2)
{
  FIXME(":(%ld,%p): stub\n",hand,x2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*********************************************************************
 *                  _fstat        (CRTDLL.111)
 * 
 * BUGS
 *   Unimplemented
 */
int __cdecl CRTDLL__fstat(int file, struct stat* buf)
{
  FIXME(":(%d,%p): stub\n",file,buf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*********************************************************************
 *                  _initterm     (CRTDLL.135)
 */
DWORD __cdecl CRTDLL__initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
	_INITTERMFUN	*current;

	TRACE("(%p,%p)\n",start,end);
	current=start;
	while (current<end) {
		if (*current) (*current)();
		current++;
	}
	return 0;
}

/*********************************************************************
 *                  _fsopen     (CRTDLL.110)
 */
CRTDLL_FILE * __cdecl CRTDLL__fsopen(LPCSTR x, LPCSTR y, INT z) {
	FIXME("(%s,%s,%d),stub!\n",x,y,z);
	return NULL;
}

/*********************************************************************
 *                  _fdopen     (CRTDLL.91)
 */
CRTDLL_FILE * __cdecl CRTDLL__fdopen(INT handle, LPCSTR mode)
{
    CRTDLL_FILE *file;

    switch (handle) 
    {
    case 0:
        file = CRTDLL_stdin;
        if (!file->handle) file->handle = GetStdHandle( STD_INPUT_HANDLE );
        break;
    case 1:
        file = CRTDLL_stdout;
        if (!file->handle) file->handle = GetStdHandle( STD_OUTPUT_HANDLE );
        break;
    case 2:
        file=CRTDLL_stderr;
        if (!file->handle) file->handle = GetStdHandle( STD_ERROR_HANDLE );
        break;
    default:
        file = HeapAlloc( GetProcessHeap(), 0, sizeof(*file) );
        file->handle = handle;
        break;
    }
  TRACE("open handle %d mode %s  got file %p\n",
	       handle, mode, file);
  return file;
}


/*******************************************************************
 *         _global_unwind2  (CRTDLL.129)
 */
void __cdecl CRTDLL__global_unwind2( PEXCEPTION_FRAME frame )
{
    RtlUnwind( frame, 0, NULL, 0 );
}

/*******************************************************************
 *         _local_unwind2  (CRTDLL.173)
 */
void __cdecl CRTDLL__local_unwind2( PEXCEPTION_FRAME endframe, DWORD nr )
{
    TRACE("(%p,%ld)\n",endframe,nr);
}
/*******************************************************************
 *         _setjmp  (CRTDLL.264)
 */
INT __cdecl CRTDLL__setjmp(LPDWORD *jmpbuf)
{
  FIXME(":(%p): stub\n",jmpbuf);
  return 0;
}

/*********************************************************************
 *                  fopen     (CRTDLL.372)
 */
CRTDLL_FILE * __cdecl CRTDLL_fopen(LPCSTR path, LPCSTR mode)
{
  CRTDLL_FILE *file = NULL;
  HFILE handle;
#if 0
  DOS_FULL_NAME full_name;
  
  if (!DOSFS_GetFullName( path, FALSE, &full_name )) {
    WARN("file %s bad name\n",path);
   return 0;
  }
  
  file=fopen(full_name.long_name ,mode);
#endif
  DWORD access = 0, creation = 0;

  if ((strchr(mode,'r')&&strchr(mode,'a'))||
      (strchr(mode,'r')&&strchr(mode,'w'))||
      (strchr(mode,'w')&&strchr(mode,'a')))
    return NULL;
       
  if (mode[0] == 'r')
  {
      access = GENERIC_READ;
      creation = OPEN_EXISTING;
      if (mode[1] == '+') access |= GENERIC_WRITE;
  }
  else if (mode[0] == 'w')
  {
      access = GENERIC_WRITE;
      creation = CREATE_ALWAYS;
      if (mode[1] == '+') access |= GENERIC_READ;
  }
  else if (mode[0] == 'a')
  {
      /* FIXME: there is no O_APPEND in CreateFile, should emulate it */
      access = GENERIC_WRITE;
      creation = OPEN_ALWAYS;
      if (mode[1] == '+') access |= GENERIC_READ;
  }
  /* FIXME: should handle text/binary mode */

  if ((handle = CreateFileA( path, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, creation, FILE_ATTRIBUTE_NORMAL,
                               -1 )) != INVALID_HANDLE_VALUE)
  {
      file = HeapAlloc( GetProcessHeap(), 0, sizeof(*file) );
      file->handle = handle;
  }
  TRACE("file %s mode %s got handle %d file %p\n",
		 path,mode,handle,file);
  return file;
}

/*********************************************************************
 *                  fread     (CRTDLL.377)
 */
DWORD __cdecl CRTDLL_fread(LPVOID ptr, INT size, INT nmemb, CRTDLL_FILE *file)
{
#if 0
  int i=0;
  void *temp=ptr;

  /* If we would honour CR/LF <-> LF translation, we could do it like this.
     We should keep track of all files opened, and probably files with \
     known binary extensions must be unchanged */
  while ( (i < (nmemb*size)) && (ret==1)) {
    ret=fread(temp,1,1,file);
    TRACE("got %c 0x%02x ret %d\n",
		 (isalpha(*(unsigned char*)temp))? *(unsigned char*)temp:
		 ' ',*(unsigned char*)temp, ret);
    if (*(unsigned char*)temp != 0xd) { /* skip CR */
      temp++;
      i++;
    }
    else
      TRACE("skipping ^M\n");
  }
  TRACE("0x%08x items of size %d from file %p to %p\n",
	       nmemb,size,file,ptr,);
  if(i!=nmemb)
    WARN(" failed!\n");

  return i;
#else
  DWORD ret;

  TRACE("0x%08x items of size %d from file %p to %p\n",
	       nmemb,size,file,ptr);
  if (!ReadFile( file->handle, ptr, size * nmemb, &ret, NULL ))
      WARN(" failed!\n");

  return ret / size;
#endif
}
/*********************************************************************
 *                  freopen    (CRTDLL.379)
 * 
 * BUGS
 *   Unimplemented
 */
DWORD __cdecl CRTDLL_freopen(LPCSTR path, LPCSTR mode, LPVOID stream)
{
  FIXME(":(%s,%s,%p): stub\n", path, mode, stream);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*********************************************************************
 *                  fscanf     (CRTDLL.381)
 */
INT __cdecl CRTDLL_fscanf( CRTDLL_FILE *stream, LPSTR format, ... )
{
#if 0
    va_list valist;
    INT res;

    va_start( valist, format );
#ifdef HAVE_VFSCANF
    res = vfscanf( xlat_file_ptr(stream), format, valist );
#endif
    va_end( valist );
    return res;
#endif
    FIXME("broken\n");
    return 0;
}

/*********************************************************************
 *                  _lseek     (CRTDLL.179)
 */
LONG __cdecl CRTDLL__lseek( INT fd, LONG offset, INT whence)
{
  TRACE("fd %d to 0x%08lx pos %s\n",
        fd,offset,(whence==SEEK_SET)?"SEEK_SET":
        (whence==SEEK_CUR)?"SEEK_CUR":
        (whence==SEEK_END)?"SEEK_END":"UNKNOWN");
  return SetFilePointer( fd, offset, NULL, whence );
}

/*********************************************************************
 *                  fseek     (CRTDLL.382)
 */
LONG __cdecl CRTDLL_fseek( CRTDLL_FILE *file, LONG offset, INT whence)
{
  TRACE("file %p to 0x%08lx pos %s\n",
        file,offset,(whence==SEEK_SET)?"SEEK_SET":
        (whence==SEEK_CUR)?"SEEK_CUR":
        (whence==SEEK_END)?"SEEK_END":"UNKNOWN");
  if (SetFilePointer( file->handle, offset, NULL, whence ) != 0xffffffff)
      return 0;
  WARN(" failed!\n");
  return -1;
}
  
/*********************************************************************
 *                  fsetpos     (CRTDLL.383)
 */
INT __cdecl CRTDLL_fsetpos( CRTDLL_FILE *file, INT *pos )
{
    TRACE("file %p pos %d\n", file, *pos );
    return CRTDLL_fseek(file, *pos, SEEK_SET);
}

/*********************************************************************
 *                  ftell     (CRTDLL.384)
 */
LONG __cdecl CRTDLL_ftell( CRTDLL_FILE *file )
{
    return SetFilePointer( file->handle, 0, NULL, SEEK_CUR );
}
  
/*********************************************************************
 *                  fwrite     (CRTDLL.386)
 */
DWORD __cdecl CRTDLL_fwrite( LPVOID ptr, INT size, INT nmemb, CRTDLL_FILE *file )
{
    DWORD ret;

    TRACE("0x%08x items of size %d to file %p(%d) from %p\n",
          nmemb,size,file,file-(CRTDLL_FILE*)CRTDLL_iob,ptr);
	
    
    if (!WriteFile( file->handle, ptr, size * nmemb, &ret, NULL ))
        WARN(" failed!\n");
    return ret / size;
}

/*********************************************************************
 *                  setbuf     (CRTDLL.452)
 */
INT __cdecl CRTDLL_setbuf(CRTDLL_FILE *file, LPSTR buf)
{
  TRACE("(file %p buf %p)\n", file, buf);
  /* this doesn't work:"void value not ignored as it ought to be" 
  return setbuf(file,buf); 
  */
  /* FIXME: no buffering for now */
  return 0;
}

/*********************************************************************
 *                  _open_osfhandle         (CRTDLL.240)
 */
HFILE __cdecl CRTDLL__open_osfhandle(LONG osfhandle, INT flags)
{
HFILE handle;
 
	switch (osfhandle) {
	case STD_INPUT_HANDLE :
	case 0 :
	  handle=0;
	  break;
 	case STD_OUTPUT_HANDLE:
 	case 1:
	  handle=1;
	  break;
	case STD_ERROR_HANDLE:
	case 2:
	  handle=2;
	  break;
	default:
	  return (-1);
	}
	TRACE("(handle %08lx,flags %d) return %d\n",
		     osfhandle,flags,handle);
	return handle;
	
}

/*********************************************************************
 *                  srand         (CRTDLL.460)
 */
void __cdecl CRTDLL_srand(DWORD seed)
{
	/* FIXME: should of course be thread? process? local */
	srand(seed);
}

/*********************************************************************
 *                  vfprintf       (CRTDLL.373)
 */
INT __cdecl CRTDLL_vfprintf( CRTDLL_FILE *file, LPSTR format, va_list args )
{
    char buffer[2048];  /* FIXME... */

    vsprintf( buffer, format, args );
    return CRTDLL_fwrite( buffer, 1, strlen(buffer), file );
}

/*********************************************************************
 *                  fprintf       (CRTDLL.373)
 */
INT __cdecl CRTDLL_fprintf( CRTDLL_FILE *file, LPSTR format, ... )
{
    va_list valist;
    INT res;

    va_start( valist, format );
    res = CRTDLL_vfprintf( file, format, valist );
    va_end( valist );
    return res;
}

/*********************************************************************
 *                  time          (CRTDLL.488)
 */
time_t __cdecl CRTDLL_time(time_t *timeptr)
{
	time_t	curtime = time(NULL);

	if (timeptr)
		*timeptr = curtime;
	return curtime;
}

/*********************************************************************
 *                  difftime      (CRTDLL.357)
 */
double __cdecl CRTDLL_difftime (time_t time1, time_t time2)
{
	double timediff;

	timediff = (double)(time1 - time2);
	return timediff;
}

/*********************************************************************
 *                  clock         (CRTDLL.350)
 */
clock_t __cdecl CRTDLL_clock(void)
{
	struct tms alltimes;
	clock_t res;

	times(&alltimes);
	res = alltimes.tms_utime + alltimes.tms_stime+
               alltimes.tms_cutime + alltimes.tms_cstime;
	/* Fixme: We need some symbolic representation
	   for (Hostsystem_)CLOCKS_PER_SEC 
	   and (Emulated_system_)CLOCKS_PER_SEC
	   10 holds only for Windows/Linux_i86)
	   */
	return 10*res;
}

/*********************************************************************
 *                  _isatty       (CRTDLL.137)
 */
BOOL __cdecl CRTDLL__isatty(DWORD x)
{
	TRACE("(%ld)\n",x);
	return TRUE;
}

/*********************************************************************
 *                  _read     (CRTDLL.256)
 *
 */
INT __cdecl CRTDLL__read(INT fd, LPVOID buf, UINT count)
{
    TRACE("0x%08x bytes fd %d to %p\n", count,fd,buf);
    if (!fd) fd = GetStdHandle( STD_INPUT_HANDLE );
    return _lread( fd, buf, count );
}

/*********************************************************************
 *                  _write        (CRTDLL.332)
 */
INT __cdecl CRTDLL__write(INT fd,LPCVOID buf,UINT count)
{
        INT len=0;

	if (fd == -1)
	  len = -1;
	else if (fd<=2)
	  len = (UINT)write(fd,buf,(LONG)count);
	else
	  len = _lwrite(fd,buf,count);
	TRACE("%d/%d byte to dfh %d from %p,\n",
		       len,count,fd,buf);
	return len;
}


/*********************************************************************
 *                  _cexit          (CRTDLL.49)
 *
 *  FIXME: What the heck is the difference between 
 *  FIXME           _c_exit         (CRTDLL.47)
 *  FIXME           _cexit          (CRTDLL.49)
 *  FIXME           _exit           (CRTDLL.87)
 *  FIXME           exit            (CRTDLL.359)
 *
 * atexit-processing comes to mind -- MW.
 *
 */
void __cdecl CRTDLL__cexit(INT ret)
{
        TRACE("(%d)\n",ret);
	ExitProcess(ret);
}


/*********************************************************************
 *                  exit          (CRTDLL.359)
 */
void __cdecl CRTDLL_exit(DWORD ret)
{
        TRACE("(%ld)\n",ret);
	ExitProcess(ret);
}


/*********************************************************************
 *                  _abnormal_termination          (CRTDLL.36)
 */
INT __cdecl CRTDLL__abnormal_termination(void)
{
        TRACE("(void)\n");
	return 0;
}


/*********************************************************************
 *                  _access          (CRTDLL.37)
 */
INT __cdecl CRTDLL__access(LPCSTR filename, INT mode)
{
    DWORD attr = GetFileAttributesA(filename);

    if (attr == -1)
    {
        if (GetLastError() == ERROR_INVALID_ACCESS)
            errno = EACCES;
        else
            errno = ENOENT;
        return -1;
    }

    if ((attr & FILE_ATTRIBUTE_READONLY) && (mode & W_OK))
    {
        errno = EACCES;
        return -1;
    }
    else
        return 0;
}


/*********************************************************************
 *                  fflush        (CRTDLL.365)
 */
INT __cdecl CRTDLL_fflush( CRTDLL_FILE *file )
{
    return FlushFileBuffers( file->handle ) ? 0 : -1;
}


/*********************************************************************
 *                  rand          (CRTDLL.446)
 */
INT __cdecl CRTDLL_rand()
{
    return (rand() & CRTDLL_RAND_MAX); 
}


/*********************************************************************
 *                  putchar       (CRTDLL.442)
 */
void __cdecl CRTDLL_putchar( INT x )
{
    putchar(x);
}


/*********************************************************************
 *                  fputc       (CRTDLL.374)
 */
INT __cdecl CRTDLL_fputc( INT c, CRTDLL_FILE *file )
{
    char ch = (char)c;
    DWORD res;
    TRACE("%c to file %p\n",c,file);
    if (!WriteFile( file->handle, &ch, 1, &res, NULL )) return -1;
    return c;
}


/*********************************************************************
 *                  fputs       (CRTDLL.375)
 */
INT __cdecl CRTDLL_fputs( LPCSTR s, CRTDLL_FILE *file )
{
    DWORD res;
    TRACE("%s to file %p\n",s,file);
    if (!WriteFile( file->handle, s, strlen(s), &res, NULL )) return -1;
    return res;
}


/*********************************************************************
 *                  puts       (CRTDLL.443)
 */
INT __cdecl CRTDLL_puts(LPCSTR s)
{
    TRACE("%s \n",s);
    return puts(s);
}


/*********************************************************************
 *                  putc       (CRTDLL.441)
 */
INT __cdecl CRTDLL_putc( INT c, CRTDLL_FILE *file )
{
    return CRTDLL_fputc( c, file );
}

/*********************************************************************
 *                  fgetc       (CRTDLL.366)
 */
INT __cdecl CRTDLL_fgetc( CRTDLL_FILE *file )
{
    DWORD res;
    char ch;
    if (!ReadFile( file->handle, &ch, 1, &res, NULL )) return -1;
    if (res != 1) return -1;
    return ch;
}


/*********************************************************************
 *                  getc       (CRTDLL.388)
 */
INT __cdecl CRTDLL_getc( CRTDLL_FILE *file )
{
    return CRTDLL_fgetc( file );
}


/*********************************************************************
 *                  fgets       (CRTDLL.368)
 */
CHAR* __cdecl CRTDLL_fgets( LPSTR s, INT size, CRTDLL_FILE *file )
{
    int    cc;
    LPSTR  buf_start = s;

    /* BAD, for the whole WINE process blocks... just done this way to test
     * windows95's ftp.exe.
     */

    for(cc = CRTDLL_fgetc(file); cc != EOF && cc != '\n'; cc = CRTDLL_fgetc(file))
	if (cc != '\r')
        {
            if (--size <= 0) break;
            *s++ = (char)cc;
        }
    if ((cc == EOF) &&(s == buf_start)) /* If nothing read, return 0*/
      return 0;
    if (cc == '\n')
      if (--size > 0)
	*s++ = '\n';
    *s = '\0';

    TRACE("got '%s'\n", buf_start);
    return buf_start;
}


/*********************************************************************
 *                  gets          (CRTDLL.391)
 */
LPSTR __cdecl CRTDLL_gets(LPSTR buf)
{
    int    cc;
    LPSTR  buf_start = buf;

    /* BAD, for the whole WINE process blocks... just done this way to test
     * windows95's ftp.exe.
     */

    for(cc = fgetc(stdin); cc != EOF && cc != '\n'; cc = fgetc(stdin))
	if(cc != '\r') *buf++ = (char)cc;

    *buf = '\0';

    TRACE("got '%s'\n", buf_start);
    return buf_start;
}


/*********************************************************************
 *                  _rotl          (CRTDLL.259)
 */
UINT __cdecl CRTDLL__rotl(UINT x,INT shift)
{
   unsigned int ret = (x >> shift)|( x >>((sizeof(x))-shift));

   TRACE("got 0x%08x rot %d ret 0x%08x\n",
		  x,shift,ret);
   return ret;
    
}
/*********************************************************************
 *                  _lrotl          (CRTDLL.176)
 */
DWORD __cdecl CRTDLL__lrotl(DWORD x,INT shift)
{
   unsigned long ret = (x >> shift)|( x >>((sizeof(x))-shift));

   TRACE("got 0x%08lx rot %d ret 0x%08lx\n",
		  x,shift,ret);
   return ret;
    
}


/*********************************************************************
 *                  _mbsicmp      (CRTDLL.204)
 */
int __cdecl CRTDLL__mbsicmp(unsigned char *x,unsigned char *y)
{
    do {
	if (!*x)
	    return !!*y;
	if (!*y)
	    return !!*x;
	/* FIXME: MBCS handling... */
	if (*x!=*y)
	    return 1;
        x++;
        y++;
    } while (1);
}


/*********************************************************************
 *                  vsprintf      (CRTDLL.500)
 */
INT __cdecl CRTDLL_vsprintf( LPSTR buffer, LPCSTR spec, va_list args )
{
    return wvsprintfA( buffer, spec, args );
}

/*********************************************************************
 *                  vswprintf      (CRTDLL.501)
 */
INT __cdecl CRTDLL_vswprintf( LPWSTR buffer, LPCWSTR spec, va_list args )
{
    return wvsprintfW( buffer, spec, args );
}

/*********************************************************************
 *                  _strcmpi   (CRTDLL.282) (CRTDLL.287)
 */
INT __cdecl CRTDLL__strcmpi( LPCSTR s1, LPCSTR s2 )
{
    return lstrcmpiA( s1, s2 );
}


/*********************************************************************
 *                  _strnicmp   (CRTDLL.293)
 */
INT __cdecl CRTDLL__strnicmp( LPCSTR s1, LPCSTR s2, INT n )
{
    return lstrncmpiA( s1, s2, n );
}


/*********************************************************************
 *                  _strlwr      (CRTDLL.293)
 *
 * convert a string in place to lowercase 
 */
LPSTR __cdecl CRTDLL__strlwr(LPSTR x)
{
  unsigned char *y =x;
  
  TRACE("CRTDLL_strlwr got %s\n", x);
  while (*y) {
    if ((*y > 0x40) && (*y< 0x5b))
      *y = *y + 0x20;
    y++;
  }
  TRACE("   returned %s\n", x);
		 
  return x;
}

/*********************************************************************
 *                  system       (CRTDLL.485)
 */
INT __cdecl CRTDLL_system(LPSTR x)
{
#define SYSBUF_LENGTH 1500
  char buffer[SYSBUF_LENGTH];
  unsigned char *y = x;
  unsigned char *bp;
  int i;

  sprintf( buffer, "%s \"", argv0 );
  bp = buffer + strlen(buffer);
  i = strlen(buffer) + strlen(x) +2;

  /* Calculate needed buffer size to prevent overflow.  */
  while (*y) {
    if (*y =='\\') i++;
    y++;
  }
  /* If buffer too short, exit.  */
  if (i > SYSBUF_LENGTH) {
    TRACE("_system buffer to small\n");
    return 127;
  }
  
  y =x;

  while (*y) {
    *bp = *y;
    bp++; y++;
    if (*(y-1) =='\\') *bp++ = '\\';
  }
  /* Remove spaces from end of string.  */
  while (*(y-1) == ' ') {
    bp--;y--;
  }
  *bp++ = '"';
  *bp = 0;
  TRACE("_system got '%s', executing '%s'\n",x,buffer);

  return system(buffer);
}

/*********************************************************************
 *                  _strupr       (CRTDLL.300)
 */
LPSTR __cdecl CRTDLL__strupr(LPSTR x)
{
	LPSTR	y=x;

	while (*y) {
		*y=toupper(*y);
		y++;
	}
	return x;
}

/*********************************************************************
 *                  longjmp        (CRTDLL.426)
 */
VOID __cdecl CRTDLL_longjmp(jmp_buf env, int val)
{
    FIXME("CRTDLL_longjmp semistup, expect crash\n");
    longjmp(env, val);
}

/*********************************************************************
 *                  malloc        (CRTDLL.427)
 */
VOID* __cdecl CRTDLL_malloc(DWORD size)
{
    return HeapAlloc(GetProcessHeap(),0,size);
}

/*********************************************************************
 *                  new           (CRTDLL.001)
 */
VOID* __cdecl CRTDLL_new(DWORD size)
{
    VOID* result;
    if(!(result = HeapAlloc(GetProcessHeap(),0,size)) && new_handler)
	(*new_handler)();
    return result;
}

/*********************************************************************
 *                  set_new_handler(CRTDLL.003)
 */
new_handler_type __cdecl CRTDLL_set_new_handler(new_handler_type func)
{
    new_handler_type old_handler = new_handler;
    new_handler = func;
    return old_handler;
}

/*********************************************************************
 *                  calloc        (CRTDLL.350)
 */
VOID* __cdecl CRTDLL_calloc(DWORD size, DWORD count)
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size * count );
}

/*********************************************************************
 *                  realloc        (CRTDLL.447)
 */
VOID* __cdecl CRTDLL_realloc( VOID *ptr, DWORD size )
{
    return HeapReAlloc( GetProcessHeap(), 0, ptr, size );
}

/*********************************************************************
 *                  free          (CRTDLL.427)
 */
VOID __cdecl CRTDLL_free(LPVOID ptr)
{
    HeapFree(GetProcessHeap(),0,ptr);
}

/*********************************************************************
 *                  delete       (CRTDLL.002)
 */
VOID __cdecl CRTDLL_delete(VOID* ptr)
{
    HeapFree(GetProcessHeap(),0,ptr);
}

/*********************************************************************
 *                  _strdup          (CRTDLL.285)
 */
LPSTR __cdecl CRTDLL__strdup(LPCSTR ptr)
{
    return HEAP_strdupA(GetProcessHeap(),0,ptr);
}

/*********************************************************************
 *                  fclose           (CRTDLL.362)
 */
INT __cdecl CRTDLL_fclose( CRTDLL_FILE *file )
{
    TRACE("%p\n", file );
    if (!CloseHandle( file->handle )) return -1;
    HeapFree( GetProcessHeap(), 0, file );
    return 0;
}

/*********************************************************************
 *                  _unlink           (CRTDLL.315)
 */
INT __cdecl CRTDLL__unlink(LPCSTR pathname)
{
    return DeleteFileA( pathname ) ? 0 : -1;
}

/*********************************************************************
 *                  rename           (CRTDLL.449)
 */
INT __cdecl CRTDLL_rename(LPCSTR oldpath,LPCSTR newpath)
{
    BOOL ok = MoveFileExA( oldpath, newpath, MOVEFILE_REPLACE_EXISTING );
    return ok ? 0 : -1;
}


/*********************************************************************
 *                  _stat          (CRTDLL.280)
 */

struct win_stat
{
    UINT16 win_st_dev;
    UINT16 win_st_ino;
    UINT16 win_st_mode;
    INT16  win_st_nlink;
    INT16  win_st_uid;
    INT16  win_st_gid;
    UINT win_st_rdev;
    INT  win_st_size;
    INT  win_st_atime;
    INT  win_st_mtime;
    INT  win_st_ctime;
};

int __cdecl CRTDLL__stat(const char * filename, struct win_stat * buf)
{
    int ret=0;
    DOS_FULL_NAME full_name;
    struct stat mystat;

    if (!DOSFS_GetFullName( filename, TRUE, &full_name ))
    {
      WARN("CRTDLL__stat filename %s bad name\n",filename);
      return -1;
    }
    ret=stat(full_name.long_name,&mystat);
    TRACE("CRTDLL__stat %s\n", filename);
    if(ret) 
      WARN(" Failed!\n");

    /* FIXME: should check what Windows returns */

    buf->win_st_dev   = mystat.st_dev;
    buf->win_st_ino   = mystat.st_ino;
    buf->win_st_mode  = mystat.st_mode;
    buf->win_st_nlink = mystat.st_nlink;
    buf->win_st_uid   = mystat.st_uid;
    buf->win_st_gid   = mystat.st_gid;
    buf->win_st_rdev  = mystat.st_rdev;
    buf->win_st_size  = mystat.st_size;
    buf->win_st_atime = mystat.st_atime;
    buf->win_st_mtime = mystat.st_mtime;
    buf->win_st_ctime = mystat.st_ctime;
    return ret;
}

/*********************************************************************
 *                  _open           (CRTDLL.239)
 */
HFILE __cdecl CRTDLL__open(LPCSTR path,INT flags)
{
    DWORD access = 0, creation = 0;
    HFILE ret;
    
    /* FIXME:
       the flags in lcc's header differ from the ones in Linux, e.g.
       Linux: define O_APPEND         02000   (= 0x400)
       lcc:  define _O_APPEND       0x0008  
       so here a scheme to translate them
       Probably lcc is wrong here, but at least a hack to get is going
       */
    switch(flags & 3)
    {
    case O_RDONLY: access |= GENERIC_READ; break;
    case O_WRONLY: access |= GENERIC_WRITE; break;
    case O_RDWR:   access |= GENERIC_WRITE | GENERIC_READ; break;
    }

    if (flags & 0x0100) /* O_CREAT */
    {
        if (flags & 0x0400) /* O_EXCL */
            creation = CREATE_NEW;
        else if (flags & 0x0200) /* O_TRUNC */
            creation = CREATE_ALWAYS;
        else
            creation = OPEN_ALWAYS;
    }
    else  /* no O_CREAT */
    {
        if (flags & 0x0200) /* O_TRUNC */
            creation = TRUNCATE_EXISTING;
        else
            creation = OPEN_EXISTING;
    }
    if (flags & 0x0008) /* O_APPEND */
        FIXME("O_APPEND not supported\n" );
    if (!(flags & 0x8000 /* O_BINARY */ ) || (flags & 0x4000 /* O_TEXT */))
	FIXME(":text mode not supported\n");
    if (flags & 0xf0f4) 
      TRACE("CRTDLL_open file unsupported flags 0x%04x\n",flags);
    /* End Fixme */

    ret = CreateFileA( path, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL, creation, FILE_ATTRIBUTE_NORMAL, -1 );
    TRACE("CRTDLL_open file %s mode 0x%04x got handle %d\n", path,flags,ret);
    return ret;
}

/*********************************************************************
 *                  _close           (CRTDLL.57)
 */
INT __cdecl CRTDLL__close(HFILE fd)
{
    int ret=_lclose(fd);

    TRACE("(%d)\n",fd);
    if(ret)
      WARN(" Failed!\n");

    return ret;
}

/*********************************************************************
 *                  feof           (CRTDLL.363)
 * FIXME: Care for large files
 * FIXME: Check errors
 */
INT __cdecl CRTDLL_feof( CRTDLL_FILE *file )
{
  DWORD curpos=SetFilePointer( file->handle, 0, NULL, SEEK_CUR );
  DWORD endpos=SetFilePointer( file->handle, 0, NULL, FILE_END );
  
  if (curpos==endpos)
    return TRUE;
  else
    SetFilePointer( file->handle, curpos,0,FILE_BEGIN);
  return FALSE;
}

/*********************************************************************
 *                  setlocale           (CRTDLL.453)
 */
LPSTR __cdecl CRTDLL_setlocale(INT category,LPCSTR locale)
{
	LPSTR categorystr;

	switch (category) {
	case CRTDLL_LC_ALL: categorystr="LC_ALL";break;
	case CRTDLL_LC_COLLATE: categorystr="LC_COLLATE";break;
	case CRTDLL_LC_CTYPE: categorystr="LC_CTYPE";break;
	case CRTDLL_LC_MONETARY: categorystr="LC_MONETARY";break;
	case CRTDLL_LC_NUMERIC: categorystr="LC_NUMERIC";break;
	case CRTDLL_LC_TIME: categorystr="LC_TIME";break;
	default: categorystr = "UNKNOWN?";break;
	}
	FIXME("(%s,%s),stub!\n",categorystr,locale);
	return "C";
}

/*********************************************************************
 *                  _setmode           (CRTDLL.265)
 * FIXME: At present we ignore the request to translate CR/LF to LF.
 *
 * We allways translate when we read with fgets, we never do with fread
 *
 */
INT __cdecl CRTDLL__setmode( INT fh,INT mode)
{
	/* FIXME */
#define O_TEXT     0x4000
#define O_BINARY   0x8000

	FIXME("on fhandle %d mode %s, STUB.\n",
		      fh,(mode=O_TEXT)?"O_TEXT":
		      (mode=O_BINARY)?"O_BINARY":"UNKNOWN");
	return -1;
}

/*********************************************************************
 *                  _fpreset           (CRTDLL.107)
 */
VOID __cdecl CRTDLL__fpreset(void)
{
       FIXME(" STUB.\n");
}

/*********************************************************************
 *                  atexit           (CRTDLL.345)
 */
INT __cdecl CRTDLL_atexit(LPVOID x)
{
	FIXME("(%p), STUB.\n",x);
	return 0; /* successful */
}

/*********************************************************************
 *                  _isctype           (CRTDLL.138)
 */
BOOL __cdecl CRTDLL__isctype(CHAR x,CHAR type)
{
	if ((type & CRTDLL_SPACE) && isspace(x))
		return TRUE;
	if ((type & CRTDLL_PUNCT) && ispunct(x))
		return TRUE;
	if ((type & CRTDLL_LOWER) && islower(x))
		return TRUE;
	if ((type & CRTDLL_UPPER) && isupper(x))
		return TRUE;
	if ((type & CRTDLL_ALPHA) && isalpha(x))
		return TRUE;
	if ((type & CRTDLL_DIGIT) && isdigit(x))
		return TRUE;
	if ((type & CRTDLL_CONTROL) && iscntrl(x))
		return TRUE;
	/* check CRTDLL_LEADBYTE */
	return FALSE;
}

/*********************************************************************
 *                  _chdrive           (CRTDLL.52)
 *
 *  newdir      [I] drive to change to, A=1
 *
 */
BOOL __cdecl CRTDLL__chdrive(INT newdrive)
{
	/* FIXME: generates errnos */
	return DRIVE_SetCurrentDrive(newdrive-1);
}

/*********************************************************************
 *                  _chdir           (CRTDLL.51)
 */
INT __cdecl CRTDLL__chdir(LPCSTR newdir)
{
	if (!SetCurrentDirectoryA(newdir))
		return 1;
	return 0;
}

/*********************************************************************
 *                  _fullpath           (CRTDLL.114)
 */
LPSTR __cdecl CRTDLL__fullpath(LPSTR buf, LPCSTR name, INT size)
{
  DOS_FULL_NAME full_name;

  if (!buf)
  {
      size = 256;
      if(!(buf = CRTDLL_malloc(size))) return NULL;
  }
  if (!DOSFS_GetFullName( name, FALSE, &full_name )) return NULL;
  lstrcpynA(buf,full_name.short_name,size);
  TRACE("CRTDLL_fullpath got %s\n",buf);
  return buf;
}

/*********************************************************************
 *                  _splitpath           (CRTDLL.279)
 */
VOID __cdecl CRTDLL__splitpath(LPCSTR path, LPSTR drive, LPSTR directory, LPSTR filename, LPSTR extension )
{
  /* drive includes :
     directory includes leading and trailing (forward and backward slashes)
     filename without dot and slashes
     extension with leading dot
     */
  char * drivechar,*dirchar,*namechar;

  TRACE("CRTDLL__splitpath got %s\n",path);

  drivechar  = strchr(path,':');
  dirchar    = strrchr(path,'/');
  namechar   = strrchr(path,'\\');
  dirchar = max(dirchar,namechar);
  if (dirchar)
    namechar   = strrchr(dirchar,'.');
  else
    namechar   = strrchr(path,'.');
  
  
  if (drive) 
    {
      *drive = 0x00;
      if (drivechar) 
	{
	  strncat(drive,path,drivechar-path+1);
	  path = drivechar+1;
	}
    }
  if (directory) 
    {
      *directory = 0x00;
      if (dirchar)
	{
	  strncat(directory,path,dirchar-path+1);
	  path = dirchar+1;
	}
    }
  if (filename)
    {
      *filename = 0x00;
      if (namechar)
	{
	  strncat(filename,path,namechar-path);
	  if (extension) 
	    {
	      *extension = 0x00;
	      strcat(extension,namechar);
	    }
	}
    }

  TRACE("CRTDLL__splitpath found %s %s %s %s\n",drive,directory,filename,extension);
  
}


/*********************************************************************
 *                  _makepath           (CRTDLL.182)
 */

VOID __cdecl CRTDLL__makepath(LPSTR path, LPCSTR drive, 
			      LPCSTR directory, LPCSTR filename, 
			      LPCSTR extension )
{
	char ch;
	TRACE("CRTDLL__makepath got %s %s %s %s\n", drive, directory, 
	      filename, extension);

	if ( !path )
		return;

	path[0] = 0;
	if ( drive ) 
		if ( drive[0] ) {
			sprintf(path, "%c:", drive[0]);
		}
	if ( directory ) 
		if ( directory[0] ) {
			strcat(path, directory);
			ch = path[strlen(path)-1];
			if (ch != '/' && ch != '\\')
				strcat(path,"\\");
		}
	if ( filename ) 
		if ( filename[0] ) {
			strcat(path, filename);
			if ( extension ) {
				if ( extension[0] ) {
					if ( extension[0] != '.' ) {
						strcat(path,".");
					} 
					strcat(path,extension);
				}
			}
		}
	
	TRACE("CRTDLL__makepath returns %s\n",path);  
}

/*********************************************************************
 *                  _getcwd           (CRTDLL.120)
 */
CHAR* __cdecl CRTDLL__getcwd(LPSTR buf, INT size)
{
  char test[1];
  int len;

  len = size;
  if (!buf) {
    if (size < 0) /* allocate as big as nescessary */
      len =GetCurrentDirectoryA(1,test) + 1;
    if(!(buf = CRTDLL_malloc(len)))
    {
	/* set error to OutOfRange */
	return( NULL );
    }
  }
  size = len;
  if(!(len =GetCurrentDirectoryA(len,buf)))
    {
      return NULL;
    }
  if (len > size)
    {
      /* set error to ERANGE */
      TRACE("CRTDLL_getcwd buffer to small\n");
      return NULL;
    }
  return buf;

}

/*********************************************************************
 *                  _getdcwd           (CRTDLL.121)
 */
CHAR* __cdecl CRTDLL__getdcwd(INT drive,LPSTR buf, INT size)
{
  char test[1];
  int len;

  FIXME("(\"%c:\",%s,%d)\n",drive+'A',buf,size);
  len = size;
  if (!buf) {
    if (size < 0) /* allocate as big as nescessary */
      len =GetCurrentDirectoryA(1,test) + 1;
    if(!(buf = CRTDLL_malloc(len)))
    {
	/* set error to OutOfRange */
	return( NULL );
    }
  }
  size = len;
  if(!(len =GetCurrentDirectoryA(len,buf)))
    {
      return NULL;
    }
  if (len > size)
    {
      /* set error to ERANGE */
      TRACE("buffer to small\n");
      return NULL;
    }
  return buf;

}

/*********************************************************************
 *                  _getdrive           (CRTDLL.124)
 *
 *  Return current drive, 1 for A, 2 for B
 */
INT __cdecl CRTDLL__getdrive(VOID)
{
    return DRIVE_GetCurrentDrive() + 1;
}

/*********************************************************************
 *                  _mkdir           (CRTDLL.234)
 */
INT __cdecl CRTDLL__mkdir(LPCSTR newdir)
{
	if (!CreateDirectoryA(newdir,NULL))
		return -1;
	return 0;
}

/*********************************************************************
 *                  remove           (CRTDLL.448)
 */
INT __cdecl CRTDLL_remove(LPCSTR file)
{
	if (!DeleteFileA(file))
		return -1;
	return 0;
}

/*********************************************************************
 *                  _errno           (CRTDLL.52)
 * Yes, this is a function.
 */
LPINT __cdecl CRTDLL__errno()
{
	static	int crtdllerrno;
	
	/* FIXME: we should set the error at the failing function call time */
	crtdllerrno = LastErrorToErrno(GetLastError());
	return &crtdllerrno;
}

/*********************************************************************
 *                  _tempnam           (CRTDLL.305)
 * 
 */
LPSTR __cdecl CRTDLL__tempnam(LPCSTR dir, LPCSTR prefix)
{

     char *ret;
     DOS_FULL_NAME tempname;
     
     if ((ret = tempnam(dir,prefix))==NULL) {
       WARN("Unable to get unique filename\n");
       return NULL;
     }
     if (!DOSFS_GetFullName(ret,FALSE,&tempname))
     {
       TRACE("Wrong path?\n");
       return NULL;
     }
     free(ret);
     if ((ret = CRTDLL_malloc(strlen(tempname.short_name)+1)) == NULL) {
	 WARN("CRTDL_malloc for shortname failed\n");
	 return NULL;
     }
     if ((ret = strcpy(ret,tempname.short_name)) == NULL) { 
       WARN("Malloc for shortname failed\n");
       return NULL;
     }
     
     TRACE("dir %s prefix %s got %s\n",
		    dir,prefix,ret);
     return ret;

}
/*********************************************************************
 *                  tmpnam           (CRTDLL.490)
 *
 * lcclnk from lcc-win32 relies on a terminating dot in the name returned
 * 
 */
LPSTR __cdecl CRTDLL_tmpnam(LPSTR s)
{
     char *ret;

     if ((ret =tmpnam(s))== NULL) {
       WARN("Unable to get unique filename\n");
       return NULL;
     }
     if (!DOSFS_GetFullName(ret,FALSE,&CRTDLL_tmpname))
     {
       TRACE("Wrong path?\n");
       return NULL;
     }
     strcat(CRTDLL_tmpname.short_name,".");
     TRACE("for buf %p got %s\n",
		    s,CRTDLL_tmpname.short_name);
     TRACE("long got %s\n",
		    CRTDLL_tmpname.long_name);
     if ( s != NULL) 
       return strcpy(s,CRTDLL_tmpname.short_name);
     else 
       return CRTDLL_tmpname.short_name;

}

/*********************************************************************
 *                  _itoa           (CRTDLL.165)
 */
LPSTR  __cdecl CRTDLL__itoa(INT x,LPSTR buf,INT buflen)
{
    wsnprintfA(buf,buflen,"%d",x);
    return buf;
}

/*********************************************************************
 *                  _ltoa           (CRTDLL.180)
 */
LPSTR  __cdecl CRTDLL__ltoa(long x,LPSTR buf,INT radix)
{
    switch(radix) {
        case  2: FIXME("binary format not implemented !\n");
                 break;
        case  8: wsnprintfA(buf,0x80,"%o",x);
                 break;
        case 10: wsnprintfA(buf,0x80,"%d",x);
                 break;
        case 16: wsnprintfA(buf,0x80,"%x",x);
                 break;
        default: FIXME("radix %d not implemented !\n", radix);
    }
    return buf;
}

/*********************************************************************
 *                  _ultoa           (CRTDLL.311)
 */
LPSTR  __cdecl CRTDLL__ultoa(long x,LPSTR buf,INT radix)
{
    switch(radix) {
        case  2: FIXME("binary format not implemented !\n");
                 break;
        case  8: wsnprintfA(buf,0x80,"%lo",x);
                 break;
        case 10: wsnprintfA(buf,0x80,"%ld",x);
                 break;
        case 16: wsnprintfA(buf,0x80,"%lx",x);
                 break;
        default: FIXME("radix %d not implemented !\n", radix);
    }
    return buf;
}

typedef VOID (*sig_handler_type)(VOID);

/*********************************************************************
 *                  signal           (CRTDLL.455)
 */
void * __cdecl CRTDLL_signal(int sig, sig_handler_type ptr)
{
    FIXME("(%d %p):stub.\n", sig, ptr);
    return (void*)-1;
}

/*********************************************************************
 *                  _ftol            (CRTDLL.113)
 */
#ifdef USING_REAL_FPU
LONG __cdecl CRTDLL__ftol(void) {
	/* don't just do DO_FPU("fistp",retval), because the rounding
	 * mode must also be set to "round towards zero"... */
	double fl;
	POP_FPU(fl);
	return (LONG)fl;
}
#else
LONG __cdecl CRTDLL__ftol(double fl) {
	FIXME("should be register function\n");
	return (LONG)fl;
}
#endif

/*********************************************************************
 *                  _CIpow           (CRTDLL.14)
 */
#ifdef USING_REAL_FPU
LONG __cdecl CRTDLL__CIpow(void) {
	double x,y;
	POP_FPU(y);
	POP_FPU(x);
	return pow(x,y);
}
#else
LONG __cdecl CRTDLL__CIpow(double x,double y) {
	FIXME("should be register function\n");
	return pow(x,y);
}
#endif

/*********************************************************************
 *                  _sleep           (CRTDLL.267)
 */
VOID __cdecl CRTDLL__sleep(unsigned long timeout) 
{
  TRACE("CRTDLL__sleep for %ld milliseconds\n",timeout);
  Sleep((timeout)?timeout:1);
}

/*********************************************************************
 *                  getenv           (CRTDLL.437)
 */
LPSTR __cdecl CRTDLL_getenv(const char *name) 
{
     LPSTR environ = GetEnvironmentStringsA();
     LPSTR pp,pos = NULL;
     unsigned int length;
  
     for (pp = environ; (*pp); pp = pp + strlen(pp) +1)
       {
	 pos =strchr(pp,'=');
	 if (pos)
	   length = pos -pp;
	 else
	   length = strlen(pp);
	 if (!strncmp(pp,name,length)) break;
       }
     if ((pp)&& (pos)) 
       {
	 pp = pos+1;
	 TRACE("got %s\n",pp);
       }
     FreeEnvironmentStringsA( environ );
     return pp;
}

/*********************************************************************
 *                  _mbsrchr           (CRTDLL.223)
 */
LPSTR __cdecl CRTDLL__mbsrchr(LPSTR s,CHAR x) {
	/* FIXME: handle multibyte strings */
	return strrchr(s,x);
}

/*********************************************************************
 *                  _memicmp           (CRTDLL.233)(NTDLL.868)
 * A stringcompare, without \0 check
 * RETURNS
 *	-1:if first string is alphabetically before second string
 *	1:if second ''    ''      ''          ''   first   ''
 *      0:if both are equal.
 */
INT __cdecl CRTDLL__memicmp(
	LPCSTR s1,	/* [in] first string */
	LPCSTR s2,	/* [in] second string */
	DWORD len	/* [in] length to compare */
) { 
	int	i;

	for (i=0;i<len;i++) {
		if (tolower(s1[i])<tolower(s2[i]))
			return -1;
		if (tolower(s1[i])>tolower(s2[i]))
			return  1;
	}
	return 0;
}
/*********************************************************************
 *                  __dllonexit           (CRTDLL.25)
 */
VOID __cdecl CRTDLL___dllonexit ()
{	
	FIXME("stub\n");
}

/*********************************************************************
 *                  wcstol           (CRTDLL.520)
 * Like strtol, but for wide character strings.
 */
INT __cdecl CRTDLL_wcstol(LPWSTR s,LPWSTR *end,INT base) {
	LPSTR	sA = HEAP_strdupWtoA(GetProcessHeap(),0,s),endA;
	INT	ret = strtol(sA,&endA,base);

	HeapFree(GetProcessHeap(),0,sA);
	if (end) *end = s+(endA-sA); /* pointer magic checked. */
	return ret;
}
/*********************************************************************
 *                  _strdate          (CRTDLL.283)
 */
LPSTR __cdecl CRTDLL__strdate (LPSTR date)
{	FIXME("%p stub\n", date);
	return 0;
}

/*********************************************************************
 *                  _strtime          (CRTDLL.299)
 */
LPSTR __cdecl CRTDLL__strtime (LPSTR date)
{	FIXME("%p stub\n", date);
	return 0;
}

/*********************************************************************
 *                  _except_handler2  (CRTDLL.78)
 */
INT __cdecl CRTDLL__except_handler2 (
	PEXCEPTION_RECORD rec,
	PEXCEPTION_FRAME frame,
	PCONTEXT context,
	PEXCEPTION_FRAME  *dispatcher)
{
	FIXME ("exception %lx flags=%lx at %p handler=%p %p %p stub\n",
	rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
	frame->Handler, context, dispatcher);
	return ExceptionContinueSearch;
}
