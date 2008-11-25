/*
 * Process definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_PROCESS_H
#define __WINE_PROCESS_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif

/* Process creation flags */
#define _P_WAIT    0
#define _P_NOWAIT  1
#define _P_OVERLAY 2
#define _P_NOWAITO 3
#define _P_DETACH  4

#define _WAIT_CHILD      0
#define _WAIT_GRANDCHILD 1

#ifndef __stdcall
# ifdef __i386__
#  ifdef __GNUC__
#   ifdef __APPLE__ /* Mac OS X uses a 16-byte aligned stack and not a 4-byte one */
#    define __stdcall __attribute__((__stdcall__)) __attribute__((__force_align_arg_pointer__))
#   else
#    define __stdcall __attribute__((__stdcall__))
#   endif
#  elif defined(_MSC_VER)
    /* Nothing needs to be done. __stdcall already exists */
#  else
#   error You need to define __stdcall for your compiler
#  endif
# elif defined(__x86_64__) && defined (__GNUC__)
#  define __stdcall __attribute__((ms_abi))
# else
#  define __stdcall
# endif
#endif /* __stdcall */

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*_beginthread_start_routine_t)(void *);
typedef unsigned int (__stdcall *_beginthreadex_start_routine_t)(void *);

uintptr_t _beginthread(_beginthread_start_routine_t,unsigned int,void*);
uintptr_t _beginthreadex(void*,unsigned int,_beginthreadex_start_routine_t,void*,unsigned int,unsigned int*);
intptr_t    _cwait(int*,intptr_t,int);
void        _endthread(void);
void        _endthreadex(unsigned int);
intptr_t    _execl(const char*,const char*,...);
intptr_t    _execle(const char*,const char*,...);
intptr_t    _execlp(const char*,const char*,...);
intptr_t    _execlpe(const char*,const char*,...);
intptr_t    _execv(const char*,char* const *);
intptr_t    _execve(const char*,char* const *,const char* const *);
intptr_t    _execvp(const char*,char* const *);
intptr_t    _execvpe(const char*,char* const *,const char* const *);
int         _getpid(void);
intptr_t    _spawnl(int,const char*,const char*,...);
intptr_t    _spawnle(int,const char*,const char*,...);
intptr_t    _spawnlp(int,const char*,const char*,...);
intptr_t    _spawnlpe(int,const char*,const char*,...);
intptr_t    _spawnv(int,const char*,const char* const *);
intptr_t    _spawnve(int,const char*,const char* const *,const char* const *);
intptr_t    _spawnvp(int,const char*,const char* const *);
intptr_t    _spawnvpe(int,const char*,const char* const *,const char* const *);

void        _c_exit(void);
void        _cexit(void);
void        _exit(int);
void        abort(void);
void        exit(int);
int         system(const char*);

#ifndef _WPROCESS_DEFINED
#define _WPROCESS_DEFINED
intptr_t    _wexecl(const wchar_t*,const wchar_t*,...);
intptr_t    _wexecle(const wchar_t*,const wchar_t*,...);
intptr_t    _wexeclp(const wchar_t*,const wchar_t*,...);
intptr_t    _wexeclpe(const wchar_t*,const wchar_t*,...);
intptr_t    _wexecv(const wchar_t*,const wchar_t* const *);
intptr_t    _wexecve(const wchar_t*,const wchar_t* const *,const wchar_t* const *);
intptr_t    _wexecvp(const wchar_t*,const wchar_t* const *);
intptr_t    _wexecvpe(const wchar_t*,const wchar_t* const *,const wchar_t* const *);
intptr_t    _wspawnl(int,const wchar_t*,const wchar_t*,...);
intptr_t    _wspawnle(int,const wchar_t*,const wchar_t*,...);
intptr_t    _wspawnlp(int,const wchar_t*,const wchar_t*,...);
intptr_t    _wspawnlpe(int,const wchar_t*,const wchar_t*,...);
intptr_t    _wspawnv(int,const wchar_t*,const wchar_t* const *);
intptr_t    _wspawnve(int,const wchar_t*,const wchar_t* const *,const wchar_t* const *);
intptr_t    _wspawnvp(int,const wchar_t*,const wchar_t* const *);
intptr_t    _wspawnvpe(int,const wchar_t*,const wchar_t* const *,const wchar_t* const *);
int         _wsystem(const wchar_t*);
#endif /* _WPROCESS_DEFINED */

#ifdef __cplusplus
}
#endif


#define P_WAIT          _P_WAIT
#define P_NOWAIT        _P_NOWAIT
#define P_OVERLAY       _P_OVERLAY
#define P_NOWAITO       _P_NOWAITO
#define P_DETACH        _P_DETACH

#define WAIT_CHILD      _WAIT_CHILD
#define WAIT_GRANDCHILD _WAIT_GRANDCHILD

static inline intptr_t cwait(int *status, intptr_t pid, int action) { return _cwait(status, pid, action); }
static inline int getpid(void) { return _getpid(); }
static inline intptr_t execv(const char* name, char* const* argv) { return _execv(name, argv); }
static inline intptr_t execve(const char* name, char* const* argv, const char* const* envv) { return _execve(name, argv, envv); }
static inline intptr_t execvp(const char* name, char* const* argv) { return _execvp(name, argv); }
static inline intptr_t execvpe(const char* name, char* const* argv, const char* const* envv) { return _execvpe(name, argv, envv); }
static inline intptr_t spawnv(int flags, const char* name, const char* const* argv) { return _spawnv(flags, name, argv); }
static inline intptr_t spawnve(int flags, const char* name, const char* const* argv, const char* const* envv) { return _spawnve(flags, name, argv, envv); }
static inline intptr_t spawnvp(int flags, const char* name, const char* const* argv) { return _spawnvp(flags, name, argv); }
static inline intptr_t spawnvpe(int flags, const char* name, const char* const* argv, const char* const* envv) { return _spawnvpe(flags, name, argv, envv); }

#if defined(__GNUC__) && (__GNUC__ < 4)
extern intptr_t execl(const char*,const char*,...) __attribute__((alias("_execl")));
extern intptr_t execle(const char*,const char*,...) __attribute__((alias("_execle")));
extern intptr_t execlp(const char*,const char*,...) __attribute__((alias("_execlp")));
extern intptr_t execlpe(const char*,const char*,...) __attribute__((alias("_execlpe")));
extern intptr_t spawnl(int,const char*,const char*,...) __attribute__((alias("_spawnl")));
extern intptr_t spawnle(int,const char*,const char*,...) __attribute__((alias("_spawnle")));
extern intptr_t spawnlp(int,const char*,const char*,...) __attribute__((alias("_spawnlp")));
extern intptr_t spawnlpe(int,const char*,const char*,...) __attribute__((alias("_spawnlpe")));
#else
#define execl    _execl
#define execle   _execle
#define execlp   _execlp
#define execlpe  _execlpe
#define spawnl   _spawnl
#define spawnle  _spawnle
#define spawnlp  _spawnlp
#define spawnlpe _spawnlpe
#endif  /* __GNUC__ */

#endif /* __WINE_PROCESS_H */
