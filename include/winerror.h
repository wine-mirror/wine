#ifndef __WINE_WINERROR_H
#define __WINE_WINERROR_H


extern int WIN32_LastError;

#define FACILITY_ITF		4
#define FACILITY_WIN32		7

#define SEVERITY_ERROR		1


#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
#define MAKE_SCODE(sev,fac,code) \
        ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
#define SUCCEEDED(stat) ((HRESULT)(stat)>=0)
#define FAILED(stat) ((HRESULT)(stat)<0)

/* ERROR_UNKNOWN is a placeholder for error conditions which haven't
 * been tested yet so we're not exactly sure what will be returned.
 * All instances of ERROR_UNKNOWN should be tested under Win95/NT
 * and replaced.
 */
#define ERROR_UNKNOWN               99999

#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1

#define NO_ERROR                    0
#define ERROR_SUCCESS               0
#define ERROR_INVALID_FUNCTION      1
#define ERROR_FILE_NOT_FOUND        2
#define ERROR_PATH_NOT_FOUND        3
#define ERROR_TOO_MANY_OPEN_FILES   4
#define ERROR_ACCESS_DENIED         5
#define ERROR_INVALID_HANDLE        6
#define ERROR_NOT_ENOUGH_MEMORY     8
#define ERROR_BAD_FORMAT            11
#define ERROR_INVALID_ACCESS        12
#define ERROR_INVALID_DATA          13
#define ERROR_OUTOFMEMORY           14
#define ERROR_INVALID_DRIVE         15
#define ERROR_CURRENT_DIRECTORY     16
#define ERROR_NOT_SAME_DEVICE       17
#define ERROR_NO_MORE_FILES         18
#define ERROR_WRITE_PROTECT         19
#define ERROR_BAD_UNIT              20
#define ERROR_NOT_READY             21
#define ERROR_BAD_COMMAND           22
#define ERROR_CRC                   23
#define ERROR_BAD_LENGTH            24
#define ERROR_SEEK                  25
#define ERROR_NOT_DOS_DISK          26
#define ERROR_SECTOR_NOT_FOUND      27
#define ERROR_WRITE_FAULT           29
#define ERROR_READ_FAULT            30
#define ERROR_ALREADY_ASSIGNED      85
#define ERROR_INVALID_PASSWORD      86
#define ERROR_NET_WRITE_FAULT       88
#define ERROR_SHARING_VIOLATION     32
#define ERROR_LOCK_VIOLATION        33
#define ERROR_WRONG_DISK            34
#define ERROR_SHARING_BUFFER_EXCEEDED 36
#define ERROR_HANDLE_EOF            38
#define ERROR_HANDLE_DISK_FULL      39
#define ERROR_DUP_NAME              52
#define ERROR_BAD_NETPATH           53
#define ERROR_NETWORK_BUSY          54
#define ERROR_DEV_NOT_EXIST         55
#define ERROR_ADAP_HDW_ERR          57
#define ERROR_BAD_NET_RESP          58
#define ERROR_UNEXP_NET_ERR         59
#define ERROR_BAD_REM_ADAP          60
#define ERROR_NO_SPOOL_SPACE        62
#define ERROR_NETNAME_DELETED       64
#define ERROR_NETWORK_ACCESS_DENIED 65
#define ERROR_BAD_DEV_TYPE          66
#define ERROR_BAD_NET_NAME          67
#define ERROR_TOO_MANY_NAMES        68
#define ERROR_SHARING_PAUSED        70
#define ERROR_REQ_NOT_ACCEP         71
#define ERROR_FILE_EXISTS           80
#define ERROR_CANNOT_MAKE           82
#define ERROR_INVALID_PARAMETER     87
#define ERROR_DISK_CHANGE           107
#define ERROR_DRIVE_LOCKED          108
#define ERROR_BROKEN_PIPE           109
#define ERROR_BUFFER_OVERFLOW       111
#define ERROR_DISK_FULL             112
#define ERROR_NO_MORE_SEARCH_HANDLES 113
#define ERROR_INVALID_TARGET_HANDLE 114
#define ERROR_INVALID_CATEGORY      117
#define ERROR_CALL_NOT_IMPLEMENTED  120
#define ERROR_INSUFFICIENT_BUFFER   122
#define ERROR_INVALID_NAME          123
#define ERROR_INVALID_LEVEL         124
#define ERROR_NO_VOLUME_LABEL       125
#define ERROR_NEGATIVE_SEEK         131
#define ERROR_SEEK_ON_DEVICE        132
#define ERROR_DIR_NOT_ROOT          144
#define ERROR_DIR_NOT_EMPTY         145
#define ERROR_LABEL_TOO_LONG        154
#define ERROR_BAD_PATHNAME          161
#define ERROR_LOCK_FAILED           167
#define ERROR_BUSY                  170
#define ERROR_INVALID_ORDINAL       182
#define ERROR_ALREADY_EXISTS        183
#define ERROR_INVALID_EXE_SIGNATURE 191
#define ERROR_BAD_EXE_FORMAT        193
#define ERROR_FILENAME_EXCED_RANGE  206
#define ERROR_META_EXPANSION_TOO_LONG 208
#define ERROR_BAD_PIPE              230
#define ERROR_PIPE_BUSY             231
#define ERROR_NO_DATA               232
#define ERROR_PIPE_NOT_CONNECTED    233
#define ERROR_MORE_DATA             234
#define ERROR_NO_MORE_ITEMS         259
#define ERROR_DIRECTORY             267
#define ERROR_NOT_OWNER             288
#define ERROR_TOO_MANY_POSTS        298
#define ERROR_INVALID_ADDRESS       487
#define ERROR_OPERATION_ABORTED     995
#define ERROR_IO_INCOMPLETE         996
#define ERROR_IO_PENDING            997
#define ERROR_INVALID_ACCESS_TO_MEM 998 
#define ERROR_SWAPERROR             999
#define ERROR_CAN_NOT_COMPLETE      1003
#define ERROR_BADKEY                1010 /* Config reg key invalid */
#define ERROR_CANTREAD              1012 /* Config reg key couldn't be read */
#define ERROR_CANTWRITE             1013 /* Config reg key couldn't be written */
#define ERROR_IO_DEVICE             1117
#define ERROR_POSSIBLE_DEADLOCK     1131
#define ERROR_BAD_DEVICE            1200
#define ERROR_NO_NETWORK            1222
#define ERROR_ALREADY_INITIALIZED   1247
#define ERROR_PRIVILEGE_NOT_HELD    1314
#define ERROR_INVALID_WINDOW_HANDLE 1400
#define ERROR_CANNOT_FIND_WND_CLASS 1407
#define ERROR_WINDOW_OF_OTHER_THREAD 1408
#define ERROR_CLASS_ALREADY_EXISTS  1410
#define ERROR_CLASS_DOES_NOT_EXIST  1411
#define ERROR_CLASS_HAS_WINDOWS     1412
#define ERROR_COMMITMENT_LIMIT      1455
#define ERROR_INVALID_PRINTER_NAME  1801

/* HRESULT values for OLE, SHELL and other Interface stuff */
/* the codes 4000-40ff are reserved for OLE */
#define NOERROR                                0L
#define S_OK                                   ((HRESULT)0L)
#define S_FALSE                                ((HRESULT)1L)

#define DISP_E_BADVARTYPE   0x80020008L
#define DISP_E_OVERFLOW     0x8002000AL
#define DISP_E_TYPEMISMATCH 0x80020005L
#define DISP_E_ARRAYISLOCKED  0x8002000D
#define DISP_E_BADINDEX       0x8002000B



/* Drag and Drop */
#define DRAGDROP_S_DROP   0x00040100L
#define DRAGDROP_S_CANCEL 0x00040101L

#define	E_UNEXPECTED			0x8000FFFF

#define E_NOTIMPL			0x80004001
#define E_NOINTERFACE			0x80004002
#define E_POINTER			0x80004003
#define E_ABORT				0x80004004
#define E_FAIL				0x80004005

/*#define CO_E_INIT_TLS			0x80004006
#define CO_E_INIT_SHARED_ALLOCATOR	0x80004007
#define CO_E_INIT_MEMORY_ALLOCATOR	0x80004008
#define CO_E_INIT_CLASS_CACHE		0x80004009
#define CO_E_INIT_RPC_CHANNEL		0x8000400A
#define CO_E_INIT_TLS_SET_CHANNEL_CONTROL	0x8000400B
#define CO_E_INIT_TLS_CHANNEL_CONTROL	0x8000400C
#define CO_E_INIT_UNACCEPTED_USER_ALLOCATOR	0x8000400D
#define CO_E_INIT_SCM_MUTEX_EXISTS	0x8000400E
#define CO_E_INIT_SCM_FILE_MAPPING_EXISTS	0x8000400F
#define CO_E_INIT_SCM_MAP_VIEW_OF_FILE	0x80004010
#define CO_E_INIT_SCM_EXEC_FAILURE	0x80004011
#define CO_E_INIT_ONLY_SINGLE_THREADED	0x80004012 */

#define CO_E_OBJISREG                   0x800401FB
#define	OLE_E_ENUM_NOMORE		      0x80040002
#define CLASS_E_NOAGGREGATION     0x80040110
#define	CLASS_E_CLASSNOTAVAILABLE	0x80040111
#define E_ACCESSDENIED			      0x80070005
#define E_HANDLE            			0x80070006
#define	E_OUTOFMEMORY			        0x8007000E
#define	E_INVALIDARG			        0x80070057

/*#define OLE_E_FIRST 0x80040000L */
/*#define OLE_E_LAST  0x800400FFL */
/*#define OLE_S_FIRST 0x00040000L */
/*#define OLE_S_LAST  0x000400FFL */

#define STG_E_INVALIDFUNCTION		0x80030001
#define STG_E_FILENOTFOUND		0x80030002
#define STG_E_PATHNOTFOUND		0x80030003
#define STG_E_TOOMANYOPENFILES		0x80030004
#define STG_E_ACCESSDENIED		0x80030005
#define STG_E_INVALIDHANDLE		0x80030006
#define STG_E_INSUFFICIENTMEMORY	0x80030008
#define STG_E_INVALIDPOINTER		0x80030009
#define STG_E_NOMOREFILES		0x80030012
#define STG_E_DISKISWRITEPROTECTED	0x80030013
#define STG_E_SEEKERROR			0x80030019
#define STG_E_WRITEFAULT		0x8003001D
#define STG_E_READFAULT			0x8003001E
#define STG_E_SHAREVIOLATION		0x80030020
#define STG_E_LOCKVIOLATION		0x80030021
#define STG_E_FILEALREADYEXISTS		0x80030050
#define STG_E_INVALIDPARAMETER		0x80030057
#define STG_E_MEDIUMFULL		0x80030070
#define STG_E_ABNORMALAPIEXIT		0x800300FA
#define STG_E_INVALIDHEADER		0x800300FB
#define STG_E_INVALIDNAME		0x800300FC
#define STG_E_UNKNOWN			0x800300FD
#define STG_E_UNIMPLEMENTEDFUNCTION	0x800300FE
#define STG_E_INVALIDFLAG		0x800300FF
#define STG_E_INUSE			0x80030100
#define STG_E_NOTCURRENT		0x80030101
#define STG_E_REVERTED			0x80030102
#define STG_E_CANTSAVE			0x80030103
#define STG_E_OLDFORMAT			0x80030104
#define STG_E_OLDDLL			0x80030105
#define STG_E_SHAREREQUIRED		0x80030106
#define STG_E_NOTFILEBASEDSTORAGE	0x80030107
#define STG_E_EXTANTMARSHALLINGS	0x80030108

/* alten versionen
#define E_NOTIMPL			0x80000001
#define E_OUTOFMEMORY			0x80000002
#define E_INVALIDARG			0x80000003
#define E_NOINTERFACE			0x80000004
#define E_POINTER			0x80000005
#define E_HANDLE			0x80000006
#define E_ABORT				0x80000007
#define	E_FAIL				0x80000008
#define E_ACCESSDENIED			0x80000009 */

/* Obtained from lcc-win32 include files */
#define GDI_ERROR			0xffffffff


/* registry errors */
#define REGDB_E_READREGDB               0x80040150
#define REGDB_E_CLASSNOTREG             0x80040154

#endif  /* __WINE_WINERROR_H */
