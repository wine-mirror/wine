/* Initial setupapi.h.  

   FIXME: Missing A LOT of definitions / declarations! 
*/

#ifndef __SETUPAPI__
#define __SETUPAPI__

/* Define type for handle to a loaded inf file */
typedef PVOID HINF;

/* Define type for handle to a device information set */
typedef PVOID HDEVINFO;

/* inf structure. */
typedef struct _INFCONTEXT
{
   PVOID Inf;
   PVOID CurrentInf;
   UINT  Section;
   UINT  Line;
} INFCONTEXT, *PINFCONTEXT;

/* Device Information structure (references a device instance that is a member
   of a device information set) */
typedef struct _SP_DEVINFO_DATA
{
   DWORD cbSize;
   GUID  ClassGuid;
   DWORD DevInst;   /* DEVINST handle */
   DWORD Reserved;
} SP_DEVINFO_DATA, *PSP_DEVINFO_DATA;

#endif
