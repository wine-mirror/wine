/*
 * Helper functions for ntdll
 */
#include <time.h>
#include <math.h>

#include "config.h"

#include "debugtools.h"
#include "ntdll_misc.h"

DEFAULT_DEBUG_CHANNEL(ntdll);

#if defined(__GNUC__) && defined(__i386__)
#define USING_REAL_FPU
#define DO_FPU(x,y) __asm__ __volatile__( x " %0;fwait" : "=m" (y) : )
#define POP_FPU(x) DO_FPU("fstpl",x)
#endif

void dump_ObjectAttributes (POBJECT_ATTRIBUTES oa)
{
	if (oa)
	  TRACE("%p:(name=%s, attr=0x%08lx, hRoot=0x%08x, sd=%p) \n",
	    oa, debugstr_us(oa->ObjectName),
	    oa->Attributes, oa->RootDirectory, oa->SecurityDescriptor);
}

void dump_AnsiString(PANSI_STRING as, BOOLEAN showstring)
{
	if (as)
	{
	  if (showstring)
	    TRACE("%p %p(%s) (%u %u)\n", as, as->Buffer, debugstr_as(as), as->Length, as->MaximumLength);
	  else
	    TRACE("%p %p(<OUT>) (%u %u)\n", as, as->Buffer, as->Length, as->MaximumLength);
	}
}

void dump_UnicodeString(PUNICODE_STRING us, BOOLEAN showstring)
{
	if (us)
	{
	  if (showstring)
	    TRACE("%p %p(%s) (%u %u)\n", us, us->Buffer, debugstr_us(us), us->Length, us->MaximumLength);
	  else
	    TRACE("%p %p(<OUT>) (%u %u)\n", us, us->Buffer, us->Length, us->MaximumLength);
	}
}

LPCSTR debugstr_as (PANSI_STRING us)
{
	if (!us) return NULL;
	return debugstr_an(us->Buffer, us->Length);
}

LPCSTR debugstr_us (PUNICODE_STRING us)
{
	if (!us) return NULL;
	return debugstr_wn(us->Buffer, us->Length);
}

/*********************************************************************
 *                  _ftol   (NTDLL)
 */
#ifdef USING_REAL_FPU
LONG __cdecl NTDLL__ftol(void)
{
	/* don't just do DO_FPU("fistp",retval), because the rounding
	 * mode must also be set to "round towards zero"... */
	double fl;
	POP_FPU(fl);
	return (LONG)fl;
}
#else
LONG __cdecl NTDLL__ftol(double fl)
{
	FIXME("should be register function\n");
	return (LONG)fl;
}
#endif

/*********************************************************************
 *                  _CIpow   (NTDLL)
 */
#ifdef USING_REAL_FPU
LONG __cdecl NTDLL__CIpow(void)
{
	double x,y;
	POP_FPU(y);
	POP_FPU(x);
	return pow(x,y);
}
#else
LONG __cdecl NTDLL__CIpow(double x,double y)
{
	FIXME("should be register function\n");
	return pow(x,y);
}
#endif
