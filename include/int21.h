#ifndef INT21_H
#define INT21_H
#include <dirent.h>

struct dosdirent {
	int  inuse;
	DIR *ds;
	char unixpath[256];
	char filename[256];
	char filemask[12];
	char attribute;
	long filesize;
	long filetime;
	long filedate;
};

struct diskinfo {
	unsigned int infolevel;
	unsigned long serialnumber;
	char	label[11];
	char	fstype[8];
};

#define DosVersion 	  0x0303;

#define SectorSize 	  0x200;
#define SectorsPerCluster 0x04;

#define EAX context->sc_eax
#define EBX context->sc_ebx
#define ECX context->sc_ecx
#define EDX context->sc_edx

#define AX (context->sc_eax & 0x0000ffffL)
#define BX (context->sc_ebx & 0x0000ffffL)
#define CX (context->sc_ecx & 0x0000ffffL)
#define DX (context->sc_edx & 0x0000ffffL)

#define CS context->sc_cs
#define DS context->sc_ds
#define ES context->sc_es

#define DI context->sc_edi
#define SI context->sc_esi

#define EFL context->sc_efl

#define pointer(a,b) 	(((unsigned int) a << 16) | b)
#define segment(a) 	(a >> 16)
#define offset(a)	(a & 0xffff)

#define SetCflag	(EFL |= 0x00000001L)
#define ResetCflag	(EFL &= 0xfffffffeL)

/* extended error codes */

#define NoError		0x00
#define InvalidFunction 0x01
#define FileNotFound	0x02
#define PathNotFound	0x03
#define AccessDenied	0x05
#define InvalidHandle	0x06
#define MCBDestroyed	0x07
#define	OutOfMemory	0x08
#define MCBInvalid	0x09
#define DataInvalid	0x0d
#define InvalidDrive	0x0f
#define CanNotRemoveCwd	0x10
#define	NotSameDevice	0x11
#define NoMoreFiles	0x12
#define WriteProtected  0x13
#define	UnknownUnit	0x14
#define DriveNotReady	0x15
#define UnknownCommand	0x16
#define CRCError	0x17
#define	BadRqLength	0x18
#define SeekError	0x19
#define	UnknownMedia	0x1a
#define	SectorNotFound	0x1b
#define OutOfPaper	0x1c
#define WriteFault	0x1d
#define ReadFault	0x1e
#define	GeneralFailure	0x1f
#define ShareViolation	0x20
#define LockViolation	0x21
#define DiskFull	0x27
#define NoNetwork	0x49
#define FileExists	0x50
#define CanNotMakeDir	0x52

/* Error classes */

#define EC_Temporary	0x02
#define EC_AccessDenied	0x03
#define EC_AppError	0x04
#define EC_SystemFailure 0x06
#define EC_NotFound	0x08
#define	EC_MediaError	0x0b
#define EC_Exists	0x0c
#define	EC_Unknown	0x0d

/* Suggested actions */

#define	SA_Retry	0x01
#define SA_Abort	0x04
#define SA_Ignore	0x06
#define SA_Ask4Retry	0x07

/* Error locus */

#define EL_Unknown	0x01
#define EL_Disk		0x02
#define EL_Network	0x03
#define EL_Memory	0x05

#endif /* INT21_H */
