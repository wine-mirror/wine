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
#include "winioctl.h"

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
			       GENERIC_WRITE | GENERIC_EXECUTE /*FIXME*/, TRUE, -1 );
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
 * This is one of those big ugly nasty procedure which can do
 * a million and one things when it comes to devices. It can also be
 * used for VxD communication.
 *
 * A return value of FALSE indicates that something has gone wrong which
 * GetLastError can decypher.
 */
BOOL32 WINAPI DeviceIoControl(HANDLE32 hDevice, DWORD dwIoControlCode, 
			      LPVOID lpvlnBuffer, DWORD cblnBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
	DEVICE_OBJECT	*dev = (DEVICE_OBJECT *)HANDLE_GetObjPtr(
		PROCESS_Current(), hDevice, K32OBJ_DEVICE_IOCTL, 0 /*FIXME*/, NULL );

        FIXME(win32, "(%ld,%ld,%p,%ld,%p,%ld,%p,%p), stub\n",
		hDevice,dwIoControlCode,lpvlnBuffer,cblnBuffer,
		lpvOutBuffer,cbOutBuffer,lpcbBytesReturned,lpOverlapped
	);

	if (!dev)
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	/* Check if this is a user defined control code for a VxD */
        if( HIWORD( dwIoControlCode ) == 0 )
        {
	    /* FIXME: Set appropriate error */
	    FIXME(win32," VxD device %s msg\n",dev->devname);
 
	    if (!strcmp(dev->devname,"VTDAPI"))
            {
		switch (dwIoControlCode)
                {
		case 5:	if (lpvOutBuffer && (cbOutBuffer>=4))
				*(DWORD*)lpvOutBuffer = timeGetTime();
			if (lpcbBytesReturned)
				*lpcbBytesReturned = 4;
			return TRUE;
		default:
			break;
		}
	
           }
	}
	else
	{
		switch( dwIoControlCode )
		{
		case FSCTL_DELETE_REPARSE_POINT:
		case FSCTL_DISMOUNT_VOLUME:
		case FSCTL_GET_COMPRESSION:
		case FSCTL_GET_REPARSE_POINT:
		case FSCTL_LOCK_VOLUME:
		case FSCTL_QUERY_ALLOCATED_RANGES:
		case FSCTL_SET_COMPRESSION:
		case FSCTL_SET_REPARSE_POINT:
		case FSCTL_SET_SPARSE:
		case FSCTL_SET_ZERO_DATA:
		case FSCTL_UNLOCK_VOLUME:
		case IOCTL_DISK_CHECK_VERIFY:
		case IOCTL_DISK_EJECT_MEDIA:
		case IOCTL_DISK_FORMAT_TRACKS:
		case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		case IOCTL_DISK_GET_DRIVE_LAYOUT:
		case IOCTL_DISK_GET_MEDIA_TYPES:
		case IOCTL_DISK_GET_PARTITION_INFO:
		case IOCTL_DISK_LOAD_MEDIA:
		case IOCTL_DISK_MEDIA_REMOVAL:
		case IOCTL_DISK_PERFORMANCE:
		case IOCTL_DISK_REASSIGN_BLOCKS:
		case IOCTL_DISK_SET_DRIVE_LAYOUT:
		case IOCTL_DISK_SET_PARTITION_INFO:
		case IOCTL_DISK_VERIFY:
		case IOCTL_SERIAL_LSRMST_INSERT:
		case IOCTL_STORAGE_CHECK_VERIFY:
		case IOCTL_STORAGE_EJECT_MEDIA:
		case IOCTL_STORAGE_GET_MEDIA_TYPES:
		case IOCTL_STORAGE_LOAD_MEDIA:
		case IOCTL_STORAGE_MEDIA_REMOVAL:
    			FIXME( win32, "unimplemented dwIoControlCode=%08lx\n", dwIoControlCode);
    			SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    			return FALSE;
    			break;
		default:
    			FIXME( win32, "ignored dwIoControlCode=%08lx\n",dwIoControlCode);
    			SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    			return FALSE;
    			break;
		}
	}
   	return FALSE;
}
