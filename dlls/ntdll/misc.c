/*
 * Helper functions for ntdll
 */
#include <time.h>

#include "config.h"

#include "debugtools.h"
#include "ntdll_misc.h"

DEFAULT_DEBUG_CHANNEL(ntdll);

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

LPSTR debugstr_as (PANSI_STRING us)
{
	if (!us) return NULL;
	return debugstr_an(us->Buffer, us->Length);
}

LPSTR debugstr_us (PUNICODE_STRING us)
{
	if (!us) return NULL;
	return debugstr_wn(us->Buffer, us->Length);
}

