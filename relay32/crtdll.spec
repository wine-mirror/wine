# C RunTime DLL. All functions use cdecl!
name	crtdll
type	win32

  1 cdecl ??2@YAPAXI@Z(long) CRTDLL_new
  2 cdecl ??3@YAXPAX@Z(long) CRTDLL_delete
  3 cdecl ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) CRTDLL_set_new_handler
  4 stub _CIacos
  5 stub _CIasin
  6 stub _CIatan
  7 stub _CIatan2
  8 stub _CIcos
  9 stub _CIcosh
 10 stub _CIexp
 11 stub _CIfmod
 12 stub _CIlog
 13 stub _CIlog10
 14 stub _CIpow
 15 stub _CIsin
 16 stub _CIsinh
 17 stub _CIsqrt
 18 stub _CItan
 19 stub _CItanh
 20 stub _HUGE_dll
 21 stub _XcptFilter
 22 cdecl __GetMainArgs(ptr ptr ptr long) CRTDLL__GetMainArgs
 23 extern __argc_dll CRTDLL_argc_dll
 24 extern __argv_dll CRTDLL_argv_dll
 25 cdecl __dllonexit() CRTDLL__dllonexit
 26 stub __doserrno
 27 stub __fpecode
 28 stub __isascii
 29 stub __iscsym
 30 stub __iscsymf
 31 stub __mb_cur_max_dll
 32 stub __pxcptinfoptrs
 33 stub __threadhandle
 34 stub __threadid
 35 stub __toascii
 36 cdecl _abnormal_termination() CRTDLL__abnormal_termination
 37 cdecl _access(str long) CRTDLL__access
 38 extern _acmdln_dll CRTDLL_acmdln_dll
 39 stub _aexit_rtn_dll
 40 stub _amsg_exit
 41 stub _assert
 42 extern _basemajor_dll CRTDLL_basemajor_dll
 43 extern _baseminor_dll CRTDLL_baseminor_dll
 44 extern _baseversion_dll CRTDLL_baseversion_dll
 45 stub _beep
 46 stub _beginthread
 47 stub _c_exit
 48 stub _cabs
 49 cdecl _cexit(long) CRTDLL__cexit
 50 stub _cgets
 51 cdecl _chdir(str) CRTDLL__chdir
 52 cdecl _chdrive(long) CRTDLL__chdrive
 53 stub _chgsign
 54 stub _chmod
 55 stub _chsize
 56 stub _clearfp
 57 cdecl _close(long) CRTDLL__close
 58 stub _commit
 59 extern _commode_dll CRTDLL_commode_dll
 60 stub _control87
 61 stub _controlfp
 62 stub _copysign
 63 stub _cprintf
 64 stub _cpumode_dll
 65 stub _cputs
 66 stub _creat
 67 stub _cscanf
 68 stub _ctype
 69 stub _cwait
 70 stub _daylight_dll
 71 stub _dup
 72 stub _dup2
 73 stub _ecvt
 74 stub _endthread
 75 extern _environ_dll CRTDLL_environ_dll
 76 stub _eof
 77 cdecl _errno() CRTDLL__errno
 78 stub _except_handler2
 79 stub _execl
 80 stub _execle
 81 stub _execlp
 82 stub _execlpe
 83 stub _execv
 84 stub _execve
 85 stub _execvp
 86 stub _execvpe
 87 stub _exit
 88 stub _expand
 89 stub _fcloseall
 90 stub _fcvt
 91 cdecl _fdopen(long ptr) CRTDLL__fdopen
 92 stub _fgetchar
 93 stub _fgetwchar
 94 stub _filbuf
 95 stub _fileinfo_dll
 96 stub _filelength
 97 stub _fileno
 98 stub _findclose
 99 cdecl _findfirst(str ptr) CRTDLL__findfirst
100 cdecl _findnext(long ptr) CRTDLL__findnext
101 stub _finite
102 stub _flsbuf
103 stub _flushall
104 extern _fmode_dll CRTDLL_fmode_dll
105 stub _fpclass
106 stub _fpieee_flt
107 cdecl _fpreset() CRTDLL__fpreset
108 stub _fputchar
109 stub _fputwchar
110 stub _fsopen
111 cdecl _fstat(long ptr) CRTDLL__fstat
112 stub _ftime
113 cdecl _ftol(double) CRTDLL__ftol
114 cdecl _fullpath(ptr str long) CRTDLL__fullpath
115 stub _futime
116 stub _gcvt
117 stub _get_osfhandle
118 stub _getch
119 stub _getche
120 cdecl _getcwd(ptr long) CRTDLL__getcwd
121 cdecl _getdcwd(long ptr long) CRTDLL__getdcwd
122 stub _getdiskfree
123 stub _getdllprocaddr
124 cdecl _getdrive() CRTDLL__getdrive
125 stub _getdrives
126 stub _getpid
127 stub _getsystime
128 stub _getw
129 cdecl _global_unwind2(ptr) CRTDLL__global_unwind2
130 stub _heapchk
131 stub _heapmin
132 stub _heapset
133 stub _heapwalk
134 cdecl _hypot(double double) hypot
135 cdecl _initterm(ptr ptr) CRTDLL__initterm
136 extern _iob CRTDLL_iob
137 cdecl _isatty(long) CRTDLL__isatty
138 cdecl _isctype(long long) CRTDLL__isctype
139 stub _ismbbalnum
140 stub _ismbbalpha
141 stub _ismbbgraph
142 stub _ismbbkalnum
143 stub _ismbbkana
144 stub _ismbbkpunct
145 stub _ismbblead
146 stub _ismbbprint
147 stub _ismbbpunct
148 stub _ismbbtrail
149 stub _ismbcalpha
150 stub _ismbcdigit
151 stub _ismbchira
152 stub _ismbckata
153 stub _ismbcl0
154 stub _ismbcl1
155 stub _ismbcl2
156 stub _ismbclegal
157 stub _ismbclower
158 stub _ismbcprint
159 stub _ismbcspace
160 stub _ismbcsymbol
161 stub _ismbcupper
162 stub _ismbslead
163 stub _ismbstrail
164 stub _isnan
165 cdecl _itoa(long ptr long) CRTDLL__itoa
166 stub _itow
167 cdecl _j0(double) j0
168 cdecl _j1(double) j1
169 cdecl _jn(long double) jn
170 stub _kbhit
171 stub _lfind
172 stub _loaddll
173 cdecl _local_unwind2(ptr long) CRTDLL__local_unwind2
174 stub _locking
175 stub _logb
176 cdecl _lrotl (long long) CRTDLL__lrotl
177 stub _lrotr
178 stub _lsearch
179 stub _lseek
180 cdecl _ltoa(long str long) CRTDLL__ltoa
181 stub _ltow
182 stub _makepath
183 stub _matherr
184 stub _mbbtombc
185 stub _mbbtype
186 stub _mbccpy
187 stub _mbcjistojms
188 stub _mbcjmstojis
189 stub _mbclen
190 stub _mbctohira
191 stub _mbctokata
192 stub _mbctolower
193 stub _mbctombb
194 stub _mbctoupper
195 stub _mbctype
196 stub _mbsbtype
197 cdecl _mbscat(str str) CRTDLL__mbscat
198 stub _mbschr
199 stub _mbscmp
200 cdecl _mbscpy(ptr str) CRTDLL__mbscpy
201 stub _mbscspn
202 stub _mbsdec
203 stub _mbsdup
204 cdecl _mbsicmp(str str) CRTDLL__mbsicmp
205 cdecl _mbsinc(str) CRTDLL__mbsinc
206 stub _mbslen
207 stub _mbslwr
208 stub _mbsnbcat
209 stub _mbsnbcmp
210 stub _mbsnbcnt
211 stub _mbsnbcpy
212 stub _mbsnbicmp
213 stub _mbsnbset
214 stub _mbsncat
215 stub _mbsnccnt
216 stub _mbsncmp
217 stub _mbsncpy
218 stub _mbsnextc
219 stub _mbsnicmp
220 stub _mbsninc
221 stub _mbsnset
222 stub _mbspbrk
223 cdecl _mbsrchr(str long) CRTDLL__mbsrchr
224 stub _mbsrev
225 stub _mbsset
226 stub _mbsspn
227 stub _mbsspnp
228 stub _mbsstr
229 stub _mbstok
230 stub _mbstrlen
231 stub _mbsupr
232 stub _memccpy
233 cdecl _memicmp(str str long) CRTDLL__memicmp
234 cdecl _mkdir(str) CRTDLL__mkdir
235 stub _mktemp
236 stub _msize
237 stub _nextafter
238 stub _onexit
239 cdecl _open(str long) CRTDLL__open
240 cdecl _open_osfhandle(long long) CRTDLL__open_osfhandle
241 extern _osmajor_dll CRTDLL_osmajor_dll
242 extern _osminor_dll CRTDLL_osminor_dll
243 extern _osmode_dll CRTDLL_osmode_dll
244 extern _osver_dll CRTDLL_osver_dll
245 extern _osversion_dll CRTDLL_osversion_dll
246 stub _pclose
247 stub _pctype_dll
248 stub _pgmptr_dll
249 stub _pipe
250 stub _popen
251 stub _purecall
252 stub _putch
253 stub _putenv
254 stub _putw
255 stub _pwctype_dll
256 cdecl _read(long ptr long) CRTDLL__read
257 stub _rmdir
258 stub _rmtmp
259 cdecl _rotl (long long) CRTDLL__rotl
260 stub _rotr
261 stub _scalb
262 stub _searchenv
263 stub _seterrormode
264 stub _setjmp
265 cdecl _setmode(long long) CRTDLL__setmode
266 stub _setsystime
267 cdecl _sleep(long) CRTDLL__sleep
268 stub _snprintf
269 stub _snwprintf
270 stub _sopen
271 stub _spawnl
272 stub _spawnle
273 stub _spawnlp
274 stub _spawnlpe
275 stub _spawnv
276 stub _spawnve
277 stub _spawnvp
278 stub _spawnvpe
279 cdecl _splitpath (str ptr ptr ptr ptr) CRTDLL__splitpath
280 cdecl _stat (str ptr) CRTDLL__stat
281 stub _statusfp
282 cdecl _strcmpi(str str) CRTDLL__strcmpi
283 stub _strdate
284 stub _strdec
285 cdecl _strdup(str) CRTDLL__strdup
286 stub _strerror
287 cdecl _stricmp(str str) CRTDLL__strcmpi
288 stub _stricoll
289 stub _strinc
290 cdecl _strlwr(str) CRTDLL__strlwr
291 stub _strncnt
292 stub _strnextc
293 cdecl _strnicmp(str str long) CRTDLL__strnicmp
294 stub _strninc
295 stub _strnset
296 stub _strrev
297 stub _strset
298 stub _strspnp
299 stub _strtime
300 cdecl _strupr(str) CRTDLL__strupr
301 stub _swab
302 stub _sys_errlist
303 stub _sys_nerr_dll
304 stub _tell
305 cdecl _tempnam(str ptr) CRTDLL__tempnam
306 stub _timezone_dll
307 stub _tolower
308 stub _toupper
309 stub _tzname
310 stub _tzset
311 stub _ultoa
312 stub _ultow
313 stub _umask
314 stub _ungetch
315 cdecl _unlink(str) CRTDLL__unlink
316 stub _unloaddll
317 stub _utime
318 stub _vsnprintf
319 stub _vsnwprintf
320 cdecl _wcsdup(wstr) CRTDLL__wcsdup
321 cdecl _wcsicmp(wstr wstr) CRTDLL__wcsicmp
322 cdecl _wcsicoll(wstr wstr) CRTDLL__wcsicoll
323 cdecl _wcslwr(wstr) CRTDLL__wcslwr
324 cdecl _wcsnicmp(wstr wstr long) CRTDLL__wcsnicmp
325 stub _wcsnset
326 cdecl _wcsrev(wstr) CRTDLL__wcsrev
327 stub _wcsset
328 cdecl _wcsupr(wstr) CRTDLL__wcsupr
329 extern _winmajor_dll CRTDLL_winmajor_dll
330 extern _winminor_dll CRTDLL_winminor_dll
331 extern _winver_dll CRTDLL_winver_dll
332 cdecl _write(long ptr long) CRTDLL__write
333 stub _wtoi
334 stub _wtol
335 cdecl _y0(double) y0
336 cdecl _y1(double) y1
337 cdecl _yn(long double) yn
338 stub abort
339 cdecl abs(long) abs
340 cdecl acos(double) acos
341 cdecl asctime(ptr) asctime
342 cdecl asin(double) asin
343 cdecl atan(double) atan
344 cdecl atan2(double double) atan2
345 cdecl atexit(ptr) CRTDLL_atexit
346 cdecl atof(str) atof
347 cdecl atoi(str) atoi
348 cdecl atol(str) atol
349 stub bsearch
350 cdecl calloc(long long) CRTDLL_calloc
351 cdecl ceil(double) ceil
352 stub clearerr
353 cdecl clock() CRTDLL_clock
354 cdecl cos(double) cos
355 cdecl cosh(double) cosh
356 cdecl ctime(ptr) ctime
357 stub difftime
358 cdecl div(long long) div
359 cdecl exit(long) CRTDLL_exit
360 cdecl exp(double) exp
361 cdecl fabs(double) fabs
362 cdecl fclose(ptr) CRTDLL_fclose
363 cdecl feof(ptr) CRTDLL_feof
364 stub ferror
365 cdecl fflush(ptr) CRTDLL_fflush
366 cdecl fgetc(ptr) CRTDLL_fgetc
367 stub fgetpos
368 cdecl fgets(ptr long ptr) CRTDLL_fgets
369 stub fgetwc
370 cdecl floor(double) floor
371 cdecl fmod(double double) fmod
372 cdecl fopen(str str) CRTDLL_fopen
373 varargs fprintf() CRTDLL_fprintf
374 cdecl fputc(long ptr) CRTDLL_fputc
375 cdecl fputs(str ptr) CRTDLL_fputs
376 stub fputwc
377 cdecl fread(ptr long long ptr) CRTDLL_fread
378 cdecl free(ptr) CRTDLL_free
379 cdecl freopen(str str ptr) CRTDLL_freopen
380 cdecl frexp(double ptr) frexp
381 varargs fscanf() CRTDLL_fscanf
382 cdecl fseek(ptr long long) CRTDLL_fseek
383 cdecl fsetpos(ptr ptr) CRTDLL_fsetpos
384 cdecl ftell(ptr) CRTDLL_ftell
385 stub fwprintf
386 cdecl fwrite(ptr long long ptr) CRTDLL_fwrite 
387 stub fwscanf
388 cdecl getc(ptr) CRTDLL_getc
389 stub getchar
390 cdecl getenv (str) CRTDLL_getenv
391 cdecl gets(ptr) CRTDLL_gets
392 cdecl gmtime(ptr) gmtime
393 stub is_wctype
394 cdecl isalnum(long) isalnum
395 cdecl isalpha(long) isalpha
396 cdecl iscntrl(long) iscntrl
397 cdecl isdigit(long) isdigit
398 cdecl isgraph(long) isgraph
399 stub isleadbyte
400 cdecl islower(long) islower
401 cdecl isprint(long) isprint
402 cdecl ispunct(long) ispunct
403 cdecl isspace(long) isspace
404 cdecl isupper(long) isupper
405 stub iswalnum
406 stub iswalpha
407 stub iswascii
408 stub iswcntrl
409 stub iswctype
410 stub iswdigit
411 stub iswgraph
412 stub iswlower
413 stub iswprint
414 stub iswpunct
415 stub iswspace
416 stub iswupper
417 stub iswxdigit
418 cdecl isxdigit(long) isxdigit
419 cdecl labs(long) labs
420 cdecl ldexp(double long) ldexp
421 cdecl ldiv(long long) ldiv
422 stub localeconv
423 cdecl localtime(ptr) localtime
424 cdecl log(double) log
425 cdecl log10(double) log10
426 cdecl longjmp(ptr long) CRTDLL_longjmp
427 cdecl malloc(ptr) CRTDLL_malloc
428 cdecl mblen(str long) CRTDLL_mblen
429 cdecl mbstowcs(ptr str long) CRTDLL_mbstowcs
430 cdecl mbtowc(ptr ptr long) CRTDLL_mbtowc
431 cdecl memchr(ptr long long) memchr
432 cdecl memcmp(ptr ptr long) memcmp
433 cdecl memcpy(ptr ptr long) memcpy
434 cdecl memmove(ptr ptr long) memmove
435 cdecl memset(ptr long long) memset 
436 cdecl mktime(ptr) mktime
437 cdecl modf(double ptr) modf
438 stub perror
439 cdecl pow(double double) pow
440 varargs printf() printf
441 cdecl putc(long ptr) CRTDLL_putc
442 cdecl putchar(long) CRTDLL_putchar
443 cdecl puts(str) CRTDLL_puts
444 cdecl qsort(ptr long long ptr) qsort
445 stub raise
446 cdecl rand() CRTDLL_rand
447 cdecl realloc(ptr long) CRTDLL_realloc
448 cdecl remove(str) CRTDLL_remove
449 cdecl rename(str str) CRTDLL_rename
450 stub rewind
451 stub scanf
452 cdecl setbuf(ptr ptr) CRTDLL_setbuf
453 cdecl setlocale(long ptr) CRTDLL_setlocale
454 stub setvbuf
455 cdecl signal(long ptr) CRTDLL_signal
456 cdecl sin(double) sin
457 cdecl sinh(double) sinh
458 varargs sprintf() sprintf
459 cdecl sqrt(double) sqrt
460 cdecl srand(long) CRTDLL_srand
461 varargs sscanf() sscanf
462 cdecl strcat(str str) strcat
463 cdecl strchr(str long) strchr
464 cdecl strcmp(str str) strcmp
465 cdecl strcoll(str str) strcoll
466 cdecl strcpy(ptr str) strcpy
467 cdecl strcspn(str str) strcspn
468 stub strerror
469 cdecl strftime(ptr long str ptr) strftime
470 cdecl strlen(str) strlen
471 cdecl strncat(str str long) strncat
472 cdecl strncmp(str str long) strncmp
473 cdecl strncpy(ptr str long) strncpy
474 cdecl strpbrk(str str) strpbrk
475 cdecl strrchr(str long) strrchr
476 cdecl strspn(str str) strspn
477 cdecl strstr(str str) strstr
478 cdecl strtod(str ptr) strtod
479 cdecl strtok(str str) strtok
480 cdecl strtol(str ptr long) strtol
481 cdecl strtoul(str ptr long) strtoul
482 cdecl strxfrm(ptr str long) strxfrm
483 varargs swprintf() wsprintf32W
484 stub swscanf
485 cdecl system(str) CRTDLL_system
486 cdecl tan(double) tan
487 cdecl tanh(double) tanh
488 cdecl time(ptr) CRTDLL_time
489 stub tmpfile
490 cdecl tmpnam(str) CRTDLL_tmpnam
491 cdecl tolower(long) tolower
492 cdecl toupper(long) toupper
493 cdecl towlower(long) towlower
494 cdecl towupper(long) towupper
495 stub ungetc
496 stub ungetwc
497 cdecl vfprintf(ptr str ptr) CRTDLL_vfprintf
498 stub vfwprintf
499 stub vprintf
500 cdecl vsprintf(ptr str ptr) CRTDLL_vsprintf
501 cdecl vswprintf(ptr wstr ptr) CRTDLL_vswprintf
502 stub vwprintf
503 cdecl wcscat(wstr wstr) CRTDLL_wcscat
504 cdecl wcschr(wstr long) CRTDLL_wcschr
505 cdecl wcscmp(wstr wstr) CRTDLL_wcscmp
506 cdecl wcscoll(wstr wstr) CRTDLL_wcscoll
507 cdecl wcscpy(ptr wstr) CRTDLL_wcscpy
508 cdecl wcscspn(wstr wstr) CRTDLL_wcscspn
509 stub wcsftime
510 cdecl wcslen(wstr) CRTDLL_wcslen
511 cdecl wcsncat(wstr wstr long) CRTDLL_wcsncat
512 cdecl wcsncmp(wstr wstr long) CRTDLL_wcsncmp
513 cdecl wcsncpy(ptr wstr long) CRTDLL_wcsncpy
514 stub wcspbrk
515 cdecl wcsrchr(wstr long) CRTDLL_wcsrchr
516 cdecl wcsspn(wstr wstr) CRTDLL_wcsspn
517 cdecl wcsstr(wstr wstr) CRTDLL_wcsstr
518 stub wcstod
519 cdecl wcstok(wstr wstr) CRTDLL_wcstok
520 cdecl wcstol(wstr ptr long) CRTDLL_wcstol
521 cdecl wcstombs(ptr ptr long) CRTDLL_wcstombs
522 stub wcstoul
523 stub wcsxfrm
524 stub wctomb
525 stub wprintf
526 stub wscanf
