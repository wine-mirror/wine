# C RunTime DLL. All functions use cdecl!
name	crtdll
type	win32
init    CRTDLL_Init

import kernel32.dll
import ntdll.dll

debug_channels (crtdll)

@ cdecl ??2@YAPAXI@Z(long) CRTDLL_new
@ cdecl ??3@YAXPAX@Z(long) CRTDLL_delete
@ cdecl ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) CRTDLL_set_new_handler
@ cdecl _CIacos() CRTDLL__CIacos
@ cdecl _CIasin() CRTDLL__CIasin
@ cdecl _CIatan() CRTDLL__CIatan
@ cdecl _CIatan2() CRTDLL__CIatan2
@ cdecl _CIcos() CRTDLL__CIcos
@ cdecl _CIcosh() CRTDLL__CIcosh
@ cdecl _CIexp() CRTDLL__CIexp
@ cdecl _CIfmod() CRTDLL__CIfmod
@ cdecl _CIlog() CRTDLL__CIlog
@ cdecl _CIlog10() CRTDLL__CIlog10
@ cdecl _CIpow() CRTDLL__CIpow
@ cdecl _CIsin() CRTDLL__CIsin
@ cdecl _CIsinh() CRTDLL__CIsinh
@ cdecl _CIsqrt() CRTDLL__CIsqrt
@ cdecl _CItan() CRTDLL__CItan
@ cdecl _CItanh() CRTDLL__CItanh
@ extern _HUGE_dll CRTDLL_HUGE_dll
@ stub _XcptFilter
@ cdecl __GetMainArgs(ptr ptr ptr long) CRTDLL__GetMainArgs
@ extern __argc_dll CRTDLL_argc_dll
@ extern __argv_dll CRTDLL_argv_dll
@ cdecl __dllonexit() CRTDLL___dllonexit
@ cdecl __doserrno() CRTDLL___doserrno
@ stub __fpecode
@ cdecl __isascii(long) CRTDLL___isascii
@ cdecl __iscsym(long) CRTDLL___iscsym
@ cdecl __iscsymf(long) CRTDLL___iscsymf
@ extern __mb_cur_max_dll CRTDLL__mb_cur_max_dll
@ stub __pxcptinfoptrs
@ forward __threadhandle kernel32.GetCurrentThread
@ forward __threadid kernel32.GetCurrentThreadId
@ cdecl __toascii(long) CRTDLL___toascii
@ cdecl _abnormal_termination() CRTDLL__abnormal_termination
@ cdecl _access(str long) CRTDLL__access
@ extern _acmdln_dll CRTDLL_acmdln_dll
@ stub _aexit_rtn_dll
@ cdecl _amsg_exit(long) CRTDLL__amsg_exit
@ cdecl _assert(ptr ptr long) CRTDLL__assert
@ extern _basemajor_dll CRTDLL_basemajor_dll
@ extern _baseminor_dll CRTDLL_baseminor_dll
@ extern _baseversion_dll CRTDLL_baseversion_dll
@ cdecl _beep(long long) CRTDLL__beep
@ stub _beginthread
@ cdecl _c_exit() CRTDLL__c_exit
@ cdecl _cabs(long) CRTDLL__cabs
@ cdecl _cexit() CRTDLL__cexit
@ stub _cgets
@ cdecl _chdir(str) CRTDLL__chdir
@ cdecl _chdrive(long) CRTDLL__chdrive
@ cdecl _chgsign(double) CRTDLL__chgsign
@ cdecl _chmod(str long) CRTDLL__chmod
@ stub _chsize
@ cdecl _clearfp() CRTDLL__clearfp
@ cdecl _close(long) CRTDLL__close
@ cdecl _commit(long) CRTDLL__commit
@ extern _commode_dll CRTDLL_commode_dll
@ cdecl _control87(long long) CRTDLL__control87
@ cdecl _controlfp(long long) CRTDLL__controlfp
@ cdecl _copysign(double double) CRTDLL__copysign
@ stub _cprintf
@ stub _cpumode_dll
@ stub _cputs
@ cdecl _creat(str long) CRTDLL__creat
@ stub _cscanf
@ extern _ctype CRTDLL_ctype
@ stub _cwait
@ stub _daylight_dll
@ stub _dup
@ stub _dup2
@ cdecl _ecvt(double long ptr ptr) ecvt
@ stub _endthread
@ extern _environ_dll CRTDLL_environ_dll
@ cdecl _eof(long) CRTDLL__eof
@ cdecl _errno() CRTDLL__errno
@ cdecl _except_handler2(ptr ptr ptr ptr) CRTDLL__except_handler2
@ stub _execl
@ stub _execle
@ stub _execlp
@ stub _execlpe
@ stub _execv
@ stub _execve
@ stub _execvp
@ stub _execvpe
@ cdecl _exit(long) CRTDLL__exit
@ cdecl _expand(ptr long) CRTDLL__expand
@ cdecl _fcloseall() CRTDLL__fcloseall
@ cdecl _fcvt(double long ptr ptr) fcvt
@ cdecl _fdopen(long ptr) CRTDLL__fdopen
@ cdecl _fgetchar() CRTDLL__fgetchar
@ stub _fgetwchar
@ cdecl _filbuf(ptr) CRTDLL__filbuf
@ stub _fileinfo_dll
@ cdecl _filelength(long) CRTDLL__filelength
@ cdecl _fileno(ptr) CRTDLL__fileno
@ cdecl _findclose(long) CRTDLL__findclose
@ cdecl _findfirst(str ptr) CRTDLL__findfirst
@ cdecl _findnext(long ptr) CRTDLL__findnext
@ cdecl _finite(double) CRTDLL__finite
@ cdecl _flsbuf(long ptr) CRTDLL__flsbuf
@ cdecl _flushall() CRTDLL__flushall
@ extern _fmode_dll CRTDLL_fmode_dll
@ cdecl _fpclass(double) CRTDLL__fpclass
@ stub _fpieee_flt
@ cdecl _fpreset() CRTDLL__fpreset
@ cdecl _fputchar(long) CRTDLL__fputchar
@ stub _fputwchar
@ cdecl _fsopen(str str long) CRTDLL__fsopen
@ cdecl _fstat(long ptr) CRTDLL__fstat
@ cdecl _ftime(ptr) CRTDLL__ftime
@ forward _ftol ntdll._ftol
@ cdecl _fullpath(ptr str long) CRTDLL__fullpath
@ cdecl _futime(long ptr) CRTDLL__futime
@ cdecl _gcvt(double long str) gcvt
@ cdecl _get_osfhandle(long) CRTDLL__get_osfhandle
@ stub _getch
@ stub _getche
@ cdecl _getcwd(ptr long) CRTDLL__getcwd
@ cdecl _getdcwd(long ptr long) CRTDLL__getdcwd
@ cdecl _getdiskfree(long ptr) CRTDLL__getdiskfree
@ forward _getdllprocaddr kernel32.GetProcAddress
@ cdecl _getdrive() CRTDLL__getdrive
@ forward _getdrives kernel32.GetLogicalDrives
@ forward _getpid kernel32.GetCurrentProcessId
@ stub _getsystime
@ cdecl _getw(ptr) CRTDLL__getw
@ cdecl _global_unwind2(ptr) CRTDLL__global_unwind2
@ cdecl _heapchk() CRTDLL__heapchk
@ cdecl _heapmin() CRTDLL__heapmin
@ cdecl _heapset(long) CRTDLL__heapset
@ cdecl _heapwalk(ptr) CRTDLL__heapwalk
@ cdecl _hypot(double double) hypot
@ cdecl _initterm(ptr ptr) CRTDLL__initterm
@ extern _iob __CRTDLL_iob
@ cdecl _isatty(long) CRTDLL__isatty
@ cdecl _isctype(long long) CRTDLL__isctype
@ stub _ismbbalnum
@ stub _ismbbalpha
@ stub _ismbbgraph
@ stub _ismbbkalnum
@ stub _ismbbkana
@ stub _ismbbkpunct
@ stub _ismbblead
@ stub _ismbbprint
@ stub _ismbbpunct
@ stub _ismbbtrail
@ stub _ismbcalpha
@ stub _ismbcdigit
@ stub _ismbchira
@ stub _ismbckata
@ stub _ismbcl0
@ stub _ismbcl1
@ stub _ismbcl2
@ stub _ismbclegal
@ stub _ismbclower
@ stub _ismbcprint
@ stub _ismbcspace
@ stub _ismbcsymbol
@ stub _ismbcupper
@ stub _ismbslead
@ stub _ismbstrail
@ cdecl _isnan(double) CRTDLL__isnan
@ forward _itoa ntdll._itoa
@ cdecl _itow(long str long) CRTDLL__itow
@ cdecl _j0(double) j0
@ cdecl _j1(double) j1
@ cdecl _jn(long double) jn
@ stub _kbhit
@ cdecl _lfind(ptr ptr ptr long ptr) CRTDLL__lfind
@ cdecl _loaddll(str) CRTDLL__loaddll
@ cdecl _local_unwind2(ptr long) CRTDLL__local_unwind2
@ stub _locking
@ cdecl _logb(double) CRTDLL__logb
@ cdecl _lrotl (long long) CRTDLL__lrotl
@ cdecl _lrotr (long long) CRTDLL__lrotr
@ cdecl _lsearch(ptr ptr long long ptr) CRTDLL__lsearch
@ cdecl _lseek(long long long) CRTDLL__lseek
@ forward _ltoa ntdll._ltoa
@ cdecl _ltow(long str long) CRTDLL__ltow
@ cdecl _makepath (ptr str str str str) CRTDLL__makepath
@ cdecl _matherr(ptr) CRTDLL__matherr
@ stub _mbbtombc
@ stub _mbbtype
@ cdecl _mbccpy (str str) CRTDLL__mbccpy
@ stub _mbcjistojms
@ stub _mbcjmstojis
@ stub _mbclen
@ stub _mbctohira
@ stub _mbctokata
@ stub _mbctolower
@ stub _mbctombb
@ stub _mbctoupper
@ stub _mbctype
@ stub _mbsbtype
@ cdecl _mbscat(str str) strcat
@ stub _mbschr
@ stub _mbscmp
@ cdecl _mbscpy(ptr str) strcpy
@ stub _mbscspn
@ stub _mbsdec
@ cdecl _mbsdup(str) CRTDLL__strdup
@ cdecl _mbsicmp(str str) CRTDLL__mbsicmp
@ cdecl _mbsinc(str) CRTDLL__mbsinc
@ cdecl _mbslen(str) CRTDLL__mbslen
@ stub _mbslwr
@ stub _mbsnbcat
@ stub _mbsnbcmp
@ stub _mbsnbcnt
@ stub _mbsnbcpy
@ stub _mbsnbicmp
@ stub _mbsnbset
@ stub _mbsncat
@ stub _mbsnccnt
@ stub _mbsncmp
@ stub _mbsncpy
@ stub _mbsnextc
@ stub _mbsnicmp
@ stub _mbsninc
@ stub _mbsnset
@ stub _mbspbrk
@ cdecl _mbsrchr(str long) CRTDLL__mbsrchr
@ stub _mbsrev
@ stub _mbsset
@ stub _mbsspn
@ stub _mbsspnp
@ stub _mbsstr
@ stub _mbstok
@ stub _mbstrlen
@ stub _mbsupr
@ cdecl _memccpy(ptr ptr long long) memccpy
@ forward _memicmp ntdll._memicmp
@ cdecl _mkdir(str) CRTDLL__mkdir
@ cdecl _mktemp(str) CRTDLL__mktemp
@ cdecl _msize(ptr) CRTDLL__msize
@ cdecl _nextafter(double double) CRTDLL__nextafter
@ cdecl _onexit(ptr) CRTDLL__onexit
@ cdecl _open(str long) CRTDLL__open
@ cdecl _open_osfhandle(long long) CRTDLL__open_osfhandle
@ extern _osmajor_dll CRTDLL_osmajor_dll
@ extern _osminor_dll CRTDLL_osminor_dll
@ extern _osmode_dll CRTDLL_osmode_dll
@ extern _osver_dll CRTDLL_osver_dll
@ extern _osversion_dll CRTDLL_osversion_dll
@ stub _pclose
@ extern _pctype_dll CRTDLL_pctype_dll
@ stub _pgmptr_dll
@ stub _pipe
@ stub _popen
@ cdecl _purecall() CRTDLL__purecall
@ stub _putch
@ stub _putenv
@ cdecl _putw(long ptr) CRTDLL__putw
@ stub _pwctype_dll
@ cdecl _read(long ptr long) CRTDLL__read
@ cdecl _rmdir(str) CRTDLL__rmdir
@ stub _rmtmp
@ cdecl _rotl (long long) CRTDLL__rotl
@ cdecl _rotr (long long) CRTDLL__rotr
@ cdecl _scalb (double long) CRTDLL__scalb
@ stub _searchenv
@ stub _seterrormode
@ cdecl _setjmp (ptr) CRTDLL__setjmp
@ cdecl _setmode(long long) CRTDLL__setmode
@ stub _setsystime
@ cdecl _sleep(long) CRTDLL__sleep
@ varargs _snprintf(ptr long ptr) snprintf
@ stub _snwprintf
@ stub _sopen
@ stub _spawnl
@ stub _spawnle
@ stub _spawnlp
@ stub _spawnlpe
@ stub _spawnv
@ cdecl _spawnve(long str ptr ptr) CRTDLL__spawnve
@ stub _spawnvp
@ stub _spawnvpe
@ cdecl _splitpath (str ptr ptr ptr ptr) CRTDLL__splitpath
@ cdecl _stat (str ptr) CRTDLL__stat
@ cdecl _statusfp() CRTDLL__statusfp
@ cdecl _strcmpi(str str) strcasecmp
@ cdecl _strdate(str) CRTDLL__strdate
@ cdecl _strdec(str str) CRTDLL__strdec
@ cdecl _strdup(str) CRTDLL__strdup
@ cdecl _strerror(long) CRTDLL__strerror
@ cdecl _stricmp(str str) strcasecmp
@ stub _stricoll
@ cdecl _strinc(str) CRTDLL__strinc
@ forward _strlwr ntdll._strlwr
@ cdecl _strncnt(str long) CRTDLL__strncnt
@ cdecl _strnextc(str) CRTDLL__strnextc
@ cdecl _strnicmp(str str long) strncasecmp
@ cdecl _strninc(str long) CRTDLL__strninc
@ cdecl _strnset(str long long) CRTDLL__strnset
@ cdecl _strrev(str) CRTDLL__strrev
@ cdecl _strset(str long) CRTDLL__strset
@ cdecl _strspnp(str str) CRTDLL__strspnp
@ cdecl _strtime(str) CRTDLL__strtime
@ forward _strupr ntdll._strupr
@ cdecl _swab(str str long) CRTDLL__swab
@ extern _sys_errlist sys_errlist
@ extern _sys_nerr_dll CRTDLL__sys_nerr
@ cdecl _tell(long) CRTDLL__tell
@ cdecl _tempnam(str ptr) CRTDLL__tempnam
@ stub _timezone_dll
@ cdecl _tolower(long) CRTDLL__toupper
@ cdecl _toupper(long) CRTDLL__tolower
@ stub _tzname
@ stub _tzset
@ forward _ultoa ntdll._ultoa
@ cdecl _ultow(long str long) CRTDLL__ultow
@ cdecl _umask(long) CRTDLL__umask
@ stub _ungetch
@ cdecl _unlink(str) CRTDLL__unlink
@ cdecl _unloaddll(long) CRTDLL__unloaddll
@ cdecl _utime(str ptr) CRTDLL__utime
@ cdecl _vsnprintf(ptr long ptr ptr) vsnprintf
@ stub _vsnwprintf
@ cdecl _wcsdup(wstr) CRTDLL__wcsdup
@ forward _wcsicmp ntdll._wcsicmp
@ cdecl _wcsicoll(wstr wstr) CRTDLL__wcsicoll
@ forward _wcslwr ntdll._wcslwr
@ forward _wcsnicmp ntdll._wcsnicmp
@ cdecl _wcsnset(wstr long long) CRTDLL__wcsnset
@ cdecl _wcsrev(wstr) CRTDLL__wcsrev
@ cdecl _wcsset(wstr long) CRTDLL__wcsset
@ forward _wcsupr ntdll._wcsupr
@ extern _winmajor_dll CRTDLL_winmajor_dll
@ extern _winminor_dll CRTDLL_winminor_dll
@ extern _winver_dll CRTDLL_winver_dll
@ cdecl _write(long ptr long) CRTDLL__write
@ stub _wtoi
@ stub _wtol
@ cdecl _y0(double) CRTDLL__y0
@ cdecl _y1(double) CRTDLL__y1
@ cdecl _yn(long double) CRTDLL__yn
@ cdecl abort() CRTDLL_abort
@ cdecl abs(long) abs
@ cdecl acos(double) acos
@ cdecl asctime(ptr) asctime
@ cdecl asin(double) asin
@ cdecl atan(double) atan
@ cdecl atan2(double double) atan2
@ cdecl atexit(ptr) CRTDLL_atexit
@ cdecl atof(str) atof
@ cdecl atoi(str) atoi
@ cdecl atol(str) atol
@ cdecl bsearch(ptr ptr long long ptr) bsearch
@ cdecl calloc(long long) CRTDLL_calloc
@ cdecl ceil(double) ceil
@ cdecl clearerr(ptr) CRTDLL_clearerr
@ cdecl clock() CRTDLL_clock
@ cdecl cos(double) cos
@ cdecl cosh(double) cosh
@ cdecl ctime(ptr) ctime
@ cdecl difftime(long long) CRTDLL_difftime
@ cdecl -noimport div(long long) CRTDLL_div
@ cdecl exit(long) CRTDLL_exit
@ cdecl exp(double) exp
@ cdecl fabs(double) fabs
@ cdecl fclose(ptr) CRTDLL_fclose
@ cdecl feof(ptr) CRTDLL_feof
@ cdecl ferror(ptr) CRTDLL_ferror
@ cdecl fflush(ptr) CRTDLL_fflush
@ cdecl fgetc(ptr) CRTDLL_fgetc
@ cdecl fgetpos(ptr ptr) CRTDLL_fgetpos
@ cdecl fgets(ptr long ptr) CRTDLL_fgets
@ stub fgetwc
@ cdecl floor(double) floor
@ cdecl fmod(double double) fmod
@ cdecl fopen(str str) CRTDLL_fopen
@ varargs fprintf(ptr str) CRTDLL_fprintf
@ cdecl fputc(long ptr) CRTDLL_fputc
@ cdecl fputs(str ptr) CRTDLL_fputs
@ stub fputwc
@ cdecl fread(ptr long long ptr) CRTDLL_fread
@ cdecl free(ptr) CRTDLL_free
@ cdecl freopen(str str ptr) CRTDLL_freopen
@ cdecl frexp(double ptr) frexp
@ varargs fscanf(ptr str) CRTDLL_fscanf
@ cdecl fseek(ptr long long) CRTDLL_fseek
@ cdecl fsetpos(ptr ptr) CRTDLL_fsetpos
@ cdecl ftell(ptr) CRTDLL_ftell
@ stub fwprintf
@ cdecl fwrite(ptr long long ptr) CRTDLL_fwrite 
@ stub fwscanf
@ cdecl getc(ptr) CRTDLL_getc
@ cdecl getchar() CRTDLL_getchar
@ cdecl getenv (str) CRTDLL_getenv
@ cdecl gets(ptr) CRTDLL_gets
@ cdecl gmtime(ptr) gmtime
@ forward is_wctype ntdll.iswctype
@ cdecl isalnum(long) CRTDLL_isalnum
@ cdecl isalpha(long) CRTDLL_isalpha
@ cdecl iscntrl(long) CRTDLL_iscntrl
@ cdecl isdigit(long) CRTDLL_isdigit
@ cdecl isgraph(long) CRTDLL_isgraph
@ cdecl isleadbyte(long) CRTDLL_isleadbyte
@ cdecl islower(long) CRTDLL_islower
@ cdecl isprint(long) CRTDLL_isprint
@ cdecl ispunct(long) CRTDLL_ispunct
@ cdecl isspace(long) CRTDLL_isspace
@ cdecl isupper(long) CRTDLL_isupper
@ cdecl iswalnum(long) CRTDLL_iswalnum
@ forward iswalpha ntdll.iswalpha
@ cdecl iswascii(long) CRTDLL_iswascii
@ cdecl iswcntrl(long) CRTDLL_iswcntrl
@ forward iswctype ntdll.iswctype
@ cdecl iswdigit(long) CRTDLL_iswdigit
@ cdecl iswgraph(long) CRTDLL_iswgraph
@ cdecl iswlower(long) CRTDLL_iswlower
@ cdecl iswprint(long) CRTDLL_iswprint
@ cdecl iswpunct(long) CRTDLL_iswpunct
@ cdecl iswspace(long) CRTDLL_iswspace
@ cdecl iswupper(long) CRTDLL_iswupper
@ cdecl iswxdigit(long) CRTDLL_iswxdigit
@ cdecl isxdigit(long) CRTDLL_isxdigit
@ cdecl labs(long) labs
@ cdecl ldexp(double long) CRTDLL_ldexp
@ cdecl -noimport ldiv(long long) CRTDLL_ldiv
@ stub localeconv
@ cdecl localtime(ptr) localtime
@ cdecl log(double) log
@ cdecl log10(double) log10
@ cdecl longjmp(ptr long) CRTDLL_longjmp
@ cdecl malloc(ptr) CRTDLL_malloc
@ cdecl mblen(str long) mblen
@ forward mbstowcs ntdll.mbstowcs
@ cdecl mbtowc(ptr ptr long) CRTDLL_mbtowc
@ cdecl memchr(ptr long long) memchr
@ cdecl memcmp(ptr ptr long) memcmp
@ cdecl memcpy(ptr ptr long) memcpy
@ cdecl memmove(ptr ptr long) memmove
@ cdecl memset(ptr long long) memset 
@ cdecl mktime(ptr) mktime
@ cdecl modf(double ptr) modf
@ cdecl perror(str) CRTDLL_perror
@ cdecl pow(double double) pow
@ varargs printf() printf
@ cdecl putc(long ptr) CRTDLL_putc
@ cdecl putchar(long) CRTDLL_putchar
@ cdecl puts(str) CRTDLL_puts
@ cdecl qsort(ptr long long ptr) qsort
@ stub raise
@ cdecl rand() CRTDLL_rand
@ cdecl realloc(ptr long) CRTDLL_realloc
@ cdecl remove(str) CRTDLL_remove
@ cdecl rename(str str) CRTDLL_rename
@ cdecl rewind(ptr) CRTDLL_rewind
@ stub scanf
@ cdecl setbuf(ptr ptr) CRTDLL_setbuf
@ cdecl setlocale(long ptr) CRTDLL_setlocale
@ stub setvbuf
@ cdecl signal(long ptr) CRTDLL_signal
@ cdecl sin(double) sin
@ cdecl sinh(double) sinh
@ varargs sprintf(ptr ptr) sprintf
@ cdecl sqrt(double) sqrt
@ cdecl srand(long) srand
@ varargs sscanf() sscanf
@ cdecl strcat(str str) strcat
@ cdecl strchr(str long) strchr
@ cdecl strcmp(str str) strcmp
@ cdecl strcoll(str str) strcoll
@ cdecl strcpy(ptr str) strcpy
@ cdecl strcspn(str str) strcspn
@ cdecl strerror(long) CRTDLL_strerror
@ cdecl strftime(ptr long str ptr) strftime
@ cdecl strlen(str) strlen
@ cdecl strncat(str str long) strncat
@ cdecl strncmp(str str long) strncmp
@ cdecl strncpy(ptr str long) strncpy
@ cdecl strpbrk(str str) strpbrk
@ cdecl strrchr(str long) strrchr
@ cdecl strspn(str str) strspn
@ cdecl strstr(str str) strstr
@ cdecl strtod(str ptr) strtod
@ cdecl strtok(str str) strtok
@ cdecl strtol(str ptr long) strtol
@ cdecl strtoul(str ptr long) strtoul
@ cdecl strxfrm(ptr str long) strxfrm
@ stub swprintf
@ stub swscanf
@ cdecl system(str) CRTDLL_system
@ cdecl tan(double) tan
@ cdecl tanh(double) tanh
@ cdecl time(ptr) CRTDLL_time
@ stub tmpfile
@ cdecl tmpnam(str) CRTDLL_tmpnam
@ cdecl tolower(long) tolower
@ cdecl toupper(long) toupper
@ forward towlower ntdll.towlower
@ forward towupper ntdll.towupper
@ stub ungetc
@ stub ungetwc
@ cdecl vfprintf(ptr str ptr) CRTDLL_vfprintf
@ stub vfwprintf
@ stub vprintf
@ cdecl vsprintf(ptr str ptr) vsprintf
@ stub vswprintf
@ stub vwprintf
@ forward wcscat ntdll.wcscat
@ forward wcschr ntdll.wcschr
@ forward wcscmp ntdll.wcscmp
@ cdecl wcscoll(wstr wstr) CRTDLL_wcscoll
@ forward wcscpy ntdll.wcscpy
@ forward wcscspn ntdll.wcscspn
@ stub wcsftime
@ forward wcslen ntdll.wcslen
@ forward wcsncat ntdll.wcsncat
@ forward wcsncmp ntdll.wcsncmp
@ forward wcsncpy ntdll.wcsncpy
@ cdecl wcspbrk(wstr wstr) CRTDLL_wcspbrk
@ forward wcsrchr ntdll.wcsrchr
@ forward wcsspn ntdll.wcsspn
@ forward wcsstr ntdll.wcsstr
@ stub wcstod
@ forward wcstok ntdll.wcstok
@ forward wcstol ntdll.wcstol
@ forward wcstombs ntdll.wcstombs
@ stub wcstoul
@ stub wcsxfrm
@ cdecl wctomb(ptr long) CRTDLL_wctomb
@ stub wprintf
@ stub wscanf
