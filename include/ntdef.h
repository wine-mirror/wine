#ifndef __WINE_NTDEF_H
#define __WINE_NTDEF_H

#define NTAPI   __stdcall 

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

/* NT lowlevel Strings (handled by Rtl* functions in NTDLL)
 * If they are zero terminated, Length does not include the terminating 0.
 */

typedef struct _STRING {
	UINT16	Length;
	UINT16	MaximumLength;
	LPSTR	Buffer;
} STRING,*PSTRING,ANSI_STRING,*PANSI_STRING;

typedef struct _CSTRING {
	UINT16	Length;
	UINT16	MaximumLength;
	LPCSTR	Buffer;
} CSTRING,*PCSTRING;

typedef struct _UNICODE_STRING {
	UINT16	Length;		/* bytes */
	UINT16	MaximumLength;	/* bytes */
	LPWSTR	Buffer;
} UNICODE_STRING,*PUNICODE_STRING;

/*
	Objects
*/

#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_VALID_ATTRIBUTES    0x000003F2L

typedef struct _OBJECT_ATTRIBUTES 
{   ULONG Length;
    HANDLE32 RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        /* type SECURITY_DESCRIPTOR */
    PVOID SecurityQualityOfService;  /* type SECURITY_QUALITY_OF_SERVICE */
} OBJECT_ATTRIBUTES;

typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;


#endif
