/*
 * Copyright 2001 Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "winbase.h"
#include "winnls.h"
#include "mmsystem.h"
#include "winerror.h"
#include "vfw.h"
#include "debugtools.h"
#include "avifile_private.h"

DEFAULT_DEBUG_CHANNEL(avifile);

WINE_AVIFILE_DATA AVIFILE_data;


/***********************************************************************
 *		AVIFILE_InitProcess (internal)
 */
static BOOL AVIFILE_InitProcess( void )
{
	TRACE("()\n");

	AVIFILE_data.dwAVIFileRef = 0;
	AVIFILE_data.dwClassObjRef = 0;
	AVIFILE_data.hHeap = (HANDLE)NULL;

	AVIFILE_data.hHeap = HeapCreate( 0, 0x10000, 0 );
	if ( AVIFILE_data.hHeap  == (HANDLE)NULL )
	{
		ERR( "cannot allocate heap for AVIFILE.\n" );
		return FALSE;
	}

	return TRUE;
}

/***********************************************************************
 *		AVIFILE_UninitProcess (internal)
 */
static void AVIFILE_UninitProcess( void )
{
	TRACE("()\n");

	if ( AVIFILE_data.dwAVIFileRef != 0 )
		ERR( "you must call AVIFileExit()\n" );

	if ( AVIFILE_data.dwClassObjRef != 0 )
		ERR( "you must release some objects allocated from AVIFile.\n" );

	if ( AVIFILE_data.hHeap != (HANDLE)NULL )
	{
		HeapDestroy( AVIFILE_data.hHeap );
		AVIFILE_data.hHeap = (HANDLE)NULL;
	}
}

/***********************************************************************
 *		AVIFILE_DllMain
 */
BOOL WINAPI AVIFILE_DllMain(
	HINSTANCE hInstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved )
{
	switch ( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
		if ( !AVIFILE_InitProcess() )
			return FALSE;
		break;
	case DLL_PROCESS_DETACH:
		AVIFILE_UninitProcess();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}


