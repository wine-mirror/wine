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
@ cdecl _cabs(long)
@ cdecl _cexit()
@ cdecl _cgets(ptr)
@ cdecl _chdir(str)
@ cdecl _chdrive(long)
@ cdecl _chgsign(double)
@ cdecl _chmod(str long) MSVCRT__chmod
@ cdecl _chsize(long long) MSVCRT__chsize
@ cdecl _clearfp()
@ cdecl _close(long) MSVCRT__close
@ cdecl _commit(long) MSVCRT__commit
@ extern _commode_dll MSVCRT__commode
@ cdecl _control87(long long)
@ cdecl _controlfp(long long)
@ cdecl _copysign(double double)
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
@ cdecl _ecvt(double long ptr ptr)
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
@ cdecl _fcvt(double long ptr ptr)
@ cdecl _fdopen(long str) MSVCRT__fdopen
@ cdecl _fgetchar() MSVCRT__fgetchar
@ cdecl _fgetwchar() MSVCRT__fgetwchar
@ cdecl _filbuf(ptr) MSVCRT__filbuf
# extern _fileinfo_dll
@ cdecl _filelength(long) MSVCRT__filelength
@ cdecl _fileno(ptr) MSVCRT__fileno
@ cdecl _findclose(long)
@ cdecl _findfirst(str ptr)
@ cdecl _findnext(long ptr)
@ cdecl _finite(double)
@ cdecl _flsbuf(long ptr) MSVCRT__flsbuf
@ cdecl _flushall() MSVCRT__flushall
@ extern _fmode_dll MSVCRT__fmode
@ cdecl _fpclass(double)
@ cdecl -arch=i386,x86_64,arm,arm64 _fpieee_flt(long ptr ptr)
@ cdecl _fpreset()
@ cdecl _fputchar(long) MSVCRT__fputchar
@ cdecl _fputwchar(long) MSVCRT__fputwchar
@ cdecl _fsopen(str str long) MSVCRT__fsopen
@ cdecl _fstat(long ptr) MSVCRT__fstat
@ cdecl -arch=win32 _ftime(ptr) _ftime32
@ cdecl -arch=win64 _ftime(ptr) _ftime64
@ cdecl -arch=i386 -ret64 _ftol()
@ cdecl _fullpath(ptr str long)
@ cdecl -arch=win32 _futime(long ptr) _futime32
@ cdecl -arch=win64 _futime(long ptr) _futime64
@ cdecl _gcvt(double long str)
@ cdecl _get_osfhandle(long) MSVCRT__get_osfhandle
@ cdecl _getch()
@ cdecl _getche()
@ cdecl _getcwd(str long)
@ cdecl _getdcwd(long str long)
@ cdecl _getdiskfree(long ptr)
@ cdecl _getdllprocaddr(long str long)
@ cdecl _getdrive()
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
@ cdecl _isnan(double)
@ cdecl _itoa(long ptr long)
@ cdecl _itow(long ptr long) ntdll._itow
@ cdecl _j0(double)
@ cdecl _j1(double)
@ cdecl _jn(long double)
@ cdecl _kbhit()
@ cdecl _lfind(ptr ptr ptr long ptr)
@ cdecl _loaddll(str)
@ cdecl -arch=i386 _local_unwind2(ptr long)
@ cdecl _locking(long long long) MSVCRT__locking
@ cdecl _logb(double)
@ cdecl _lrotl(long long)
@ cdecl _lrotr(long long)
@ cdecl _lsearch(ptr ptr ptr long ptr)
@ cdecl _lseek(long long long) MSVCRT__lseek
@ cdecl _ltoa(long ptr long) ntdll._ltoa
@ cdecl _ltow(long ptr long) ntdll._ltow
@ cdecl _makepath(ptr str str str str)
@ cdecl _matherr(ptr)
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
@ cdecl _mkdir(str)
@ cdecl _mktemp(str) MSVCRT__mktemp
@ cdecl _msize(ptr)
@ cdecl _nextafter(double double)
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
@ cdecl _rmdir(str)
@ cdecl _rmtmp() MSVCRT__rmtmp
@ cdecl _rotl(long long)
@ cdecl _rotr(long long)
@ cdecl _scalb(double long)
@ cdecl _searchenv(str str ptr)
@ cdecl _seterrormode(long)
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay _setjmp(ptr) MSVCRT__setjmp
@ cdecl _setmode(long long) MSVCRT__setmode
@ stub _setsystime(ptr long)
@ cdecl _sleep(long)
@ varargs _snprintf(ptr long str)
@ varargs _snwprintf(ptr long wstr)
@ varargs _sopen(str long long) MSVCRT__sopen
@ varargs _spawnl(long str str)
@ varargs _spawnle(long str str)
@ varargs _spawnlp(long str str)
@ varargs _spawnlpe(long str str)
@ cdecl _spawnv(long str ptr)
@ cdecl _spawnve(long str ptr ptr)
@ cdecl _spawnvp(long str ptr)
@ cdecl _spawnvpe(long str ptr ptr)
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ cdecl _stat(str ptr) MSVCRT_stat
@ cdecl _statusfp()
@ cdecl _strcmpi(str str) _stricmp
@ cdecl _strdate(ptr)
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
@ cdecl _strtime(ptr)
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
@ cdecl _tzset()
@ cdecl _ultoa(long ptr long) ntdll._ultoa
@ cdecl _umask(long) MSVCRT__umask
@ cdecl _ungetch(long)
@ cdecl _unlink(str) MSVCRT__unlink
@ cdecl _unloaddll(long)
@ cdecl -arch=win32 _utime(str ptr) _utime32
@ cdecl -arch=win64 _utime(str ptr) _utime64
@ cdecl _vsnprintf(ptr long str ptr)
@ cdecl _vsnwprintf(ptr long wstr ptr)
@ cdecl _wcsdup(wstr)
@ cdecl _wcsicmp(wstr wstr)
@ cdecl _wcsicoll(wstr wstr)
@ cdecl _wcslwr(wstr)
@ cdecl _wcsnicmp(wstr wstr long)
@ cdecl _wcsnset(wstr long long)
@ cdecl _wcsrev(wstr)
@ cdecl _wcsset(wstr long)
@ cdecl _wcsupr(wstr)
@ extern _winmajor_dll MSVCRT__winmajor
@ extern _winminor_dll MSVCRT__winminor
@ extern _winver_dll MSVCRT__winver
@ cdecl _write(long ptr long) MSVCRT__write
@ cdecl _wtoi(wstr)
@ cdecl _wtol(wstr)
@ cdecl _y0(double)
@ cdecl _y1(double)
@ cdecl _yn(long double)
@ cdecl abort()
@ cdecl abs(long)
@ cdecl acos(double)
@ cdecl asctime(ptr)
@ cdecl asin(double)
@ cdecl atan(double)
@ cdecl atan2(double double)
@ cdecl -private atexit(ptr) MSVCRT_atexit
@ cdecl atof(str)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl calloc(long long)
@ cdecl ceil(double)
@ cdecl clearerr(ptr) MSVCRT_clearerr
@ cdecl clock()
@ cdecl cos(double)
@ cdecl cosh(double)
@ cdecl -arch=win32 ctime(ptr) _ctime32
@ cdecl -arch=win64 ctime(ptr) _ctime64
@ cdecl -arch=win32 difftime(long long) _difftime32
@ cdecl -arch=win64 difftime(long long) _difftime64
@ cdecl -ret64 div(long long)
@ cdecl exit(long)
@ cdecl exp(double)
@ cdecl fabs(double)
@ cdecl fclose(ptr) MSVCRT_fclose
@ cdecl feof(ptr) MSVCRT_feof
@ cdecl ferror(ptr) MSVCRT_ferror
@ cdecl fflush(ptr) MSVCRT_fflush
@ cdecl fgetc(ptr) MSVCRT_fgetc
@ cdecl fgetpos(ptr ptr) MSVCRT_fgetpos
@ cdecl fgets(ptr long ptr) MSVCRT_fgets
@ cdecl fgetwc(ptr) MSVCRT_fgetwc
@ cdecl floor(double)
@ cdecl fmod(double double)
@ cdecl fopen(str str) MSVCRT_fopen
@ varargs fprintf(ptr str) MSVCRT_fprintf
@ cdecl fputc(long ptr) MSVCRT_fputc
@ cdecl fputs(str ptr) MSVCRT_fputs
@ cdecl fputwc(long ptr) MSVCRT_fputwc
@ cdecl fread(ptr long long ptr) MSVCRT_fread
@ cdecl free(ptr)
@ cdecl freopen(str str ptr) MSVCRT_freopen
@ cdecl frexp(double ptr)
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
@ cdecl -arch=win32 gmtime(ptr) _gmtime32
@ cdecl -arch=win64 gmtime(ptr) _gmtime64
@ cdecl is_wctype(long long) iswctype
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
@ cdecl iswalnum(long)
@ cdecl iswalpha(long)
@ cdecl iswascii(long)
@ cdecl iswcntrl(long)
@ cdecl iswctype(long long)
@ cdecl iswdigit(long)
@ cdecl iswgraph(long)
@ cdecl iswlower(long)
@ cdecl iswprint(long)
@ cdecl iswpunct(long)
@ cdecl iswspace(long)
@ cdecl iswupper(long)
@ cdecl iswxdigit(long)
@ cdecl isxdigit(long)
@ cdecl labs(long)
@ cdecl ldexp(double long)
@ cdecl -ret64 ldiv(long long)
@ cdecl localeconv()
@ cdecl -arch=win32 localtime(ptr) _localtime32
@ cdecl -arch=win64 localtime(ptr) _localtime64
@ cdecl log(double)
@ cdecl log10(double)
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
@ cdecl -arch=win32 mktime(ptr) _mktime32
@ cdecl -arch=win64 mktime(ptr) _mktime64
@ cdecl modf(double ptr)
@ cdecl perror(str)
@ cdecl pow(double double)
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
@ cdecl sin(double)
@ cdecl sinh(double)
@ varargs sprintf(ptr str)
@ cdecl sqrt(double)
@ cdecl srand(long)
@ varargs sscanf(str str)
@ cdecl strcat(str str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcoll(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcspn(str str)
@ cdecl strerror(long)
@ cdecl strftime(ptr long str ptr)
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
@ varargs swprintf(ptr wstr) _swprintf
@ varargs swscanf(wstr wstr)
@ cdecl system(str)
@ cdecl tan(double)
@ cdecl tanh(double)
@ cdecl -arch=win32 time(ptr) _time32
@ cdecl -arch=win64 time(ptr) _time64
@ cdecl tmpfile() MSVCRT_tmpfile
@ cdecl tmpnam(ptr) MSVCRT_tmpnam
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long)
@ cdecl towupper(long)
@ cdecl ungetc(long ptr) MSVCRT_ungetc
@ cdecl ungetwc(long ptr) MSVCRT_ungetwc
@ cdecl vfprintf(ptr str ptr) MSVCRT_vfprintf
@ cdecl vfwprintf(ptr wstr ptr) MSVCRT_vfwprintf
@ cdecl vprintf(str ptr) MSVCRT_vprintf
@ cdecl vsprintf(ptr str ptr)
@ cdecl vswprintf(ptr wstr ptr) _vswprintf
@ cdecl vwprintf(wstr ptr) MSVCRT_vwprintf
@ cdecl wcscat(wstr wstr)
@ cdecl wcschr(wstr long)
@ cdecl wcscmp(wstr wstr)
@ cdecl wcscoll(wstr wstr)
@ cdecl wcscpy(ptr wstr)
@ cdecl wcscspn(wstr wstr) ntdll.wcscspn
@ cdecl wcsftime(ptr long wstr ptr)
@ cdecl wcslen(wstr)
@ cdecl wcsncat(wstr wstr long) ntdll.wcsncat
@ cdecl wcsncmp(wstr wstr long)
@ cdecl wcsncpy(ptr wstr long)
@ cdecl wcspbrk(wstr wstr)
@ cdecl wcsrchr(wstr long)
@ cdecl wcsspn(wstr wstr) ntdll.wcsspn
@ cdecl wcsstr(wstr wstr)
@ cdecl wcstod(wstr ptr)
@ cdecl wcstok(wstr wstr)
@ cdecl wcstol(wstr ptr long)
@ cdecl wcstombs(ptr ptr long)
@ cdecl wcstoul(wstr ptr long)
@ cdecl wcsxfrm(ptr wstr long)
@ cdecl wctomb(ptr long)
@ varargs wprintf(wstr) MSVCRT_wprintf
@ varargs wscanf(wstr)
