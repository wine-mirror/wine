/* 
 *  shpolicy.h - contains defs of policy data for SHRestricted
 * 
 *  Created 1999 by Ian Schmidt, <ischmidt@cfl.rr.com>
 *  Up to date as of SHELL32 v4.72 (Win98, Win95 with MSIE 5)
 */

#ifndef __WINE_SHPOLICY_H
#define __WINE_SHPOLICY_H

#define SHELL_MAX_POLICIES 57

#define SHELL_NO_POLICY 0xffffffff

/*
 * Note: we don't need pshpack1.h / poppack here because we don't
 * rely on structure packing and nothing outside SHRestricted
 * accesses this structure.
 */

typedef struct tagPOLICYDAT
{
  DWORD polflags;        /* flags value passed to SHRestricted */
  LPSTR appstr;          /* application str such as "Explorer" */
  LPSTR keystr;          /* name of the actual registry key / policy */
  DWORD cache;           /* cached value or 0xffffffff for invalid */
} POLICYDATA, *LPPOLICYDATA;

extern POLICYDATA sh32_policy_table[SHELL_MAX_POLICIES];

/* policy functions */

BOOL WINAPI SHInitRestricted(LPSTR, LPSTR);

#endif /* __WINE_SHPOLICY_H */







