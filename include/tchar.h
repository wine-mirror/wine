#ifndef __WINE_TCHAR_H
#define __WINE_TCHAR_H


/* FIXME: this should be in direct.h but since it's a standard C library include (on some systems)... */
#define _chdir chdir
#define _rmdir rmdir

/* FIXME: this should be in io.h but I believe it's a standard include on some systems... */
/* would need unistd.h */
#define _access access
/* would need sys/stat.h */
#define _chmod chmod
/* would need fcntl.h */
#define _creat creat
#define _open open
/* FIXME: we need _S_IWRITE, _O_CREAT ... */
/* FIXME: _fsopen is not implemented */
#define sopen _sopen
/* FIXME: _sopen is not implemented */

/* FIXME: this should be in stdio.h but since it's a standard C library include... */
#define fgetchar getchar
#define fputchar putchar
#define _popen popen
#define _tempnam tempnam
#define _vsnprintf vsnprintf

/* FIXME: this should be in stdlib.h but since it's a standard C library include... */
/* FIXME: itoa and ltoa are missing */
/* FIXME: _makepath is not implemented */
/* FIXME: _searchenv is not implemented */
/* FIXME: _splitpath is not implemented */

/* FIXME: this should be in string.h but since it's a standard C library include... */
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _strdup strdup
/* FIXME: stricoll is not implemented but strcasecmp is probably close enough in most cases */
#define _stricoll strcasecmp
#define stricoll _stricoll
#define strlwr _strlwr
/* FIXME: _strlwr is not implemented */
#define strnset _strnset
/* FIXME: _strnset is not implemented */
#define strrev _strrev
/* FIXME: _strrev is not implemented */
#define strset _strset
/* FIXME: _strset is not implemented */
#define strupr _strupr
/* FIXME: _strupr is not implemented */
#define ultoa _ultoa
/* FIXME: _ultoa is not implemented */

/* FIXME: this should be in sys/stat.h but since it's a standard C library include... */
#define _stat stat

/* FIXME: this should be in time.h but since it's a standard C library include... */
/* FIXME: _strdate is not implemented */
/* FIXME: _strtime is not implemented */

/* FIXME: this should be in utime.h but since it's a standard C library include... */
#define _utime utime


/*****************************************************************************
 * tchar routines
 */
#define _strdec(start,current)  (start<current?(current)-1:NULL)
#define _strinc(current) ((current)+1)
/* FIXME: _strncnt ans strncnt are missing */
/* FIXME: _strspnp is not implemented */


/*****************************************************************************
 * tchar mappings
 */
#ifndef _UNICODE
#include <string.h>
#ifndef _MBCS
#define WINE_tchar_routine(std,mbcs,unicode) std
#else /* _MBCS defined */
#define WINE_tchar_routine(std,mbcs,unicode) mbcs
#endif
#else /* _UNICODE defined */
#define WINE_tchar_routine(std,mbcs,unicode) unicode
#endif

#define WINE_tchar_true(a) (1)
#define WINE_tchar_false(a) (0)
#define WINE_tchar_tclen(a) (1)
#define WINE_tchar_tccpy(a,b) do { *(a)=*(b); } while (0)

#define _fgettc       WINE_tchar_routine(fgetc,           fgetc,       fgetwc)
#define _fgettchar    WINE_tchar_routine(fgetchar,        fgetchar,    _fgetwchar)
#define _fgetts       WINE_tchar_routine(fgets,           fgets,       fgetws)
#define _fputtc       WINE_tchar_routine(fputc,           fputc,       fputwc)
#define _fputtchar    WINE_tchar_routine(fputchar,        fputchar,    _fputwchar)
#define _fputts       WINE_tchar_routine(fputs,           fputs,       fputws)
#define _ftprintf     WINE_tchar_routine(fprintf,         fprintf,     fwprintf)
#define _ftscanf      WINE_tchar_routine(fscanf,          fscanf,      fwscanf)
#define _gettc        WINE_tchar_routine(getc,            getc,        getwc)
#define _gettchar     WINE_tchar_routine(getchar,         getchar,     getwchar)
#define _getts        WINE_tchar_routine(gets,            gets,        getws)
#define _isalnum      WINE_tchar_routine(isalnum,         _ismbcalnum, iswalnum)
#define _istalpha     WINE_tchar_routine(isalpha,         _ismbcalpha, iswalpha)
#define _istascii     WINE_tchar_routine(__isascii,       __isascii,   iswascii)
#define _istcntrl     WINE_tchar_routine(iscntrl,         iscntrl,     iswcntrl)
#define _istdigit     WINE_tchar_routine(isdigit,         _ismbcdigit, iswdigit)
#define _istgraph     WINE_tchar_routine(isgraph,         _ismbcgraph, iswgraph)
#define _istlead      WINE_tchar_routine(WINE_tchar_false,_ismbblead,  WINE_tchar_false)
#define _istleadbyte  WINE_tchar_routine(WINE_tchar_false,isleadbyte,  WINE_tchar_false)
#define _istlegal     WINE_tchar_routine(WINE_tchar_true, _ismbclegal, WINE_tchar_true)
#define _istlower     WINE_tchar_routine(islower,         _ismbcslower,iswlower)
#define _istprint     WINE_tchar_routine(isprint,         _ismbcprint, iswprint)
#define _istpunct     WINE_tchar_routine(ispunct,         _ismbcpunct, iswpunct)
#define _istspace     WINE_tchar_routine(isspace,         _ismbcspace, iswspace)
#define _istupper     WINE_tchar_routine(isupper,         _ismbcupper, iswupper)
#define _istxdigit    WINE_tchar_routine(isxdigit,        isxdigit,    iswxdigit)
#define _itot         WINE_tchar_routine(_itoa,           _itoa,       _itow)
#define _ltot         WINE_tchar_routine(_ltoa,           _ltoa,       _ltow)
#define _puttc        WINE_tchar_routine(putc,            putc,        putwc)
#define _puttchar     WINE_tchar_routine(putchar,         putchar,     putwchar)
#define _putts        WINE_tchar_routine(puts,            puts,        putws)
#define _tmain        WINE_tchar_routine(main,            main,        wmain)
#define _sntprintf    WINE_tchar_routine(sprintf,         sprintf,     swprintf)
#define _stscanf      WINE_tchar_routine(sscanf,          sscanf,      swscanf)
#define _taccess      WINE_tchar_routine(_access,         _access,     _waccess)
#define _tasctime     WINE_tchar_routine(asctime,         asctime,     _wasctime)
#define _tccpy        WINE_tchar_routine(WINE_tchar_tccpy,_mbccpy,     WINE_tchar_tccpy)
#define _tchdir       WINE_tchar_routine(_chdir,          _chdir,      _wchdir)
#define _tclen        WINE_tchar_routine(WINE_tchar_tclen,_mbclen,     WINE_tchar_tclen)
#define _tchmod       WINE_tchar_routine(_chmod,          _chmod,      _wchmod)
#define _tcreat       WINE_tchar_routine(_creat,          _creat,      _wcreat)
#define _tcscat       WINE_tchar_routine(strcat,          _mbscat,     wcscat)
#define _tcschr       WINE_tchar_routine(strchr,          _mbschr,     wcschr)
#define _tcsclen      WINE_tchar_routine(strlen,          _mbslen,     wcslen)
#define _tcscmp       WINE_tchar_routine(strcmp,          _mbscmp,     wcscmp)
#define _tcscoll      WINE_tchar_routine(strcoll,         _mbscoll,    wcscoll)
#define _tcscpy       WINE_tchar_routine(strcpy,          _mbscpy,     wcscpy)
#define _tcscspn      WINE_tchar_routine(strcspn,         _mbscspn,    wcscspn)
#define _tcsdec       WINE_tchar_routine(_strdec,         _mbsdec,     _wcsdec)
#define _tcsdup       WINE_tchar_routine(_strdup,         _mbsdup,     _wcsdup)
#define _tcsftime     WINE_tchar_routine(strftime,        strftime,    wcsftime)
#define _tcsicmp      WINE_tchar_routine(_stricmp,        _mbsicmp,    _wcsicmp)
#define _tcsicoll     WINE_tchar_routine(_stricoll,       _stricoll,   _wcsicoll)
#define _tcsinc       WINE_tchar_routine(_strinc,         _mbsinc,     _wcsinc)
#define _tcslen       WINE_tchar_routine(strlen,          strlen,      wcslen)
#define _tcslwr       WINE_tchar_routine(_strlwr,         _mbslwr,     _wcslwr)
#define _tcsnbcnt     WINE_tchar_routine(_strncnt,        _mbsnbcnt,   _wcnscnt)
#define _tcsncat      WINE_tchar_routine(strncat,         _mbsnbcat,   wcsncat)
#define _tcsnccat     WINE_tchar_routine(strncat,         _mbsncat,    wcsncat)
#define _tcsncmp      WINE_tchar_routine(strncmp,         _mbsnbcmp,   wcsncmp)
#define _tcsnccmp     WINE_tchar_routine(strncmp,         _mbsncmp,    wcsncmp)
#define _tcsnccnt     WINE_tchar_routine(_strncnt,        _mbsnccnt,   _wcsncnt)
#define _tcsnccpy     WINE_tchar_routine(strncpy,         _mbsncpy,    wcsncpy)
#define _tcsncicmp    WINE_tchar_routine(_strnicmp,       _mbsnicmp,   _wcsnicmp)
#define _tcsncpy      WINE_tchar_routine(strncpy,         _mbsnbcpy,   wcsncpy)
#define _tcsncset     WINE_tchar_routine(_strnset,        _mbsnset,    _wcsnset)
#define _tcsnextc     WINE_tchar_routine(_strnextc,       _mbsnextc,   _wcsnextc)
#define _tcsnicmp     WINE_tchar_routine(_strnicmp,       _mbsnicmp,   _wcsnicmp)
#define _tcsnicoll    WINE_tchar_routine(_strnicoll,      _strnicoll   _wcsnicoll)
#define _tcsninc      WINE_tchar_routine(_strninc,        _mbsninc,    _wcsninc)
#define _tcsnccnt     WINE_tchar_routine(_strncnt,        _mbsnccnt,   _wcsncnt)
#define _tcsnset      WINE_tchar_routine(_strnset,        _mbsnbset,   _wcsnset)
#define _tcspbrk      WINE_tchar_routine(strpbrk,         _mbspbrk,    wcspbrk)
#define _tcsspnp      WINE_tchar_routine(_strspnp,        _mbsspnp,    _wcsspnp)
#define _tcsrchr      WINE_tchar_routine(strrchr,         _mbsrchr,    wcsrchr)
#define _tcsrev       WINE_tchar_routine(_strrev,         _mbsrev,     _wcsrev)
#define _tcsset       WINE_tchar_routine(_strset,         _mbsset,     _wcsset)
#define _tcsspn       WINE_tchar_routine(strspn,          _mbsspn,     wcsspn)
#define _tcsstr       WINE_tchar_routine(strstr,          _mbsstr,     wcsstr)
#define _tcstod       WINE_tchar_routine(strtod,          strtod,      wcstod)
#define _tcstok       WINE_tchar_routine(strtok,          _mbstok,     wcstok)
#define _tcstol       WINE_tchar_routine(strtol,          strtol,      wcstol)
#define _tcstoul      WINE_tchar_routine(strtoul,         strtoul,     wcstoul)
#define _tcsupr       WINE_tchar_routine(_strupr,         _mbsupr,     _wcsupr)
#define _tcsxfrm      WINE_tchar_routine(strxfrm,         strxfrm,     wcsxfrm)
#define _tctime       WINE_tchar_routine(ctime,           ctime,       _wctime)
#define _texecl       WINE_tchar_routine(_execl,          _execl,      _wexecl)
#define _texecle      WINE_tchar_routine(_execle,         _execle,     _wexecle)
#define _texeclp      WINE_tchar_routine(_execlp,         _execlp,     _wexeclp)
#define _texeclpe     WINE_tchar_routine(_execlpe,        _execlpe,    _wexeclpe)
#define _texecv       WINE_tchar_routine(_execv,          _execv,      _wexecv)
#define _texecve      WINE_tchar_routine(_execve,         _execve,     _wexecve)
#define _texecvp      WINE_tchar_routine(_execvp,         _execvp,     _wexecvp)
#define _texecvpe     WINE_tchar_routine(_execvpe,        _execvpe,    _wexecvpe)
#define _tfdopen      WINE_tchar_routine(_fdopen,         _fdopen,     _wfdopen)
#define _tfindfirst   WINE_tchar_routine(_findfirst,      _findfirst,  _wfindfirst)
#define _tfindnext    WINE_tchar_routine(_findnext,       _findnext,   _wfindnext)
#define _tfopen       WINE_tchar_routine(fopen,           fopen,       _wfopen)
#define _tfreopen     WINE_tchar_routine(freopen,         freopen,     _wfreopen)
#define _tfsopen      WINE_tchar_routine(_fsopen,         _fsopen,     _wfsopen)
#define _tfullpath    WINE_tchar_routine(_fullpath,       _fullpath,   _wfullpath)
#define _tgetcwd      WINE_tchar_routine(_getcwd,         _getcwd,     _wgetcwd)
#define _tgetenv      WINE_tchar_routine(getenv,          getenv,      _wgetenv)
#define _tmain        WINE_tchar_routine(main,            main,        wmain)
#define _tmakepath    WINE_tchar_routine(_makepath,       _makepath,   _wmakepath)
#define _tmkdir       WINE_tchar_routine(_mkdir,          _mkdir,      _wmkdir)
#define _tmktemp      WINE_tchar_routine(_mktemp,         _mktemp,     _wmktemp)
#define _tperror      WINE_tchar_routine(perror,          perror,      _wperror)
#define _topen        WINE_tchar_routine(_open,           _open,       _wopen)
#define _totlower     WINE_tchar_routine(tolower,         _mbctolower, towlower)
#define _totupper     WINE_tchar_routine(toupper,         _mbctoupper, towupper)
#define _tpopen       WINE_tchar_routine(_popen,          _popen,      _wpopen)
#define _tprintf      WINE_tchar_routine(printf,          printf,      wprintf)
#define _tremove      WINE_tchar_routine(remove,          remove,      _wremove)
#define _trename      WINE_tchar_routine(rename,          rename,      _wrename)
#define _trmdir       WINE_tchar_routine(_rmdir,          _rmdir,      _wrmdir)
#define _tsearchenv   WINE_tchar_routine(_searchenv,      _searchenv,  _wsearchenv)
#define _tscanf       WINE_tchar_routine(scanf,           scanf,       wscanf)
#define _tsetlocale   WINE_tchar_routine(setlocale,       setlocale,   _wsetlocale)
#define _tsopen       WINE_tchar_routine(_sopen,          _sopen,      _wsopen)
#define _tspawnl      WINE_tchar_routine(_spawnl,         _spawnl,     _wspawnl)
#define _tspawnle     WINE_tchar_routine(_spawnle,        _spawnle,    _wspawnle)
#define _tspawnlp     WINE_tchar_routine(_spawnlp,        _spawnlp,    _wspawnlp)
#define _tspawnlpe    WINE_tchar_routine(_spawnlpe,       _spawnlpe,   _wspawnlpe)
#define _tspawnv      WINE_tchar_routine(_spawnv,         _spawnv,     _wspawnv)
#define _tspawnve     WINE_tchar_routine(_spawnve,        _spawnve,    _wspawnve)
#define _tspawnvp     WINE_tchar_routine(_spawnvp,        _spawnvp,    _tspawnvp)
#define _tspawnvpe    WINE_tchar_routine(_spawnvpe,       _spawnvpe,   _tspawnvpe)
#define _tsplitpath   WINE_tchar_routine(_splitpath,      _splitpath,  _wsplitpath)
#define _tstat        WINE_tchar_routine(_stat,           _stat,       _wstat)
#define _tstrdate     WINE_tchar_routine(_strdate,        _strdate,    _wstrdate)
#define _tstrtime     WINE_tchar_routine(_strtime,        _strtime,    _wstrtime)
#define _tsystem      WINE_tchar_routine(system,          system,      _wsystem)
#define _ttempnam     WINE_tchar_routine(_tempnam,        _tempnam,    _wtempnam)
#define _ttmpnam      WINE_tchar_routine(tmpnam,          tmpnam,      _wtmpnam)
#define _ttoi         WINE_tchar_routine(atoi,            atoi,        _wtoi)
#define _ttol         WINE_tchar_routine(atol,            atol,        _wtol)
#define _tutime       WINE_tchar_routine(_utime,          _utime,      _wutime)
#define _tWinMain     WINE_tchar_routine(WinMain,         WinMain,     wWinMain)
#define _ultot        WINE_tchar_routine(_ultoa,          _ultoa,      _ultow)
#define _ungettc      WINE_tchar_routine(ungetc,          ungetc,      ungetwc)
#define _vftprintf    WINE_tchar_routine(vfprintf,        vfprintf,    vfwprintf)
#define _vsntprintf   WINE_tchar_routine(_vsnprintf,      _vsnprintf,  _vsnwprintf)
#define _vstprintf    WINE_tchar_routine(vsprintf,        vsprintf,    vswprintf)
#define _vtprintf     WINE_tchar_routine(vprintf,         vprintf,     vwprintf)

#define _T(x) __T(x)
#define _TEXT(x) __T(x)
#define __T(x) x

typedef CHAR  _TCHAR32A;
typedef WCHAR _TCHAR32W;
DECL_WINELIB_TYPE_AW (_TCHAR)
typedef UCHAR  _TUCHAR32A;
typedef WCHAR _TUCHAR32W;
DECL_WINELIB_TYPE_AW (_TUCHAR)
				 
#endif /* __WINE_TCHAR_H */
