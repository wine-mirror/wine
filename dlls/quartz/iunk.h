/*
 * An implementation of IUnknown.
 *
 * hidenori@a2.ctktv.ne.jp
 */

#ifndef	WINE_DSHOW_IUNK_H
#define	WINE_DSHOW_IUNK_H

/*
	To avoid implementing IUnknown for all interfaces,

  1) To give a method to get rel-offset of IUnknown.
  2) The IUnknown knows all IIDs and offsets of interfaces.

    So each implementation must have following two members
    with the following order:

  typedef struct IDispatchImpl
  {
    ICOM_VFIELD(IDispatch);	<-pointer of the interface.
    size_t	ofsIUnknown;	<-ofs<IDispatchImpl> - ofs<QUARTZ_IUnkImpl>
  };

 */

typedef struct QUARTZ_IFEntry
{
	REFIID		piid;		/* interface ID. */
	size_t		ofsVTPtr;	/* offset from IUnknown. */
} QUARTZ_IFEntry;

typedef struct QUARTZ_IUnkImpl
{
	/* pointer of IUnknown interface. */
	ICOM_VFIELD(IUnknown);

	/* array of supported IIDs and offsets. */
	const QUARTZ_IFEntry*	pEntries;
	DWORD	dwEntries;

	/* IUnknown fields. */
	ULONG	ref;
	IUnknown*	punkControl;
} QUARTZ_IUnkImpl;


void QUARTZ_IUnkInit( QUARTZ_IUnkImpl* pImpl, IUnknown* punkOuter );


#endif	/* WINE_DSHOW_IUNK_H */
