/* Initial setupapi.h.  

   FIXME: Missing A LOT of definitions / declarations! 
*/

#ifndef __SETUPAPI__
#define __SETUPAPI__

#include "commctrl.h"

/* Define type for handle to a loaded inf file */
typedef PVOID HINF;

/* Define type for handle to a device information set */
typedef PVOID HDEVINFO;

/* Define type for setup file queue */
typedef PVOID HSPFILEQ;

/* inf structure. */
typedef struct _INFCONTEXT
{
   PVOID Inf;
   PVOID CurrentInf;
   UINT  Section;
   UINT  Line;
} INFCONTEXT, *PINFCONTEXT;

typedef UINT (CALLBACK* PSP_FILE_CALLBACK_A)( PVOID Context, UINT Notification,
                                              UINT Param1, UINT Param2 );
typedef UINT (CALLBACK* PSP_FILE_CALLBACK_W)( PVOID Context, UINT Notification,
                                              UINT Param1, UINT Param2 );
#define PSP_FILE_CALLBACK WINELIB_NAME_AW(PSP_FILE_CALLBACK_)


/* Device Information structure (references a device instance that is a member
   of a device information set) */
typedef struct _SP_DEVINFO_DATA
{
   DWORD cbSize;
   GUID  ClassGuid;
   DWORD DevInst;   /* DEVINST handle */
   DWORD Reserved;
} SP_DEVINFO_DATA, *PSP_DEVINFO_DATA;

#endif /* __SETUPAPI__ */
