/* Includefile for the decompression library, lzexpand
 *
 * Copyright 1996 Marcus Meissner
 */

INT16      LZStart(void);
HFILE      LZInit(HFILE);
void       LZDone(void);
LONG       LZSeek(HFILE,LONG,INT32);
LONG       LZCopy(HFILE,HFILE);
void       LZClose(HFILE);
LONG       CopyLZFile(HFILE,HFILE);

INT16      GetExpandedName16(LPCSTR,LPSTR);
INT32      GetExpandedName32A(LPCSTR,LPSTR);
INT32      GetExpandedName32W(LPCWSTR,LPWSTR);
#define    GetExpandedName WINELIB_NAME_AW(GetExpandedName)
HFILE      LZOpenFile16(LPCSTR,LPOFSTRUCT,UINT16);
HFILE      LZOpenFile32A(LPCSTR,LPOFSTRUCT,UINT32);
HFILE      LZOpenFile32W(LPCWSTR,LPOFSTRUCT,UINT32);
#define    LZOpenFile WINELIB_NAME_AW(LZOpenFile)
INT16      LZRead16(HFILE,SEGPTR,UINT16); 
INT32      LZRead32(HFILE,LPVOID,UINT32); 
#define    LZRead WINELIB_NAME(LZRead)

#define LZERROR_BADINHANDLE	0xFFFF	/* -1 */
#define LZERROR_BADOUTHANDLE	0xFFFE	/* -2 */
#define LZERROR_READ		0xFFFD	/* -3 */
#define LZERROR_WRITE		0xFFFC	/* -4 */
#define LZERROR_GLOBALLOC	0xFFFB	/* -5 */
#define LZERROR_GLOBLOCK	0xFFFA	/* -6 */
#define LZERROR_BADVALUE	0xFFF9	/* -7 */
#define LZERROR_UNKNOWNALG	0xFFF8	/* -8 */
