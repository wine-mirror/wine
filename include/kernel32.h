/* kernel32.h - 95-09-14  Cameron Heide
 *
 * Win32 functions, structures, and types related to kernel functions
 */
#include <stddef.h>

int KERN32_Init(void);
void SetLastError(DWORD error);
DWORD ErrnoToLastError(int errno_num);

/* Linux's wchar_t is unsigned long but Win32 wants unsigned short
 */
typedef unsigned short WCHAR;

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

typedef struct {
        WORD wYear;
        WORD wMonth;
        WORD wDayOfWeek;
        WORD wDay;
        WORD wHour;
        WORD wMinute;
        WORD wSecond;
        WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

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

