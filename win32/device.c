/*
 * Win32 device functions
 *
 * Copyright 1998 Marcus Meissner
 */

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "windows.h"
#include "winbase.h"
#include "winerror.h"
#include "file.h"
#include "process.h"
#include "mmsystem.h"
#include "heap.h"
#include "debug.h"

void DEVICE_Destroy(K32OBJ *dev);
const K32OBJ_OPS DEVICE_Ops =
{
    NULL,		/* signaled */
    NULL,		/* satisfied */
    NULL,		/* add_wait */
    NULL,		/* remove_wait */
    NULL,		/* read */
    NULL,		/* write */
    DEVICE_Destroy	/* destroy */
};

/* The device object */
typedef struct
{
    K32OBJ    header;
    int       mode;
    char     *devname;
} DEVICE_OBJECT;

HANDLE32
DEVICE_Open(LPCSTR filename, DWORD access) {
	DEVICE_OBJECT	*dev;
	HANDLE32 handle;

	dev = HeapAlloc( SystemHeap, 0, sizeof(FILE_OBJECT) );
	if (!dev)
	    return INVALID_HANDLE_VALUE32;
	dev->header.type = K32OBJ_DEVICE_IOCTL;
	dev->header.refcount = 0;
	dev->mode	= access;
	dev->devname	= HEAP_strdupA(SystemHeap,0,filename);

	handle = HANDLE_Alloc( PROCESS_Current(), &(dev->header),
			       FILE_ALL_ACCESS | GENERIC_READ |
			       GENERIC_WRITE | GENERIC_EXECUTE /*FIXME*/, TRUE );
	/* If the allocation failed, the object is already destroyed */
	if (handle == INVALID_HANDLE_VALUE32) dev = NULL;
	return handle;
}

void
DEVICE_Destroy(K32OBJ *dev) {
	assert(dev->type == K32OBJ_DEVICE_IOCTL);
}

/****************************************************************************
 *		DeviceIoControl (KERNEL32.188)
 */
BOOL32 WINAPI DeviceIoControl(HANDLE32 hDevice, DWORD dwIoControlCode, 
			      LPVOID lpvlnBuffer, DWORD cblnBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
	DEVICE_OBJECT	*dev = (DEVICE_OBJECT *)HANDLE_GetObjPtr(
		PROCESS_Current(), hDevice, K32OBJ_DEVICE_IOCTL, 0 /*FIXME*/ );

        FIXME(win32, "(%ld,%ld,%p,%ld,%p,%ld,%p,%p), stub\n",
		hDevice,dwIoControlCode,lpvlnBuffer,cblnBuffer,
		lpvOutBuffer,cbOutBuffer,lpcbBytesReturned,lpOverlapped
	);
	if (!dev)
		return FALSE;
	/* FIXME: Set appropriate error */
	FIXME(win32,"	device %s\n",dev->devname);
	if (!strcmp(dev->devname,"VTDAPI")) {
		switch (dwIoControlCode) {
		case 5:	if (lpvOutBuffer && (cbOutBuffer>=4))
				*(DWORD*)lpvOutBuffer = timeGetTime();
			if (lpcbBytesReturned)
				*lpcbBytesReturned = 4;
			return TRUE;
		default:
			break;
		}
	}
	FIXME(win32,"	(unhandled)\n");
	return FALSE;
}
