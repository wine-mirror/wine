#ifndef	WINE_DSHOW_SAMPLE_H
#define	WINE_DSHOW_SAMPLE_H

/*
		implements CMemMediaSample.

	- At least, the following interfaces should be implemented:

	IUnknown - IMediaSample - IMediaSample2
 */

typedef struct CMemMediaSample
{
	ICOM_VFIELD(IMediaSample2);

	/* IUnknown fields */
	ULONG	ref;
	/* IMediaSample2 fields */
	IMemAllocator*	pOwner; /* not addref-ed. */
	BOOL	fMediaTimeIsValid;
	LONGLONG	llMediaTimeStart;
	LONGLONG	llMediaTimeEnd;
	AM_SAMPLE2_PROPERTIES	prop;
} CMemMediaSample;


HRESULT QUARTZ_CreateMemMediaSample(
	BYTE* pbData, DWORD dwDataLength,
	IMemAllocator* pOwner,
	CMemMediaSample** ppSample );
void QUARTZ_DestroyMemMediaSample(
	CMemMediaSample* pSample );

#endif	/* WINE_DSHOW_SAMPLE_H */
