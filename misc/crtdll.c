/*
 * The C RunTime DLL
 * 
 * Implements C run-time functionality as known from UNIX.
 *
 * Copyright 1996 Marcus Meissner
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

/* FIXME: all the file handling is hopelessly broken -- AJ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include "win.h"
#include "windows.h"
#include "stddebug.h"
#include "debug.h"
#include "module.h"
#include "xmalloc.h"
#include "heap.h"
#include "crtdll.h"
#include "drive.h"
#include "file.h"

extern int FILE_GetUnixHandle( HFILE32  );

static DOS_FULL_NAME CRTDLL_tmpname;

extern INT32 WIN32_wsprintf32W( DWORD *args );

UINT32 CRTDLL_argc_dll;         /* CRTDLL.23 */
LPSTR *CRTDLL_argv_dll;         /* CRTDLL.24 */
LPSTR  CRTDLL_acmdln_dll;       /* CRTDLL.38 */
UINT32 CRTDLL_basemajor_dll;    /* CRTDLL.42 */
UINT32 CRTDLL_baseminor_dll;    /* CRTDLL.43 */
UINT32 CRTDLL_baseversion_dll;  /* CRTDLL.44 */
LPSTR  CRTDLL_environ_dll;      /* CRTDLL.75 */
UINT32 CRTDLL_osmajor_dll;      /* CRTDLL.241 */
UINT32 CRTDLL_osminor_dll;      /* CRTDLL.242 */
UINT32 CRTDLL_osver_dll;        /* CRTDLL.244 */
UINT32 CRTDLL_osversion_dll;    /* CRTDLL.245 */
UINT32 CRTDLL_winmajor_dll;     /* CRTDLL.329 */
UINT32 CRTDLL_winminor_dll;     /* CRTDLL.330 */
UINT32 CRTDLL_winver_dll;       /* CRTDLL.331 */

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

	dprintf_crtdll(stddeb,"CRTDLL__GetMainArgs(%p,%p,%p,%ld).\n",
		argc,argv,environ,flag
	);
	CRTDLL_acmdln_dll = cmdline = xstrdup( GetCommandLine32A() );
 	dprintf_crtdll(stddeb,"CRTDLL__GetMainArgs got \"%s\"\n",
		cmdline);

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
			xargv=(char**)xrealloc(xargv,sizeof(char*)*(++xargc));
			cmdline[i]='\0';
			xargv[xargc-1] = xstrdup(cmdline+afterlastspace);
			i++;
			while (cmdline[i]==' ')
				i++;
			if (cmdline[i])
				afterlastspace=i;
		} else
			i++;
	}
	xargv=(char**)xrealloc(xargv,sizeof(char*)*(++xargc));
	cmdline[i]='\0';
	xargv[xargc-1] = xstrdup(cmdline+afterlastspace);
	CRTDLL_argc_dll	= xargc;
	*argc		= xargc;
	CRTDLL_argv_dll	= xargv;
	*argv		= xargv;

	dprintf_crtdll(stddeb,"CRTDLL__GetMainArgs found %d arguments\n",
		CRTDLL_argc_dll);
	/* FIXME ... use real environment */
	*environ	= xmalloc(sizeof(LPSTR));
	CRTDLL_environ_dll = *environ;
	(*environ)[0] = NULL;
	return 0;
}


typedef void (*_INITTERMFUN)();

/*********************************************************************
 *                  _initterm     (CRTDLL.135)
 */
DWORD __cdecl CRTDLL__initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
	_INITTERMFUN	*current;

	dprintf_crtdll(stddeb,"_initterm(%p,%p)\n",start,end);
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
  dprintf_crtdll(stddeb,
		 "CRTDLL_fdopen open handle %d mode %s  got file %p\n",
		 handle, mode, file);
  return (DWORD)file;
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
    dprintf_crtdll(stddeb,"CRTDLL_fopen file %s bad name\n",path);
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
    dprintf_crtdll(stderr,
		   "CRTDLL_fopen %s in BINARY mode\n",path);
      
  dos_fildes=FILE_Open(path, flagmode);
  unix_fildes=FILE_GetUnixHandle(dos_fildes);
  file = fdopen(unix_fildes,mode);

  dprintf_crtdll(stddeb,
		 "CRTDLL_fopen file %s mode %s got ufh %d dfh %d file %p\n",
		 path,mode,unix_fildes,dos_fildes,file);
  return (DWORD)file;
}

/*********************************************************************
 *                  fread     (CRTDLL.377)
 */
DWORD __cdecl CRTDLL_fread(LPVOID ptr, INT32 size, INT32 nmemb, LPVOID file)
{
  size_t ret=1;
#if 0
  int i=0;
  void *temp=ptr;

  /* If we would honour CR/LF <-> LF translation, we could do it like this.
     We should keep track of all files opened, and probably files with \
     known binary extensions must be unchanged */
  while ( (i < (nmemb*size)) && (ret==1)) {
    ret=fread(temp,1,1,file);
    dprintf_crtdll(stddeb,
		 "CRTDLL_fread got %c 0x%02x ret %d\n",
		   (isalpha(*(unsigned char*)temp))? *(unsigned char*)temp:
		    ' ',*(unsigned char*)temp, ret);
    if (*(unsigned char*)temp != 0xd) { /* skip CR */
      temp++;
      i++;
    }
    else
      dprintf_crtdll(stddeb, "CRTDLL_fread skipping ^M\n");
  }
  dprintf_crtdll(stddeb,
		 "CRTDLL_fread 0x%08x items of size %d from file %p to %p%s\n",
		 nmemb,size,file,ptr,(i!=nmemb)?" failed":"");
  return i;
#else
    
  ret=fread(ptr,size,nmemb,file);
  dprintf_crtdll(stddeb,
		 "CRTDLL_fread 0x%08x items of size %d from file %p to %p%s\n",
		 nmemb,size,file,ptr,(ret!=nmemb)?" failed":"");
  return ret;
#endif
}
  
/*********************************************************************
 *                  fseek     (CRTDLL.382)
 */
LONG __cdecl CRTDLL_fseek(LPVOID stream, LONG offset, INT32 whence)
{
  long ret;

  ret=fseek(stream,offset,whence);
  dprintf_crtdll(stddeb,
		 "CRTDLL_fseek file %p to 0x%08lx pos %s%s\n",
		 stream,offset,(whence==SEEK_SET)?"SEEK_SET":
		 (whence==SEEK_CUR)?"SEEK_CUR":
		 (whence==SEEK_END)?"SEEK_END":"UNKNOWN",
		 (ret)?"failed":"");
  return ret;
}
  
/*********************************************************************
 *                  ftell     (CRTDLL.384)
 */
LONG __cdecl CRTDLL_ftell(LPVOID stream)
{
  long ret;

  ret=ftell(stream);
  dprintf_crtdll(stddeb,
		 "CRTDLL_ftell file %p at 0x%08lx\n",
		 stream,ret);
  return ret;
}
  
/*********************************************************************
 *                  fwrite     (CRTDLL.386)
 */
DWORD __cdecl CRTDLL_fwrite(LPVOID ptr, INT32 size, INT32 nmemb, LPVOID file)
{
  size_t ret;

  ret=fwrite(ptr,size,nmemb,file);
  dprintf_crtdll(stddeb,
		 "CRTDLL_fwrite 0x%08x items of size %d from %p to file %p%s\n",
		 nmemb,size,ptr,file,(ret!=nmemb)?" failed":"");
  return ret;
}

/*********************************************************************
 *                  setbuf     (CRTDLL.452)
 */
INT32 __cdecl CRTDLL_setbuf(LPVOID file, LPSTR buf)
{
  dprintf_crtdll(stddeb,
		 "CRTDLL_setbuf(file %p buf %p)\n",
		 file,buf);
  /* this doesn't work:"void value not ignored as it ought to be" 
  return setbuf(file,buf); 
  */
  setbuf(file,buf);
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
	dprintf_crtdll(stddeb,
		       "CRTDLL_open_osfhandle(handle %08lx,flags %d) return %d\n",
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
    res = vfprintf( file, format, valist );
    va_end( valist );
    return res;
}

/*********************************************************************
 *                  vfprintf       (CRTDLL.373)
 */
INT32 __cdecl CRTDLL_vfprintf( FILE *file, LPSTR format, va_list args )
{
    return vfprintf( file, format, args );
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
 *                  _isatty       (CRTDLL.137)
 */
BOOL32 __cdecl CRTDLL__isatty(DWORD x)
{
	dprintf_crtdll(stderr,"CRTDLL__isatty(%ld)\n",x);
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
	  len = (UINT32)write(fd,buf,(LONG)len);
	else
	  len = _lwrite32(fd,buf,count);
	dprintf_crtdll(stddeb,"CRTDLL_write %d/%d byte to dfh %d from %p,\n",
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
 */
void __cdecl CRTDLL__cexit(INT32 ret)
{
        dprintf_crtdll(stddeb,"CRTDLL__cexit(%d)\n",ret);
	ExitProcess(ret);
}


/*********************************************************************
 *                  exit          (CRTDLL.359)
 */
void __cdecl CRTDLL_exit(DWORD ret)
{
        dprintf_crtdll(stddeb,"CRTDLL_exit(%ld)\n",ret);
	ExitProcess(ret);
}


/*********************************************************************
 *                  fflush        (CRTDLL.365)
 */
INT32 __cdecl CRTDLL_fflush(LPVOID stream)
{
    int ret;

    ret = fflush(stream);
    dprintf_crtdll(stddeb,"CRTDLL_fflush %p returnd %d %s\n",stream,ret,(ret)?"":" failed");
    return ret;
}


/*********************************************************************
 *                  gets          (CRTDLL.391)
 */
LPSTR __cdecl CRTDLL_gets(LPSTR buf)
{
  /* BAD, for the whole WINE process blocks... just done this way to test
   * windows95's ftp.exe.
   */
    return gets(buf);
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
  dprintf_crtdll(stddeb,
		 "CRTDLL_fputc %c to file %p\n",c,stream);
    return fputc(c,stream);
}


/*********************************************************************
 *                  fputs       (CRTDLL.375)
 */
INT32 __cdecl CRTDLL_fputs( LPCSTR s, FILE *stream )
{
  dprintf_crtdll(stddeb,
		 "CRTDLL_fputs %s to file %p\n",s,stream);
    return fputs(s,stream);
}


/*********************************************************************
 *                  puts       (CRTDLL.443)
 */
INT32 __cdecl CRTDLL_puts(LPCSTR s)
{
  dprintf_crtdll(stddeb,
		 "CRTDLL_fputs %s \n",s);
    return puts(s);
}


/*********************************************************************
 *                  putc       (CRTDLL.441)
 */
INT32 __cdecl CRTDLL_putc(INT32 c, FILE *stream)
{
  dprintf_crtdll(stddeb,
		 "CRTDLL_putc %c to file %p\n",c,stream);
    return fputc(c,stream);
}
/*********************************************************************
 *                  fgetc       (CRTDLL.366)
 */
INT32 __cdecl CRTDLL_fgetc( FILE *stream )
{
  int ret= fgetc(stream);
  dprintf_crtdll(stddeb,
		 "CRTDLL_fgetc got %d\n",ret);
  return ret;
}


/*********************************************************************
 *                  getc       (CRTDLL.388)
 */
INT32 __cdecl CRTDLL_getc( FILE *stream )
{
  int ret= fgetc(stream);
  dprintf_crtdll(stddeb,
		 "CRTDLL_getc got %d\n",ret);
  return ret;
}

/*********************************************************************
 *                  _lrotl          (CRTDLL.176)
 */
DWORD __cdecl CRTDLL__lrotl(DWORD x,INT32 shift)
{
   unsigned long ret = (x >> shift)|( x >>((sizeof(x))-shift));

   dprintf_crtdll(stddeb,
		  "CRTDLL_lrotl got 0x%08lx rot %d ret 0x%08lx\n",
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
  
  ret=fgets(s, size,stream);
  /*FIXME: Control with CRTDLL_setmode */
  control_M= strrchr(s,'\r');
  /*delete CR if we read a DOS File */
  if (control_M)
    {
      *control_M='\n';
      *(control_M+1)=0;
    }
  dprintf_crtdll(stddeb,
		 "CRTDLL_fgets got %s for %d chars from file %p%s\n",
		 s,size,stream,(ret)?"":" failed");
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
    dprintf_crtdll(stddeb,"CRTDLL_mbscpy %s and %s\n",x,y);
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
LPSTR CRTDLL__strlwr(LPSTR x)
{
  unsigned char *y =x;
  
  dprintf_crtdll(stddeb,
		 "CRTDLL_strlwr got %s",x);
  while (*y) {
    if ((*y > 0x40) && (*y< 0x5b))
      *y = *y + 0x20;
    y++;
  }
  dprintf_crtdll(stddeb," returned %s\n",x);
		 
  return x;
}

/*********************************************************************
 *                  system       (CRTDLL.485)
 */
INT32 CRTDLL_system(LPSTR x)
{
#define SYSBUF_LENGTH 1500
  char buffer[SYSBUF_LENGTH]="wine \"";
  unsigned char *y =x;
  unsigned char *bp =buffer+strlen(buffer);
  int i =strlen(buffer) + strlen(x) +2;

  /* Calculate needed buffer size tp prevent overflow*/
  while (*y) {
    if (*y =='\\') i++;
    y++;
  }
  /* if buffer to short, exit */
  if (i > SYSBUF_LENGTH) {
    dprintf_crtdll(stddeb,"_system buffer to small\n");
    return 127;
  }
  
  y =x;

  while (*y) {
    *bp = *y;
    bp++; y++;
    if (*(y-1) =='\\') *bp++ = '\\';
  }
  /* remove spaces from end of string */
  while (*(y-1) == ' ') {
    bp--;y--;
  }
  *bp++ = '"';
  *bp = 0;
  dprintf_crtdll(stddeb,
		 "_system got \"%s\", executing \"%s\"\n",x,buffer);

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
		*y=toupper(*y);
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
		*y=tolower(*y);
		y++;
	}
	return x;
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
LPSTR __cdecl CRTDLL__strdup(LPSTR ptr)
{
    return HEAP_strdupA(GetProcessHeap(),0,ptr);
}


/*********************************************************************
 *                  fclose           (CRTDLL.362)
 */
INT32 __cdecl CRTDLL_fclose( FILE *stream )
{
    int unix_handle=fileno(stream);
    HFILE32 dos_handle=3;
    HFILE32 ret=EOF;

    if (unix_handle<4) ret= fclose(stream);
    else {
      while(FILE_GetUnixHandle(dos_handle) != unix_handle) dos_handle++;
      fclose(stream);
      ret = _lclose32( dos_handle);
    }
    dprintf_crtdll(stddeb,"CRTDLL_fclose(%p) ufh %d dfh %d%s\n",
		   stream,unix_handle,dos_handle,(ret)?" failed":"");
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
      dprintf_crtdll(stddeb,"CRTDLL_unlink file %s bad name\n",pathname);
      return EOF;
    }
  
    ret=unlink(full_name.long_name);
    dprintf_crtdll(stddeb,"CRTDLL_unlink(%s unix %s)%s\n",
		   pathname,full_name.long_name, (ret)?" failed":"");
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
      dprintf_crtdll(stddeb,"CRTDLL_open file unsupported flags 0x%04x\n",flags);
    /* End Fixme */

    ret = FILE_Open(path,wineflags);
    dprintf_crtdll(stddeb,"CRTDLL_open file %s mode 0x%04x (lccmode 0x%04x) got dfh %d\n",
		   path,wineflags,flags,ret);
    return ret;
}

/*********************************************************************
 *                  _close           (CRTDLL.57)
 */
INT32 __cdecl CRTDLL__close(HFILE32 fd)
{
    int ret=_lclose32(fd);

    dprintf_crtdll(stddeb,"CRTDLL_close(%d)%s\n",fd,(ret)?" failed":"");
    return ret;
}

/*********************************************************************
 *                  feof           (CRTDLL.363)
 */
INT32 __cdecl CRTDLL_feof( FILE *stream )
{
    int ret;
    
    ret=feof(stream);
    dprintf_crtdll(stddeb,"CRTDLL_feof(%p) %s\n",stream,(ret)?"true":"false");
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
	fprintf(stderr,"CRTDLL_setlocale(%s,%s),stub!\n",categorystr,locale);
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
LPWSTR __cdecl CRTDLL_wcschr(LPWSTR str,WCHAR xchar)
{
	LPWSTR	s;

	s=str;
	do {
		if (*s==xchar)
			return s;
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
 *                  towupper           (CRTDLL.494)
 */
WCHAR __cdecl CRTDLL_towupper(WCHAR x)
{
    return (WCHAR)toupper((CHAR)x);
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

	dprintf_crtdll(stddeb,
		       "CRTDLL._setmode on fhandle %d mode %s, STUB.\n",
		fh,(mode=O_TEXT)?"O_TEXT":
		(mode=O_BINARY)?"O_BINARY":"UNKNOWN");
	return -1;
}

/*********************************************************************
 *                  atexit           (CRTDLL.345)
 */
INT32 __cdecl CRTDLL_atexit(LPVOID x)
{
	/* FIXME */
	fprintf(stdnimp,"CRTDLL.atexit(%p), STUB.\n",x);
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
      
    dprintf_crtdll(stderr,"CRTDLL_mlen %s for max %d bytes ret %d\n",mb,size,ret);

    return ret;
}

/*********************************************************************
 *                  mbstowcs           (CRTDLL.429)
 * FIXME: check multibyte support
 */
INT32 __cdecl CRTDLL_mbstowcs(LPWSTR wcs, LPCSTR mbs, INT32 size)
{

/* Slightly modified lstrcpynAtoW functions from memory/strings.c
 *  We need the numberr of characters transfered 
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
          
    dprintf_crtdll(stddeb,"CRTDLL_mbstowcs %s for %d chars put %d wchars\n",
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
   
   dprintf_crtdll(stderr,"CRTDLL_mbtowc %s for %d chars\n",mb,size);
         
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
 */
BOOL32 __cdecl CRTDLL__chdrive(INT32 newdrive)
{
	/* FIXME: generates errnos */
	return DRIVE_SetCurrentDrive(newdrive);
}

/*********************************************************************
 *                  _chdir           (CRTDLL.51)
 */
INT32 __cdecl CRTDLL__chdir(LPCSTR newdir)
{
	if (!SetCurrentDirectory32A(newdir))
		return -1;
	return 0;
}

/*********************************************************************
 *                  _getcwd           (CRTDLL.120)
 */
CHAR* __cdecl CRTDLL__getcwd(LPSTR buf, INT32 size)
{
  DOS_FULL_NAME full_name;
  char *ret;

  dprintf_crtdll(stddeb,"CRTDLL_getcwd for buf %p size %d\n",
		 buf,size);
  if (buf == NULL)
    {
      dprintf_crtdll(stderr,"CRTDLL_getcwd malloc unsupported\n");
      printf("CRTDLL_getcwd malloc unsupported\n");
      return 0;
    }
  ret = getcwd(buf,size);
  if (!DOSFS_GetFullName( buf, FALSE, &full_name )) 
    {
      dprintf_crtdll(stddeb,"CRTDLL_getcwd failed\n");
      return 0;
    }
  if (strlen(full_name.short_name)>size) 
    {
      dprintf_crtdll(stddeb,"CRTDLL_getcwd string too long\n");
      return 0;
    }
  ret=strcpy(buf,full_name.short_name);
  if (ret) 
    dprintf_crtdll(stddeb,"CRTDLL_getcwd returned:%s\n",ret);
  return ret;
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
 *                  _errno           (CRTDLL.52)
 * Yes, this is a function.
 */
LPINT32 __cdecl CRTDLL__errno()
{
	static	int crtdllerrno;
	extern int LastErrorToErrno(DWORD);

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
       dprintf_crtdll(stddeb,
		      "CRTDLL_tempnam Unable to get unique filename\n");
       return NULL;
     }
     if (!DOSFS_GetFullName(ret,FALSE,&tempname))
     {
       dprintf_crtdll(stddeb,
		      "CRTDLL_tempnam Wrong path?\n");
       return NULL;
     }
     free(ret);
     if ((ret = CRTDLL_malloc(strlen(tempname.short_name)+1)) == NULL) {
	 dprintf_crtdll(stddeb,
			"CRTDLL_tempnam CRTDL_malloc for shortname failed\n");
	 return NULL;
     }
     if ((ret = strcpy(ret,tempname.short_name)) == NULL) { 
       dprintf_crtdll(stddeb,
		      "CRTDLL_tempnam Malloc for shortname failed\n");
       return NULL;
     }
     
     dprintf_crtdll(stddeb,"CRTDLL_tempnam dir %s prefix %s got %s\n",
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
       dprintf_crtdll(stddeb,
		      "CRTDLL_tmpnam Unable to get unique filename\n");
       return NULL;
     }
     if (!DOSFS_GetFullName(ret,FALSE,&CRTDLL_tmpname))
     {
       dprintf_crtdll(stddeb,
		      "CRTDLL_tmpnam Wrong path?\n");
       return NULL;
     }
     strcat(CRTDLL_tmpname.short_name,".");
     dprintf_crtdll(stddeb,"CRTDLL_tmpnam for buf %p got %s\n",
		    s,CRTDLL_tmpname.short_name);
     dprintf_crtdll(stddeb,"CRTDLL_tmpnam long got %s\n",
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

typedef VOID (*sig_handler_type)(VOID);

/*********************************************************************
 *                  signal           (CRTDLL.455)
 */
VOID __cdecl CRTDLL_signal(int sig, sig_handler_type ptr)
{
    dprintf_crtdll(stddeb,"CRTDLL_signal %d %p: STUB!\n",sig,ptr);
}
