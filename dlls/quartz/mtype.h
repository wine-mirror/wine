/*
 * Implements IEnumMediaTypes and helper functions. (internal)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#ifndef WINE_DSHOW_MTYPE_H
#define WINE_DSHOW_MTYPE_H

HRESULT QUARTZ_MediaType_Copy(
	AM_MEDIA_TYPE* pmtDst,
	const AM_MEDIA_TYPE* pmtSrc );
void QUARTZ_MediaType_Free(
	AM_MEDIA_TYPE* pmt );
AM_MEDIA_TYPE* QUARTZ_MediaType_Duplicate(
	const AM_MEDIA_TYPE* pmtSrc );
void QUARTZ_MediaType_Destroy(
	AM_MEDIA_TYPE* pmt );

void QUARTZ_MediaSubType_FromFourCC(
	GUID* psubtype, DWORD dwFourCC );
BOOL QUARTZ_MediaSubType_IsFourCC(
	const GUID* psubtype );

HRESULT QUARTZ_MediaSubType_FromBitmap(
	GUID* psubtype, const BITMAPINFOHEADER* pbi );

HRESULT QUARTZ_CreateEnumMediaTypes(
	IEnumMediaTypes** ppobj,
	const AM_MEDIA_TYPE* pTypes, ULONG cTypes );


#endif /* WINE_DSHOW_MTYPE_H */
