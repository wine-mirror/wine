/* Includefile for the decompression library, lzexpand
 *
 * Copyright 1996 Marcus Meissner
 */

LONG	LZCopy(HFILE,HFILE);
HFILE	LZOpenFile(LPCSTR,LPOFSTRUCT,UINT);
HFILE	LZInit(HFILE);
LONG	LZSeek(HFILE,LONG,INT);
INT	LZRead(HFILE,SEGPTR,WORD); 
void	LZClose(HFILE);
INT	LZStart(void);
LONG	CopyLZFile(HFILE,HFILE);
void	LZDone(void);
INT	GetExpandedName(LPCSTR,LPSTR);


#define LZERROR_BADINHANDLE	0xFFFF	/* -1 */
#define LZERROR_BADOUTHANDLE	0xFFFE	/* -2 */
#define LZERROR_READ		0xFFFD	/* -3 */
#define LZERROR_WRITE		0xFFFC	/* -4 */
#define LZERROR_GLOBALLOC	0xFFFB	/* -5 */
#define LZERROR_GLOBLOCK	0xFFFA	/* -6 */
#define LZERROR_BADVALUE	0xFFF9	/* -7 */
#define LZERROR_UNKNOWNALG	0xFFF8	/* -8 */
