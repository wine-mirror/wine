/*
 * NTDLL error handling
 * 
 * Copyright 2000 Alexandre Julliard
 */

#include "config.h"
#include "ntddk.h"
#include "winerror.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ntdll);

struct error_table
{
    DWORD       start;
    DWORD       end;
    const WORD *table;
};

static const struct error_table error_table[];

/**************************************************************************
 *           RtlNtStatusToDosError
 */
DWORD WINAPI RtlNtStatusToDosError( DWORD status )
{
    const struct error_table *table = error_table;

    if (!status || (status & 0x20000000)) return status;

    /* 0xd... is equivalent to 0xc... */
    if ((status & 0xf0000000) == 0xd0000000) status &= ~0x10000000;

    while (table->start)
    {
        if (status < table->start) break;
        if (status < table->end)
        {
            DWORD ret = table->table[status - table->start];
            if (!ret) ret = status;  /* 0 means 1:1 mapping */
            else if (ret == ERROR_MR_MID_NOT_FOUND) FIXME( "no mapping for %08lx\n", status );
            return ret;
        }
        table++;
    }

    /* now some special cases */
    if (HIWORD(status) == 0xc001) return LOWORD(status);
    if (HIWORD(status) == 0x8007) return LOWORD(status);
    FIXME( "no mapping for %08lx\n", status );
    return ERROR_MR_MID_NOT_FOUND;
}


/* conversion tables */

static const WORD table_00000103[11] =
{
   ERROR_IO_PENDING,                       /* 00000103 (STATUS_PENDING) */
   ERROR_MR_MID_NOT_FOUND,                 /* 00000104 */
   ERROR_MORE_DATA,                        /* 00000105 */
   ERROR_NOT_ALL_ASSIGNED,                 /* 00000106 */
   ERROR_SOME_NOT_MAPPED,                  /* 00000107 */
   ERROR_MR_MID_NOT_FOUND,                 /* 00000108 */
   ERROR_MR_MID_NOT_FOUND,                 /* 00000109 */
   ERROR_MR_MID_NOT_FOUND,                 /* 0000010a */
   ERROR_MR_MID_NOT_FOUND,                 /* 0000010b */
   ERROR_NOTIFY_ENUM_DIR,                  /* 0000010c */
   ERROR_NO_QUOTAS_FOR_ACCOUNT             /* 0000010d */
};

static const WORD table_40000002[12] =
{
   ERROR_INVALID_PARAMETER,                /* 40000002 */
   ERROR_MR_MID_NOT_FOUND,                 /* 40000003 */
   ERROR_MR_MID_NOT_FOUND,                 /* 40000004 */
   ERROR_MR_MID_NOT_FOUND,                 /* 40000005 */
   ERROR_LOCAL_USER_SESSION_KEY,           /* 40000006 */
   ERROR_MR_MID_NOT_FOUND,                 /* 40000007 */
   ERROR_MORE_WRITES,                      /* 40000008 */
   ERROR_REGISTRY_RECOVERED,               /* 40000009 */
   ERROR_MR_MID_NOT_FOUND,                 /* 4000000a */
   ERROR_MR_MID_NOT_FOUND,                 /* 4000000b */
   ERROR_COUNTER_TIMEOUT,                  /* 4000000c */
   ERROR_NULL_LM_PASSWORD                  /* 4000000d */
};

static const WORD table_40020056[1] =
{
   RPC_S_UUID_LOCAL_ONLY                   /* 40020056 */
};

static const WORD table_400200af[1] =
{
   RPC_S_SEND_INCOMPLETE                   /* 400200af */
};

static const WORD table_80000001[37] =
{
   0,                                      /* 80000001 (STATUS_GUARD_PAGE_VIOLATION) */
   ERROR_NOACCESS,                         /* 80000002 (STATUS_DATATYPE_MISALIGNMENT) */
   0,                                      /* 80000003 (STATUS_BREAKPOINT) */
   0,                                      /* 80000004 (STATUS_SINGLE_STEP) */
   ERROR_MORE_DATA,                        /* 80000005 (STATUS_BUFFER_OVERFLOW) */
   ERROR_NO_MORE_FILES,                    /* 80000006 (STATUS_NO_MORE_FILES) */
   ERROR_MR_MID_NOT_FOUND,                 /* 80000007 (STATUS_WAKE_SYSTEM_DEBUGGER) */
   ERROR_MR_MID_NOT_FOUND,                 /* 80000008 */
   ERROR_MR_MID_NOT_FOUND,                 /* 80000009 */
   ERROR_MR_MID_NOT_FOUND,                 /* 8000000a (STATUS_HANDLES_CLOSED) */
   ERROR_NO_INHERITANCE,                   /* 8000000b (STATUS_NO_INHERITANCE) */
   ERROR_MR_MID_NOT_FOUND,                 /* 8000000c (STATUS_GUID_SUBSTITUTION_MADE) */
   ERROR_PARTIAL_COPY,                     /* 8000000d (STATUS_PARTIAL_COPY) */
   ERROR_OUT_OF_PAPER,                     /* 8000000e (STATUS_DEVICE_PAPER_EMPTY) */
   ERROR_NOT_READY,                        /* 8000000f (STATUS_DEVICE_POWERED_OFF) */
   ERROR_NOT_READY,                        /* 80000010 (STATUS_DEVICE_OFF_LINE) */
   ERROR_BUSY,                             /* 80000011 (STATUS_DEVICE_BUSY) */
   ERROR_NO_MORE_ITEMS,                    /* 80000012 (STATUS_NO_MORE_EAS) */
   ERROR_INVALID_EA_NAME,                  /* 80000013 (STATUS_INVALID_EA_NAME) */
   ERROR_EA_LIST_INCONSISTENT,             /* 80000014 (STATUS_EA_LIST_INCONSISTENT) */
   ERROR_EA_LIST_INCONSISTENT,             /* 80000015 (STATUS_INVALID_EA_FLAG) */
   ERROR_MEDIA_CHANGED,                    /* 80000016 (STATUS_VERIFY_REQUIRED) */
   ERROR_MR_MID_NOT_FOUND,                 /* 80000017 (STATUS_EXTRANEOUS_INFORMATION) */
   ERROR_MR_MID_NOT_FOUND,                 /* 80000018 (STATUS_RXACT_COMMIT_NECESSARY) */
   ERROR_MR_MID_NOT_FOUND,                 /* 80000019 */
   ERROR_NO_MORE_ITEMS,                    /* 8000001a (STATUS_NO_MORE_ENTRIES) */
   ERROR_FILEMARK_DETECTED,                /* 8000001b (STATUS_FILEMARK_DETECTED) */
   ERROR_MEDIA_CHANGED,                    /* 8000001c (STATUS_MEDIA_CHANGED) */
   ERROR_BUS_RESET,                        /* 8000001d (STATUS_BUS_RESET) */
   ERROR_END_OF_MEDIA,                     /* 8000001e (STATUS_END_OF_MEDIA) */
   ERROR_BEGINNING_OF_MEDIA,               /* 8000001f (STATUS_BEGINNING_OF_MEDIA) */
   ERROR_MR_MID_NOT_FOUND,                 /* 80000020 (STATUS_MEDIA_CHECK) */
   ERROR_SETMARK_DETECTED,                 /* 80000021 (STATUS_SETMARK_DETECTED) */
   ERROR_NO_DATA_DETECTED,                 /* 80000022 (STATUS_NO_DATA_DETECTED) */
   ERROR_MR_MID_NOT_FOUND,                 /* 80000023 (STATUS_REDIRECTOR_HAS_OPEN_HANDLES) */
   ERROR_MR_MID_NOT_FOUND,                 /* 80000024 (STATUS_SERVER_HAS_OPEN_HANDLES) */
   ERROR_ACTIVE_CONNECTIONS                /* 80000025 (STATUS_ALREADY_DISCONNECTED) */
};

static const WORD table_80090300[23] =
{
   ERROR_NO_SYSTEM_RESOURCES,              /* 80090300 */
   ERROR_INVALID_HANDLE,                   /* 80090301 */
   ERROR_INVALID_FUNCTION,                 /* 80090302 */
   ERROR_BAD_NETPATH,                      /* 80090303 */
   ERROR_INTERNAL_ERROR,                   /* 80090304 */
   ERROR_NO_SUCH_PACKAGE,                  /* 80090305 */
   ERROR_NOT_OWNER,                        /* 80090306 */
   ERROR_NO_SUCH_PACKAGE,                  /* 80090307 */
   ERROR_INVALID_PARAMETER,                /* 80090308 */
   ERROR_INVALID_PARAMETER,                /* 80090309 */
   ERROR_NOT_SUPPORTED,                    /* 8009030a */
   ERROR_CANNOT_IMPERSONATE,               /* 8009030b */
   ERROR_LOGON_FAILURE,                    /* 8009030c */
   ERROR_INVALID_PARAMETER,                /* 8009030d */
   ERROR_NO_SUCH_LOGON_SESSION,            /* 8009030e */
   ERROR_ACCESS_DENIED,                    /* 8009030f */
   ERROR_ACCESS_DENIED,                    /* 80090310 */
   ERROR_NO_LOGON_SERVERS,                 /* 80090311 */
   ERROR_MR_MID_NOT_FOUND,                 /* 80090312 */
   ERROR_MR_MID_NOT_FOUND,                 /* 80090313 */
   ERROR_MR_MID_NOT_FOUND,                 /* 80090314 */
   ERROR_MR_MID_NOT_FOUND,                 /* 80090315 */
   ERROR_NO_SUCH_PACKAGE                   /* 80090316 */
};

static const WORD table_c0000001[411] =
{
   ERROR_GEN_FAILURE,                      /* c0000001 (STATUS_UNSUCCESSFUL) */
   ERROR_INVALID_FUNCTION,                 /* c0000002 (STATUS_NOT_IMPLEMENTED) */
   ERROR_INVALID_PARAMETER,                /* c0000003 (STATUS_INVALID_INFO_CLASS) */
   ERROR_BAD_LENGTH,                       /* c0000004 (STATUS_INFO_LENGTH_MISMATCH) */
   ERROR_NOACCESS,                         /* c0000005 (STATUS_ACCESS_VIOLATION) */
   ERROR_SWAPERROR,                        /* c0000006 (STATUS_IN_PAGE_ERROR) */
   ERROR_PAGEFILE_QUOTA,                   /* c0000007 (STATUS_PAGEFILE_QUOTA) */
   ERROR_INVALID_HANDLE,                   /* c0000008 (STATUS_INVALID_HANDLE) */
   ERROR_STACK_OVERFLOW,                   /* c0000009 (STATUS_BAD_INITIAL_STACK) */
   ERROR_BAD_EXE_FORMAT,                   /* c000000a (STATUS_BAD_INITIAL_PC) */
   ERROR_INVALID_PARAMETER,                /* c000000b (STATUS_INVALID_CID) */
   ERROR_MR_MID_NOT_FOUND,                 /* c000000c (STATUS_TIMER_NOT_CANCELED) */
   ERROR_INVALID_PARAMETER,                /* c000000d (STATUS_INVALID_PARAMETER) */
   ERROR_FILE_NOT_FOUND,                   /* c000000e (STATUS_NO_SUCH_DEVICE) */
   ERROR_FILE_NOT_FOUND,                   /* c000000f (STATUS_NO_SUCH_FILE) */
   ERROR_INVALID_FUNCTION,                 /* c0000010 (STATUS_INVALID_DEVICE_REQUEST) */
   ERROR_HANDLE_EOF,                       /* c0000011 (STATUS_END_OF_FILE) */
   ERROR_WRONG_DISK,                       /* c0000012 (STATUS_WRONG_VOLUME) */
   ERROR_NOT_READY,                        /* c0000013 (STATUS_NO_MEDIA_IN_DEVICE) */
   ERROR_UNRECOGNIZED_MEDIA,               /* c0000014 (STATUS_UNRECOGNIZED_MEDIA) */
   ERROR_SECTOR_NOT_FOUND,                 /* c0000015 (STATUS_NONEXISTENT_SECTOR) */
   ERROR_MORE_DATA,                        /* c0000016 (STATUS_MORE_PROCESSING_REQUIRED) */
   ERROR_NOT_ENOUGH_MEMORY,                /* c0000017 (STATUS_NO_MEMORY) */
   ERROR_INVALID_ADDRESS,                  /* c0000018 (STATUS_CONFLICTING_ADDRESSES) */
   ERROR_INVALID_ADDRESS,                  /* c0000019 (STATUS_NOT_MAPPED_VIEW) */
   ERROR_INVALID_PARAMETER,                /* c000001a (STATUS_UNABLE_TO_FREE_VM) */
   ERROR_INVALID_PARAMETER,                /* c000001b (STATUS_UNABLE_TO_DELETE_SECTION) */
   ERROR_INVALID_FUNCTION,                 /* c000001c (STATUS_INVALID_SYSTEM_SERVICE) */
   0,                                      /* c000001d (STATUS_ILLEGAL_INSTRUCTION) */
   ERROR_ACCESS_DENIED,                    /* c000001e (STATUS_INVALID_LOCK_SEQUENCE) */
   ERROR_ACCESS_DENIED,                    /* c000001f (STATUS_INVALID_VIEW_SIZE) */
   ERROR_BAD_EXE_FORMAT,                   /* c0000020 (STATUS_INVALID_FILE_FOR_SECTION) */
   ERROR_ACCESS_DENIED,                    /* c0000021 (STATUS_ALREADY_COMMITTED) */
   ERROR_ACCESS_DENIED,                    /* c0000022 (STATUS_ACCESS_DENIED) */
   ERROR_INSUFFICIENT_BUFFER,              /* c0000023 (STATUS_BUFFER_TOO_SMALL) */
   ERROR_INVALID_HANDLE,                   /* c0000024 (STATUS_OBJECT_TYPE_MISMATCH) */
   0,                                      /* c0000025 (STATUS_NONCONTINUABLE_EXCEPTION) */
   0,                                      /* c0000026 (STATUS_INVALID_DISPOSITION) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000027 (STATUS_UNWIND) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000028 (STATUS_BAD_STACK) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000029 (STATUS_INVALID_UNWIND_TARGET) */
   ERROR_NOT_LOCKED,                       /* c000002a (STATUS_NOT_LOCKED) */
   0,                                      /* c000002b (STATUS_PARITY_ERROR) */
   ERROR_INVALID_ADDRESS,                  /* c000002c (STATUS_UNABLE_TO_DECOMMIT_VM) */
   ERROR_INVALID_ADDRESS,                  /* c000002d (STATUS_NOT_COMMITTED) */
   ERROR_MR_MID_NOT_FOUND,                 /* c000002e (STATUS_INVALID_PORT_ATTRIBUTES) */
   ERROR_MR_MID_NOT_FOUND,                 /* c000002f (STATUS_PORT_MESSAGE_TOO_LONG) */
   ERROR_INVALID_PARAMETER,                /* c0000030 (STATUS_INVALID_PARAMETER_MIX) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000031 (STATUS_INVALID_QUOTA_LOWER) */
   ERROR_DISK_CORRUPT,                     /* c0000032 (STATUS_DISK_CORRUPT_ERROR) */
   ERROR_INVALID_NAME,                     /* c0000033 (STATUS_OBJECT_NAME_INVALID) */
   ERROR_FILE_NOT_FOUND,                   /* c0000034 (STATUS_OBJECT_NAME_NOT_FOUND) */
   ERROR_ALREADY_EXISTS,                   /* c0000035 (STATUS_OBJECT_NAME_COLLISION) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000036 */
   ERROR_INVALID_HANDLE,                   /* c0000037 (STATUS_PORT_DISCONNECTED) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000038 (STATUS_DEVICE_ALREADY_ATTACHED) */
   ERROR_BAD_PATHNAME,                     /* c0000039 (STATUS_OBJECT_PATH_INVALID) */
   ERROR_PATH_NOT_FOUND,                   /* c000003a (STATUS_OBJECT_PATH_NOT_FOUND) */
   ERROR_BAD_PATHNAME,                     /* c000003b (STATUS_PATH_SYNTAX_BAD) */
   ERROR_IO_DEVICE,                        /* c000003c (STATUS_DATA_OVERRUN) */
   ERROR_IO_DEVICE,                        /* c000003d (STATUS_DATA_LATE_ERROR) */
   ERROR_CRC,                              /* c000003e (STATUS_DATA_ERROR) */
   ERROR_CRC,                              /* c000003f (STATUS_CRC_ERROR) */
   ERROR_NOT_ENOUGH_MEMORY,                /* c0000040 (STATUS_SECTION_TOO_BIG) */
   ERROR_ACCESS_DENIED,                    /* c0000041 (STATUS_PORT_CONNECTION_REFUSED) */
   ERROR_INVALID_HANDLE,                   /* c0000042 (STATUS_INVALID_PORT_HANDLE) */
   ERROR_SHARING_VIOLATION,                /* c0000043 (STATUS_SHARING_VIOLATION) */
   ERROR_NOT_ENOUGH_QUOTA,                 /* c0000044 (STATUS_QUOTA_EXCEEDED) */
   ERROR_INVALID_PARAMETER,                /* c0000045 (STATUS_INVALID_PAGE_PROTECTION) */
   ERROR_NOT_OWNER,                        /* c0000046 (STATUS_MUTANT_NOT_OWNED) */
   ERROR_TOO_MANY_POSTS,                   /* c0000047 (STATUS_SEMAPHORE_LIMIT_EXCEEDED) */
   ERROR_INVALID_PARAMETER,                /* c0000048 (STATUS_PORT_ALREADY_SET) */
   ERROR_INVALID_PARAMETER,                /* c0000049 */
   ERROR_SIGNAL_REFUSED,                   /* c000004a (STATUS_SUSPEND_COUNT_EXCEEDED) */
   ERROR_ACCESS_DENIED,                    /* c000004b */
   ERROR_INVALID_PARAMETER,                /* c000004c */
   ERROR_INVALID_PARAMETER,                /* c000004d */
   ERROR_INVALID_PARAMETER,                /* c000004e */
   ERROR_MR_MID_NOT_FOUND,                 /* c000004f */
   ERROR_EA_LIST_INCONSISTENT,             /* c0000050 */
   ERROR_FILE_CORRUPT,                     /* c0000051 */
   ERROR_FILE_CORRUPT,                     /* c0000052 */
   ERROR_FILE_CORRUPT,                     /* c0000053 */
   ERROR_LOCK_VIOLATION,                   /* c0000054 (STATUS_LOCK_NOT_GRANTED) */
   ERROR_LOCK_VIOLATION,                   /* c0000055 (STATUS_FILE_LOCK_CONFLICT) */
   ERROR_ACCESS_DENIED,                    /* c0000056 */
   ERROR_NOT_SUPPORTED,                    /* c0000057 */
   ERROR_UNKNOWN_REVISION,                 /* c0000058 (STATUS_UNKNOWN_REVISION) */
   ERROR_REVISION_MISMATCH,                /* c0000059 */
   ERROR_INVALID_OWNER,                    /* c000005a */
   ERROR_INVALID_PRIMARY_GROUP,            /* c000005b */
   ERROR_NO_IMPERSONATION_TOKEN,           /* c000005c */
   ERROR_CANT_DISABLE_MANDATORY,           /* c000005d */
   ERROR_NO_LOGON_SERVERS,                 /* c000005e */
   ERROR_NO_SUCH_LOGON_SESSION,            /* c000005f */
   ERROR_NO_SUCH_PRIVILEGE,                /* c0000060 */
   ERROR_PRIVILEGE_NOT_HELD,               /* c0000061 */
   ERROR_INVALID_ACCOUNT_NAME,             /* c0000062 */
   ERROR_USER_EXISTS,                      /* c0000063 */
   ERROR_NO_SUCH_USER,                     /* c0000064 */
   ERROR_GROUP_EXISTS,                     /* c0000065 */
   ERROR_NO_SUCH_GROUP,                    /* c0000066 */
   ERROR_MEMBER_IN_GROUP,                  /* c0000067 */
   ERROR_MEMBER_NOT_IN_GROUP,              /* c0000068 */
   ERROR_LAST_ADMIN,                       /* c0000069 */
   ERROR_INVALID_PASSWORD,                 /* c000006a */
   ERROR_ILL_FORMED_PASSWORD,              /* c000006b */
   ERROR_PASSWORD_RESTRICTION,             /* c000006c */
   ERROR_LOGON_FAILURE,                    /* c000006d */
   ERROR_ACCOUNT_RESTRICTION,              /* c000006e */
   ERROR_INVALID_LOGON_HOURS,              /* c000006f */
   ERROR_INVALID_WORKSTATION,              /* c0000070 */
   ERROR_PASSWORD_EXPIRED,                 /* c0000071 */
   ERROR_ACCOUNT_DISABLED,                 /* c0000072 */
   ERROR_NONE_MAPPED,                      /* c0000073 */
   ERROR_TOO_MANY_LUIDS_REQUESTED,         /* c0000074 */
   ERROR_LUIDS_EXHAUSTED,                  /* c0000075 */
   ERROR_INVALID_SUB_AUTHORITY,            /* c0000076 */
   ERROR_INVALID_ACL,                      /* c0000077 */
   ERROR_INVALID_SID,                      /* c0000078 */
   ERROR_INVALID_SECURITY_DESCR,           /* c0000079 (STATUS_INVALID_SECURITY_DESCR) */
   ERROR_PROC_NOT_FOUND,                   /* c000007a */
   ERROR_BAD_EXE_FORMAT,                   /* c000007b */
   ERROR_NO_TOKEN,                         /* c000007c */
   ERROR_BAD_INHERITANCE_ACL,              /* c000007d */
   ERROR_NOT_LOCKED,                       /* c000007e */
   ERROR_DISK_FULL,                        /* c000007f (STATUS_DISK_FULL) */
   ERROR_SERVER_DISABLED,                  /* c0000080 */
   ERROR_SERVER_NOT_DISABLED,              /* c0000081 */
   ERROR_TOO_MANY_NAMES,                   /* c0000082 */
   ERROR_NO_MORE_ITEMS,                    /* c0000083 */
   ERROR_INVALID_ID_AUTHORITY,             /* c0000084 */
   ERROR_NO_MORE_ITEMS,                    /* c0000085 */
   ERROR_LABEL_TOO_LONG,                   /* c0000086 */
   ERROR_OUTOFMEMORY,                      /* c0000087 (STATUS_SECTION_NOT_EXTENDED) */
   ERROR_INVALID_ADDRESS,                  /* c0000088 */
   ERROR_RESOURCE_DATA_NOT_FOUND,          /* c0000089 */
   ERROR_RESOURCE_TYPE_NOT_FOUND,          /* c000008a */
   ERROR_RESOURCE_NAME_NOT_FOUND,          /* c000008b */
   0,                                      /* c000008c (STATUS_ARRAY_BOUNDS_EXCEEDED) */
   0,                                      /* c000008d (STATUS_FLOAT_DENORMAL_OPERAND) */
   0,                                      /* c000008e (STATUS_FLOAT_DIVIDE_BY_ZERO) */
   0,                                      /* c000008f (STATUS_FLOAT_INEXACT_RESULT) */
   0,                                      /* c0000090 (STATUS_FLOAT_INVALID_OPERATION) */
   0,                                      /* c0000091 (STATUS_FLOAT_OVERFLOW) */
   0,                                      /* c0000092 (STATUS_FLOAT_STACK_CHECK) */
   0,                                      /* c0000093 (STATUS_FLOAT_UNDERFLOW) */
   0,                                      /* c0000094 (STATUS_INTEGER_DIVIDE_BY_ZERO) */
   ERROR_ARITHMETIC_OVERFLOW,              /* c0000095 (STATUS_INTEGER_OVERFLOW) */
   0,                                      /* c0000096 (STATUS_PRIVILEGED_INSTRUCTION) */
   ERROR_NOT_ENOUGH_MEMORY,                /* c0000097 */
   ERROR_FILE_INVALID,                     /* c0000098 */
   ERROR_ALLOTTED_SPACE_EXCEEDED,          /* c0000099 */
   ERROR_NO_SYSTEM_RESOURCES,              /* c000009a */
   ERROR_PATH_NOT_FOUND,                   /* c000009b */
   ERROR_CRC,                              /* c000009c */
   ERROR_NOT_READY,                        /* c000009d */
   ERROR_NOT_READY,                        /* c000009e */
   ERROR_INVALID_ADDRESS,                  /* c000009f */
   ERROR_INVALID_ADDRESS,                  /* c00000a0 */
   ERROR_WORKING_SET_QUOTA,                /* c00000a1 */
   ERROR_WRITE_PROTECT,                    /* c00000a2 */
   ERROR_NOT_READY,                        /* c00000a3 */
   ERROR_INVALID_GROUP_ATTRIBUTES,         /* c00000a4 */
   ERROR_BAD_IMPERSONATION_LEVEL,          /* c00000a5 */
   ERROR_CANT_OPEN_ANONYMOUS,              /* c00000a6 */
   ERROR_BAD_VALIDATION_CLASS,             /* c00000a7 */
   ERROR_BAD_TOKEN_TYPE,                   /* c00000a8 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000a9 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000aa */
   ERROR_PIPE_BUSY,                        /* c00000ab */
   ERROR_PIPE_BUSY,                        /* c00000ac */
   ERROR_BAD_PIPE,                         /* c00000ad */
   ERROR_PIPE_BUSY,                        /* c00000ae */
   ERROR_INVALID_FUNCTION,                 /* c00000af */
   ERROR_PIPE_NOT_CONNECTED,               /* c00000b0 */
   ERROR_NO_DATA,                          /* c00000b1 */
   ERROR_PIPE_CONNECTED,                   /* c00000b2 */
   ERROR_PIPE_LISTENING,                   /* c00000b3 */
   ERROR_BAD_PIPE,                         /* c00000b4 */
   ERROR_SEM_TIMEOUT,                      /* c00000b5 */
   ERROR_HANDLE_EOF,                       /* c00000b6 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000b7 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000b8 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000b9 */
   ERROR_ACCESS_DENIED,                    /* c00000ba */
   ERROR_NOT_SUPPORTED,                    /* c00000bb */
   ERROR_REM_NOT_LIST,                     /* c00000bc */
   ERROR_DUP_NAME,                         /* c00000bd */
   ERROR_BAD_NETPATH,                      /* c00000be */
   ERROR_NETWORK_BUSY,                     /* c00000bf */
   ERROR_DEV_NOT_EXIST,                    /* c00000c0 */
   ERROR_TOO_MANY_CMDS,                    /* c00000c1 */
   ERROR_ADAP_HDW_ERR,                     /* c00000c2 */
   ERROR_BAD_NET_RESP,                     /* c00000c3 */
   ERROR_UNEXP_NET_ERR,                    /* c00000c4 */
   ERROR_BAD_REM_ADAP,                     /* c00000c5 */
   ERROR_PRINTQ_FULL,                      /* c00000c6 */
   ERROR_NO_SPOOL_SPACE,                   /* c00000c7 */
   ERROR_PRINT_CANCELLED,                  /* c00000c8 */
   ERROR_NETNAME_DELETED,                  /* c00000c9 */
   ERROR_NETWORK_ACCESS_DENIED,            /* c00000ca */
   ERROR_BAD_DEV_TYPE,                     /* c00000cb */
   ERROR_BAD_NET_NAME,                     /* c00000cc */
   ERROR_TOO_MANY_NAMES,                   /* c00000cd */
   ERROR_TOO_MANY_SESS,                    /* c00000ce */
   ERROR_SHARING_PAUSED,                   /* c00000cf */
   ERROR_REQ_NOT_ACCEP,                    /* c00000d0 */
   ERROR_REDIR_PAUSED,                     /* c00000d1 */
   ERROR_NET_WRITE_FAULT,                  /* c00000d2 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000d3 */
   ERROR_NOT_SAME_DEVICE,                  /* c00000d4 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000d5 */
   ERROR_VC_DISCONNECTED,                  /* c00000d6 */
   ERROR_NO_SECURITY_ON_OBJECT,            /* c00000d7 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000d8 */
   ERROR_NO_DATA,                          /* c00000d9 */
   ERROR_CANT_ACCESS_DOMAIN_INFO,          /* c00000da */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000db */
   ERROR_INVALID_SERVER_STATE,             /* c00000dc */
   ERROR_INVALID_DOMAIN_STATE,             /* c00000dd */
   ERROR_INVALID_DOMAIN_ROLE,              /* c00000de */
   ERROR_NO_SUCH_DOMAIN,                   /* c00000df */
   ERROR_DOMAIN_EXISTS,                    /* c00000e0 */
   ERROR_DOMAIN_LIMIT_EXCEEDED,            /* c00000e1 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000e2 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000e3 */
   ERROR_INTERNAL_DB_CORRUPTION,           /* c00000e4 */
   ERROR_INTERNAL_ERROR,                   /* c00000e5 */
   ERROR_GENERIC_NOT_MAPPED,               /* c00000e6 */
   ERROR_BAD_DESCRIPTOR_FORMAT,            /* c00000e7 */
   ERROR_INVALID_USER_BUFFER,              /* c00000e8 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000e9 */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000ea */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000eb */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000ec */
   ERROR_NOT_LOGON_PROCESS,                /* c00000ed */
   ERROR_LOGON_SESSION_EXISTS,             /* c00000ee */
   ERROR_INVALID_PARAMETER,                /* c00000ef */
   ERROR_INVALID_PARAMETER,                /* c00000f0 (STATUS_INVALID_PARAMETER_2) */
   ERROR_INVALID_PARAMETER,                /* c00000f1 */
   ERROR_INVALID_PARAMETER,                /* c00000f2 */
   ERROR_INVALID_PARAMETER,                /* c00000f3 */
   ERROR_INVALID_PARAMETER,                /* c00000f4 */
   ERROR_INVALID_PARAMETER,                /* c00000f5 */
   ERROR_INVALID_PARAMETER,                /* c00000f6 */
   ERROR_INVALID_PARAMETER,                /* c00000f7 */
   ERROR_INVALID_PARAMETER,                /* c00000f8 */
   ERROR_INVALID_PARAMETER,                /* c00000f9 */
   ERROR_INVALID_PARAMETER,                /* c00000fa */
   ERROR_PATH_NOT_FOUND,                   /* c00000fb */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000fc */
   ERROR_STACK_OVERFLOW,                   /* c00000fd (STATUS_STACK_OVERFLOW) */
   ERROR_NO_SUCH_PACKAGE,                  /* c00000fe */
   ERROR_MR_MID_NOT_FOUND,                 /* c00000ff */
   ERROR_ENVVAR_NOT_FOUND,                 /* c0000100 */
   ERROR_DIR_NOT_EMPTY,                    /* c0000101 (STATUS_DIRECTORY_NOT_EMPTY) */
   ERROR_FILE_CORRUPT,                     /* c0000102 */
   ERROR_DIRECTORY,                        /* c0000103 */
   ERROR_BAD_LOGON_SESSION_STATE,          /* c0000104 */
   ERROR_LOGON_SESSION_COLLISION,          /* c0000105 */
   ERROR_FILENAME_EXCED_RANGE,             /* c0000106 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000107 */
   ERROR_DEVICE_IN_USE,                    /* c0000108 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000109 */
   ERROR_ACCESS_DENIED,                    /* c000010a */
   ERROR_INVALID_LOGON_TYPE,               /* c000010b */
   ERROR_MR_MID_NOT_FOUND,                 /* c000010c */
   ERROR_CANNOT_IMPERSONATE,               /* c000010d */
   ERROR_SERVICE_ALREADY_RUNNING,          /* c000010e */
   ERROR_MR_MID_NOT_FOUND,                 /* c000010f */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000110 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000111 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000112 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000113 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000114 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000115 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000116 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000117 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000118 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000119 */
   ERROR_MR_MID_NOT_FOUND,                 /* c000011a */
   ERROR_BAD_EXE_FORMAT,                   /* c000011b */
   ERROR_RXACT_INVALID_STATE,              /* c000011c */
   ERROR_RXACT_COMMIT_FAILURE,             /* c000011d */
   ERROR_FILE_INVALID,                     /* c000011e */
   ERROR_TOO_MANY_OPEN_FILES,              /* c000011f (STATUS_TOO_MANY_OPENED_FILES) */
   ERROR_OPERATION_ABORTED,                /* c0000120 */
   ERROR_ACCESS_DENIED,                    /* c0000121 */
   ERROR_INVALID_COMPUTERNAME,             /* c0000122 */
   ERROR_ACCESS_DENIED,                    /* c0000123 */
   ERROR_SPECIAL_ACCOUNT,                  /* c0000124 */
   ERROR_SPECIAL_GROUP,                    /* c0000125 */
   ERROR_SPECIAL_USER,                     /* c0000126 */
   ERROR_MEMBERS_PRIMARY_GROUP,            /* c0000127 */
   ERROR_INVALID_HANDLE,                   /* c0000128 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000129 */
   ERROR_MR_MID_NOT_FOUND,                 /* c000012a */
   ERROR_TOKEN_ALREADY_IN_USE,             /* c000012b */
   ERROR_MR_MID_NOT_FOUND,                 /* c000012c */
   ERROR_COMMITMENT_LIMIT,                 /* c000012d */
   ERROR_BAD_EXE_FORMAT,                   /* c000012e */
   ERROR_BAD_EXE_FORMAT,                   /* c000012f */
   ERROR_BAD_EXE_FORMAT,                   /* c0000130 */
   ERROR_BAD_EXE_FORMAT,                   /* c0000131 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000132 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000133 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000134 */
   ERROR_MOD_NOT_FOUND,                    /* c0000135 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000136 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000137 */
   ERROR_INVALID_ORDINAL,                  /* c0000138 */
   ERROR_PROC_NOT_FOUND,                   /* c0000139 */
   ERROR_MR_MID_NOT_FOUND,                 /* c000013a (STATUS_CONTROL_C_EXIT) */
   ERROR_NETNAME_DELETED,                  /* c000013b */
   ERROR_NETNAME_DELETED,                  /* c000013c */
   ERROR_REM_NOT_LIST,                     /* c000013d */
   ERROR_UNEXP_NET_ERR,                    /* c000013e */
   ERROR_UNEXP_NET_ERR,                    /* c000013f */
   ERROR_UNEXP_NET_ERR,                    /* c0000140 */
   ERROR_UNEXP_NET_ERR,                    /* c0000141 */
   ERROR_DLL_INIT_FAILED,                  /* c0000142 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000143 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000144 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000145 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000146 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000147 */
   ERROR_INVALID_LEVEL,                    /* c0000148 */
   ERROR_INVALID_PASSWORD,                 /* c0000149 */
   ERROR_MR_MID_NOT_FOUND,                 /* c000014a */
   ERROR_BROKEN_PIPE,                      /* c000014b (STATUS_PIPE_BROKEN) */
   ERROR_BADDB,                            /* c000014c */
   ERROR_REGISTRY_IO_FAILED,               /* c000014d */
   ERROR_MR_MID_NOT_FOUND,                 /* c000014e */
   ERROR_UNRECOGNIZED_VOLUME,              /* c000014f */
   ERROR_SERIAL_NO_DEVICE,                 /* c0000150 */
   ERROR_NO_SUCH_ALIAS,                    /* c0000151 */
   ERROR_MEMBER_NOT_IN_ALIAS,              /* c0000152 */
   ERROR_MEMBER_IN_ALIAS,                  /* c0000153 */
   ERROR_ALIAS_EXISTS,                     /* c0000154 */
   ERROR_LOGON_NOT_GRANTED,                /* c0000155 */
   ERROR_TOO_MANY_SECRETS,                 /* c0000156 */
   ERROR_SECRET_TOO_LONG,                  /* c0000157 */
   ERROR_INTERNAL_DB_ERROR,                /* c0000158 */
   ERROR_FULLSCREEN_MODE,                  /* c0000159 */
   ERROR_TOO_MANY_CONTEXT_IDS,             /* c000015a */
   ERROR_LOGON_TYPE_NOT_GRANTED,           /* c000015b */
   ERROR_NOT_REGISTRY_FILE,                /* c000015c (STATUS_NOT_REGISTRY_FILE) */
   ERROR_NT_CROSS_ENCRYPTION_REQUIRED,     /* c000015d */
   ERROR_MR_MID_NOT_FOUND,                 /* c000015e */
   ERROR_IO_DEVICE,                        /* c000015f */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000160 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000161 */
   ERROR_NO_UNICODE_TRANSLATION,           /* c0000162 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000163 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000164 */
   ERROR_FLOPPY_ID_MARK_NOT_FOUND,         /* c0000165 */
   ERROR_FLOPPY_WRONG_CYLINDER,            /* c0000166 */
   ERROR_FLOPPY_UNKNOWN_ERROR,             /* c0000167 */
   ERROR_FLOPPY_BAD_REGISTERS,             /* c0000168 */
   ERROR_DISK_RECALIBRATE_FAILED,          /* c0000169 */
   ERROR_DISK_OPERATION_FAILED,            /* c000016a */
   ERROR_DISK_RESET_FAILED,                /* c000016b */
   ERROR_IRQ_BUSY,                         /* c000016c */
   ERROR_IO_DEVICE,                        /* c000016d */
   ERROR_MR_MID_NOT_FOUND,                 /* c000016e */
   ERROR_MR_MID_NOT_FOUND,                 /* c000016f */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000170 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000171 */
   ERROR_PARTITION_FAILURE,                /* c0000172 (STATUS_PARTITION_FAILURE) */
   ERROR_INVALID_BLOCK_LENGTH,             /* c0000173 (STATUS_INVALID_BLOCK_LENGTH) */
   ERROR_DEVICE_NOT_PARTITIONED,           /* c0000174 (STATUS_DEVICE_NOT_PARTITIONED) */
   ERROR_UNABLE_TO_LOCK_MEDIA,             /* c0000175 (STATUS_UNABLE_TO_LOCK_MEDIA) */
   ERROR_UNABLE_TO_UNLOAD_MEDIA,           /* c0000176 (STATUS_UNABLE_TO_UNLOAD_MEDIA) */
   ERROR_EOM_OVERFLOW,                     /* c0000177 (STATUS_EOM_OVERFLOW) */
   ERROR_NO_MEDIA_IN_DRIVE,                /* c0000178 (STATUS_NO_MEDIA) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000179 */
   ERROR_NO_SUCH_MEMBER,                   /* c000017a (STATUS_NO_SUCH_MEMBER) */
   ERROR_INVALID_MEMBER,                   /* c000017b (STATUS_INVALID_MEMBER) */
   ERROR_KEY_DELETED,                      /* c000017c (STATUS_KEY_DELETED) */
   ERROR_NO_LOG_SPACE,                     /* c000017d (STATUS_NO_LOG_SPACE) */
   ERROR_TOO_MANY_SIDS,                    /* c000017e (STATUS_TOO_MANY_SIDS) */
   ERROR_LM_CROSS_ENCRYPTION_REQUIRED,     /* c000017f (STATUS_LM_CROSS_ENCRYPTION_REQUIRED) */
   ERROR_KEY_HAS_CHILDREN,                 /* c0000180 (STATUS_KEY_HAS_CHILDREN) */
   ERROR_CHILD_MUST_BE_VOLATILE,           /* c0000181 (STATUS_CHILD_MUST_BE_VOLATILE) */
   ERROR_INVALID_PARAMETER,                /* c0000182 */
   ERROR_IO_DEVICE,                        /* c0000183 (STATUS_DRIVER_INTERNAL_ERROR) */
   ERROR_BAD_COMMAND,                      /* c0000184 (STATUS_INVALID_DEVICE_STATE) */
   ERROR_IO_DEVICE,                        /* c0000185 (STATUS_IO_DEVICE_ERROR) */
   ERROR_IO_DEVICE,                        /* c0000186 (STATUS_DEVICE_PROTOCOL_ERROR) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000187 (STATUS_BACKUP_CONTROLLER) */
   ERROR_LOG_FILE_FULL,                    /* c0000188 (STATUS_LOG_FILE_FULL) */
   ERROR_WRITE_PROTECT,                    /* c0000189 (STATUS_TOO_LATE) */
   ERROR_NO_TRUST_LSA_SECRET,              /* c000018a (STATUS_NO_TRUST_LSA_SECRET) */
   ERROR_NO_TRUST_SAM_ACCOUNT,             /* c000018b (STATUS_NO_TRUST_SAM_ACCOUNT) */
   ERROR_TRUSTED_DOMAIN_FAILURE,           /* c000018c (STATUS_TRUSTED_DOMAIN_FAILURE) */
   ERROR_TRUSTED_RELATIONSHIP_FAILURE,     /* c000018d (STATUS_TRUSTED_RELATIONSHIP_FAILURE) */
   ERROR_EVENTLOG_FILE_CORRUPT,            /* c000018e (STATUS_EVENTLOG_FILE_CORRUPT) */
   ERROR_EVENTLOG_CANT_START,              /* c000018f (STATUS_EVENTLOG_CANT_START) */
   ERROR_TRUST_FAILURE,                    /* c0000190 (STATUS_TRUST_FAILURE) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000191 (STATUS_MUTANT_LIMIT_EXCEEDED) */
   ERROR_NETLOGON_NOT_STARTED,             /* c0000192 (STATUS_NETLOGON_NOT_STARTED) */
   ERROR_ACCOUNT_EXPIRED,                  /* c0000193 (STATUS_ACCOUNT_EXPIRED) */
   ERROR_POSSIBLE_DEADLOCK,                /* c0000194 (STATUS_POSSIBLE_DEADLOCK) */
   ERROR_SESSION_CREDENTIAL_CONFLICT,      /* c0000195 (STATUS_NETWORK_CREDENTIAL_CONFLICT) */
   ERROR_REMOTE_SESSION_LIMIT_EXCEEDED,    /* c0000196 (STATUS_REMOTE_SESSION_LIMIT) */
   ERROR_EVENTLOG_FILE_CHANGED,            /* c0000197 (STATUS_EVENTLOG_FILE_CHANGED) */
   ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT,/* c0000198 (STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT) */
   ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT,/* c0000199 (STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT) */
   ERROR_NOLOGON_SERVER_TRUST_ACCOUNT,     /* c000019a (STATUS_NOLOGON_SERVER_TRUST_ACCOUNT) */
   ERROR_DOMAIN_TRUST_INCONSISTENT         /* c000019b (STATUS_DOMAIN_TRUST_INCONSISTENT) */
};

static const WORD table_c0000202[109] =
{
   ERROR_NO_USER_SESSION_KEY,              /* c0000202 */
   ERROR_UNEXP_NET_ERR,                    /* c0000203 */
   ERROR_RESOURCE_LANG_NOT_FOUND,          /* c0000204 (STATUS_RESOURCE_LANG_NOT_FOUND) */
   ERROR_NOT_ENOUGH_SERVER_MEMORY,         /* c0000205 */
   ERROR_INVALID_USER_BUFFER,              /* c0000206 */
   ERROR_INVALID_NETNAME,                  /* c0000207 */
   ERROR_INVALID_NETNAME,                  /* c0000208 */
   ERROR_TOO_MANY_NAMES,                   /* c0000209 */
   ERROR_DUP_NAME,                         /* c000020a */
   ERROR_NETNAME_DELETED,                  /* c000020b */
   ERROR_NETNAME_DELETED,                  /* c000020c */
   ERROR_NETNAME_DELETED,                  /* c000020d */
   ERROR_TOO_MANY_NAMES,                   /* c000020e */
   ERROR_UNEXP_NET_ERR,                    /* c000020f */
   ERROR_UNEXP_NET_ERR,                    /* c0000210 */
   ERROR_UNEXP_NET_ERR,                    /* c0000211 */
   ERROR_UNEXP_NET_ERR,                    /* c0000212 */
   ERROR_UNEXP_NET_ERR,                    /* c0000213 */
   ERROR_UNEXP_NET_ERR,                    /* c0000214 */
   ERROR_UNEXP_NET_ERR,                    /* c0000215 */
   ERROR_NOT_SUPPORTED,                    /* c0000216 */
   ERROR_NOT_SUPPORTED,                    /* c0000217 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000218 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000219 */
   ERROR_MR_MID_NOT_FOUND,                 /* c000021a */
   ERROR_MR_MID_NOT_FOUND,                 /* c000021b */
   ERROR_NO_BROWSER_SERVERS_FOUND,         /* c000021c */
   ERROR_MR_MID_NOT_FOUND,                 /* c000021d */
   ERROR_MR_MID_NOT_FOUND,                 /* c000021e */
   ERROR_MR_MID_NOT_FOUND,                 /* c000021f */
   ERROR_MAPPED_ALIGNMENT,                 /* c0000220 */
   ERROR_BAD_EXE_FORMAT,                   /* c0000221 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000222 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000223 */
   ERROR_PASSWORD_MUST_CHANGE,             /* c0000224 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000225 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000226 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000227 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000228 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000229 */
   0,                                      /* c000022a */
   0,                                      /* c000022b */
   ERROR_MR_MID_NOT_FOUND,                 /* c000022c */
   ERROR_MR_MID_NOT_FOUND,                 /* c000022d */
   ERROR_MR_MID_NOT_FOUND,                 /* c000022e */
   ERROR_MR_MID_NOT_FOUND,                 /* c000022f */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000230 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000231 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000232 */
   ERROR_DOMAIN_CONTROLLER_NOT_FOUND,      /* c0000233 */
   ERROR_ACCOUNT_LOCKED_OUT,               /* c0000234 */
   ERROR_INVALID_HANDLE,                   /* c0000235 */
   ERROR_CONNECTION_REFUSED,               /* c0000236 */
   ERROR_GRACEFUL_DISCONNECT,              /* c0000237 */
   ERROR_ADDRESS_ALREADY_ASSOCIATED,       /* c0000238 */
   ERROR_ADDRESS_NOT_ASSOCIATED,           /* c0000239 */
   ERROR_CONNECTION_INVALID,               /* c000023a */
   ERROR_CONNECTION_ACTIVE,                /* c000023b */
   ERROR_NETWORK_UNREACHABLE,              /* c000023c */
   ERROR_HOST_UNREACHABLE,                 /* c000023d */
   ERROR_PROTOCOL_UNREACHABLE,             /* c000023e */
   ERROR_PORT_UNREACHABLE,                 /* c000023f */
   ERROR_REQUEST_ABORTED,                  /* c0000240 */
   ERROR_CONNECTION_ABORTED,               /* c0000241 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000242 */
   ERROR_USER_MAPPED_FILE,                 /* c0000243 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000244 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000245 */
   ERROR_CONNECTION_COUNT_LIMIT,           /* c0000246 */
   ERROR_LOGIN_TIME_RESTRICTION,           /* c0000247 */
   ERROR_LOGIN_WKSTA_RESTRICTION,          /* c0000248 */
   ERROR_BAD_EXE_FORMAT,                   /* c0000249 */
   ERROR_MR_MID_NOT_FOUND,                 /* c000024a */
   ERROR_MR_MID_NOT_FOUND,                 /* c000024b */
   ERROR_MR_MID_NOT_FOUND,                 /* c000024c */
   ERROR_MR_MID_NOT_FOUND,                 /* c000024d */
   ERROR_MR_MID_NOT_FOUND,                 /* c000024e */
   ERROR_MR_MID_NOT_FOUND,                 /* c000024f */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000250 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000251 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000252 */
   ERROR_INTERNAL_ERROR,                   /* c0000253 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000254 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000255 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000256 */
   ERROR_HOST_UNREACHABLE,                 /* c0000257 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000258 */
   ERROR_LICENSE_QUOTA_EXCEEDED,           /* c0000259 */
   ERROR_MR_MID_NOT_FOUND,                 /* c000025a */
   ERROR_MR_MID_NOT_FOUND,                 /* c000025b */
   ERROR_MR_MID_NOT_FOUND,                 /* c000025c */
   ERROR_MR_MID_NOT_FOUND,                 /* c000025d */
   ERROR_SERVICE_DISABLED,                 /* c000025e */
   ERROR_MR_MID_NOT_FOUND,                 /* c000025f */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000260 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000261 */
   ERROR_INVALID_ORDINAL,                  /* c0000262 */
   ERROR_PROC_NOT_FOUND,                   /* c0000263 */
   ERROR_NOT_OWNER,                        /* c0000264 */
   ERROR_TOO_MANY_LINKS,                   /* c0000265 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000266 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000267 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000268 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000269 */
   ERROR_MR_MID_NOT_FOUND,                 /* c000026a */
   ERROR_MR_MID_NOT_FOUND,                 /* c000026b */
   ERROR_MR_MID_NOT_FOUND,                 /* c000026c */
   ERROR_MR_MID_NOT_FOUND,                 /* c000026d */
   ERROR_NOT_READY                         /* c000026e */
};

static const WORD table_c0020001[88] =
{
   RPC_S_INVALID_STRING_BINDING,           /* c0020001 */
   RPC_S_WRONG_KIND_OF_BINDING,            /* c0020002 */
   ERROR_INVALID_HANDLE,                   /* c0020003 */
   RPC_S_PROTSEQ_NOT_SUPPORTED,            /* c0020004 */
   RPC_S_INVALID_RPC_PROTSEQ,              /* c0020005 */
   RPC_S_INVALID_STRING_UUID,              /* c0020006 */
   RPC_S_INVALID_ENDPOINT_FORMAT,          /* c0020007 */
   RPC_S_INVALID_NET_ADDR,                 /* c0020008 */
   RPC_S_NO_ENDPOINT_FOUND,                /* c0020009 */
   RPC_S_INVALID_TIMEOUT,                  /* c002000a */
   RPC_S_OBJECT_NOT_FOUND,                 /* c002000b */
   RPC_S_ALREADY_REGISTERED,               /* c002000c */
   RPC_S_TYPE_ALREADY_REGISTERED,          /* c002000d */
   RPC_S_ALREADY_LISTENING,                /* c002000e */
   RPC_S_NO_PROTSEQS_REGISTERED,           /* c002000f */
   RPC_S_NOT_LISTENING,                    /* c0020010 */
   RPC_S_UNKNOWN_MGR_TYPE,                 /* c0020011 */
   RPC_S_UNKNOWN_IF,                       /* c0020012 */
   RPC_S_NO_BINDINGS,                      /* c0020013 */
   RPC_S_NO_PROTSEQS,                      /* c0020014 */
   RPC_S_CANT_CREATE_ENDPOINT,             /* c0020015 */
   RPC_S_OUT_OF_RESOURCES,                 /* c0020016 */
   RPC_S_SERVER_UNAVAILABLE,               /* c0020017 */
   RPC_S_SERVER_TOO_BUSY,                  /* c0020018 */
   RPC_S_INVALID_NETWORK_OPTIONS,          /* c0020019 */
   RPC_S_NO_CALL_ACTIVE,                   /* c002001a */
   RPC_S_CALL_FAILED,                      /* c002001b */
   RPC_S_CALL_FAILED_DNE,                  /* c002001c */
   RPC_S_PROTOCOL_ERROR,                   /* c002001d */
   ERROR_MR_MID_NOT_FOUND,                 /* c002001e */
   RPC_S_UNSUPPORTED_TRANS_SYN,            /* c002001f */
   ERROR_MR_MID_NOT_FOUND,                 /* c0020020 */
   RPC_S_UNSUPPORTED_TYPE,                 /* c0020021 */
   RPC_S_INVALID_TAG,                      /* c0020022 */
   RPC_S_INVALID_BOUND,                    /* c0020023 */
   RPC_S_NO_ENTRY_NAME,                    /* c0020024 */
   RPC_S_INVALID_NAME_SYNTAX,              /* c0020025 */
   RPC_S_UNSUPPORTED_NAME_SYNTAX,          /* c0020026 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0020027 */
   RPC_S_UUID_NO_ADDRESS,                  /* c0020028 */
   RPC_S_DUPLICATE_ENDPOINT,               /* c0020029 */
   RPC_S_UNKNOWN_AUTHN_TYPE,               /* c002002a */
   RPC_S_MAX_CALLS_TOO_SMALL,              /* c002002b */
   RPC_S_STRING_TOO_LONG,                  /* c002002c */
   RPC_S_PROTSEQ_NOT_FOUND,                /* c002002d */
   RPC_S_PROCNUM_OUT_OF_RANGE,             /* c002002e */
   RPC_S_BINDING_HAS_NO_AUTH,              /* c002002f */
   RPC_S_UNKNOWN_AUTHN_SERVICE,            /* c0020030 */
   RPC_S_UNKNOWN_AUTHN_LEVEL,              /* c0020031 */
   RPC_S_INVALID_AUTH_IDENTITY,            /* c0020032 */
   RPC_S_UNKNOWN_AUTHZ_SERVICE,            /* c0020033 */
   EPT_S_INVALID_ENTRY,                    /* c0020034 */
   EPT_S_CANT_PERFORM_OP,                  /* c0020035 */
   EPT_S_NOT_REGISTERED,                   /* c0020036 */
   RPC_S_NOTHING_TO_EXPORT,                /* c0020037 */
   RPC_S_INCOMPLETE_NAME,                  /* c0020038 */
   RPC_S_INVALID_VERS_OPTION,              /* c0020039 */
   RPC_S_NO_MORE_MEMBERS,                  /* c002003a */
   RPC_S_NOT_ALL_OBJS_UNEXPORTED,          /* c002003b */
   RPC_S_INTERFACE_NOT_FOUND,              /* c002003c */
   RPC_S_ENTRY_ALREADY_EXISTS,             /* c002003d */
   RPC_S_ENTRY_NOT_FOUND,                  /* c002003e */
   RPC_S_NAME_SERVICE_UNAVAILABLE,         /* c002003f */
   RPC_S_INVALID_NAF_ID,                   /* c0020040 */
   RPC_S_CANNOT_SUPPORT,                   /* c0020041 */
   RPC_S_NO_CONTEXT_AVAILABLE,             /* c0020042 */
   RPC_S_INTERNAL_ERROR,                   /* c0020043 */
   RPC_S_ZERO_DIVIDE,                      /* c0020044 */
   RPC_S_ADDRESS_ERROR,                    /* c0020045 */
   RPC_S_FP_DIV_ZERO,                      /* c0020046 */
   RPC_S_FP_UNDERFLOW,                     /* c0020047 */
   RPC_S_FP_OVERFLOW,                      /* c0020048 */
   RPC_S_CALL_IN_PROGRESS,                 /* c0020049 */
   RPC_S_NO_MORE_BINDINGS,                 /* c002004a */
   RPC_S_GROUP_MEMBER_NOT_FOUND,           /* c002004b */
   EPT_S_CANT_CREATE,                      /* c002004c */
   RPC_S_INVALID_OBJECT,                   /* c002004d */
   ERROR_MR_MID_NOT_FOUND,                 /* c002004e */
   RPC_S_NO_INTERFACES,                    /* c002004f */
   RPC_S_CALL_CANCELLED,                   /* c0020050 */
   RPC_S_BINDING_INCOMPLETE,               /* c0020051 */
   RPC_S_COMM_FAILURE,                     /* c0020052 */
   RPC_S_UNSUPPORTED_AUTHN_LEVEL,          /* c0020053 */
   RPC_S_NO_PRINC_NAME,                    /* c0020054 */
   RPC_S_NOT_RPC_ERROR,                    /* c0020055 */
   ERROR_MR_MID_NOT_FOUND,                 /* c0020056 */
   RPC_S_SEC_PKG_ERROR,                    /* c0020057 */
   RPC_S_NOT_CANCELLED                     /* c0020058 */
};

static const WORD table_c0030001[12] =
{
   RPC_X_NO_MORE_ENTRIES,                  /* c0030001 */
   RPC_X_SS_CHAR_TRANS_OPEN_FAIL,          /* c0030002 */
   RPC_X_SS_CHAR_TRANS_SHORT_FILE,         /* c0030003 */
   ERROR_INVALID_HANDLE,                   /* c0030004 */
   ERROR_INVALID_HANDLE,                   /* c0030005 */
   RPC_X_SS_CONTEXT_DAMAGED,               /* c0030006 */
   RPC_X_SS_HANDLES_MISMATCH,              /* c0030007 */
   RPC_X_SS_CANNOT_GET_CALL_HANDLE,        /* c0030008 */
   RPC_X_NULL_REF_POINTER,                 /* c0030009 */
   RPC_X_ENUM_VALUE_OUT_OF_RANGE,          /* c003000a */
   RPC_X_BYTE_COUNT_TOO_SMALL,             /* c003000b */
   RPC_X_BAD_STUB_DATA                     /* c003000c */
};

static const WORD table_c0030059[6] =
{
   RPC_X_INVALID_ES_ACTION,                /* c0030059 */
   RPC_X_WRONG_ES_VERSION,                 /* c003005a */
   RPC_X_WRONG_STUB_VERSION,               /* c003005b */
   RPC_X_INVALID_PIPE_OBJECT,              /* c003005c */
   RPC_X_WRONG_PIPE_ORDER,                 /* c003005d */
   RPC_X_WRONG_PIPE_VERSION                /* c003005e */
};

static const struct error_table error_table[] =
{
    { 0x00000103, 0x0000010e, table_00000103 },
    { 0x40000002, 0x4000000e, table_40000002 },
    { 0x40020056, 0x40020057, table_40020056 },
    { 0x400200af, 0x400200b0, table_400200af },
    { 0x80000001, 0x80000026, table_80000001 },
    { 0x80090300, 0x80090317, table_80090300 },
    { 0xc0000001, 0xc000019c, table_c0000001 },
    { 0xc0000202, 0xc000026f, table_c0000202 },
    { 0xc0020001, 0xc0020059, table_c0020001 },
    { 0xc0030001, 0xc003000d, table_c0030001 },
    { 0xc0030059, 0xc003005f, table_c0030059 },
    { 0, 0, 0 }  /* last entry */
};
