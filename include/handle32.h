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


/* The _*_OBJECT structures contain information needed about each
 * particular type of handle.  This information is a combination of
 * equivalent UNIX-style handles/descriptors and general information
 * that the Win32 API might request.
 *
 * The KERNEL_OBJECT structure must be the first member of any specific
 * kernel object type's structure.
 */

typedef struct {
    unsigned long       magic;
} KERNEL_OBJECT;


/* Object number definitions.  These numbers are used to
 * validate the kernel object by comparison against the
 * object's 'magic' value.
  */
#define KERNEL_OBJECT_UNUSED    2404554046UL
#define KERNEL_OBJECT_THREAD    (KERNEL_OBJECT_UNUSED + 1)
#define KERNEL_OBJECT_PROCESS   (KERNEL_OBJECT_UNUSED + 2)
#define KERNEL_OBJECT_FILE      (KERNEL_OBJECT_UNUSED + 3)
#define KERNEL_OBJECT_SEMAPHORE (KERNEL_OBJECT_UNUSED + 4)
#define KERNEL_OBJECT_EVENT     (KERNEL_OBJECT_UNUSED + 5)
#define KERNEL_OBJECT_REGKEY    (KERNEL_OBJECT_UNUSED + 6)
#define KERNEL_OBJECT_FILEMAP   (KERNEL_OBJECT_UNUSED + 7)
#define KERNEL_OBJECT_VRANGE    (KERNEL_OBJECT_UNUSED + 8)
#define KERNEL_OBJECT_HEAP      (KERNEL_OBJECT_UNUSED + 9)
#define KERNEL_OBJECT_HEAPITEM  (KERNEL_OBJECT_UNUSED + 10)

/* Functions for checking kernel objects.
 */
int ValidateKernelObject(KERNEL_OBJECT *ptr);

/* For now, CreateKernelObject and ReleaseKernelObject will
 * simply map to malloc() and free().
 */
#define CreateKernelObject(size) (malloc(size))
#define ReleaseKernelObject(ptr) (free(ptr))

#endif /* __WINE_HANDLE32_H */
