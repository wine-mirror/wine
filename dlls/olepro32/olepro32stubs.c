/*
 * OlePro32 Stubs
 *
 * Copyright 1999 Corel Corporation
 *
 * Sean Langley
 */

#include "debugtools.h"
#include "winbase.h"
#include "ole2.h"
#include "windef.h"

DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 *		DllUnregisterServer (OLEPRO32.258)
 */
HRESULT WINAPI OLEPRO32_DllUnregisterServer()
{
	FIXME("not implemented (olepro32.dll) \n");
	return S_OK;
}

/***********************************************************************
 *		DllRegisterServer (OLEPRO32.257)
 */
HRESULT WINAPI OLEPRO32_DllRegisterServer()
{
	FIXME("not implemented (olepro32.dll) \n");
	return S_OK;
}

/***********************************************************************
 *		DllCanUnloadNow (OLEPRO32.255)
 */
HRESULT WINAPI OLEPRO32_DllCanUnloadNow( )
{
	FIXME("not implemented (olepro32.dll) \n");
	return S_OK;
}

/***********************************************************************
 *		DllGetClassObject (OLEPRO32.256)
 */
HRESULT WINAPI OLEPRO32_DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppv )
{
	FIXME("not implemented (olepro32.dll) \n");
	return S_OK;
}
