#ifndef __WINE_HANDLE32_H
#define __WINE_HANDLE32_H

#include <malloc.h>

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
} KERNEL_OBJECT, *HANDLE32;

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

/* Define the invalid handle value
 */
#define INVALID_HANDLE_VALUE    ((HANDLE32)-1)

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
