/*
 * Process definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef _INC_PROCESS
#define _INC_PROCESS

typedef void (*LPBEGINTHREAD)(LPVOID);
typedef UINT (WINAPI *LPBEGINTHREADEX)(LPVOID);

ULONG _beginthread(LPBEGINTHREAD,UINT,LPVOID);
void  _endthread(void);
ULONG _beginthreadex(LPVOID,UINT,LPBEGINTHREADEX,LPVOID,UINT,LPUINT);
void  _endthreadex(UINT);

#endif  /* _INC_PROCESS */
