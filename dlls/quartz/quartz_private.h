#ifndef	QUARTZ_PRIVATE_H
#define	QUARTZ_PRIVATE_H

typedef HRESULT (*QUARTZ_pCreateIUnknown)(IUnknown* punkOuter,void** ppobj);

void* QUARTZ_AllocObj( DWORD dwSize );
void QUARTZ_FreeObj( void* pobj );
void* QUARTZ_AllocMem( DWORD dwSize );
void QUARTZ_FreeMem( void* pMem );

#endif	/* QUARTZ_PRIVATE_H */

