@ cdecl __acrt_iob_func(long) ucrtbase.__acrt_iob_func
@ cdecl __p__commode() ucrtbase.__p__commode
@ cdecl __p__fmode() ucrtbase.__p__fmode
@ cdecl __stdio_common_vfprintf(int64 ptr str ptr ptr) ucrtbase.__stdio_common_vfprintf
@ cdecl __stdio_common_vfprintf_p(int64 ptr str ptr ptr) ucrtbase.__stdio_common_vfprintf_p
@ cdecl __stdio_common_vfprintf_s(int64 ptr str ptr ptr) ucrtbase.__stdio_common_vfprintf_s
@ cdecl __stdio_common_vfscanf(int64 ptr str ptr ptr) ucrtbase.__stdio_common_vfscanf
@ cdecl __stdio_common_vfwprintf(int64 ptr wstr ptr ptr) ucrtbase.__stdio_common_vfwprintf
@ cdecl __stdio_common_vfwprintf_p(int64 ptr wstr ptr ptr) ucrtbase.__stdio_common_vfwprintf_p
@ cdecl __stdio_common_vfwprintf_s(int64 ptr wstr ptr ptr) ucrtbase.__stdio_common_vfwprintf_s
@ cdecl __stdio_common_vfwscanf(int64 ptr wstr ptr ptr) ucrtbase.__stdio_common_vfwscanf
@ cdecl __stdio_common_vsnprintf_s(int64 ptr long long str ptr ptr) ucrtbase.__stdio_common_vsnprintf_s
@ cdecl __stdio_common_vsnwprintf_s(int64 ptr long long wstr ptr ptr) ucrtbase.__stdio_common_vsnwprintf_s
@ cdecl -norelay __stdio_common_vsprintf(int64 ptr long str ptr ptr) ucrtbase.__stdio_common_vsprintf
@ cdecl __stdio_common_vsprintf_p(int64 ptr long str ptr ptr) ucrtbase.__stdio_common_vsprintf_p
@ cdecl __stdio_common_vsprintf_s(int64 ptr long str ptr ptr) ucrtbase.__stdio_common_vsprintf_s
@ cdecl __stdio_common_vsscanf(int64 ptr long str ptr ptr) ucrtbase.__stdio_common_vsscanf
@ cdecl __stdio_common_vswprintf(int64 ptr long wstr ptr ptr) ucrtbase.__stdio_common_vswprintf
@ cdecl __stdio_common_vswprintf_p(int64 ptr long wstr ptr ptr) ucrtbase.__stdio_common_vswprintf_p
@ cdecl __stdio_common_vswprintf_s(int64 ptr long wstr ptr ptr) ucrtbase.__stdio_common_vswprintf_s
@ cdecl __stdio_common_vswscanf(int64 ptr long wstr ptr ptr) ucrtbase.__stdio_common_vswscanf
@ cdecl _chsize(long long) ucrtbase._chsize
@ cdecl _chsize_s(long int64) ucrtbase._chsize_s
@ cdecl _close(long) ucrtbase._close
@ cdecl _commit(long) ucrtbase._commit
@ cdecl _creat(str long) ucrtbase._creat
@ cdecl _dup(long) ucrtbase._dup
@ cdecl _dup2(long long) ucrtbase._dup2
@ cdecl _eof(long) ucrtbase._eof
@ cdecl _fclose_nolock(ptr) ucrtbase._fclose_nolock
@ cdecl _fcloseall() ucrtbase._fcloseall
@ cdecl _fflush_nolock(ptr) ucrtbase._fflush_nolock
@ cdecl _fgetc_nolock(ptr) ucrtbase._fgetc_nolock
@ cdecl _fgetchar() ucrtbase._fgetchar
@ cdecl _fgetwc_nolock(ptr) ucrtbase._fgetwc_nolock
@ cdecl _fgetwchar() ucrtbase._fgetwchar
@ cdecl _filelength(long) ucrtbase._filelength
@ cdecl -ret64 _filelengthi64(long) ucrtbase._filelengthi64
@ cdecl _fileno(ptr) ucrtbase._fileno
@ cdecl _flushall() ucrtbase._flushall
@ cdecl _fputc_nolock(long ptr) ucrtbase._fputc_nolock
@ cdecl _fputchar(long) ucrtbase._fputchar
@ cdecl _fputwc_nolock(long ptr) ucrtbase._fputwc_nolock
@ cdecl _fputwchar(long) ucrtbase._fputwchar
@ cdecl _fread_nolock(ptr long long ptr) ucrtbase._fread_nolock
@ cdecl _fread_nolock_s(ptr long long long ptr) ucrtbase._fread_nolock_s
@ cdecl _fseek_nolock(ptr long long) ucrtbase._fseek_nolock
@ cdecl _fseeki64(ptr int64 long) ucrtbase._fseeki64
@ cdecl _fseeki64_nolock(ptr int64 long) ucrtbase._fseeki64_nolock
@ cdecl _fsopen(str str long) ucrtbase._fsopen
@ cdecl _ftell_nolock(ptr) ucrtbase._ftell_nolock
@ cdecl -ret64 _ftelli64(ptr) ucrtbase._ftelli64
@ cdecl -ret64 _ftelli64_nolock(ptr) ucrtbase._ftelli64_nolock
@ cdecl _fwrite_nolock(ptr long long ptr) ucrtbase._fwrite_nolock
@ cdecl _get_fmode(ptr) ucrtbase._get_fmode
@ cdecl _get_osfhandle(long) ucrtbase._get_osfhandle
@ cdecl _get_printf_count_output() ucrtbase._get_printf_count_output
@ cdecl _get_stream_buffer_pointers(ptr ptr ptr ptr) ucrtbase._get_stream_buffer_pointers
@ cdecl _getc_nolock(ptr) ucrtbase._getc_nolock
@ cdecl _getcwd(str long) ucrtbase._getcwd
@ cdecl _getdcwd(long str long) ucrtbase._getdcwd
@ cdecl _getmaxstdio() ucrtbase._getmaxstdio
@ cdecl _getw(ptr) ucrtbase._getw
@ cdecl _getwc_nolock(ptr) ucrtbase._getwc_nolock
@ cdecl _getws(ptr) ucrtbase._getws
@ stub _getws_s
@ cdecl _isatty(long) ucrtbase._isatty
@ cdecl _kbhit() ucrtbase._kbhit
@ cdecl _locking(long long long) ucrtbase._locking
@ cdecl _lseek(long long long) ucrtbase._lseek
@ cdecl -ret64 _lseeki64(long int64 long) ucrtbase._lseeki64
@ cdecl _mktemp(str) ucrtbase._mktemp
@ cdecl _mktemp_s(str long) ucrtbase._mktemp_s
@ varargs _open(str long) ucrtbase._open
@ cdecl _open_osfhandle(long long) ucrtbase._open_osfhandle
@ cdecl _pclose(ptr) ucrtbase._pclose
@ cdecl _pipe(ptr long long) ucrtbase._pipe
@ cdecl _popen(str str) ucrtbase._popen
@ cdecl _putc_nolock(long ptr) ucrtbase._putc_nolock
@ cdecl _putw(long ptr) ucrtbase._putw
@ cdecl _putwc_nolock(long ptr) ucrtbase._putwc_nolock
@ cdecl _putws(wstr) ucrtbase._putws
@ cdecl _read(long ptr long) ucrtbase._read
@ cdecl _rmtmp() ucrtbase._rmtmp
@ cdecl _set_fmode(long) ucrtbase._set_fmode
@ cdecl _set_printf_count_output(long) ucrtbase._set_printf_count_output
@ cdecl _setmaxstdio(long) ucrtbase._setmaxstdio
@ cdecl _setmode(long long) ucrtbase._setmode
@ varargs _sopen(str long long) ucrtbase._sopen
@ cdecl _sopen_dispatch(str long long long ptr long) ucrtbase._sopen_dispatch
@ cdecl _sopen_s(ptr str long long long) ucrtbase._sopen_s
@ cdecl _tell(long) ucrtbase._tell
@ cdecl -ret64 _telli64(long) ucrtbase._telli64
@ cdecl _tempnam(str str) ucrtbase._tempnam
@ cdecl _ungetc_nolock(long ptr) ucrtbase._ungetc_nolock
@ cdecl _ungetwc_nolock(long ptr) ucrtbase._ungetwc_nolock
@ cdecl _wcreat(wstr long) ucrtbase._wcreat
@ cdecl _wfdopen(long wstr) ucrtbase._wfdopen
@ cdecl _wfopen(wstr wstr) ucrtbase._wfopen
@ cdecl _wfopen_s(ptr wstr wstr) ucrtbase._wfopen_s
@ cdecl _wfreopen(wstr wstr ptr) ucrtbase._wfreopen
@ cdecl _wfreopen_s(ptr wstr wstr ptr) ucrtbase._wfreopen_s
@ cdecl _wfsopen(wstr wstr long) ucrtbase._wfsopen
@ cdecl _wmktemp(wstr) ucrtbase._wmktemp
@ cdecl _wmktemp_s(wstr long) ucrtbase._wmktemp_s
@ varargs _wopen(wstr long) ucrtbase._wopen
@ cdecl _wpopen(wstr wstr) ucrtbase._wpopen
@ cdecl _write(long ptr long) ucrtbase._write
@ varargs _wsopen(wstr long long) ucrtbase._wsopen
@ cdecl _wsopen_dispatch(wstr long long long ptr long) ucrtbase._wsopen_dispatch
@ cdecl _wsopen_s(ptr wstr long long long) ucrtbase._wsopen_s
@ cdecl _wtempnam(wstr wstr) ucrtbase._wtempnam
@ cdecl _wtmpnam(ptr) ucrtbase._wtmpnam
@ cdecl _wtmpnam_s(ptr long) ucrtbase._wtmpnam_s
@ cdecl clearerr(ptr) ucrtbase.clearerr
@ cdecl clearerr_s(ptr) ucrtbase.clearerr_s
@ cdecl fclose(ptr) ucrtbase.fclose
@ cdecl feof(ptr) ucrtbase.feof
@ cdecl ferror(ptr) ucrtbase.ferror
@ cdecl fflush(ptr) ucrtbase.fflush
@ cdecl fgetc(ptr) ucrtbase.fgetc
@ cdecl fgetpos(ptr ptr) ucrtbase.fgetpos
@ cdecl fgets(ptr long ptr) ucrtbase.fgets
@ cdecl fgetwc(ptr) ucrtbase.fgetwc
@ cdecl fgetws(ptr long ptr) ucrtbase.fgetws
@ cdecl fopen(str str) ucrtbase.fopen
@ cdecl fopen_s(ptr str str) ucrtbase.fopen_s
@ cdecl fputc(long ptr) ucrtbase.fputc
@ cdecl fputs(str ptr) ucrtbase.fputs
@ cdecl fputwc(long ptr) ucrtbase.fputwc
@ cdecl fputws(wstr ptr) ucrtbase.fputws
@ cdecl fread(ptr long long ptr) ucrtbase.fread
@ cdecl fread_s(ptr long long long ptr) ucrtbase.fread_s
@ cdecl freopen(str str ptr) ucrtbase.freopen
@ cdecl freopen_s(ptr str str ptr) ucrtbase.freopen_s
@ cdecl fseek(ptr long long) ucrtbase.fseek
@ cdecl fsetpos(ptr ptr) ucrtbase.fsetpos
@ cdecl ftell(ptr) ucrtbase.ftell
@ cdecl fwrite(ptr long long ptr) ucrtbase.fwrite
@ cdecl getc(ptr) ucrtbase.getc
@ cdecl getchar() ucrtbase.getchar
@ cdecl gets(str) ucrtbase.gets
@ cdecl gets_s(ptr long) ucrtbase.gets_s
@ cdecl getwc(ptr) ucrtbase.getwc
@ cdecl getwchar() ucrtbase.getwchar
@ cdecl putc(long ptr) ucrtbase.putc
@ cdecl putchar(long) ucrtbase.putchar
@ cdecl puts(str) ucrtbase.puts
@ cdecl putwc(long ptr) ucrtbase.putwc
@ cdecl putwchar(long) ucrtbase.putwchar
@ cdecl rewind(ptr) ucrtbase.rewind
@ cdecl setbuf(ptr ptr) ucrtbase.setbuf
@ cdecl setvbuf(ptr str long long) ucrtbase.setvbuf
@ cdecl tmpfile() ucrtbase.tmpfile
@ cdecl tmpfile_s(ptr) ucrtbase.tmpfile_s
@ cdecl tmpnam(ptr) ucrtbase.tmpnam
@ cdecl tmpnam_s(ptr long) ucrtbase.tmpnam_s
@ cdecl ungetc(long ptr) ucrtbase.ungetc
@ cdecl ungetwc(long ptr) ucrtbase.ungetwc
