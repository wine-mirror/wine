#include "winbase.h"
#include "ntdef.h"
#include "winnt.h"

NTSTATUS WINAPI LdrDisableThreadCalloutsForDll(HANDLE hModule)
{
    if (DisableThreadLibraryCalls(hModule))
	return STATUS_SUCCESS;
    else
	return STATUS_DLL_NOT_FOUND;
}
