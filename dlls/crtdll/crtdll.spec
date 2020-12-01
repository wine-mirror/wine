# Old C runtime library. All functions provided by msvcrt

@ cdecl -arch=win32 ??2@YAPAXI@Z(long) operator_new
@ cdecl -arch=win64 ??2@YAPEAX_K@Z(long) operator_new
@ cdecl -arch=win32 ??3@YAXPAX@Z(ptr) operator_delete
@ cdecl -arch=win64 ??3@YAXPEAX@Z(ptr) operator_delete
@ cdecl -arch=win32 ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) _set_new_handler
@ cdecl -arch=win64 ?_set_new_handler@@YAP6AH_K@ZP6AH0@Z@Z(ptr) _set_new_handler
@ cdecl -arch=i386 _CIacos()
@ cdecl -arch=i386 _CIasin()
@ cdecl -arch=i386 _CIatan()
@ cdecl -arch=i386 _CIatan2()
@ cdecl -arch=i386 _CIcos()
@ cdecl -arch=i386 _CIcosh()
@ cdecl -arch=i386 _CIexp()
@ cdecl -arch=i386 _CIfmod()
@ cdecl -arch=i386 _CIlog()
@ cdecl -arch=i386 _CIlog10()
@ cdecl -arch=i386 _CIpow()
@ cdecl -arch=i386 _CIsin()
@ cdecl -arch=i386 _CIsinh()
@ cdecl -arch=i386 _CIsqrt()
@ cdecl -arch=i386 _CItan()
@ cdecl -arch=i386 _CItanh()
@ extern _HUGE_dll MSVCRT__HUGE
@ cdecl _XcptFilter(long ptr)
@ cdecl __GetMainArgs(ptr ptr ptr long)
@ extern __argc_dll MSVCRT___argc
@ extern __argv_dll MSVCRT___argv
@ cdecl __dllonexit(ptr ptr ptr)
@ cdecl __doserrno()
@ cdecl __fpecode()
@ cdecl __isascii(long)
@ cdecl __iscsym(long)
@ cdecl __iscsymf(long)
@ extern __mb_cur_max_dll MSVCRT___mb_cur_max
@ cdecl __pxcptinfoptrs()
@ cdecl __threadhandle() kernel32.GetCurrentThread
@ cdecl __threadid() kernel32.GetCurrentThreadId
@ cdecl __toascii(long)
@ cdecl _abnormal_termination()
@ cdecl _access(str long) MSVCRT__access
@ extern _acmdln_dll MSVCRT__acmdln
@ extern _aexit_rtn_dll _aexit_rtn
@ cdecl _amsg_exit(long)
@ cdecl _assert(str str long)
@ extern _basemajor_dll CRTDLL__basemajor_dll
@ extern _baseminor_dll CRTDLL__baseminor_dll
@ extern _baseversion_dll MSVCRT__osver
@ cdecl _beep(long long)
@ cdecl _beginthread(ptr long ptr)
@ cdecl _c_exit()
@ cdecl _cabs(long) MSVCRT__cabs
@ cdecl _cexit()
@ cdecl _cgets(ptr)
@ cdecl _chdir(str) MSVCRT__chdir
@ cdecl _chdrive(long) MSVCRT__chdrive
@ cdecl _chgsign(double) MSVCRT__chgsign
@ cdecl _chmod(str long) MSVCRT__chmod
@ cdecl _chsize(long long) MSVCRT__chsize
@ cdecl _clearfp()
@ cdecl _close(long) MSVCRT__close
@ cdecl _commit(long) MSVCRT__commit
@ extern _commode_dll MSVCRT__commode
@ cdecl _control87(long long)
@ cdecl _controlfp(long long)
@ cdecl _copysign(double double) MSVCRT__copysign
@ varargs _cprintf(str)
@ extern _cpumode_dll CRTDLL__cpumode_dll
@ cdecl _cputs(str)
@ cdecl _creat(str long) MSVCRT__creat
@ varargs _cscanf(str)
@ extern _ctype MSVCRT__ctype
@ cdecl _cwait(ptr long long)
@ extern _daylight_dll MSVCRT___daylight
@ cdecl _dup(long) MSVCRT__dup
@ cdecl _dup2(long long) MSVCRT__dup2
@ cdecl _ecvt(double long ptr ptr) MSVCRT__ecvt
@ cdecl _endthread()
@ extern _environ_dll MSVCRT__environ
@ cdecl _eof(long) MSVCRT__eof
@ cdecl _errno()
@ cdecl -arch=i386 _except_handler2(ptr ptr ptr ptr)
@ varargs _execl(str str)
@ varargs _execle(str str)
@ varargs _execlp(str str)
@ varargs _execlpe(str str)
@ cdecl _execv(str ptr)
@ cdecl _execve(str ptr ptr)
@ cdecl _execvp(str ptr)
@ cdecl _execvpe(str ptr ptr)
@ cdecl _exit(long)
@ cdecl _expand(ptr long)
@ cdecl _fcloseall() MSVCRT__fcloseall
@ cdecl _fcvt(double long ptr ptr) MSVCRT__fcvt
@ cdecl _fdopen(long str) MSVCRT__fdopen
@ cdecl _fgetchar() MSVCRT__fgetchar
@ cdecl _fgetwchar() MSVCRT__fgetwchar
@ cdecl _filbuf(ptr) MSVCRT__filbuf
# extern _fileinfo_dll
@ cdecl _filelength(long) MSVCRT__filelength
@ cdecl _fileno(ptr) MSVCRT__fileno
@ cdecl _findclose(long) MSVCRT__findclose
@ cdecl _findfirst(str ptr) MSVCRT__findfirst
@ cdecl _findnext(long ptr) MSVCRT__findnext
@ cdecl _finite(double) MSVCRT__finite
@ cdecl _flsbuf(long ptr) MSVCRT__flsbuf
@ cdecl _flushall() MSVCRT__flushall
@ extern _fmode_dll MSVCRT__fmode
@ cdecl _fpclass(double) MSVCRT__fpclass
@ cdecl -arch=i386,x86_64,arm,arm64 _fpieee_flt(long ptr ptr)
@ cdecl _fpreset()
@ cdecl _fputchar(long) MSVCRT__fputchar
@ cdecl _fputwchar(long) MSVCRT__fputwchar
@ cdecl _fsopen(str str long) MSVCRT__fsopen
@ cdecl _fstat(long ptr) MSVCRT__fstat
@ cdecl _ftime(ptr) MSVCRT__ftime
@ cdecl -arch=i386 -ret64 _ftol() MSVCRT__ftol
@ cdecl _fullpath(ptr str long) MSVCRT__fullpath
@ cdecl -arch=win32 _futime(long ptr) _futime32
@ cdecl -arch=win64 _futime(long ptr) _futime64
@ cdecl _gcvt(double long str) MSVCRT__gcvt
@ cdecl _get_osfhandle(long) MSVCRT__get_osfhandle
@ cdecl _getch()
@ cdecl _getche()
@ cdecl _getcwd(str long) MSVCRT__getcwd
@ cdecl _getdcwd(long str long) MSVCRT__getdcwd
@ cdecl _getdiskfree(long ptr) MSVCRT__getdiskfree
@ cdecl _getdllprocaddr(long str long)
@ cdecl _getdrive() MSVCRT__getdrive
@ cdecl _getdrives() kernel32.GetLogicalDrives
@ cdecl _getpid()
@ stub _getsystime(ptr)
@ cdecl _getw(ptr) MSVCRT__getw
@ cdecl -arch=i386 _global_unwind2(ptr)
@ cdecl _heapchk()
@ cdecl _heapmin()
@ cdecl _heapset(long)
@ cdecl _heapwalk(ptr)
@ cdecl _hypot(double double)
@ cdecl _initterm(ptr ptr)
@ extern _iob MSVCRT__iob
@ cdecl _isatty(long) MSVCRT__isatty
@ cdecl _isctype(long long)
@ stub _ismbbalnum(long)
@ stub _ismbbalpha(long)
@ stub _ismbbgraph(long)
@ stub _ismbbkalnum(long)
@ cdecl _ismbbkana(long)
@ stub _ismbbkpunct(long)
@ cdecl _ismbblead(long)
@ stub _ismbbprint(long)
@ stub _ismbbpunct(long)
@ cdecl _ismbbtrail(long)
@ cdecl _ismbcalpha(long)
@ cdecl _ismbcdigit(long)
@ cdecl _ismbchira(long)
@ cdecl _ismbckata(long)
@ cdecl _ismbcl0(long)
@ cdecl _ismbcl1(long)
@ cdecl _ismbcl2(long)
@ cdecl _ismbclegal(long)
@ cdecl _ismbclower(long)
@ cdecl _ismbcprint(long)
@ cdecl _ismbcspace(long)
@ cdecl _ismbcsymbol(long)
@ cdecl _ismbcupper(long)
@ cdecl _ismbslead(ptr ptr)
@ cdecl _ismbstrail(ptr ptr)
@ cdecl _isnan(double) MSVCRT__isnan
@ cdecl _itoa(long ptr long)
@ cdecl _itow(long ptr long) ntdll._itow
@ cdecl _j0(double) MSVCRT__j0
@ cdecl _j1(double) MSVCRT__j1
@ cdecl _jn(long double) MSVCRT__jn
@ cdecl _kbhit()
@ cdecl _lfind(ptr ptr ptr long ptr)
@ cdecl _loaddll(str)
@ cdecl -arch=i386 _local_unwind2(ptr long)
@ cdecl _locking(long long long) MSVCRT__locking
@ cdecl _logb(double) MSVCRT__logb
@ cdecl _lrotl(long long) MSVCRT__lrotl
@ cdecl _lrotr(long long) MSVCRT__lrotr
@ cdecl _lsearch(ptr ptr ptr long ptr)
@ cdecl _lseek(long long long) MSVCRT__lseek
@ cdecl _ltoa(long ptr long) ntdll._ltoa
@ cdecl _ltow(long ptr long) ntdll._ltow
@ cdecl _makepath(ptr str str str str) MSVCRT__makepath
@ cdecl _matherr(ptr) MSVCRT__matherr
@ cdecl _mbbtombc(long)
@ cdecl _mbbtype(long long)
@ cdecl _mbccpy(ptr ptr)
@ cdecl _mbcjistojms(long)
@ cdecl _mbcjmstojis(long)
@ cdecl _mbclen(ptr)
@ cdecl _mbctohira(long)
@ cdecl _mbctokata(long)
@ cdecl _mbctolower(long)
@ cdecl _mbctombb(long)
@ cdecl _mbctoupper(long)
@ extern _mbctype MSVCRT_mbctype
@ cdecl _mbsbtype(str long)
@ cdecl _mbscat(str str)
@ cdecl _mbschr(str long)
@ cdecl _mbscmp(str str)
@ cdecl _mbscpy(ptr str)
@ cdecl _mbscspn(str str)
@ cdecl _mbsdec(ptr ptr)
@ cdecl _mbsdup(str) _strdup
@ cdecl _mbsicmp(str str)
@ cdecl _mbsinc(str)
@ cdecl _mbslen(str)
@ cdecl _mbslwr(str)
@ cdecl _mbsnbcat(str str long)
@ cdecl _mbsnbcmp(str str long)
@ cdecl _mbsnbcnt(ptr long)
@ cdecl _mbsnbcpy(ptr str long)
@ cdecl _mbsnbicmp(str str long)
@ cdecl _mbsnbset(ptr long long)
@ cdecl _mbsncat(str str long)
@ cdecl _mbsnccnt(str long)
@ cdecl _mbsncmp(str str long)
@ cdecl _mbsncpy(ptr str long)
@ cdecl _mbsnextc(str)
@ cdecl _mbsnicmp(str str long)
@ cdecl _mbsninc(str long)
@ cdecl _mbsnset(ptr long long)
@ cdecl _mbspbrk(str str)
@ cdecl _mbsrchr(str long)
@ cdecl _mbsrev(str)
@ cdecl _mbsset(ptr long)
@ cdecl _mbsspn(str str)
@ cdecl _mbsspnp(str str)
@ cdecl _mbsstr(str str)
@ cdecl _mbstok(str str)
@ cdecl _mbstrlen(str)
@ cdecl _mbsupr(str)
@ cdecl _memccpy(ptr ptr long long) ntdll._memccpy
@ cdecl _memicmp(str str long)
@ cdecl _mkdir(str) MSVCRT__mkdir
@ cdecl _mktemp(str) MSVCRT__mktemp
@ cdecl _msize(ptr)
@ cdecl _nextafter(double double) MSVCRT__nextafter
@ cdecl _onexit(ptr)
@ varargs _open(str long) MSVCRT__open
@ cdecl _open_osfhandle(long long) MSVCRT__open_osfhandle
@ extern _osmajor_dll MSVCRT__winmajor
@ extern _osminor_dll MSVCRT__winminor
@ extern _osmode_dll CRTDLL__osmode_dll
@ extern _osver_dll MSVCRT__osver
@ extern _osversion_dll MSVCRT__winver
@ cdecl _pclose(ptr)
@ extern _pctype_dll MSVCRT__pctype
@ extern _pgmptr_dll MSVCRT__pgmptr
@ cdecl _pipe(ptr long long) MSVCRT__pipe
@ cdecl _popen(str str)
@ cdecl _purecall()
@ cdecl _putch(long)
@ cdecl _putenv(str)
@ cdecl _putw(long ptr) MSVCRT__putw
# extern _pwctype_dll
@ cdecl _read(long ptr long) MSVCRT__read
@ cdecl _rmdir(str) MSVCRT__rmdir
@ cdecl _rmtmp() MSVCRT__rmtmp
@ cdecl _rotl(long long)
@ cdecl _rotr(long long)
@ cdecl _scalb(double long) MSVCRT__scalb
@ cdecl _searchenv(str str ptr) MSVCRT__searchenv
@ cdecl _seterrormode(long)
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay _setjmp(ptr) MSVCRT__setjmp
@ cdecl _setmode(long long) MSVCRT__setmode
@ stub _setsystime(ptr long)
@ cdecl _sleep(long)
@ varargs _snprintf(ptr long str) MSVCRT__snprintf
@ varargs _snwprintf(ptr long wstr) MSVCRT__snwprintf
@ varargs _sopen(str long long) MSVCRT__sopen
@ varargs _spawnl(long str str)
@ varargs _spawnle(long str str)
@ varargs _spawnlp(long str str)
@ varargs _spawnlpe(long str str)
@ cdecl _spawnv(long str ptr)
@ cdecl _spawnve(long str ptr ptr)
@ cdecl _spawnvp(long str ptr)
@ cdecl _spawnvpe(long str ptr ptr)
@ cdecl _splitpath(str ptr ptr ptr ptr) MSVCRT__splitpath
@ cdecl _stat(str ptr) MSVCRT_stat
@ cdecl _statusfp()
@ cdecl _strcmpi(str str) _stricmp
@ cdecl _strdate(ptr) MSVCRT__strdate
@ cdecl _strdec(str str)
@ cdecl _strdup(str)
@ cdecl _strerror(long)
@ cdecl _stricmp(str str)
@ cdecl _stricoll(str str)
@ cdecl _strinc(str)
@ cdecl _strlwr(str)
@ cdecl _strncnt(str long) __strncnt
@ cdecl _strnextc(str)
@ cdecl _strnicmp(str str long)
@ cdecl _strninc(str long)
@ cdecl _strnset(str long long)
@ cdecl _strrev(str)
@ cdecl _strset(str long)
@ cdecl _strspnp(str str)
@ cdecl _strtime(ptr) MSVCRT__strtime
@ cdecl _strupr(str)
@ cdecl _swab(str str long)
@ extern _sys_errlist MSVCRT__sys_errlist
@ extern _sys_nerr_dll MSVCRT__sys_nerr
@ cdecl _tell(long) MSVCRT__tell
@ cdecl _tempnam(str str) MSVCRT__tempnam
@ extern _timezone_dll MSVCRT___timezone
@ cdecl _tolower(long)
@ cdecl _toupper(long)
@ extern _tzname MSVCRT__tzname
@ cdecl _tzset() MSVCRT__tzset
@ cdecl _ultoa(long ptr long) ntdll._ultoa
@ cdecl _umask(long) MSVCRT__umask
@ cdecl _ungetch(long)
@ cdecl _unlink(str) MSVCRT__unlink
@ cdecl _unloaddll(long)
@ cdecl -arch=win32 _utime(str ptr) _utime32
@ cdecl -arch=win64 _utime(str ptr) _utime64
@ cdecl _vsnprintf(ptr long str ptr)
@ cdecl _vsnwprintf(ptr long wstr ptr) MSVCRT_vsnwprintf
@ cdecl _wcsdup(wstr) MSVCRT__wcsdup
@ cdecl _wcsicmp(wstr wstr) MSVCRT__wcsicmp
@ cdecl _wcsicoll(wstr wstr) MSVCRT__wcsicoll
@ cdecl _wcslwr(wstr) MSVCRT__wcslwr
@ cdecl _wcsnicmp(wstr wstr long) MSVCRT__wcsnicmp
@ cdecl _wcsnset(wstr long long) MSVCRT__wcsnset
@ cdecl _wcsrev(wstr) MSVCRT__wcsrev
@ cdecl _wcsset(wstr long) MSVCRT__wcsset
@ cdecl _wcsupr(wstr) MSVCRT__wcsupr
@ extern _winmajor_dll MSVCRT__winmajor
@ extern _winminor_dll MSVCRT__winminor
@ extern _winver_dll MSVCRT__winver
@ cdecl _write(long ptr long) MSVCRT__write
@ cdecl _wtoi(wstr) MSVCRT__wtoi
@ cdecl _wtol(wstr) MSVCRT__wtol
@ cdecl _y0(double) MSVCRT__y0
@ cdecl _y1(double) MSVCRT__y1
@ cdecl _yn(long double) MSVCRT__yn
@ cdecl abort()
@ cdecl abs(long) MSVCRT_abs
@ cdecl acos(double) MSVCRT_acos
@ cdecl asctime(ptr) MSVCRT_asctime
@ cdecl asin(double) MSVCRT_asin
@ cdecl atan(double) MSVCRT_atan
@ cdecl atan2(double double) MSVCRT_atan2
@ cdecl -private atexit(ptr) MSVCRT_atexit
@ cdecl atof(str)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl calloc(long long)
@ cdecl ceil(double) MSVCRT_ceil
@ cdecl clearerr(ptr) MSVCRT_clearerr
@ cdecl clock() MSVCRT_clock
@ cdecl cos(double) MSVCRT_cos
@ cdecl cosh(double) MSVCRT_cosh
@ cdecl ctime(ptr) MSVCRT_ctime
@ cdecl difftime(long long) MSVCRT_difftime
@ cdecl -ret64 div(long long) MSVCRT_div
@ cdecl exit(long)
@ cdecl exp(double) MSVCRT_exp
@ cdecl fabs(double) MSVCRT_fabs
@ cdecl fclose(ptr) MSVCRT_fclose
@ cdecl feof(ptr) MSVCRT_feof
@ cdecl ferror(ptr) MSVCRT_ferror
@ cdecl fflush(ptr) MSVCRT_fflush
@ cdecl fgetc(ptr) MSVCRT_fgetc
@ cdecl fgetpos(ptr ptr) MSVCRT_fgetpos
@ cdecl fgets(ptr long ptr) MSVCRT_fgets
@ cdecl fgetwc(ptr) MSVCRT_fgetwc
@ cdecl floor(double) MSVCRT_floor
@ cdecl fmod(double double) MSVCRT_fmod
@ cdecl fopen(str str) MSVCRT_fopen
@ varargs fprintf(ptr str) MSVCRT_fprintf
@ cdecl fputc(long ptr) MSVCRT_fputc
@ cdecl fputs(str ptr) MSVCRT_fputs
@ cdecl fputwc(long ptr) MSVCRT_fputwc
@ cdecl fread(ptr long long ptr) MSVCRT_fread
@ cdecl free(ptr)
@ cdecl freopen(str str ptr) MSVCRT_freopen
@ cdecl frexp(double ptr) MSVCRT_frexp
@ varargs fscanf(ptr str)
@ cdecl fseek(ptr long long) MSVCRT_fseek
@ cdecl fsetpos(ptr ptr) MSVCRT_fsetpos
@ cdecl ftell(ptr) MSVCRT_ftell
@ varargs fwprintf(ptr wstr) MSVCRT_fwprintf
@ cdecl fwrite(ptr long long ptr) MSVCRT_fwrite
@ varargs fwscanf(ptr wstr)
@ cdecl getc(ptr) MSVCRT_getc
@ cdecl getchar() MSVCRT_getchar
@ cdecl getenv(str)
@ cdecl gets(str) MSVCRT_gets
@ cdecl gmtime(ptr) MSVCRT_gmtime
@ cdecl is_wctype(long long) MSVCRT_iswctype
@ cdecl isalnum(long)
@ cdecl isalpha(long)
@ cdecl iscntrl(long)
@ cdecl isdigit(long)
@ cdecl isgraph(long)
@ cdecl isleadbyte(long)
@ cdecl islower(long)
@ cdecl isprint(long)
@ cdecl ispunct(long)
@ cdecl isspace(long)
@ cdecl isupper(long)
@ cdecl iswalnum(long) MSVCRT_iswalnum
@ cdecl iswalpha(long) MSVCRT_iswalpha
@ cdecl iswascii(long)
@ cdecl iswcntrl(long) MSVCRT_iswcntrl
@ cdecl iswctype(long long) MSVCRT_iswctype
@ cdecl iswdigit(long) MSVCRT_iswdigit
@ cdecl iswgraph(long) MSVCRT_iswgraph
@ cdecl iswlower(long) MSVCRT_iswlower
@ cdecl iswprint(long) MSVCRT_iswprint
@ cdecl iswpunct(long) MSVCRT_iswpunct
@ cdecl iswspace(long) MSVCRT_iswspace
@ cdecl iswupper(long) MSVCRT_iswupper
@ cdecl iswxdigit(long) MSVCRT_iswxdigit
@ cdecl isxdigit(long)
@ cdecl labs(long) MSVCRT_labs
@ cdecl ldexp(double long) MSVCRT_ldexp
@ cdecl -ret64 ldiv(long long) MSVCRT_ldiv
@ cdecl localeconv()
@ cdecl localtime(ptr) MSVCRT_localtime
@ cdecl log(double) MSVCRT_log
@ cdecl log10(double) MSVCRT_log10
@ cdecl -arch=i386,x86_64,arm,arm64 longjmp(ptr long) MSVCRT_longjmp
@ cdecl malloc(long)
@ cdecl mblen(ptr long)
@ cdecl mbstowcs(ptr str long)
@ cdecl mbtowc(ptr str long)
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memmove(ptr ptr long)
@ cdecl memset(ptr long long)
@ cdecl mktime(ptr) MSVCRT_mktime
@ cdecl modf(double ptr) MSVCRT_modf
@ cdecl perror(str)
@ cdecl pow(double double) MSVCRT_pow
@ varargs printf(str) MSVCRT_printf
@ cdecl putc(long ptr) MSVCRT_putc
@ cdecl putchar(long) MSVCRT_putchar
@ cdecl puts(str) MSVCRT_puts
@ cdecl qsort(ptr long long ptr)
@ cdecl raise(long)
@ cdecl rand()
@ cdecl realloc(ptr long)
@ cdecl remove(str) MSVCRT_remove
@ cdecl rename(str str) MSVCRT_rename
@ cdecl rewind(ptr) MSVCRT_rewind
@ varargs scanf(str)
@ cdecl setbuf(ptr ptr) MSVCRT_setbuf
@ cdecl setlocale(long str)
@ cdecl setvbuf(ptr str long long) MSVCRT_setvbuf
@ cdecl signal(long long)
@ cdecl sin(double) MSVCRT_sin
@ cdecl sinh(double) MSVCRT_sinh
@ varargs sprintf(ptr str) MSVCRT_sprintf
@ cdecl sqrt(double) MSVCRT_sqrt
@ cdecl srand(long)
@ varargs sscanf(str str)
@ cdecl strcat(str str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcoll(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcspn(str str)
@ cdecl strerror(long)
@ cdecl strftime(ptr long str ptr) MSVCRT_strftime
@ cdecl strlen(str)
@ cdecl strncat(str str long)
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl strpbrk(str str)
@ cdecl strrchr(str long)
@ cdecl strspn(str str) ntdll.strspn
@ cdecl strstr(str str)
@ cdecl strtod(str ptr)
@ cdecl strtok(str str)
@ cdecl strtol(str ptr long)
@ cdecl strtoul(str ptr long)
@ cdecl strxfrm(ptr str long)
@ varargs swprintf(ptr wstr) MSVCRT_swprintf
@ varargs swscanf(wstr wstr)
@ cdecl system(str)
@ cdecl tan(double) MSVCRT_tan
@ cdecl tanh(double) MSVCRT_tanh
@ cdecl time(ptr) MSVCRT_time
@ cdecl tmpfile() MSVCRT_tmpfile
@ cdecl tmpnam(ptr) MSVCRT_tmpnam
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long) MSVCRT_towlower
@ cdecl towupper(long) MSVCRT_towupper
@ cdecl ungetc(long ptr) MSVCRT_ungetc
@ cdecl ungetwc(long ptr) MSVCRT_ungetwc
@ cdecl vfprintf(ptr str ptr) MSVCRT_vfprintf
@ cdecl vfwprintf(ptr wstr ptr) MSVCRT_vfwprintf
@ cdecl vprintf(str ptr) MSVCRT_vprintf
@ cdecl vsprintf(ptr str ptr) MSVCRT_vsprintf
@ cdecl vswprintf(ptr wstr ptr) MSVCRT_vswprintf
@ cdecl vwprintf(wstr ptr) MSVCRT_vwprintf
@ cdecl wcscat(wstr wstr) MSVCRT_wcscat
@ cdecl wcschr(wstr long) MSVCRT_wcschr
@ cdecl wcscmp(wstr wstr) MSVCRT_wcscmp
@ cdecl wcscoll(wstr wstr) MSVCRT_wcscoll
@ cdecl wcscpy(ptr wstr) MSVCRT_wcscpy
@ cdecl wcscspn(wstr wstr) ntdll.wcscspn
@ cdecl wcsftime(ptr long wstr ptr) MSVCRT_wcsftime
@ cdecl wcslen(wstr) MSVCRT_wcslen
@ cdecl wcsncat(wstr wstr long) ntdll.wcsncat
@ cdecl wcsncmp(wstr wstr long) MSVCRT_wcsncmp
@ cdecl wcsncpy(ptr wstr long) MSVCRT_wcsncpy
@ cdecl wcspbrk(wstr wstr) MSVCRT_wcspbrk
@ cdecl wcsrchr(wstr long) MSVCRT_wcsrchr
@ cdecl wcsspn(wstr wstr) ntdll.wcsspn
@ cdecl wcsstr(wstr wstr) MSVCRT_wcsstr
@ cdecl wcstod(wstr ptr) MSVCRT_wcstod
@ cdecl wcstok(wstr wstr) MSVCRT_wcstok
@ cdecl wcstol(wstr ptr long) MSVCRT_wcstol
@ cdecl wcstombs(ptr ptr long) MSVCRT_wcstombs
@ cdecl wcstoul(wstr ptr long) MSVCRT_wcstoul
@ cdecl wcsxfrm(ptr wstr long) MSVCRT_wcsxfrm
@ cdecl wctomb(ptr long) MSVCRT_wctomb
@ varargs wprintf(wstr) MSVCRT_wprintf
@ varargs wscanf(wstr)
