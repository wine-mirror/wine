#ifndef __WINE_AVIFILE_PRIVATE_H
#define __WINE_AVIFILE_PRIVATE_H

typedef struct
{
	HANDLE	hHeap;
	DWORD	dwAVIFileRef;
	DWORD	dwClassObjRef;
	BOOL	fInitCOM;
} WINE_AVIFILE_DATA;

extern WINE_AVIFILE_DATA	AVIFILE_data;

INT AVIFILE_strlenAtoW( LPCSTR lpstr );
INT AVIFILE_strlenWtoA( LPCWSTR lpwstr );
LPWSTR AVIFILE_strncpyAtoW( LPWSTR lpwstr, LPCSTR lpstr, INT wbuflen );
LPSTR AVIFILE_strncpyWtoA( LPSTR lpstr, LPCWSTR lpwstr, INT abuflen );
LPWSTR AVIFILE_strdupAtoW( LPCSTR lpstr );
LPSTR AVIFILE_strdupWtoA( LPCWSTR lpwstr );

HRESULT WINAPI AVIFILE_DllGetClassObject(const CLSID* pclsid,const IID* piid,void** ppv);

HRESULT AVIFILE_CreateIAVIFile(void** ppobj);
HRESULT AVIFILE_IAVIFile_Open( PAVIFILE paf, LPCWSTR szFile, UINT uMode );
HRESULT AVIFILE_IAVIFile_GetIndexTable( PAVIFILE paf, DWORD dwStreamIndex,
					AVIINDEXENTRY** ppIndexEntry,
					DWORD* pdwCountOfIndexEntry );
HRESULT AVIFILE_IAVIFile_ReadMovieData( PAVIFILE paf, DWORD dwOffset,
					DWORD dwLength, LPVOID lpvBuf );

HRESULT AVIFILE_CreateIAVIStream(void** ppobj);

HRESULT AVIFILE_CreateIGetFrame(void** ppobj,
				IAVIStream* pstr,LPBITMAPINFOHEADER lpbi);


typedef struct
{
	DWORD	dwStreamIndex;
	AVIStreamHeader*	pstrhdr;
	BYTE*	pbFmt;
	DWORD	dwFmtLen;
} WINE_AVISTREAM_DATA;

WINE_AVISTREAM_DATA* AVIFILE_Alloc_IAVIStreamData( DWORD dwFmtLen );
void AVIFILE_Free_IAVIStreamData( WINE_AVISTREAM_DATA* pData );

/* this should be moved to vfw.h */
#ifndef FIND_DIR
#define FIND_DIR	0x0000000FL
#define FIND_NEXT	0x00000001L
#define FIND_PREV	0x00000004L
#define FIND_FROM_START	0x00000008L

#define FIND_TYPE	0x000000F0L
#define FIND_KEY	0x00000010L
#define FIND_ANY	0x00000020L
#define FIND_FORMAT	0x00000040L

#define FIND_RET	0x0000F000L
#define FIND_POS	0x00000000L
#define FIND_LENGTH	0x00001000L
#define FIND_OFFSET	0x00002000L
#define FIND_SIZE	0x00003000L
#define FIND_INDEX	0x00004000L
#endif

#endif  /* __WINE_AVIFILE_PRIVATE_H */
