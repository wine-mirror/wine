@ cdecl _access(str long) ucrtbase._access
@ cdecl _access_s(str long) ucrtbase._access_s
@ cdecl _chdir(str) ucrtbase._chdir
@ cdecl _chdrive(long) ucrtbase._chdrive
@ cdecl _chmod(str long) ucrtbase._chmod
@ cdecl _findclose(long) ucrtbase._findclose
@ cdecl _findfirst32(str ptr) ucrtbase._findfirst32
@ stub _findfirst32i64
@ cdecl _findfirst64(str ptr) ucrtbase._findfirst64
@ cdecl _findfirst64i32(str ptr) ucrtbase._findfirst64i32
@ cdecl _findnext32(long ptr) ucrtbase._findnext32
@ stub _findnext32i64
@ cdecl _findnext64(long ptr) ucrtbase._findnext64
@ cdecl _findnext64i32(long ptr) ucrtbase._findnext64i32
@ cdecl _fstat32(long ptr) ucrtbase._fstat32
@ cdecl _fstat32i64(long ptr) ucrtbase._fstat32i64
@ cdecl _fstat64(long ptr) ucrtbase._fstat64
@ cdecl _fstat64i32(long ptr) ucrtbase._fstat64i32
@ cdecl _fullpath(ptr str long) ucrtbase._fullpath
@ cdecl _getdiskfree(long ptr) ucrtbase._getdiskfree
@ cdecl _getdrive() ucrtbase._getdrive
@ cdecl _getdrives() ucrtbase._getdrives
@ cdecl _lock_file(ptr) ucrtbase._lock_file
@ cdecl _makepath(ptr str str str str) ucrtbase._makepath
@ cdecl _makepath_s(ptr long str str str str) ucrtbase._makepath_s
@ cdecl _mkdir(str) ucrtbase._mkdir
@ cdecl _rmdir(str) ucrtbase._rmdir
@ cdecl _splitpath(str ptr ptr ptr ptr) ucrtbase._splitpath
@ cdecl _splitpath_s(str ptr long ptr long ptr long ptr long) ucrtbase._splitpath_s
@ cdecl _stat32(str ptr) ucrtbase._stat32
@ cdecl _stat32i64(str ptr) ucrtbase._stat32i64
@ cdecl _stat64(str ptr) ucrtbase._stat64
@ cdecl _stat64i32(str ptr) ucrtbase._stat64i32
@ cdecl _umask(long) ucrtbase._umask
@ stub _umask_s
@ cdecl _unlink(str) ucrtbase._unlink
@ cdecl _unlock_file(ptr) ucrtbase._unlock_file
@ cdecl _waccess(wstr long) ucrtbase._waccess
@ cdecl _waccess_s(wstr long) ucrtbase._waccess_s
@ cdecl _wchdir(wstr) ucrtbase._wchdir
@ cdecl _wchmod(wstr long) ucrtbase._wchmod
@ cdecl _wfindfirst32(wstr ptr) ucrtbase._wfindfirst32
@ stub _wfindfirst32i64
@ cdecl _wfindfirst64(wstr ptr) ucrtbase._wfindfirst64
@ cdecl _wfindfirst64i32(wstr ptr) ucrtbase._wfindfirst64i32
@ stub _wfindnext32
@ stub _wfindnext32i64
@ cdecl _wfindnext64(long ptr) ucrtbase._wfindnext64
@ cdecl _wfindnext64i32(long ptr) ucrtbase._wfindnext64i32
@ cdecl _wfullpath(ptr wstr long) ucrtbase._wfullpath
@ cdecl _wmakepath(ptr wstr wstr wstr wstr) ucrtbase._wmakepath
@ cdecl _wmakepath_s(ptr long wstr wstr wstr wstr) ucrtbase._wmakepath_s
@ cdecl _wmkdir(wstr) ucrtbase._wmkdir
@ cdecl _wremove(wstr) ucrtbase._wremove
@ cdecl _wrename(wstr wstr) ucrtbase._wrename
@ cdecl _wrmdir(wstr) ucrtbase._wrmdir
@ cdecl _wsplitpath(wstr ptr ptr ptr ptr) ucrtbase._wsplitpath
@ cdecl _wsplitpath_s(wstr ptr long ptr long ptr long ptr long) ucrtbase._wsplitpath_s
@ cdecl _wstat32(wstr ptr) ucrtbase._wstat32
@ cdecl _wstat32i64(wstr ptr) ucrtbase._wstat32i64
@ cdecl _wstat64(wstr ptr) ucrtbase._wstat64
@ cdecl _wstat64i32(wstr ptr) ucrtbase._wstat64i32
@ cdecl _wunlink(wstr) ucrtbase._wunlink
@ cdecl remove(str) ucrtbase.remove
@ cdecl rename(str str) ucrtbase.rename
