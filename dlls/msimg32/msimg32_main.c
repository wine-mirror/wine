#include "winbase.h"
#include "winerror.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(msimg32);

BOOL WINAPI GradientFill(HDC hdc, void * vert_array, ULONG nvert, 
		    void * grad_array, ULONG ngrad, ULONG mode)
{
  FIXME("stub: %ld vertices %ld gradients mode %lx\n", nvert, ngrad, mode);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
