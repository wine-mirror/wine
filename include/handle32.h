#ifndef __WINE_HANDLE32_H
#define __WINE_HANDLE32_H

#include <stdlib.h>
#include "wintypes.h"

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

typedef struct {
    KERNEL_OBJECT       common;
    unsigned long       thread_id;
    unsigned long       process_id;
} THREAD_OBJECT;

typedef struct {
    KERNEL_OBJECT       common;
    unsigned long       process_id;
    unsigned long       main_thread_id;
} PROCESS_OBJECT;

/* The FILE object includes things like disk files, pipes, and
 * character devices (com ports, consoles, ...).
 */
typedef struct {
    KERNEL_OBJECT       common;
    int                 fd;                 /* UNIX fd */
    int                 type;               /* FILE_TYPE_* */
    unsigned long       misc_flags;         /* special flags */
    unsigned long       access_flags;       /* UNIX access flags */
    unsigned long       create_flags;       /* UNIX creation flags */
} FILE_OBJECT;

typedef struct {
    KERNEL_OBJECT	common;
    FILE_OBJECT	       *file_obj;
    int			prot;
    unsigned long	size;
} FILEMAP_OBJECT;

typedef struct {
    KERNEL_OBJECT       common;
} SEMAPHORE_OBJECT;

typedef struct {
    KERNEL_OBJECT       common;
} EVENT_OBJECT;

/* Should this even be here?
 */
typedef struct {
    KERNEL_OBJECT       common;
} REGKEY_OBJECT;

typedef struct _VRANGE_OBJECT{
	KERNEL_OBJECT		common;
	DWORD				start;
	DWORD				size;
	struct _VRANGE_OBJECT *next;
} VRANGE_OBJECT;

struct _HEAPITEM_OBJECT;
typedef struct{
	KERNEL_OBJECT		common;
	LPVOID	start;
	DWORD	size;
	DWORD	maximum;
	DWORD	flags;
	struct _HEAPITEM_OBJECT *first,*last;
} HEAP_OBJECT;

typedef struct _HEAPITEM_OBJECT{
	KERNEL_OBJECT	common;
	HEAP_OBJECT	*heap;
	DWORD size;		/* size including header */
	struct _HEAPITEM_OBJECT *next,*prev;
} HEAPITEM_OBJECT;


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

/* Prototypes for the Close*Handle functions
 */
int CloseFileHandle(FILE_OBJECT *hFile);

#endif /* __WINE_HANDLE32_H */
