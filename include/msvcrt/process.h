/*
 * Process definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_PROCESS_H
#define __WINE_PROCESS_H
#define __WINE_USE_MSVCRT

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
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


#ifdef __cplusplus
extern "C" {
#endif

typedef void (__cdecl *_beginthread_start_routine_t)(void *);
typedef unsigned int (__stdcall *_beginthreadex_start_routine_t)(void *);

unsigned long _beginthread(_beginthread_start_routine_t,unsigned int,void*);
unsigned long _beginthreadex(void*,unsigned int,_beginthreadex_start_routine_t,void*,unsigned int,unsigned int*);
int         _cwait(int*,int,int);
void        _endthread(void);
void        _endthreadex(unsigned int);
int         _execl(const char*,const char*,...);
int         _execle(const char*,const char*,...);
int         _execlp(const char*,const char*,...);
int         _execlpe(const char*,const char*,...);
int         _execv(const char*,char* const *);
int         _execve(const char*,char* const *,const char* const *);
int         _execvp(const char*,char* const *);
int         _execvpe(const char*,char* const *,const char* const *);
int         _getpid(void);
int         _spawnl(int,const char*,const char*,...);
int         _spawnle(int,const char*,const char*,...);
int         _spawnlp(int,const char*,const char*,...);
int         _spawnlpe(int,const char*,const char*,...);
int         _spawnv(int,const char*,const char* const *);
int         _spawnve(int,const char*,const char* const *,const char* const *);
int         _spawnvp(int,const char*,const char* const *);
int         _spawnvpe(int,const char*,const char* const *,const char* const *);

void        MSVCRT(_c_exit)(void);
void        MSVCRT(_cexit)(void);
void        MSVCRT(_exit)(int);
void        MSVCRT(abort)(void);
void        MSVCRT(exit)(int);
int         MSVCRT(system)(const char*);

#ifndef MSVCRT_WPROCESS_DEFINED
#define MSVCRT_WPROCESS_DEFINED
int         _wexecl(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexecle(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexeclp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexeclpe(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexecv(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wexecve(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wexecvp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wexecvpe(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wspawnl(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnle(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnlp(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnlpe(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnv(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wspawnve(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wspawnvp(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wspawnvpe(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wsystem(const MSVCRT(wchar_t)*);
#endif /* MSVCRT_WPROCESS_DEFINED */

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define P_WAIT          _P_WAIT
#define P_NOWAIT        _P_NOWAIT
#define P_OVERLAY       _P_OVERLAY
#define P_NOWAITO       _P_NOWAITO
#define P_DETACH        _P_DETACH

#define WAIT_CHILD      _WAIT_CHILD
#define WAIT_GRANDCHILD _WAIT_GRANDCHILD

#define cwait    _cwait
#define getpid   _getpid
#define execl    _execl
#define execle   _execle
#define execlp   _execlp
#define execlpe  _execlpe
#define execv    _execv
#define execve   _execve
#define execvp   _execvp
#define execvpe  _execvpe
#define spawnl   _spawnl
#define spawnle  _spawnle
#define spawnlp  _spawnlp
#define spawnlpe _spawnlpe
#define spawnv   _spawnv
#define spawnve  _spawnve
#define spawnvp  _spawnvp
#define spawnvpe _spawnvpe
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_PROCESS_H */
