/*
 *	MSRLE32 Driver
 *
 *	Copyright 2001 TAKESHIMA Hidenori <hidenori@a2.ctktv.ne.jp>
 *
 *	FIXME - encoding
 *	FIXME - RLE4
 *
 */

#include "config.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmsystem.h"
#include "vfw.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(msrle32);




typedef struct CodecImpl
{
	BOOL	bInCompress;
	BOOL	bInDecompress;
	BOOL	bInDecompressEx;
} CodecImpl;

/***********************************************************************/

static LONG MSRLE32_DecompressRLE4(
	BYTE* pDst, LONG pitch,
	const BYTE* pSrc,
	LONG width, LONG height )
{
	FIXME( "RLE4 - not implemented yet\n" );
	return ICERR_UNSUPPORTED;
}

static LONG MSRLE32_DecompressRLE8(
	BYTE* pDst, LONG pitch,
	const BYTE* pSrc,
	LONG width, LONG height )
{
	LONG	x,y;
	int		len;
	UINT	data;

	x = 0; y = 0;
	while ( y < height )
	{
		len = *pSrc++; data = *pSrc++;
		if ( len > 0 )
		{
			/* run length encoding */
			while ( len-- > 0 && x < width )
				pDst[x++] = (BYTE)data;
		}
		else
		{
			switch ( data )
			{
			case 0: /* EOL */
				x = 0; y++; pDst += pitch;
				break;
			case 1: /* END */
				return ICERR_OK;
			case 2: /* DELTA */
				x += (LONG)*pSrc++; y += (LONG)*pSrc++;
				break;
			default: /* RAW */
				len = data;
				if ( (len+x) > width )
					len = width-x;
				memcpy( &pDst[x], pSrc, len );
				x += len;
				pSrc += (data+1)&(~1);
				break;
			}
		}
	}

	return ICERR_OK;
}

/***********************************************************************/

static BOOL MSRLE32_IsValidInfoHeader( const BITMAPINFO* pbiIn )
{
	if ( pbiIn->bmiHeader.biSize < sizeof(BITMAPINFOHEADER) ||
		 pbiIn->bmiHeader.biWidth <= 0 ||
		 pbiIn->bmiHeader.biHeight == 0 ||
		 pbiIn->bmiHeader.biPlanes != 1 )
		return FALSE;

	return TRUE;
}

static DWORD MSRLE32_GetClrUsed( const BITMAPINFO* pbiIn )
{
	if ( pbiIn->bmiHeader.biBitCount > 8 )
		return 0;
	return (pbiIn->bmiHeader.biClrUsed == 0) ? (1<<pbiIn->bmiHeader.biBitCount) : pbiIn->bmiHeader.biClrUsed;
}

static DWORD MSRLE32_GetUncompressedPitch( const BITMAPINFOHEADER* pbiIn )
{
	return ((((pbiIn->biWidth*pbiIn->biBitCount)+7)>>3)+3)&(~3);
}

static DWORD MSRLE32_GetUncompressedSize( const BITMAPINFOHEADER* pbiIn )
{
	return MSRLE32_GetUncompressedPitch( pbiIn ) * abs(pbiIn->biHeight);
}


static BOOL MSRLE32_IsValidRGB( const BITMAPINFO* pbiIn )
{
	if ( !MSRLE32_IsValidInfoHeader( pbiIn ) )
		return FALSE;

	if ( pbiIn->bmiHeader.biSizeImage != 0 &&
		 pbiIn->bmiHeader.biSizeImage < MSRLE32_GetUncompressedSize( &pbiIn->bmiHeader ) )
		return FALSE;


	switch ( pbiIn->bmiHeader.biCompression )
	{
	case 0:
	case mmioFOURCC('R','G','B',' '):
		if ( pbiIn->bmiHeader.biBitCount == 1 ||
			 pbiIn->bmiHeader.biBitCount == 4 ||
			 pbiIn->bmiHeader.biBitCount == 8 ||
			 pbiIn->bmiHeader.biBitCount == 16 ||
			 pbiIn->bmiHeader.biBitCount == 24 ||
			 pbiIn->bmiHeader.biBitCount == 32 )
			return TRUE;
		break;
	default:
		break;
	}

	return FALSE;
}

static BOOL MSRLE32_IsValidRLE( const BITMAPINFO* pbiIn )
{
	if ( !MSRLE32_IsValidInfoHeader( pbiIn ) )
		return FALSE;

	switch ( pbiIn->bmiHeader.biCompression )
	{
	case 1:
	case 2:
	case mmioFOURCC('R','L','E',' '):
	case mmioFOURCC('R','L','E','8'):
	case mmioFOURCC('R','L','E','4'):
	case mmioFOURCC('M','R','L','E'):
		if ( pbiIn->bmiHeader.biBitCount == 8 ||
			 pbiIn->bmiHeader.biBitCount == 4 )
			return TRUE;
		break;
	default:
		break;
	}

	return FALSE;
}

static BOOL MSRLE32_CompareInfoHeader( const BITMAPINFO* pbiIn, const BITMAPINFO* pbiOut )
{
	if ( !MSRLE32_IsValidInfoHeader( pbiIn ) ||
		 !MSRLE32_IsValidInfoHeader( pbiOut ) )
		return FALSE;

	if ( pbiIn->bmiHeader.biWidth != pbiOut->bmiHeader.biWidth ||
		 pbiIn->bmiHeader.biHeight != pbiOut->bmiHeader.biHeight ||
		 pbiIn->bmiHeader.biPlanes != pbiOut->bmiHeader.biPlanes ||
		 pbiIn->bmiHeader.biBitCount != pbiOut->bmiHeader.biBitCount )
		return FALSE;

	return TRUE;
}


/***********************************************************************/

static LONG Codec_DrvQueryConfigure( CodecImpl* This )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_DrvConfigure( CodecImpl* This, HWND hwnd, DRVCONFIGINFO* pinfo )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_QueryAbout( void )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_About( HWND hwnd )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_CompressQuery( CodecImpl* This, BITMAPINFO* pbiIn, BITMAPINFO* pbiOut )
{
	FIXME( "compression is not implemented!\n" );
	return ICERR_UNSUPPORTED;
}

static LONG Codec_CompressBegin( CodecImpl* This, BITMAPINFO* pbiIn, BITMAPINFO* pbiOut )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_Compress( CodecImpl* This, ICCOMPRESS* picc, DWORD dwSize )
{
	FIXME( "compression is not implemented!\n" );
	return ICERR_UNSUPPORTED;
}

static LONG Codec_CompressEnd( CodecImpl* This )
{
	This->bInCompress = FALSE;
	return ICERR_UNSUPPORTED;
}

static LONG Codec_CompressFramesInfo( CodecImpl* This, ICCOMPRESSFRAMES* piccf, DWORD dwSize )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_CompressGetFormat( CodecImpl* This, BITMAPINFO* pbiIn, BITMAPINFO* pbiOut )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_CompressGetSize( CodecImpl* This, BITMAPINFO* pbiIn, BITMAPINFO* pbiOut )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_ICQueryConfigure( CodecImpl* This )
{
	return ICERR_OK;
}
static LONG Codec_ICConfigure( CodecImpl* This, HWND hwnd )
{
	MessageBoxA( hwnd, "Wine RLE Driver", "MSRLE32", MB_OK );
	return ICERR_OK;
}

static LONG Codec_DecompressQuery( CodecImpl* This, BITMAPINFO* pbiIn, BITMAPINFO* pbiOut )
{
	if ( pbiIn == NULL )
		return ICERR_BADPARAM;
	if ( !MSRLE32_IsValidRLE( pbiIn ) )
		return ICERR_UNSUPPORTED;

	if ( pbiOut != NULL )
	{
		if ( !MSRLE32_IsValidRGB( pbiOut ) )
			return ICERR_UNSUPPORTED;
		if ( !MSRLE32_CompareInfoHeader( pbiIn, pbiOut ) )
			return ICERR_UNSUPPORTED;
	}

	return ICERR_OK;
}

static LONG Codec_DecompressBegin( CodecImpl* This, BITMAPINFO* pbiIn, BITMAPINFO* pbiOut )
{
	LONG	lr;

	if ( pbiIn == NULL || pbiOut == NULL )
		return ICERR_BADPARAM;
	lr = Codec_DecompressQuery( This, pbiIn, pbiOut );
	if ( lr != ICERR_OK )
		return lr;

	This->bInDecompress = TRUE;

	return ICERR_OK;
}

static LONG Codec_Decompress( CodecImpl* This, ICDECOMPRESS* picd, DWORD dwSize )
{
	LONG	lr;

	if ( !This->bInDecompress )
		return ICERR_BADHANDLE; /* FIXME? */

	if ( ( picd->dwFlags & ICDECOMPRESS_NOTKEYFRAME ) &&
		 ( picd->dwFlags & ICDECOMPRESS_UPDATE ) )
		return ICERR_CANTUPDATE; /* FIXME? */

	if ( picd->lpbiInput == NULL ||
		 picd->lpInput == NULL ||
		 picd->lpbiOutput == NULL ||
		 picd->lpOutput == NULL )
		return ICERR_BADPARAM;

	switch ( picd->lpbiInput->biBitCount )
	{
	case 4: /* RLE4 */
		lr = MSRLE32_DecompressRLE4(
			(BYTE*)picd->lpOutput,
			MSRLE32_GetUncompressedPitch( picd->lpbiInput ),
			(const BYTE*)picd->lpInput,
			picd->lpbiInput->biWidth,
			picd->lpbiInput->biHeight );
		break;
	case 8: /* RLE8 */
		lr = MSRLE32_DecompressRLE8(
			(BYTE*)picd->lpOutput,
			MSRLE32_GetUncompressedPitch( picd->lpbiInput ),
			(const BYTE*)picd->lpInput,
			picd->lpbiInput->biWidth,
			picd->lpbiInput->biHeight );
		break;
	default:
		lr = ICERR_BADBITDEPTH;
		break;
	}

	return lr;
}

static LONG Codec_DecompressEnd( CodecImpl* This )
{
	This->bInDecompress = FALSE;
	return ICERR_OK;
}

static LONG Codec_DecompressGetFormat( CodecImpl* This, BITMAPINFO* pbiIn, BITMAPINFO* pbiOut )
{
	DWORD	biClrUsed;
	LONG	lr;

	if ( pbiIn == NULL )
		return ICERR_BADPARAM;

	lr = Codec_DecompressQuery( This, pbiIn, NULL );
	if ( lr != ICERR_OK )
		return ( pbiOut == NULL ) ? 0 : lr;

	biClrUsed = MSRLE32_GetClrUsed(pbiIn);
	if ( pbiOut == NULL )
		return sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * biClrUsed;

	ZeroMemory( pbiOut, sizeof(BITMAPINFOHEADER) );
	memcpy( &pbiOut->bmiHeader, &pbiIn->bmiHeader, sizeof(BITMAPINFOHEADER) );
	memcpy( &pbiOut->bmiColors, &pbiIn->bmiColors, sizeof(RGBQUAD) * biClrUsed );
	pbiOut->bmiHeader.biCompression = 0;
	pbiOut->bmiHeader.biSizeImage = MSRLE32_GetUncompressedSize( &pbiOut->bmiHeader );

	return ICERR_OK;
}

static LONG Codec_DecompressGetPalette( CodecImpl* This, BITMAPINFO* pbiIn, BITMAPINFO* pbiOut )
{
	if ( pbiIn == NULL )
		return ICERR_BADPARAM;

	return (pbiOut == NULL) ? 0 : ICERR_UNSUPPORTED;
}

static LONG Codec_DecompressSetPalette( CodecImpl* This, BITMAPINFO* pbiIn )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_DecompressExQuery( CodecImpl* This, ICDECOMPRESSEX* picdex, DWORD dwSize )
{
	FIXME( "DecompressEx is not implemented!\n" );
	return ICERR_UNSUPPORTED;
}

static LONG Codec_DecompressExBegin( CodecImpl* This, ICDECOMPRESSEX* picdex, DWORD dwSize )
{
	FIXME( "DecompressEx is not implemented!\n" );
	return ICERR_UNSUPPORTED;
}

static LONG Codec_DecompressEx( CodecImpl* This, ICDECOMPRESSEX* picdex, DWORD dwSize )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_DecompressExEnd( CodecImpl* This )
{
	This->bInDecompressEx = FALSE;
	return ICERR_UNSUPPORTED;
}

static LONG Codec_GetInfo( CodecImpl* This, ICINFO* pici, DWORD dwSize )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_GetQuality( CodecImpl* This, DWORD* pdwQuality )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_GetState( CodecImpl* This, LPVOID pvState, DWORD dwSize )
{
	if ( pvState == NULL )
		return 0;

	/* no driver-specific state */

	return 0;
}

static LONG Codec_SetQuality( CodecImpl* This, DWORD* pdwQuality )
{
	return ICERR_UNSUPPORTED;
}

static LONG Codec_SetStatusProc(CodecImpl* This, ICSETSTATUSPROC* picssp, DWORD dwSize )
{
	if ( picssp == NULL )
		return 0;

	/* no driver-specific state */

	return 0;
}

static LONG Codec_SetState( CodecImpl* This, LPVOID pvState, DWORD dwSize )
{
	return ICERR_UNSUPPORTED;
}

static CodecImpl* Codec_AllocDriver( void )
{
	CodecImpl*	This;

	This = HeapAlloc( GetProcessHeap(), 0, sizeof(CodecImpl) );
	if ( This == NULL )
		return NULL;
	ZeroMemory( This, sizeof(CodecImpl) );
	This->bInCompress = FALSE;
	This->bInDecompress = FALSE;
	This->bInDecompressEx = FALSE;

	return This;
}

static void Codec_Close( CodecImpl* This )
{
	if ( This->bInCompress )
		Codec_CompressEnd(This);
	if ( This->bInDecompress )
		Codec_DecompressEnd(This);
	if ( This->bInDecompressEx )
		Codec_DecompressExEnd(This);

	HeapFree( GetProcessHeap(), 0, This );
}






LONG APIENTRY MSRLE32_DriverProc(
	DWORD dwDriverId, HDRVR hdrvr, UINT msg, LONG lParam1, LONG lParam2 )
{
	TRACE( "DriverProc(%08lx,%08x,%08x,%08lx,%08lx)\n",
			 dwDriverId, hdrvr, msg, lParam1, lParam2 );

	switch ( msg )
	{
	case DRV_LOAD:
		TRACE("DRV_LOAD\n");
		return TRUE;
	case DRV_FREE:
		TRACE("DRV_FREE\n");
		return TRUE;
	case DRV_OPEN:
		TRACE("DRV_OPEN\n");
		return (LONG)Codec_AllocDriver();
	case DRV_CLOSE:
		TRACE("DRV_CLOSE\n");
		Codec_Close( (CodecImpl*)dwDriverId );
		return TRUE;
	case DRV_ENABLE:
		TRACE("DRV_ENABLE\n");
		return TRUE;
	case DRV_DISABLE:
		TRACE("DRV_DISABLE\n");
		return TRUE;
	case DRV_QUERYCONFIGURE:
		TRACE("DRV_QUERYCONFIGURE\n");
		return Codec_DrvQueryConfigure( (CodecImpl*)dwDriverId );
	case DRV_CONFIGURE:
		TRACE("DRV_CONFIGURE\n");
		return Codec_DrvConfigure( (CodecImpl*)dwDriverId,
					(HWND)lParam1, (DRVCONFIGINFO*)lParam2 );
	case DRV_INSTALL:
		TRACE("DRV_INSTALL\n");
		return DRVCNF_OK;
	case DRV_REMOVE:
		TRACE("DRV_REMOVE\n");
		return 0;
	case DRV_POWER:
		TRACE("DRV_POWER\n");
		return TRUE;

	case ICM_ABOUT:
		TRACE("ICM_ABOUT\n");
		return (lParam1 == -1) ? Codec_QueryAbout() : Codec_About( (HWND)lParam1 );
	case ICM_COMPRESS:
		TRACE("ICM_COMPRESS\n");
		return Codec_Compress((CodecImpl*)dwDriverId,(ICCOMPRESS*)lParam1,(DWORD)lParam2);
	case ICM_COMPRESS_BEGIN:
		TRACE("ICM_COMPRESS_BEGIN\n");
		return Codec_CompressBegin((CodecImpl*)dwDriverId,(BITMAPINFO*)lParam1,(BITMAPINFO*)lParam2);
	case ICM_COMPRESS_END:
		TRACE("ICM_COMPRESS_END\n");
		return Codec_CompressEnd((CodecImpl*)dwDriverId);
	case ICM_COMPRESS_FRAMES_INFO:
		TRACE("ICM_COMPRESS_FRAMES_INFO\n");
		return Codec_CompressFramesInfo((CodecImpl*)dwDriverId,(ICCOMPRESSFRAMES*)lParam1,(DWORD)lParam2);
	case ICM_COMPRESS_GET_FORMAT:
		TRACE("ICM_COMPRESS_GET_FORMAT\n");
		return Codec_CompressGetFormat((CodecImpl*)dwDriverId,(BITMAPINFO*)lParam1,(BITMAPINFO*)lParam2);
	case ICM_COMPRESS_GET_SIZE:
		TRACE("ICM_COMPRESS_GET_SIZE\n");
		return Codec_CompressGetSize((CodecImpl*)dwDriverId,(BITMAPINFO*)lParam1,(BITMAPINFO*)lParam2);
	case ICM_COMPRESS_QUERY:
		TRACE("ICM_COMPRESS_GET_SIZE\n");
		return Codec_CompressQuery((CodecImpl*)dwDriverId,(BITMAPINFO*)lParam1,(BITMAPINFO*)lParam2);

	case ICM_CONFIGURE:
		TRACE("ICM_CONFIGURE\n");
		return ( lParam1 == -1 ) ? Codec_ICQueryConfigure( (CodecImpl*)dwDriverId ) : Codec_ICConfigure( (CodecImpl*)dwDriverId, (HWND)lParam1 );

	case ICM_DECOMPRESS:
		TRACE( "ICM_DECOMPRESS\n" );
		return Codec_Decompress((CodecImpl*)dwDriverId,(ICDECOMPRESS*)lParam1,(DWORD)lParam2);
	case ICM_DECOMPRESS_BEGIN:
		TRACE( "ICM_DECOMPRESS_BEGIN\n" );
		return Codec_DecompressBegin((CodecImpl*)dwDriverId,(BITMAPINFO*)lParam1,(BITMAPINFO*)lParam2);
	case ICM_DECOMPRESS_END:
		TRACE( "ICM_DECOMPRESS_END\n" );
		return Codec_DecompressEnd((CodecImpl*)dwDriverId);
	case ICM_DECOMPRESS_GET_FORMAT:
		TRACE( "ICM_DECOMPRESS_GET_FORMAT\n" );
		return Codec_DecompressGetFormat((CodecImpl*)dwDriverId,(BITMAPINFO*)lParam1,(BITMAPINFO*)lParam2);
	case ICM_DECOMPRESS_GET_PALETTE:
		TRACE( "ICM_DECOMPRESS_GET_PALETTE\n" );
		return Codec_DecompressGetPalette((CodecImpl*)dwDriverId,(BITMAPINFO*)lParam1,(BITMAPINFO*)lParam2);
	case ICM_DECOMPRESS_QUERY:
		TRACE( "ICM_DECOMPRESS_QUERY\n" );
		return Codec_DecompressQuery((CodecImpl*)dwDriverId,(BITMAPINFO*)lParam1,(BITMAPINFO*)lParam2);
	case ICM_DECOMPRESS_SET_PALETTE:
		TRACE( "ICM_DECOMPRESS_SET_PALETTE\n" );
		return Codec_DecompressSetPalette((CodecImpl*)dwDriverId,(BITMAPINFO*)lParam1);

	case ICM_DECOMPRESSEX_BEGIN:
		TRACE( "ICM_DECOMPRESSEX_BEGIN\n" );
		return Codec_DecompressExBegin((CodecImpl*)dwDriverId,(ICDECOMPRESSEX*)lParam1,(DWORD)lParam2);
	case ICM_DECOMPRESSEX:
		TRACE( "ICM_DECOMPRESSEX\n" );
		return Codec_DecompressEx((CodecImpl*)dwDriverId,(ICDECOMPRESSEX*)lParam1,(DWORD)lParam2);
	case ICM_DECOMPRESSEX_END:
		TRACE( "ICM_DECOMPRESSEX_END\n" );
		return Codec_DecompressExEnd((CodecImpl*)dwDriverId);
	case ICM_DECOMPRESSEX_QUERY:
		TRACE( "ICM_DECOMPRESSEX_QUERY\n" );
		return Codec_DecompressExQuery((CodecImpl*)dwDriverId,(ICDECOMPRESSEX*)lParam1,(DWORD)lParam2);

	case ICM_GETINFO:
		TRACE( "ICM_GETINFO\n" );
		return Codec_GetInfo((CodecImpl*)dwDriverId,(ICINFO*)lParam1,(DWORD)lParam2);
	case ICM_GETQUALITY:
		TRACE( "ICM_SETQUALITY\n");
		return Codec_GetQuality((CodecImpl*)dwDriverId,(DWORD*)lParam1);
	case ICM_GETSTATE:
		TRACE( "ICM_GETSTATE\n" );
		return Codec_GetState((CodecImpl*)dwDriverId,(LPVOID)lParam1,(DWORD)lParam2);
	case ICM_SETQUALITY:
		TRACE( "ICM_SETQUALITY\n");
		return Codec_SetQuality((CodecImpl*)dwDriverId,(DWORD*)lParam1);
	case ICM_SET_STATUS_PROC:
		TRACE( "ICM_SET_STATUS_PROC\n" );
		return Codec_SetStatusProc((CodecImpl*)dwDriverId,(ICSETSTATUSPROC*)lParam1,(DWORD)lParam2);
	case ICM_SETSTATE:
		TRACE( "ICM_SETSTATE\n" );
		return Codec_SetState((CodecImpl*)dwDriverId,(LPVOID)lParam1,(DWORD)lParam2);

	}

	return DefDriverProc( dwDriverId, hdrvr, msg, lParam1, lParam2 );
}

BOOL APIENTRY MSRLE32_DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved )
{
	TRACE( "(%08x,%08lx,%p)\n",hInst,dwReason,lpvReserved );

	return TRUE;
}
