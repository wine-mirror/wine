#ifndef __WINE_WINERROR_H
#define __WINE_WINERROR_H


extern int WIN32_LastError;

#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
#define MAKE_SCODE(sev,fac,code) \
        ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )


/* ERROR_UNKNOWN is a placeholder for error conditions which haven't
 * been tested yet so we're not exactly sure what will be returned.
 * All instances of ERROR_UNKNOWN should be tested under Win95/NT
 * and replaced.
 */
#define ERROR_UNKNOWN               99999

#define ERROR_SUCCESS               0
#define ERROR_FILE_NOT_FOUND        2
#define ERROR_PATH_NOT_FOUND        3
#define ERROR_TOO_MANY_OPEN_FILES   4
#define ERROR_ACCESS_DENIED         5
#define ERROR_INVALID_HANDLE        6
#define ERROR_NOT_ENOUGH_MEMORY     8
#define ERROR_BAD_FORMAT            11
#define ERROR_OUTOFMEMORY           14
#define ERROR_NO_MORE_FILES         18
#define ERROR_SHARING_VIOLATION     32
#define ERROR_LOCK_VIOLATION        33
#define ERROR_DUP_NAME              52
#define ERROR_FILE_EXISTS           80
#define ERROR_INVALID_PARAMETER     87
#define ERROR_BROKEN_PIPE           109
#define ERROR_DISK_FULL             112
#define ERROR_CALL_NOT_IMPLEMENTED  120
#define ERROR_INSUFFICIENT_BUFFER   122
#define ERROR_SEEK_ON_DEVICE        132
#define ERROR_DIR_NOT_EMPTY         145
#define ERROR_BUSY                  170
#define ERROR_ALREADY_EXISTS        183
#define ERROR_FILENAME_EXCED_RANGE  206
#define ERROR_MORE_DATA             234
#define ERROR_NO_MORE_ITEMS         259
#define ERROR_NOT_OWNER             288
#define ERROR_TOO_MANY_POSTS        298
#define ERROR_INVALID_ADDRESS       487
#define ERROR_CAN_NOT_COMPLETE      1003
#define ERROR_IO_DEVICE             1117
#define ERROR_POSSIBLE_DEADLOCK     1131
#define ERROR_BAD_DEVICE            1200
#define ERROR_NO_NETWORK            1222
#define ERROR_COMMITMENT_LIMIT      1455

/* HRESULT values for OLE, SHELL and other Interface stuff */
#define	NOERROR				0
#define	S_OK				0
#define	E_FAIL				0x80000008
#define	E_UNEXPECTED			0x8000FFFF

#define	OLE_E_ENUM_NOMORE		0x80040002
#define	CLASS_E_CLASSNOTAVAILABLE	0x80040111

#define	E_OUTOFMEMORY			0x8007000E
#define	E_INVALIDARG			0x80070057

#endif  /* __WINE_WINERROR_H */
