extern int WIN32_LastError;

/* ERROR_UNKNOWN is a placeholder for error conditions which haven't
 * been tested yet so we're not exactly sure what will be returned.
 * All instances of ERROR_UNKNOWN should be tested under Win95/NT
 * and replaced.
 */
#define     ERROR_UNKNOWN               99999

#define     ERROR_FILE_NOT_FOUND        2
#define     ERROR_TOO_MANY_OPEN_FILES   4
#define     ERROR_ACCESS_DENIED         5
#define     ERROR_INVALID_HANDLE        6
#define     ERROR_BAD_FORMAT            11
#define     ERROR_OUTOFMEMORY           14
#define     ERROR_FILE_EXISTS           80
#define     ERROR_INVALID_PARAMETER     87
#define     ERROR_BROKEN_PIPE           109
#define     ERROR_DISK_FULL             112
#define     ERROR_CALL_NOT_IMPLEMENTED  120
#define     ERROR_SEEK_ON_DEVICE        132
#define     ERROR_DIR_NOT_EMPTY         145
#define     ERROR_BUSY                  170
#define     ERROR_FILENAME_EXCED_RANGE  206
#define     ERROR_IO_DEVICE             1117
#define     ERROR_POSSIBLE_DEADLOCK     1131
#define     ERROR_BAD_DEVICE            1200
