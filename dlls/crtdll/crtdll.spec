# C RunTime DLL. All functions use cdecl!
name	crtdll
type	win32
init    CRTDLL_Init

@ cdecl ??2@YAPAXI@Z(long) CRTDLL_new
@ cdecl ??3@YAXPAX@Z(long) CRTDLL_delete
@ cdecl ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) CRTDLL_set_new_handler
@ stub _CIacos
@ stub _CIasin
@ stub _CIatan
@ stub _CIatan2
@ stub _CIcos
@ stub _CIcosh
@ stub _CIexp
@ stub _CIfmod
@ stub _CIlog
@ stub _CIlog10
@ cdecl _CIpow() CRTDLL__CIpow
@ stub _CIsin
@ stub _CIsinh
@ stub _CIsqrt
@ stub _CItan
@ stub _CItanh
@ stub _HUGE_dll
@ stub _XcptFilter
@ cdecl __GetMainArgs(ptr ptr ptr long) CRTDLL__GetMainArgs
@ extern __argc_dll CRTDLL_argc_dll
@ extern __argv_dll CRTDLL_argv_dll
@ cdecl __dllonexit() CRTDLL__dllonexit
@ stub __doserrno
@ stub __fpecode
@ stub __isascii
@ stub __iscsym
@ stub __iscsymf
@ stub __mb_cur_max_dll
@ stub __pxcptinfoptrs
@ cdecl __threadhandle() GetCurrentThread
@ cdecl __threadid() GetCurrentThreadId
@ stub __toascii
@ cdecl _abnormal_termination() CRTDLL__abnormal_termination
@ cdecl _access(str long) CRTDLL__access
@ extern _acmdln_dll CRTDLL_acmdln_dll
@ stub _aexit_rtn_dll
@ stub _amsg_exit
@ stub _assert
@ extern _basemajor_dll CRTDLL_basemajor_dll
@ extern _baseminor_dll CRTDLL_baseminor_dll
@ extern _baseversion_dll CRTDLL_baseversion_dll
@ stub _beep
@ stub _beginthread
@ stub _c_exit
@ stub _cabs
@ cdecl _cexit(long) CRTDLL__cexit
@ stub _cgets
@ cdecl _chdir(str) CRTDLL__chdir
@ cdecl _chdrive(long) CRTDLL__chdrive
@ stub _chgsign
@ stub _chmod
@ stub _chsize
@ stub _clearfp
@ cdecl _close(long) CRTDLL__close
@ stub _commit
@ extern _commode_dll CRTDLL_commode_dll
@ stub _control87
@ stub _controlfp
@ stub _copysign
@ stub _cprintf
@ stub _cpumode_dll
@ stub _cputs
@ stub _creat
@ stub _cscanf
@ stub _ctype
@ stub _cwait
@ stub _daylight_dll
@ stub _dup
@ stub _dup2
@ stub _ecvt
@ stub _endthread
@ extern _environ_dll CRTDLL_environ_dll
@ stub _eof
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
@ stub _exit
@ stub _expand
@ stub _fcloseall
@ stub _fcvt
@ cdecl _fdopen(long ptr) CRTDLL__fdopen
@ stub _fgetchar
@ stub _fgetwchar
@ stub _filbuf
@ stub _fileinfo_dll
@ stub _filelength
@ stub _fileno
@ stub _findclose
@ cdecl _findfirst(str ptr) CRTDLL__findfirst
@ cdecl _findnext(long ptr) CRTDLL__findnext
@ stub _finite
@ stub _flsbuf
@ stub _flushall
@ extern _fmode_dll CRTDLL_fmode_dll
@ stub _fpclass
@ stub _fpieee_flt
@ cdecl _fpreset() CRTDLL__fpreset
@ stub _fputchar
@ stub _fputwchar
@ cdecl _fsopen(str str long) CRTDLL__fsopen
@ cdecl _fstat(long ptr) CRTDLL__fstat
@ stub _ftime
@ cdecl _ftol() CRTDLL__ftol
@ cdecl _fullpath(ptr str long) CRTDLL__fullpath
@ stub _futime
@ stub _gcvt
@ stub _get_osfhandle
@ stub _getch
@ stub _getche
@ cdecl _getcwd(ptr long) CRTDLL__getcwd
@ cdecl _getdcwd(long ptr long) CRTDLL__getdcwd
@ stub _getdiskfree
@ stub _getdllprocaddr
@ cdecl _getdrive() CRTDLL__getdrive
@ stub _getdrives
@ stub _getpid
@ stub _getsystime
@ stub _getw
@ cdecl _global_unwind2(ptr) CRTDLL__global_unwind2
@ stub _heapchk
@ stub _heapmin
@ stub _heapset
@ stub _heapwalk
@ cdecl _hypot(double double) hypot
@ cdecl _initterm(ptr ptr) CRTDLL__initterm
@ extern _iob CRTDLL_iob
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
@ stub _isnan
@ cdecl _itoa(long ptr long) CRTDLL__itoa
@ stub _itow
@ cdecl _j0(double) j0
@ cdecl _j1(double) j1
@ cdecl _jn(long double) jn
@ stub _kbhit
@ stub _lfind
@ stub _loaddll
@ cdecl _local_unwind2(ptr long) CRTDLL__local_unwind2
@ stub _locking
@ stub _logb
@ cdecl _lrotl (long long) CRTDLL__lrotl
@ stub _lrotr
@ stub _lsearch
@ stub _lseek
@ cdecl _ltoa(long str long) CRTDLL__ltoa
@ stub _ltow
@ cdecl _makepath (ptr str str str str) CRTDLL__makepath
@ stub _matherr
@ stub _mbbtombc
@ stub _mbbtype
@ stub _mbccpy
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
@ stub _memccpy
@ cdecl _memicmp(str str long) CRTDLL__memicmp
@ cdecl _mkdir(str) CRTDLL__mkdir
@ stub _mktemp
@ stub _msize
@ stub _nextafter
@ stub _onexit
@ cdecl _open(str long) CRTDLL__open
@ cdecl _open_osfhandle(long long) CRTDLL__open_osfhandle
@ extern _osmajor_dll CRTDLL_osmajor_dll
@ extern _osminor_dll CRTDLL_osminor_dll
@ extern _osmode_dll CRTDLL_osmode_dll
@ extern _osver_dll CRTDLL_osver_dll
@ extern _osversion_dll CRTDLL_osversion_dll
@ stub _pclose
@ stub _pctype_dll
@ stub _pgmptr_dll
@ stub _pipe
@ stub _popen
@ stub _purecall
@ stub _putch
@ stub _putenv
@ stub _putw
@ stub _pwctype_dll
@ cdecl _read(long ptr long) CRTDLL__read
@ stub _rmdir
@ stub _rmtmp
@ cdecl _rotl (long long) CRTDLL__rotl
@ stub _rotr
@ stub _scalb
@ stub _searchenv
@ stub _seterrormode
@ cdecl _setjmp (ptr) CRTDLL__setjmp
@ cdecl _setmode(long long) CRTDLL__setmode
@ stub _setsystime
@ cdecl _sleep(long) CRTDLL__sleep
@ stub _snprintf
@ stub _snwprintf
@ stub _sopen
@ stub _spawnl
@ stub _spawnle
@ stub _spawnlp
@ stub _spawnlpe
@ stub _spawnv
@ stub _spawnve
@ stub _spawnvp
@ stub _spawnvpe
@ cdecl _splitpath (str ptr ptr ptr ptr) CRTDLL__splitpath
@ cdecl _stat (str ptr) CRTDLL__stat
@ stub _statusfp
@ cdecl _strcmpi(str str) CRTDLL__strcmpi
@ cdecl _strdate(str) CRTDLL__strdate
@ stub _strdec
@ cdecl _strdup(str) CRTDLL__strdup
@ stub _strerror
@ cdecl _stricmp(str str) CRTDLL__strcmpi
@ stub _stricoll
@ stub _strinc
@ cdecl _strlwr(str) CRTDLL__strlwr
@ stub _strncnt
@ stub _strnextc
@ cdecl _strnicmp(str str long) CRTDLL__strnicmp
@ stub _strninc
@ stub _strnset
@ stub _strrev
@ stub _strset
@ stub _strspnp
@ cdecl _strtime(str) CRTDLL__strtime
@ cdecl _strupr(str) CRTDLL__strupr
@ stub _swab
@ stub _sys_errlist
@ stub _sys_nerr_dll
@ stub _tell
@ cdecl _tempnam(str ptr) CRTDLL__tempnam
@ stub _timezone_dll
@ stub _tolower
@ stub _toupper
@ stub _tzname
@ stub _tzset
@ cdecl _ultoa(long ptr long) CRTDLL__ultoa
@ stub _ultow
@ stub _umask
@ stub _ungetch
@ cdecl _unlink(str) CRTDLL__unlink
@ stub _unloaddll
@ stub _utime
@ stub _vsnprintf
@ stub _vsnwprintf
@ cdecl _wcsdup(wstr) CRTDLL__wcsdup
@ cdecl _wcsicmp(wstr wstr) CRTDLL__wcsicmp
@ cdecl _wcsicoll(wstr wstr) CRTDLL__wcsicoll
@ cdecl _wcslwr(wstr) CRTDLL__wcslwr
@ cdecl _wcsnicmp(wstr wstr long) CRTDLL__wcsnicmp
@ cdecl _wcsnset(wstr long long) CRTDLL__wcsnset
@ cdecl _wcsrev(wstr) CRTDLL__wcsrev
@ cdecl _wcsset(wstr long) CRTDLL__wcsset
@ cdecl _wcsupr(wstr) CRTDLL__wcsupr
@ extern _winmajor_dll CRTDLL_winmajor_dll
@ extern _winminor_dll CRTDLL_winminor_dll
@ extern _winver_dll CRTDLL_winver_dll
@ cdecl _write(long ptr long) CRTDLL__write
@ stub _wtoi
@ stub _wtol
@ cdecl _y0(double) y0
@ cdecl _y1(double) y1
@ cdecl _yn(long double) yn
@ stub abort
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
@ stub clearerr
@ cdecl clock() CRTDLL_clock
@ cdecl cos(double) cos
@ cdecl cosh(double) cosh
@ cdecl ctime(ptr) ctime
@ stub difftime
@ cdecl div(long long) div
@ cdecl exit(long) CRTDLL_exit
@ cdecl exp(double) exp
@ cdecl fabs(double) fabs
@ cdecl fclose(ptr) CRTDLL_fclose
@ cdecl feof(ptr) CRTDLL_feof
@ stub ferror
@ cdecl fflush(ptr) CRTDLL_fflush
@ cdecl fgetc(ptr) CRTDLL_fgetc
@ stub fgetpos
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
@ stub getchar
@ cdecl getenv (str) CRTDLL_getenv
@ cdecl gets(ptr) CRTDLL_gets
@ cdecl gmtime(ptr) gmtime
@ stub is_wctype
@ cdecl isalnum(long) isalnum
@ cdecl isalpha(long) isalpha
@ cdecl iscntrl(long) iscntrl
@ cdecl isdigit(long) isdigit
@ cdecl isgraph(long) isgraph
@ stub isleadbyte
@ cdecl islower(long) islower
@ cdecl isprint(long) isprint
@ cdecl ispunct(long) ispunct
@ cdecl isspace(long) isspace
@ cdecl isupper(long) isupper
@ cdecl iswalnum(long) CRTDLL_iswalnum
@ cdecl iswalpha(long) CRTDLL_iswalpha
@ stub iswascii
@ cdecl iswcntrl(long) CRTDLL_iswcntrl
@ cdecl iswctype(long long) CRTDLL_iswctype
@ cdecl iswdigit(long) CRTDLL_iswdigit
@ cdecl iswgraph(long) CRTDLL_iswgraph
@ cdecl iswlower(long) CRTDLL_iswlower
@ cdecl iswprint(long) CRTDLL_iswprint
@ cdecl iswpunct(long) CRTDLL_iswpunct
@ cdecl iswspace(long) CRTDLL_iswspace
@ cdecl iswupper(long) CRTDLL_iswupper
@ cdecl iswxdigit(long) CRTDLL_iswxdigit
@ cdecl isxdigit(long) isxdigit
@ cdecl labs(long) labs
@ cdecl ldexp(double long) ldexp
@ cdecl ldiv(long long) ldiv
@ stub localeconv
@ cdecl localtime(ptr) localtime
@ cdecl log(double) log
@ cdecl log10(double) log10
@ cdecl longjmp(ptr long) CRTDLL_longjmp
@ cdecl malloc(ptr) CRTDLL_malloc
@ cdecl mblen(str long) mblen
@ cdecl mbstowcs(ptr str long) CRTDLL_mbstowcs
@ cdecl mbtowc(ptr ptr long) CRTDLL_mbtowc
@ cdecl memchr(ptr long long) memchr
@ cdecl memcmp(ptr ptr long) memcmp
@ cdecl memcpy(ptr ptr long) memcpy
@ cdecl memmove(ptr ptr long) memmove
@ cdecl memset(ptr long long) memset 
@ cdecl mktime(ptr) mktime
@ cdecl modf(double ptr) modf
@ stub perror
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
@ stub rewind
@ stub scanf
@ cdecl setbuf(ptr ptr) CRTDLL_setbuf
@ cdecl setlocale(long ptr) CRTDLL_setlocale
@ stub setvbuf
@ cdecl signal(long ptr) CRTDLL_signal
@ cdecl sin(double) sin
@ cdecl sinh(double) sinh
@ varargs sprintf() sprintf
@ cdecl sqrt(double) sqrt
@ cdecl srand(long) CRTDLL_srand
@ varargs sscanf() sscanf
@ cdecl strcat(str str) strcat
@ cdecl strchr(str long) strchr
@ cdecl strcmp(str str) strcmp
@ cdecl strcoll(str str) strcoll
@ cdecl strcpy(ptr str) strcpy
@ cdecl strcspn(str str) strcspn
@ cdecl strerror(long) strerror
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
@ varargs swprintf() wsprintfW
@ stub swscanf
@ cdecl system(str) CRTDLL_system
@ cdecl tan(double) tan
@ cdecl tanh(double) tanh
@ cdecl time(ptr) CRTDLL_time
@ stub tmpfile
@ cdecl tmpnam(str) CRTDLL_tmpnam
@ cdecl tolower(long) tolower
@ cdecl toupper(long) toupper
@ cdecl towlower(long) towlower
@ cdecl towupper(long) towupper
@ stub ungetc
@ stub ungetwc
@ cdecl vfprintf(ptr str ptr) CRTDLL_vfprintf
@ stub vfwprintf
@ stub vprintf
@ cdecl vsprintf(ptr str ptr) CRTDLL_vsprintf
@ cdecl vswprintf(ptr wstr ptr) CRTDLL_vswprintf
@ stub vwprintf
@ cdecl wcscat(wstr wstr) CRTDLL_wcscat
@ cdecl wcschr(wstr long) CRTDLL_wcschr
@ cdecl wcscmp(wstr wstr) CRTDLL_wcscmp
@ cdecl wcscoll(wstr wstr) CRTDLL_wcscoll
@ cdecl wcscpy(ptr wstr) CRTDLL_wcscpy
@ cdecl wcscspn(wstr wstr) CRTDLL_wcscspn
@ stub wcsftime
@ cdecl wcslen(wstr) CRTDLL_wcslen
@ cdecl wcsncat(wstr wstr long) CRTDLL_wcsncat
@ cdecl wcsncmp(wstr wstr long) CRTDLL_wcsncmp
@ cdecl wcsncpy(ptr wstr long) CRTDLL_wcsncpy
@ cdecl wcspbrk(wstr wstr) CRTDLL_wcspbrk
@ cdecl wcsrchr(wstr long) CRTDLL_wcsrchr
@ cdecl wcsspn(wstr wstr) CRTDLL_wcsspn
@ cdecl wcsstr(wstr wstr) CRTDLL_wcsstr
@ stub wcstod
@ cdecl wcstok(wstr wstr) CRTDLL_wcstok
@ cdecl wcstol(wstr ptr long) CRTDLL_wcstol
@ cdecl wcstombs(ptr ptr long) CRTDLL_wcstombs
@ stub wcstoul
@ stub wcsxfrm
@ cdecl wctomb(ptr long) CRTDLL_wctomb
@ stub wprintf
@ stub wscanf
