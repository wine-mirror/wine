/*
 * The C RunTime DLL
 * 
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997 Uwe Bonnes
 */

/*
Unresolved issues Uwe Bonnes 970904:
- Handling of Binary/Text Files is crude. If in doubt, use fromdos or recode
- Arguments in crtdll.spec for functions with double argument
- system-call calls another wine process, but without debugging arguments
              and uses the first wine executable in the path
- tested with ftp://ftp.remcomp.com/pub/remcomp/lcc-win32.zip, a C-Compiler
 		for Win32, based on lcc, from Jacob Navia
*/

/* NOTE: This file also implements the wcs* functions. They _ARE_ in 
 * the newer Linux libcs, but use 4 byte wide characters, so are unusable,
 * since we need 2 byte wide characters. - Marcus Meissner, 981031
 */

/* FIXME: all the file handling is hopelessly broken -- AJ */

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <setjmp.h>
#include "win.h"
#include "windows.h"
#include "winerror.h"
#include "debug.h"
#include "module.h"
#include "heap.h"
#include "crtdll.h"
#include "drive.h"
#include "file.h"
#include "except.h"
#include "options.h"
#include "winnls.h"

extern int FILE_GetUnixHandle( HFILE32  );

static DOS_FULL_NAME CRTDLL_tmpname;

UINT32 CRTDLL_argc_dll;         /* CRTDLL.23 */
LPSTR *CRTDLL_argv_dll;         /* CRTDLL.24 */
LPSTR  CRTDLL_acmdln_dll;       /* CRTDLL.38 */
UINT32 CRTDLL_basemajor_dll;    /* CRTDLL.42 */
UINT32 CRTDLL_baseminor_dll;    /* CRTDLL.43 */
UINT32 CRTDLL_baseversion_dll;  /* CRTDLL.44 */
UINT32 CRTDLL_commode_dll;      /* CRTDLL.59 */
LPSTR  CRTDLL_environ_dll;      /* CRTDLL.75 */
UINT32 CRTDLL_fmode_dll;        /* CRTDLL.104 */
UINT32 CRTDLL_osmajor_dll;      /* CRTDLL.241 */
UINT32 CRTDLL_osminor_dll;      /* CRTDLL.242 */
UINT32 CRTDLL_osmode_dll;       /* CRTDLL.243 */
UINT32 CRTDLL_osver_dll;        /* CRTDLL.244 */
UINT32 CRTDLL_osversion_dll;    /* CRTDLL.245 */
UINT32 CRTDLL_winmajor_dll;     /* CRTDLL.329 */
UINT32 CRTDLL_winminor_dll;     /* CRTDLL.330 */
UINT32 CRTDLL_winver_dll;       /* CRTDLL.331 */

BYTE CRTDLL_iob[32*3];  /* FIXME */

typedef VOID (*new_handler_type)(VOID);

static new_handler_type new_handler;

/*********************************************************************
 *                  _GetMainArgs  (CRTDLL.022)
 */
DWORD __cdecl CRTDLL__GetMainArgs(LPDWORD argc,LPSTR **argv,
                                LPSTR *environ,DWORD flag)
{
        char *cmdline;
        char  **xargv;
	int	xargc,i,afterlastspace;
	DWORD	version;

	TRACE(crtdll,"(%p,%p,%p,%ld).\n",
		argc,argv,environ,flag
	);
	CRTDLL_acmdln_dll = cmdline = HEAP_strdupA( GetProcessHeap(), 0,
                                                    GetCommandLine32A() );
 	TRACE(crtdll,"got '%s'\n", cmdline);

	version	= GetVersion32();
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

	i=0;xargv=NULL;xargc=0;afterlastspace=0;
	while (cmdline[i]) {
		if (cmdline[i]==' ') {
			xargv=(char**)HeapReAlloc( GetProcessHeap(), 0, xargv,
                                                   sizeof(char*)*(++xargc));
			cmdline[i]='\0';
			xargv[xargc-1] = HEAP_strdupA( GetProcessHeap(), 0,
                                                       cmdline+afterlastspace);
			i++;
			while (cmdline[i]==' ')
				i++;
			if (cmdline[i])
				afterlastspace=i;
		} else
			i++;
	}
	xargv=(char**)HeapReAlloc( GetProcessHeap(), 0, xargv,
                                   sizeof(char*)*(++xargc));
	cmdline[i]='\0';
	xargv[xargc-1] = HEAP_strdupA( GetProcessHeap(), 0,
                                       cmdline+afterlastspace);
	CRTDLL_argc_dll	= xargc;
	*argc		= xargc;
	CRTDLL_argv_dll	= xargv;
	*argv		= xargv;

	TRACE(crtdll,"found %d arguments\n",
		CRTDLL_argc_dll);
	CRTDLL_environ_dll = *environ = GetEnvironmentStrings32A();
	return 0;
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
  FIXME(crtdll, ":(%s,%p): stub\n",fname,x2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*********************************************************************
 *                  _findnext     (CRTDLL.100)
 * 
 * BUGS
 *   Unimplemented
 */
INT32 __cdecl CRTDLL__findnext(DWORD hand, struct find_t * x2)
{
  FIXME(crtdll, ":(%ld,%p): stub\n",hand,x2);
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
  FIXME(crtdll, ":(%d,%p): stub\n",file,buf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*********************************************************************
 *                  _initterm     (CRTDLL.135)
 */
DWORD __cdecl CRTDLL__initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
	_INITTERMFUN	*current;

	TRACE(crtdll,"(%p,%p)\n",start,end);
	current=start;
	while (current<end) {
		if (*current) (*current)();
		current++;
	}
	return 0;
}

/*********************************************************************
 *                  _fdopen     (CRTDLL.91)
 */
DWORD __cdecl CRTDLL__fdopen(INT32 handle, LPCSTR mode)
{
  FILE *file;

  switch (handle) 
    {
    case 0 : file=stdin;
      break;
    case 1 : file=stdout;
      break;
    case 2 : file=stderr;
      break;
    default:
      file=fdopen(handle,mode);
    }
  TRACE(crtdll, "open handle %d mode %s  got file %p\n",
	       handle, mode, file);
  return (DWORD)file;
}

static FILE *xlat_file_ptr(void *ptr)
{
    unsigned long dif;
    
    /* CRT sizeof(FILE) == 32 */
    dif = ((char *)ptr - (char *)CRTDLL_iob) / 32;
    switch(dif)
    {
	case 0: return stdin;
	case 1: return stdout;
	case 2: return stderr;
    }
    return (FILE*)ptr;
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
    TRACE(crtdll,"(%p,%ld)\n",endframe,nr);
}
/*********************************************************************
 *                  _read     (CRTDLL.256)
 * 
 * BUGS
 *   Unimplemented
 */
INT32 __cdecl CRTDLL__read(INT32 fd, LPVOID buf, UINT32 count)
{
  FIXME(crtdll,":(%d,%p,%d): stub\n",fd,buf,count);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*********************************************************************
 *                  fopen     (CRTDLL.372)
 */
DWORD __cdecl CRTDLL_fopen(LPCSTR path, LPCSTR mode)
{
  FILE *file;
  HFILE32 dos_fildes;
#if 0
  DOS_FULL_NAME full_name;
  
  if (!DOSFS_GetFullName( path, FALSE, &full_name )) {
    WARN(crtdll, "file %s bad name\n",path);
   return 0;
  }
  
  file=fopen(full_name.long_name ,mode);
#endif
  INT32 flagmode=0;
  int unix_fildes=0;

  if ((strchr(mode,'r')&&strchr(mode,'a'))||
      (strchr(mode,'r')&&strchr(mode,'w'))||
      (strchr(mode,'w')&&strchr(mode,'a')))
    return 0;
       
  if (strstr(mode,"r+")) flagmode=O_RDWR;
  else if (strchr(mode,'r')) flagmode = O_RDONLY;
  else if (strstr(mode,"w+")) flagmode= O_RDWR | O_TRUNC | O_CREAT;
  else if (strchr(mode,'w')) flagmode = O_WRONLY | O_TRUNC | O_CREAT;
  else if (strstr(mode,"a+")) flagmode= O_RDWR | O_CREAT | O_APPEND;
  else if (strchr(mode,'w')) flagmode = O_RDWR | O_CREAT | O_APPEND;
  else if (strchr(mode,'b'))
    TRACE(crtdll, "%s in BINARY mode\n",path);
      
  dos_fildes=FILE_Open(path, flagmode,0);
  unix_fildes=FILE_GetUnixHandle(dos_fildes);
  file = fdopen(unix_fildes,mode);

  TRACE(crtdll, "file %s mode %s got ufh %d dfh %d file %p\n",
		 path,mode,unix_fildes,dos_fildes,file);
  return (DWORD)file;
}

/*********************************************************************
 *                  fread     (CRTDLL.377)
 */
DWORD __cdecl CRTDLL_fread(LPVOID ptr, INT32 size, INT32 nmemb, LPVOID vfile)
{
  size_t ret=1;
  FILE *file=xlat_file_ptr(vfile);
#if 0
  int i=0;
  void *temp=ptr;

  /* If we would honour CR/LF <-> LF translation, we could do it like this.
     We should keep track of all files opened, and probably files with \
     known binary extensions must be unchanged */
  while ( (i < (nmemb*size)) && (ret==1)) {
    ret=fread(temp,1,1,file);
    TRACE(crtdll, "got %c 0x%02x ret %d\n",
		 (isalpha(*(unsigned char*)temp))? *(unsigned char*)temp:
		 ' ',*(unsigned char*)temp, ret);
    if (*(unsigned char*)temp != 0xd) { /* skip CR */
      temp++;
      i++;
    }
    else
      TRACE(crtdll, "skipping ^M\n");
  }
  TRACE(crtdll, "0x%08x items of size %d from file %p to %p\n",
	       nmemb,size,file,ptr,);
  if(i!=nmemb)
    WARN(crtdll, " failed!\n");

  return i;
#else
    
  ret=fread(ptr,size,nmemb,file);
  TRACE(crtdll, "0x%08x items of size %d from file %p to %p\n",
	       nmemb,size,file,ptr);
  if(ret!=nmemb)
    WARN(crtdll, " failed!\n");

  return ret;
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
  FIXME(crtdll, ":(%s,%s,%p): stub\n", path, mode, stream);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*********************************************************************
 *                  fscanf     (CRTDLL.381)
 */
INT32 __cdecl CRTDLL_fscanf( LPVOID stream, LPSTR format, ... )
{
    va_list valist;
    INT32 res;

    va_start( valist, format );
#ifdef HAVE_VFSCANF
    res = vfscanf( xlat_file_ptr(stream), format, valist );
#endif
    va_end( valist );
    return res;
}

/*********************************************************************
 *                  fseek     (CRTDLL.382)
 */
LONG __cdecl CRTDLL_fseek(LPVOID stream, LONG offset, INT32 whence)
{
  long ret;

  ret=fseek(xlat_file_ptr(stream),offset,whence);
  TRACE(crtdll, "file %p to 0x%08lx pos %s\n",
	       stream,offset,(whence==SEEK_SET)?"SEEK_SET":
	       (whence==SEEK_CUR)?"SEEK_CUR":
	       (whence==SEEK_END)?"SEEK_END":"UNKNOWN");
  if(ret)
    WARN(crtdll, " failed!\n");

  return ret;
}
  
/*********************************************************************
 *                  fsetpos     (CRTDLL.383)
 */
INT32 __cdecl CRTDLL_fsetpos(LPVOID stream, fpos_t *pos)
{
  TRACE(crtdll, "file %p\n", stream);
  return fseek(xlat_file_ptr(stream), *pos, SEEK_SET);
}

/*********************************************************************
 *                  ftell     (CRTDLL.384)
 */
LONG __cdecl CRTDLL_ftell(LPVOID stream)
{
  long ret;

  ret=ftell(xlat_file_ptr(stream));
  TRACE(crtdll, "file %p at 0x%08lx\n",
	       stream,ret);
  return ret;
}
  
/*********************************************************************
 *                  fwrite     (CRTDLL.386)
 */
DWORD __cdecl CRTDLL_fwrite(LPVOID ptr, INT32 size, INT32 nmemb, LPVOID vfile)
{
  size_t ret;
  FILE *file=xlat_file_ptr(vfile);

  ret=fwrite(ptr,size,nmemb,file);
  TRACE(crtdll, "0x%08x items of size %d from %p to file %p\n",
	       nmemb,size,ptr,file);
  if(ret!=nmemb)
    WARN(crtdll, " Failed!\n");

  return ret;
}

/*********************************************************************
 *                  setbuf     (CRTDLL.452)
 */
INT32 __cdecl CRTDLL_setbuf(LPVOID file, LPSTR buf)
{
  TRACE(crtdll, "(file %p buf %p)\n", file, buf);
  /* this doesn't work:"void value not ignored as it ought to be" 
  return setbuf(file,buf); 
  */
  setbuf(xlat_file_ptr(file),buf);
  return 0;
}

/*********************************************************************
 *                  _open_osfhandle         (CRTDLL.240)
 */
HFILE32 __cdecl CRTDLL__open_osfhandle(LONG osfhandle, INT32 flags)
{
HFILE32 handle;
 
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
	TRACE(crtdll, "(handle %08lx,flags %d) return %d\n",
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
 *                  fprintf       (CRTDLL.373)
 */
INT32 __cdecl CRTDLL_fprintf( FILE *file, LPSTR format, ... )
{
    va_list valist;
    INT32 res;

    va_start( valist, format );
    res = vfprintf( xlat_file_ptr(file), format, valist );
    va_end( valist );
    return res;
}

/*********************************************************************
 *                  vfprintf       (CRTDLL.373)
 */
INT32 __cdecl CRTDLL_vfprintf( FILE *file, LPSTR format, va_list args )
{
    return vfprintf( xlat_file_ptr(file), format, args );
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
 *                            (CRTDLL.350)
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
BOOL32 __cdecl CRTDLL__isatty(DWORD x)
{
	TRACE(crtdll,"(%ld)\n",x);
	return TRUE;
}

/*********************************************************************
 *                  _write        (CRTDLL.332)
 */
INT32 __cdecl CRTDLL__write(INT32 fd,LPCVOID buf,UINT32 count)
{
        INT32 len=0;

	if (fd == -1)
	  len = -1;
	else if (fd<=2)
	  len = (UINT32)write(fd,buf,(LONG)count);
	else
	  len = _lwrite32(fd,buf,count);
	TRACE(crtdll,"%d/%d byte to dfh %d from %p,\n",
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
void __cdecl CRTDLL__cexit(INT32 ret)
{
        TRACE(crtdll,"(%d)\n",ret);
	ExitProcess(ret);
}


/*********************************************************************
 *                  exit          (CRTDLL.359)
 */
void __cdecl CRTDLL_exit(DWORD ret)
{
        TRACE(crtdll,"(%ld)\n",ret);
	ExitProcess(ret);
}


/*********************************************************************
 *                  _abnormal_termination          (CRTDLL.36)
 */
INT32 __cdecl CRTDLL__abnormal_termination(void)
{
        TRACE(crtdll,"(void)\n");
	return 0;
}


/*********************************************************************
 *                  _access          (CRTDLL.37)
 */
INT32 __cdecl CRTDLL__access(LPCSTR filename, INT32 mode)
{
    DWORD attr = GetFileAttributes32A(filename);

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
INT32 __cdecl CRTDLL_fflush(LPVOID stream)
{
    int ret;

    ret = fflush(xlat_file_ptr(stream));
    TRACE(crtdll,"%p returnd %d\n",stream,ret);
    if(ret)
      WARN(crtdll, " Failed!\n");

    return ret;
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

    TRACE(crtdll,"got '%s'\n", buf_start);
    return buf_start;
}


/*********************************************************************
 *                  rand          (CRTDLL.446)
 */
INT32 __cdecl CRTDLL_rand()
{
    return rand();
}


/*********************************************************************
 *                  putchar       (CRTDLL.442)
 */
void __cdecl CRTDLL_putchar( INT32 x )
{
    putchar(x);
}


/*********************************************************************
 *                  fputc       (CRTDLL.374)
 */
INT32 __cdecl CRTDLL_fputc( INT32 c, FILE *stream )
{
    TRACE(crtdll, "%c to file %p\n",c,stream);
    return fputc(c,xlat_file_ptr(stream));
}


/*********************************************************************
 *                  fputs       (CRTDLL.375)
 */
INT32 __cdecl CRTDLL_fputs( LPCSTR s, FILE *stream )
{
    TRACE(crtdll, "%s to file %p\n",s,stream);
    return fputs(s,xlat_file_ptr(stream));
}


/*********************************************************************
 *                  puts       (CRTDLL.443)
 */
INT32 __cdecl CRTDLL_puts(LPCSTR s)
{
    TRACE(crtdll, "%s \n",s);
    return puts(s);
}


/*********************************************************************
 *                  putc       (CRTDLL.441)
 */
INT32 __cdecl CRTDLL_putc(INT32 c, FILE *stream)
{
    TRACE(crtdll, " %c to file %p\n",c,stream);
    return fputc(c,xlat_file_ptr(stream));
}
/*********************************************************************
 *                  fgetc       (CRTDLL.366)
 */
INT32 __cdecl CRTDLL_fgetc( FILE *stream )
{
  int ret= fgetc(xlat_file_ptr(stream));
  TRACE(crtdll, "got %d\n",ret);
  return ret;
}


/*********************************************************************
 *                  getc       (CRTDLL.388)
 */
INT32 __cdecl CRTDLL_getc( FILE *stream )
{
  int ret= fgetc(xlat_file_ptr(stream));
  TRACE(crtdll, "got %d\n",ret);
  return ret;
}

/*********************************************************************
 *                  _rotl          (CRTDLL.259)
 */
UINT32 __cdecl CRTDLL__rotl(UINT32 x,INT32 shift)
{
   unsigned int ret = (x >> shift)|( x >>((sizeof(x))-shift));

   TRACE(crtdll, "got 0x%08x rot %d ret 0x%08x\n",
		  x,shift,ret);
   return ret;
    
}
/*********************************************************************
 *                  _lrotl          (CRTDLL.176)
 */
DWORD __cdecl CRTDLL__lrotl(DWORD x,INT32 shift)
{
   unsigned long ret = (x >> shift)|( x >>((sizeof(x))-shift));

   TRACE(crtdll, "got 0x%08lx rot %d ret 0x%08lx\n",
		  x,shift,ret);
   return ret;
    
}


/*********************************************************************
 *                  fgets       (CRTDLL.368)
 */
CHAR* __cdecl CRTDLL_fgets(LPSTR s,INT32 size, LPVOID stream)
{
  char * ret;
  char * control_M;
  
  ret=fgets(s, size,xlat_file_ptr(stream));
  /*FIXME: Control with CRTDLL_setmode */
  control_M= strrchr(s,'\r');
  /*delete CR if we read a DOS File */
  if (control_M)
    {
      *control_M='\n';
      *(control_M+1)=0;
    }
  TRACE(crtdll, "got %s for %d chars from file %p\n",
		 s,size,stream);
  if(ret)
    WARN(crtdll, " Failed!\n");

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
 *                  _mbsinc       (CRTDLL.205)
 */
unsigned char * __cdecl CRTDLL__mbsinc(unsigned char *x)
{
    /* FIXME: mbcs */
    return x++;
}


/*********************************************************************
 *                  vsprintf      (CRTDLL.500)
 */
INT32 __cdecl CRTDLL_vsprintf( LPSTR buffer, LPCSTR spec, va_list args )
{
    return wvsprintf32A( buffer, spec, args );
}

/*********************************************************************
 *                  vswprintf      (CRTDLL.501)
 */
INT32 __cdecl CRTDLL_vswprintf( LPWSTR buffer, LPCWSTR spec, va_list args )
{
    return wvsprintf32W( buffer, spec, args );
}

/*********************************************************************
 *                  _mbscpy       (CRTDLL.200)
 */
unsigned char* __cdecl CRTDLL__mbscpy(unsigned char *x,unsigned char *y)
{
    TRACE(crtdll,"CRTDLL_mbscpy %s and %s\n",x,y);
    return strcpy(x,y);
}


/*********************************************************************
 *                  _mbscat       (CRTDLL.197)
 */
unsigned char* __cdecl CRTDLL__mbscat(unsigned char *x,unsigned char *y)
{
    return strcat(x,y);
}


/*********************************************************************
 *                  _strcmpi   (CRTDLL.282) (CRTDLL.287)
 */
INT32 __cdecl CRTDLL__strcmpi( LPCSTR s1, LPCSTR s2 )
{
    return lstrcmpi32A( s1, s2 );
}


/*********************************************************************
 *                  _strnicmp   (CRTDLL.293)
 */
INT32 __cdecl CRTDLL__strnicmp( LPCSTR s1, LPCSTR s2, INT32 n )
{
    return lstrncmpi32A( s1, s2, n );
}


/*********************************************************************
 *                  _strlwr      (CRTDLL.293)
 *
 * convert a string in place to lowercase 
 */
LPSTR __cdecl CRTDLL__strlwr(LPSTR x)
{
  unsigned char *y =x;
  
  TRACE(crtdll, "CRTDLL_strlwr got %s\n", x);
  while (*y) {
    if ((*y > 0x40) && (*y< 0x5b))
      *y = *y + 0x20;
    y++;
  }
  TRACE(crtdll, "   returned %s\n", x);
		 
  return x;
}

/*********************************************************************
 *                  system       (CRTDLL.485)
 */
INT32 __cdecl CRTDLL_system(LPSTR x)
{
#define SYSBUF_LENGTH 1500
  char buffer[SYSBUF_LENGTH];
  unsigned char *y = x;
  unsigned char *bp;
  int i;

  sprintf( buffer, "%s \"", Options.argv0 );
  bp = buffer + strlen(buffer);
  i = strlen(buffer) + strlen(x) +2;

  /* Calculate needed buffer size to prevent overflow.  */
  while (*y) {
    if (*y =='\\') i++;
    y++;
  }
  /* If buffer too short, exit.  */
  if (i > SYSBUF_LENGTH) {
    TRACE(crtdll,"_system buffer to small\n");
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
  TRACE(crtdll, "_system got '%s', executing '%s'\n",x,buffer);

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
 *                  _wcsupr       (CRTDLL.328)
 */
LPWSTR __cdecl CRTDLL__wcsupr(LPWSTR x)
{
	LPWSTR	y=x;

	while (*y) {
		*y=towupper(*y);
		y++;
	}
	return x;
}

/*********************************************************************
 *                  _wcslwr       (CRTDLL.323)
 */
LPWSTR __cdecl CRTDLL__wcslwr(LPWSTR x)
{
	LPWSTR	y=x;

	while (*y) {
		*y=towlower(*y);
		y++;
	}
	return x;
}


/*********************************************************************
 *                  longjmp        (CRTDLL.426)
 */
VOID __cdecl CRTDLL_longjmp(jmp_buf env, int val)
{
    FIXME(crtdll,"CRTDLL_longjmp semistup, expect crash\n");
    return longjmp(env, val);
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
 *                  _wcsdup          (CRTDLL.320)
 */
LPWSTR __cdecl CRTDLL__wcsdup(LPCWSTR ptr)
{
    return HEAP_strdupW(GetProcessHeap(),0,ptr);
}

/*********************************************************************
 *                  fclose           (CRTDLL.362)
 */
INT32 __cdecl CRTDLL_fclose( FILE *stream )
{
    int unix_handle;
    HFILE32 dos_handle=1;
    HFILE32 ret=EOF;

    stream=xlat_file_ptr(stream);
    unix_handle=fileno(stream);

    if (unix_handle<4) ret= fclose(stream);
    else {
      while(FILE_GetUnixHandle(dos_handle) != unix_handle) dos_handle++;
      fclose(stream);
      ret = _lclose32( dos_handle);
    }
    TRACE(crtdll,"(%p) ufh %d dfh %d\n",
		   stream,unix_handle,dos_handle);

    if(ret)
      WARN(crtdll, " Failed!\n");

    return ret;
}

/*********************************************************************
 *                  _unlink           (CRTDLL.315)
 */
INT32 __cdecl CRTDLL__unlink(LPCSTR pathname)
{
    int ret=0;
    DOS_FULL_NAME full_name;

    if (!DOSFS_GetFullName( pathname, FALSE, &full_name )) {
      WARN(crtdll, "CRTDLL_unlink file %s bad name\n",pathname);
      return EOF;
    }
  
    ret=unlink(full_name.long_name);
    TRACE(crtdll,"(%s unix %s)\n",
		   pathname,full_name.long_name);
    if(ret)
      WARN(crtdll, " Failed!\n");

    return ret;
}

/*********************************************************************
 *                  rename           (CRTDLL.449)
 */
INT32 __cdecl CRTDLL_rename(LPCSTR oldpath,LPCSTR newpath)
{
    BOOL32 ok = MoveFileEx32A( oldpath, newpath, MOVEFILE_REPLACE_EXISTING );
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
    UINT32 win_st_rdev;
    INT32  win_st_size;
    INT32  win_st_atime;
    INT32  win_st_mtime;
    INT32  win_st_ctime;
};

int __cdecl CRTDLL__stat(const char * filename, struct win_stat * buf)
{
    int ret=0;
    DOS_FULL_NAME full_name;
    struct stat mystat;

    if (!DOSFS_GetFullName( filename, TRUE, &full_name ))
    {
      WARN(crtdll, "CRTDLL__stat filename %s bad name\n",filename);
      return -1;
    }
    ret=stat(full_name.long_name,&mystat);
    TRACE(crtdll,"CRTDLL__stat %s\n", filename);
    if(ret) 
      WARN(crtdll, " Failed!\n");

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
HFILE32 __cdecl CRTDLL__open(LPCSTR path,INT32 flags)
{
    HFILE32 ret=0;
    int wineflags=0;
    
    /* FIXME:
       the flags in lcc's header differ from the ones in Linux, e.g.
       Linux: define O_APPEND         02000   (= 0x400)
       lcc:  define _O_APPEND       0x0008  
       so here a scheme to translate them
       Probably lcc is wrong here, but at least a hack to get is going
       */
    wineflags = (flags & 3);
    if (flags & 0x0008 ) wineflags |= O_APPEND;
    if (flags & 0x0100 ) wineflags |= O_CREAT;
    if (flags & 0x0200 ) wineflags |= O_TRUNC;
    if (flags & 0x0400 ) wineflags |= O_EXCL;
    if (flags & 0xf0f4 ) 
      TRACE(crtdll,"CRTDLL_open file unsupported flags 0x%04x\n",flags);
    /* End Fixme */

    ret = FILE_Open(path,wineflags,0);
    TRACE(crtdll,"CRTDLL_open file %s mode 0x%04x (lccmode 0x%04x) got dfh %d\n",
		   path,wineflags,flags,ret);
    return ret;
}

/*********************************************************************
 *                  _close           (CRTDLL.57)
 */
INT32 __cdecl CRTDLL__close(HFILE32 fd)
{
    int ret=_lclose32(fd);

    TRACE(crtdll,"(%d)\n",fd);
    if(ret)
      WARN(crtdll, " Failed!\n");

    return ret;
}

/*********************************************************************
 *                  feof           (CRTDLL.363)
 */
INT32 __cdecl CRTDLL_feof( FILE *stream )
{
    int ret;
    
    ret=feof(xlat_file_ptr(stream));
    TRACE(crtdll,"(%p) %s\n",stream,(ret)?"true":"false");
    return ret;
}

/*********************************************************************
 *                  setlocale           (CRTDLL.453)
 */
LPSTR __cdecl CRTDLL_setlocale(INT32 category,LPCSTR locale)
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
	FIXME(crtdll,"(%s,%s),stub!\n",categorystr,locale);
	return "C";
}

/*********************************************************************
 *                  wcscat           (CRTDLL.503)
 */
LPWSTR __cdecl CRTDLL_wcscat( LPWSTR s1, LPCWSTR s2 )
{
    return lstrcat32W( s1, s2 );
}

/*********************************************************************
 *                  wcschr           (CRTDLL.504)
 */
LPWSTR __cdecl CRTDLL_wcschr(LPCWSTR str,WCHAR xchar)
{
	LPCWSTR	s;

	s=str;
	do {
		if (*s==xchar)
			return (LPWSTR)s;
	} while (*s++);
	return NULL;
}

/*********************************************************************
 *                  wcscmp           (CRTDLL.505)
 */
INT32 __cdecl CRTDLL_wcscmp( LPCWSTR s1, LPCWSTR s2 )
{
    return lstrcmp32W( s1, s2 );
}

/*********************************************************************
 *                  wcscpy           (CRTDLL.507)
 */
LPWSTR __cdecl CRTDLL_wcscpy( LPWSTR s1, LPCWSTR s2 )
{
    return lstrcpy32W( s1, s2 );
}

/*********************************************************************
 *                  wcscspn           (CRTDLL.508)
 */
INT32 __cdecl CRTDLL_wcscspn(LPWSTR str,LPWSTR reject)
{
	LPWSTR	s,t;

	s=str;
	do {
		t=reject;
		while (*t) { if (*t==*s) break;t++;}
		if (*t) break;
		s++;
	} while (*s);
	return s-str; /* nr of wchars */
}

/*********************************************************************
 *                  wcslen           (CRTDLL.510)
 */
INT32 __cdecl CRTDLL_wcslen( LPCWSTR s )
{
    return lstrlen32W( s );
}

/*********************************************************************
 *                  wcsncat           (CRTDLL.511)
 */
LPWSTR __cdecl CRTDLL_wcsncat( LPWSTR s1, LPCWSTR s2, INT32 n )
{
    return lstrcatn32W( s1, s2, n );
}

/*********************************************************************
 *                  wcsncmp           (CRTDLL.512)
 */
INT32 __cdecl CRTDLL_wcsncmp( LPCWSTR s1, LPCWSTR s2, INT32 n )
{
    return lstrncmp32W( s1, s2, n );
}

/*********************************************************************
 *                  wcsncpy           (CRTDLL.513)
 */
LPWSTR __cdecl CRTDLL_wcsncpy( LPWSTR s1, LPCWSTR s2, INT32 n )
{
    return lstrcpyn32W( s1, s2, n );
}

/*********************************************************************
 *                  wcsspn           (CRTDLL.516)
 */
INT32 __cdecl CRTDLL_wcsspn(LPWSTR str,LPWSTR accept)
{
	LPWSTR	s,t;

	s=str;
	do {
		t=accept;
		while (*t) { if (*t==*s) break;t++;}
		if (!*t) break;
		s++;
	} while (*s);
	return s-str; /* nr of wchars */
}

/*********************************************************************
 *                  _wcsicmp           (CRTDLL.321)
 */
DWORD __cdecl CRTDLL__wcsicmp( LPCWSTR s1, LPCWSTR s2 )
{
    return lstrcmpi32W( s1, s2 );
}

/*********************************************************************
 *                  _wcsicoll           (CRTDLL.322)
 */
DWORD __cdecl CRTDLL__wcsicoll(LPCWSTR a1,LPCWSTR a2)
{
    /* FIXME: handle collates */
    return lstrcmpi32W(a1,a2);
}

/*********************************************************************
 *                  _wcsnicmp           (CRTDLL.324)
 */
DWORD __cdecl CRTDLL__wcsnicmp( LPCWSTR s1, LPCWSTR s2, INT32 len )
{
    return lstrncmpi32W( s1, s2, len );
}

/*********************************************************************
 *                  wcscoll           (CRTDLL.506)
 */
DWORD __cdecl CRTDLL_wcscoll(LPWSTR a1,LPWSTR a2)
{
    /* FIXME: handle collates */
    return lstrcmp32W(a1,a2);
}

/*********************************************************************
 *                  _wcsrev           (CRTDLL.326)
 */
VOID __cdecl CRTDLL__wcsrev(LPWSTR s) {
	LPWSTR	e;

	e=s;
	while (*e)
		e++;
	while (s<e) {
		WCHAR	a;

		a=*s;*s=*e;*e=a;
		s++;e--;
	}
}

/*********************************************************************
 *                  wcsstr           (CRTDLL.517)
 */
LPWSTR __cdecl CRTDLL_wcsstr(LPWSTR s,LPWSTR b)
{
	LPWSTR	x,y,c;

	x=s;
	while (*x) {
		if (*x==*b) {
			y=x;c=b;
			while (*y && *c && *y==*c) { c++;y++; }
			if (!*c)
				return x;
		}
		x++;
	}
	return NULL;
}

/*********************************************************************
 *                  wcstombs   (CRTDLL.521)
 */
INT32 __cdecl CRTDLL_wcstombs( LPSTR dst, LPCWSTR src, INT32 len )
{
    lstrcpynWtoA( dst, src, len );
    return strlen(dst);  /* FIXME: is this right? */
}

/*********************************************************************
 *                  wcsrchr           (CRTDLL.515)
 */
LPWSTR __cdecl CRTDLL_wcsrchr(LPWSTR str,WCHAR xchar)
{
	LPWSTR	s;

	s=str+lstrlen32W(str);
	do {
		if (*s==xchar)
			return s;
		s--;
	} while (s>=str);
	return NULL;
}

/*********************************************************************
 *                  _setmode           (CRTDLL.265)
 * FIXME: At present we ignore the request to translate CR/LF to LF.
 *
 * We allways translate when we read with fgets, we never do with fread
 *
 */
INT32 __cdecl CRTDLL__setmode( INT32 fh,INT32 mode)
{
	/* FIXME */
#define O_TEXT     0x4000
#define O_BINARY   0x8000

	FIXME(crtdll, "on fhandle %d mode %s, STUB.\n",
		      fh,(mode=O_TEXT)?"O_TEXT":
		      (mode=O_BINARY)?"O_BINARY":"UNKNOWN");
	return -1;
}

/*********************************************************************
 *                  _fpreset           (CRTDLL.107)
 */
VOID __cdecl CRTDLL__fpreset(void)
{
       FIXME(crtdll," STUB.\n");
}

/*********************************************************************
 *                  atexit           (CRTDLL.345)
 */
INT32 __cdecl CRTDLL_atexit(LPVOID x)
{
	FIXME(crtdll,"(%p), STUB.\n",x);
	return 0; /* successful */
}

/*********************************************************************
 *                  mblen          (CRTDLL.428)
 * FIXME: check multibyte support
 */
WCHAR  __cdecl CRTDLL_mblen(CHAR *mb,INT32 size)
{

    int ret=1;
    
    if (!mb)
      ret = 0;
    else if ((size<1)||(!*(mb+1)))
      ret = -1;
    else if (!(*mb))
      ret =0;
      
    TRACE(crtdll,"CRTDLL_mlen %s for max %d bytes ret %d\n",mb,size,ret);

    return ret;
}

/*********************************************************************
 *                  mbstowcs           (CRTDLL.429)
 * FIXME: check multibyte support
 */
INT32 __cdecl CRTDLL_mbstowcs(LPWSTR wcs, LPCSTR mbs, INT32 size)
{

/* Slightly modified lstrcpynAtoW functions from memory/strings.c
 *  We need the number of characters transfered 
 *  FIXME: No multibyte support yet
 */

    LPWSTR p = wcs;
    LPCSTR src= mbs;
    int ret, n=size;

    while ((n-- > 0) && *src) {
      *p++ = (WCHAR)(unsigned char)*src++;
	}
    p++;
    ret = (p -wcs);
          
    TRACE(crtdll,"CRTDLL_mbstowcs %s for %d chars put %d wchars\n",
		   mbs,size,ret);
    return ret;
}

/*********************************************************************
 *                  mbtowc           (CRTDLL.430)
 * FIXME: check multibyte support
 */
WCHAR __cdecl CRTDLL_mbtowc(WCHAR* wc,CHAR* mb,INT32 size) 
{
   int ret;

   if (!mb)
     ret = 0;
   else if (!wc)
     ret =-1;
   else 
     if ( (ret = mblen(mb,size)) != -1 )
       {
	 if (ret <= sizeof(char))
	   *wc = (WCHAR) ((unsigned char)*mb);
        else
        ret=   -1;
        }   
     else
       ret = -1;
   
   TRACE(crtdll,"CRTDLL_mbtowc %s for %d chars\n",mb,size);
         
   return ret;
}

/*********************************************************************
 *                  _isctype           (CRTDLL.138)
 */
BOOL32 __cdecl CRTDLL__isctype(CHAR x,CHAR type)
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
BOOL32 __cdecl CRTDLL__chdrive(INT32 newdrive)
{
	/* FIXME: generates errnos */
	return DRIVE_SetCurrentDrive(newdrive-1);
}

/*********************************************************************
 *                  _chdir           (CRTDLL.51)
 */
INT32 __cdecl CRTDLL__chdir(LPCSTR newdir)
{
	if (!SetCurrentDirectory32A(newdir))
		return 1;
	return 0;
}

/*********************************************************************
 *                  _fullpath           (CRTDLL.114)
 */
LPSTR __cdecl CRTDLL__fullpath(LPSTR buf, LPCSTR name, INT32 size)
{
  DOS_FULL_NAME full_name;

  if (!buf)
  {
      size = 256;
      if(!(buf = CRTDLL_malloc(size))) return NULL;
  }
  if (!DOSFS_GetFullName( name, FALSE, &full_name )) return NULL;
  lstrcpyn32A(buf,full_name.short_name,size);
  TRACE(crtdll,"CRTDLL_fullpath got %s\n",buf);
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

  TRACE(crtdll,"CRTDLL__splitpath got %s\n",path);

  drivechar  = strchr(path,':');
  dirchar    = strrchr(path,'/');
  namechar   = strrchr(path,'\\');
  dirchar = MAX(dirchar,namechar);
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

  TRACE(crtdll,"CRTDLL__splitpath found %s %s %s %s\n",drive,directory,filename,extension);
  
}

/*********************************************************************
 *                  _getcwd           (CRTDLL.120)
 */
CHAR* __cdecl CRTDLL__getcwd(LPSTR buf, INT32 size)
{
  char test[1];
  int len;

  len = size;
  if (!buf) {
    if (size < 0) /* allocate as big as nescessary */
      len =GetCurrentDirectory32A(1,test) + 1;
    if(!(buf = CRTDLL_malloc(len)))
    {
	/* set error to OutOfRange */
	return( NULL );
    }
  }
  size = len;
  if(!(len =GetCurrentDirectory32A(len,buf)))
    {
      return NULL;
    }
  if (len > size)
    {
      /* set error to ERANGE */
      TRACE(crtdll,"CRTDLL_getcwd buffer to small\n");
      return NULL;
    }
  return buf;

}

/*********************************************************************
 *                  _getdcwd           (CRTDLL.121)
 */
CHAR* __cdecl CRTDLL__getdcwd(INT32 drive,LPSTR buf, INT32 size)
{
  char test[1];
  int len;

  FIXME(crtdll,"(\"%c:\",%s,%d)\n",drive+'A',buf,size);
  len = size;
  if (!buf) {
    if (size < 0) /* allocate as big as nescessary */
      len =GetCurrentDirectory32A(1,test) + 1;
    if(!(buf = CRTDLL_malloc(len)))
    {
	/* set error to OutOfRange */
	return( NULL );
    }
  }
  size = len;
  if(!(len =GetCurrentDirectory32A(len,buf)))
    {
      return NULL;
    }
  if (len > size)
    {
      /* set error to ERANGE */
      TRACE(crtdll,"buffer to small\n");
      return NULL;
    }
  return buf;

}

/*********************************************************************
 *                  _getdrive           (CRTDLL.124)
 *
 *  Return current drive, 1 for A, 2 for B
 */
INT32 __cdecl CRTDLL__getdrive(VOID)
{
    return DRIVE_GetCurrentDrive() + 1;
}

/*********************************************************************
 *                  _mkdir           (CRTDLL.234)
 */
INT32 __cdecl CRTDLL__mkdir(LPCSTR newdir)
{
	if (!CreateDirectory32A(newdir,NULL))
		return -1;
	return 0;
}

/*********************************************************************
 *                  remove           (CRTDLL.448)
 */
INT32 __cdecl CRTDLL_remove(LPCSTR file)
{
	if (!DeleteFile32A(file))
		return -1;
	return 0;
}

/*********************************************************************
 *                  _errno           (CRTDLL.52)
 * Yes, this is a function.
 */
LPINT32 __cdecl CRTDLL__errno()
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
       WARN(crtdll, "Unable to get unique filename\n");
       return NULL;
     }
     if (!DOSFS_GetFullName(ret,FALSE,&tempname))
     {
       TRACE(crtdll, "Wrong path?\n");
       return NULL;
     }
     free(ret);
     if ((ret = CRTDLL_malloc(strlen(tempname.short_name)+1)) == NULL) {
	 WARN(crtdll, "CRTDL_malloc for shortname failed\n");
	 return NULL;
     }
     if ((ret = strcpy(ret,tempname.short_name)) == NULL) { 
       WARN(crtdll, "Malloc for shortname failed\n");
       return NULL;
     }
     
     TRACE(crtdll,"dir %s prefix %s got %s\n",
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
       WARN(crtdll, "Unable to get unique filename\n");
       return NULL;
     }
     if (!DOSFS_GetFullName(ret,FALSE,&CRTDLL_tmpname))
     {
       TRACE(crtdll, "Wrong path?\n");
       return NULL;
     }
     strcat(CRTDLL_tmpname.short_name,".");
     TRACE(crtdll,"for buf %p got %s\n",
		    s,CRTDLL_tmpname.short_name);
     TRACE(crtdll,"long got %s\n",
		    CRTDLL_tmpname.long_name);
     if ( s != NULL) 
       return strcpy(s,CRTDLL_tmpname.short_name);
     else 
       return CRTDLL_tmpname.short_name;

}

/*********************************************************************
 *                  _itoa           (CRTDLL.165)
 */
LPSTR  __cdecl CRTDLL__itoa(INT32 x,LPSTR buf,INT32 buflen)
{
    wsnprintf32A(buf,buflen,"%d",x);
    return buf;
}

/*********************************************************************
 *                  _ltoa           (CRTDLL.180)
 */
LPSTR  __cdecl CRTDLL__ltoa(long x,LPSTR buf,INT32 radix)
{
    switch(radix) {
        case  2: FIXME(crtdll, "binary format not implemented !\n");
                 break;
        case  8: wsnprintf32A(buf,0x80,"%o",x);
                 break;
        case 10: wsnprintf32A(buf,0x80,"%d",x);
                 break;
        case 16: wsnprintf32A(buf,0x80,"%x",x);
                 break;
        default: FIXME(crtdll, "radix %d not implemented !\n", radix);
    }
    return buf;
}

typedef VOID (*sig_handler_type)(VOID);

/*********************************************************************
 *                  signal           (CRTDLL.455)
 */
VOID __cdecl CRTDLL_signal(int sig, sig_handler_type ptr)
{
    FIXME(crtdll, "(%d %p):stub.\n", sig, ptr);
}

/*********************************************************************
 *                  _ftol           (CRTDLL.113)
 */
LONG __cdecl CRTDLL__ftol(double fl) {
	return (LONG)fl;
}
/*********************************************************************
 *                  _sleep           (CRTDLL.267)
 */
VOID __cdecl CRTDLL__sleep(unsigned long timeout) 
{
  TRACE(crtdll,"CRTDLL__sleep for %ld milliseconds\n",timeout);
  Sleep((timeout)?timeout:1);
}

/*********************************************************************
 *                  getenv           (CRTDLL.437)
 */
LPSTR __cdecl CRTDLL_getenv(const char *name) 
{
     LPSTR environ = GetEnvironmentStrings32A();
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
	 TRACE(crtdll,"got %s\n",pp);
       }
     FreeEnvironmentStrings32A( environ );
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
INT32 __cdecl CRTDLL__memicmp(
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
VOID __cdecl CRTDLL__dllonexit ()
{	
	FIXME(crtdll,"stub\n");
}

/*********************************************************************
 *                  wcstok           (CRTDLL.519)
 * Like strtok, but for wide character strings. s is modified, yes.
 */
LPWSTR CRTDLL_wcstok(LPWSTR s,LPCWSTR delim) {
	static LPWSTR nexttok = NULL;
	LPWSTR	x,ret;

	if (!s)
		s = nexttok;
	if (!s)	
		return NULL;
	x = s;
	while (*x && !CRTDLL_wcschr(delim,*x))
		x++;
	ret = nexttok;
	if (*x) {
		*x='\0';
		nexttok = x+1;
	} else
		nexttok = NULL;
	return ret;
}

/*********************************************************************
 *                  wcstol           (CRTDLL.520)
 * Like strtol, but for wide character strings.
 */
INT32 CRTDLL_wcstol(LPWSTR s,LPWSTR *end,INT32 base) {
	LPSTR	sA = HEAP_strdupWtoA(GetProcessHeap(),0,s),endA;
	INT32	ret = strtol(sA,&endA,base);

	HeapFree(GetProcessHeap(),0,sA);
	if (end) *end = s+(endA-sA); /* pointer magic checked. */
	return ret;
}
/*********************************************************************
 *                  strdate           (CRTDLL.283)
 */
LPSTR __cdecl CRTDLL__strdate (LPSTR date)
{	FIXME (crtdll,"%p stub\n", date);
	return 0;
}
/*********************************************************************
 *                  strtime           (CRTDLL.299)
 */
LPSTR __cdecl CRTDLL__strtime (LPSTR date)
{	FIXME (crtdll,"%p stub\n", date);
	return 0;
}
