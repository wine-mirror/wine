/*
 * KERNEL32 objects
 *
 * Copyright 1996, 1998 Alexandre Julliard
 */

#ifndef __WINE_K32OBJ_H
#define __WINE_K32OBJ_H

#include "wintypes.h"
#include "windows.h"

/* Object types */
typedef enum
{
    K32OBJ_UNKNOWN = 0,
    K32OBJ_SEMAPHORE,
    K32OBJ_EVENT,
    K32OBJ_MUTEX,
    K32OBJ_CRITICAL_SECTION,
    K32OBJ_PROCESS,
    K32OBJ_THREAD,
    K32OBJ_FILE,
    K32OBJ_CHANGE,
    K32OBJ_CONSOLE,
    K32OBJ_SCREEN_BUFFER,
    K32OBJ_MEM_MAPPED_FILE,
    K32OBJ_SERIAL,
    K32OBJ_DEVICE_IOCTL,
    K32OBJ_PIPE,
    K32OBJ_MAILSLOT,
    K32OBJ_TOOLHELP_SNAPSHOT,
    K32OBJ_SOCKET,
    K32OBJ_NBOBJECTS
} K32OBJ_TYPE;

/* Kernel object */
typedef struct
{
    K32OBJ_TYPE   type;
    LONG          refcount;
} K32OBJ;

/* Kernel object operations */
typedef struct
{
    BOOL32 (*signaled)(K32OBJ*,DWORD);    /* Is object signaled? */
    BOOL32 (*satisfied)(K32OBJ*,DWORD);   /* Wait on object is satisfied */
    void   (*add_wait)(K32OBJ*,DWORD);    /* Add thread to wait queue */
    void   (*remove_wait)(K32OBJ*,DWORD); /* Remove thread from wait queue */
    BOOL32 (*read)(K32OBJ*,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
    BOOL32 (*write)(K32OBJ*,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);
    void   (*destroy)(K32OBJ *);          /* Destroy object on refcount==0 */
} K32OBJ_OPS;

extern const K32OBJ_OPS * const K32OBJ_Ops[K32OBJ_NBOBJECTS];

#define K32OBJ_OPS(obj) (K32OBJ_Ops[(obj)->type])

extern void K32OBJ_IncCount( K32OBJ *ptr );
extern void K32OBJ_DecCount( K32OBJ *ptr );
extern BOOL32 K32OBJ_IsValid( K32OBJ *ptr, K32OBJ_TYPE type );
extern BOOL32 K32OBJ_AddName( K32OBJ *obj, LPCSTR name );
extern K32OBJ *K32OBJ_Create( K32OBJ_TYPE type, DWORD size, LPCSTR name,
                              DWORD access, HANDLE32 *handle );
extern K32OBJ *K32OBJ_FindName( LPCSTR name );
extern K32OBJ *K32OBJ_FindNameType( LPCSTR name, K32OBJ_TYPE type );

#endif /* __WINE_K32OBJ_H */
