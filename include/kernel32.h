/* kernel32.h - 95-09-14  Cameron Heide
 *
 * Win32 functions, structures, and types related to kernel functions
 */
#ifndef __WINE_KERNEL32_H
#define __WINE_KERNEL32_H

#include <stddef.h>

int KERN32_Init(void);
void SetLastError(DWORD error);
DWORD ErrnoToLastError(int errno_num);

/* Linux's wchar_t is unsigned long but Win32 wants unsigned short
 */
typedef unsigned short WCHAR;
typedef WCHAR *LPTSTR;

/* Code page information.
 */
typedef struct {
        DWORD MaxCharSize;
        BYTE DefaultChar[2];
        BYTE LeadBytes[5];
} CPINFO, *LPCPINFO;

/* The 'overlapped' data structure used by async I/O functions.
 */
typedef struct {
        DWORD Internal;
        DWORD InternalHigh;
        DWORD Offset;
        DWORD OffsetHigh;
        HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

/* Process startup information.
 */
typedef struct {
        DWORD cb;
        LPSTR lpReserved;
        LPSTR lpDesktop;
        LPSTR lpTitle;
        DWORD dwX;
        DWORD dwY;
        DWORD dwXSize;
        DWORD dwYSize;
        DWORD dwXCountChars;
        DWORD dwYCountChars;
        DWORD dwFillAttribute;
        DWORD dwFlags;
        WORD wShowWindow;
        WORD cbReserved2;
        BYTE *lpReserved2;
        HANDLE hStdInput;
        HANDLE hStdOutput;
        HANDLE hStdError;
} STARTUPINFO, *LPSTARTUPINFO;

/* SYSTEMTIME, and LPSYSTEMTIME moved to include/windows.h (JBP) */

typedef struct {
        LONG Bias;
        WCHAR StandardName[32];
        SYSTEMTIME StandardDate;
        LONG StandardBias;
        WCHAR DaylightName[32];
        SYSTEMTIME DaylightDate;
        LONG DaylightBias;
} TIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

#define TIME_ZONE_ID_UNKNOWN    0
#define TIME_ZONE_ID_STANDARD   1
#define TIME_ZONE_ID_DAYLIGHT   2


/* File object type definitions
 */
#define FILE_TYPE_UNKNOWN       0
#define FILE_TYPE_DISK          1
#define FILE_TYPE_CHAR          2
#define FILE_TYPE_PIPE          3
#define FILE_TYPE_REMOTE        32768

/* File creation flags
 */
#define GENERIC_READ            0x80000000L
#define GENERIC_WRITE           0x40000000L
#define CREATE_NEW              1
#define CREATE_ALWAYS           2
#define OPEN_EXISTING           3
#define OPEN_ALWAYS             4
#define TRUNCATE_EXISTING       5

/* Standard handle identifiers
 */
#define STD_INPUT_HANDLE        ((DWORD) -10)
#define STD_OUTPUT_HANDLE       ((DWORD) -11)
#define STD_ERROR_HANDLE        ((DWORD) -12)

/* The security attributes structure 
 */
typedef struct {
    DWORD nLength;
    void *lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct
{
  int dwLowDateTime;
  int dwHighDateTime;
} FILETIME;

typedef struct
{
  int dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  int dwVolumeSerialNumber;
  int nFileSizeHigh;
  int nFileSizeLow;
  int nNumberOfLinks;
  int nFileIndexHigh;
  int nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION ;

/* File attribute flags
 */
#define FILE_ATTRIBUTE_ARCHIVE          0x0020
#define FILE_ATTRIBUTE_COMPRESSED       0x0800
#define FILE_ATTRIBUTE_DIRECTORY        0x0010
#define FILE_ATTRIBUTE_HIDDEN           0x0002
#define FILE_ATTRIBUTE_NORMAL           0x0080
#define FILE_ATTRIBUTE_READONLY         0x0001
#define FILE_ATTRIBUTE_SYSTEM           0x0004
#define FILE_ATTRIBUTE_TEMPORARY        0x0100
#define FILE_ATTRIBUTE_ATOMIC_WRITE     0x0200
#define FILE_ATTRIBUTE_XACTION_WRITE    0x0400

#endif  /* __WINE_KERNEL32_H */
