/*
 *	OLEAUT32
 *
 */
#include <string.h>
#include "winuser.h"
#include "winerror.h"
#include "oleauto.h"
#include "wine/obj_base.h"
#include "heap.h"
#include "ldt.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ole)

HRESULT WINAPI RegisterActiveObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4)
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx), stub!\n",x1,x2,x3,x4);
	return E_FAIL;
}
