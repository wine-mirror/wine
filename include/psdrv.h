/*
 *	Postscript driver definitions
 *
 *	Copyright 1998  Huw D M Davies
 */
#include "windows.h"
#include "font.h"

typedef struct {
    float	llx, lly, urx, ury;
} AFMBBOX;

typedef struct _tagAFMLIGS {
    char		*successor;
    char		*ligature;
    struct _tagAFMLIGS	*next;
} AFMLIGS;

typedef struct _tagAFMMETRICS {
    int				C;			/* character */  
    float			WX;
    char			*N;			/* name */
    AFMBBOX			B;
    AFMLIGS			*L;			/* Ligatures */
    struct _tagAFMMETRICS	*next;
} AFMMETRICS;

typedef struct _tagAFM {
    char		*FontName;
    char		*FullName;
    char		*FamilyName;
    int			Weight;			/* FW_NORMAL etc. */
    float		ItalicAngle;
    BOOL32		IsFixedPitch;
    float		UnderlinePosition;
    float		UnderlineThickness;
    AFMBBOX		FontBBox;
    float		CapHeight;
    float		XHeight;
    float		Ascender;
    float		Descender;
    float		CharWidths[256];
    int			NumofMetrics;
    AFMMETRICS		*Metrics;
    struct _tagAFM	*next;
} AFM; /* CharWidths is a shortcut to the WX values of numbered glyphs */

typedef struct _tagFontFamily {
    char			*FamilyName; /* family name */
    AFM				*afm;	     /* list of afms for this family */
    struct _tagFontFamily	*next;       /* next family */
} FontFamily;

extern FontFamily *PSDRV_AFMFontList;

typedef struct {
    AFM			*afm;
    TEXTMETRIC32A	tm;
    INT32		size;
    float		scale;
    INT32		escapement;
    BOOL32		set;		/* Have we done a setfont yet */
} PSFONT;

typedef struct {
    HANDLE16		hJob;
    LPSTR		output;		/* Output file/port */
    BOOL32              banding;        /* Have we received a NEXTBAND */
    BOOL32		NeedPageHeader; /* Page header not sent yet */
    INT32		PageNo;
} JOB;

typedef struct
{
    PSFONT		font;		/* Current PS font */
    JOB			job;
} PSDRV_PDEVICE;


extern BOOL32 PSDRV_GetFontMetrics(void);

extern BOOL32 PSDRV_Init(void);
extern HFONT16 PSDRV_FONT_SelectObject( DC *dc, HFONT16 hfont, FONTOBJ *font);
extern BOOL32 PSDRV_SetFont( DC *dc );

extern INT32 PSDRV_WriteHeader( DC *dc, char *title, int len );
extern INT32 PSDRV_WriteFooter( DC *dc );
extern INT32 PSDRV_WriteNewPage( DC *dc );
extern INT32 PSDRV_WriteEndPage( DC *dc );
extern BOOL32 PSDRV_WriteMoveTo(DC *dc, INT32 x, INT32 y);
extern BOOL32 PSDRV_WriteLineTo(DC *dc, INT32 x, INT32 y);
extern BOOL32 PSDRV_WriteStroke(DC *dc);
extern BOOL32 PSDRV_WriteRectangle(DC *dc, INT32 x, INT32 y, INT32 width, 
			INT32 height);
extern BOOL32 PSDRV_WriteSetFont(DC *dc);
extern BOOL32 PSDRV_WriteShow(DC *dc, char *str, INT32 count);
extern BOOL32 PSDRV_WriteReencodeFont(DC *dc);








extern BOOL32 PSDRV_EnumDeviceFonts( DC* dc, LPLOGFONT16 plf, 
				        DEVICEFONTENUMPROC proc, LPARAM lp );
extern INT32 PSDRV_Escape( DC *dc, INT32 nEscape, INT32 cbInput, 
                              SEGPTR lpInData, SEGPTR lpOutData );
extern BOOL32 PSDRV_ExtTextOut( DC *dc, INT32 x, INT32 y, UINT32 flags,
                   const RECT32 *lprect, LPCSTR str, UINT32 count,
                   const INT32 *lpDx );
extern BOOL32 PSDRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT32 count,
                                  LPSIZE32 size );
extern BOOL32 PSDRV_GetTextMetrics( DC *dc, TEXTMETRIC32A *metrics );
extern BOOL32 PSDRV_LineTo( DC *dc, INT32 x, INT32 y );
extern BOOL32 PSDRV_MoveToEx( DC *dc, INT32 x, INT32 y, LPPOINT32 pt );
extern HGDIOBJ32 PSDRV_SelectObject( DC *dc, HGDIOBJ32 handle );

extern BOOL32 PSDRV_Rectangle(DC *dc, INT32 left, INT32 top, INT32 right,
		       INT32 bottom);
extern BOOL32 PSDRV_Ellipse(DC *dc, INT32 left, INT32 top, INT32 right,
		       INT32 bottom);
