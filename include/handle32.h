/*
 * KERNEL32 objects
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_HANDLE32_H
#define __WINE_HANDLE32_H

#include "wintypes.h"

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
    DWORD         refcount;
} K32OBJ;

extern void K32OBJ_IncCount( K32OBJ *ptr );
extern void K32OBJ_DecCount( K32OBJ *ptr );

#endif /* __WINE_HANDLE32_H */
