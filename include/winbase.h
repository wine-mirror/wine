#ifndef __WINE_WINBASE_H
#define __WINE_WINBASE_H



#ifdef UNICODE
#define LoadAccelerators LoadAcceleratorsW
#define TranslateAccelat
#else
#define LoadAccelerators LoadAcceleratorsA
#endif


#define INVALID_HANDLE_VALUE ((HANDLE) -1)

#define WAIT_FAILED		0xffffffff
#define WAIT_OBJECT_0		0
#define WAIT_ABANDONED		STATUS_ABANDONED_WAIT_0
#define WAIT_ABANDONED_0	STATUS_ABANDONED_WAIT_0
#define WAIT_TIMEOUT		STATUS_TIMEOUT

#define	PAGE_NOACCESS		0x01
#define	PAGE_READONLY		0x02
#define	PAGE_READWRITE		0x04
#define	PAGE_WRITECOPY		0x08
#define	PAGE_EXECUTE		0x10
#define	PAGE_EXECUTE_READ	0x20
#define	PAGE_EXECUTE_READWRITE	0x40
#define	PAGE_EXECUTE_WRITECOPY	0x80
#define	PAGE_GUARD		0x100
#define	PAGE_NOCACHE		0x200

#define MEM_COMMIT              0x00001000
#define MEM_RESERVE             0x00002000
#define MEM_DECOMMIT            0x00004000
#define MEM_RELEASE             0x00008000
#define MEM_FREE                0x00010000
#define MEM_PRIVATE             0x00020000
#define MEM_MAPPED              0x00040000
#define MEM_TOP_DOWN            0x00100000

struct _EXCEPTION_POINTERS;

typedef LONG (TOP_LEVEL_EXCEPTION_FILTER)(struct _EXCEPTION_POINTERS *);

WINAPI  TOP_LEVEL_EXCEPTION_FILTER *SetUnhandledExceptionFilter(TOP_LEVEL_EXCEPTION_FILTER *func);

/*WINAPI int  SetErrorMode(int);*/

#define STATUS_WAIT_0                    0x00000000    
#define STATUS_ABANDONED_WAIT_0          0x00000080    
#define STATUS_USER_APC                  0x000000C0    
#define STATUS_TIMEOUT                   0x00000102    
#define STATUS_PENDING                   0x00000103    
#define STATUS_GUARD_PAGE_VIOLATION      0x80000001    
#define STATUS_DATATYPE_MISALIGNMENT     0x80000002    
#define STATUS_BREAKPOINT                0x80000003    
#define STATUS_SINGLE_STEP               0x80000004    
#define STATUS_ACCESS_VIOLATION          0xC0000005    
#define STATUS_IN_PAGE_ERROR             0xC0000006    
#define STATUS_NO_MEMORY                 0xC0000017    
#define STATUS_ILLEGAL_INSTRUCTION       0xC000001D    
#define STATUS_NONCONTINUABLE_EXCEPTION  0xC0000025    
#define STATUS_INVALID_DISPOSITION       0xC0000026    
#define STATUS_ARRAY_BOUNDS_EXCEEDED     0xC000008C    
#define STATUS_FLOAT_DENORMAL_OPERAND    0xC000008D    
#define STATUS_FLOAT_DIVIDE_BY_ZERO      0xC000008E    
#define STATUS_FLOAT_INEXACT_RESULT      0xC000008F    
#define STATUS_FLOAT_INVALID_OPERATION   0xC0000090    
#define STATUS_FLOAT_OVERFLOW            0xC0000091    
#define STATUS_FLOAT_STACK_CHECK         0xC0000092    
#define STATUS_FLOAT_UNDERFLOW           0xC0000093    
#define STATUS_INTEGER_DIVIDE_BY_ZERO    0xC0000094    
#define STATUS_INTEGER_OVERFLOW          0xC0000095    
#define STATUS_PRIVILEGED_INSTRUCTION    0xC0000096    
#define STATUS_STACK_OVERFLOW            0xC00000FD    
#define STATUS_CONTROL_C_EXIT            0xC000013A    

#define DUPLICATE_CLOSE_SOURCE		0x00000001
#define DUPLICATE_SAME_ACCESS		0x00000002

typedef struct 
{
  int type;
} exception;

typedef struct 
{
  int pad[39];
  int edi;
  int esi;
  int ebx;
  int edx;
  int ecx;
  int eax;

  int ebp;
  int eip;
  int cs;
  int eflags;
  int esp;
  int ss;
} exception_info;

/* Could this type be considered opaque? */
typedef struct {
	LPVOID	DebugInfo;
	LONG LockCount;
	LONG RecursionCount;
	HANDLE OwningThread;
	HANDLE LockSemaphore;
	DWORD Reserved;
}CRITICAL_SECTION;

/*DWORD WINAPI GetVersion( void );*/

int
WINAPI WinMain(HINSTANCE, HINSTANCE prev, char *cmd, int show);

#define GMEM_FIXED          0x0000
#define GMEM_MOVEABLE 	    0x0002

DECLARE_HANDLE(HACCEL);

HACCEL WINAPI LoadAcceleratorsA(   HINSTANCE, const char *);
#define FreeModule(hLibModule) FreeLibrary((hLibModule))
#define MakeProcInstance(lpProc,hInstance) (lpProc)
#define FreeProcInstance(lpProc) (lpProc)


WINAPI void     DeleteCriticalSection(CRITICAL_SECTION *lpCrit);
WINAPI void     EnterCriticalSection(CRITICAL_SECTION *lpCrit);
WINAPI int      GetCurrentProcessId(void);
WINAPI HANDLE32 GetProcessHeap(void);
WINAPI LPVOID   HeapAlloc(HANDLE32,DWORD,DWORD);
WINAPI DWORD    HeapCompact(HANDLE32,DWORD);
WINAPI HANDLE32 HeapCreate(DWORD,DWORD,DWORD);
WINAPI BOOL     HeapDestroy(HANDLE32);
WINAPI BOOL     HeapFree(HANDLE32,DWORD,LPVOID);
WINAPI BOOL     HeapLock(HANDLE32);
WINAPI LPVOID   HeapReAlloc(HANDLE32,DWORD,LPVOID,DWORD);
WINAPI DWORD    HeapSize(HANDLE32,DWORD,LPVOID);
WINAPI BOOL     HeapUnlock(HANDLE32);
WINAPI BOOL     HeapValidate(HANDLE32,DWORD,LPVOID);
WINAPI void     InitializeCriticalSection(CRITICAL_SECTION *lpCrit);
WINAPI void     LeaveCriticalSection(CRITICAL_SECTION *lpCrit);
WINAPI HANDLE   OpenProcess(DWORD access, BOOL inherit, DWORD id);
WINAPI int      TerminateProcess(HANDLE h, int ret);
WINAPI LPVOID   VirtualAlloc(LPVOID addr,DWORD size,DWORD type,DWORD protect);
WINAPI BOOL     VirtualFree( LPVOID addr, DWORD size, DWORD type );

#endif  /* __WINE_WINBASE_H */
