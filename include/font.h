#ifndef __WINE_FONT_H
#define __WINE_FONT_H

extern void Font_Init( void );
extern int FONT_GetObject( FONTOBJ * font, int count, LPSTR buffer );
extern HFONT FONT_SelectObject( DC * dc, HFONT hfont, FONTOBJ * font );

#endif /* __WINE_FONT_H */
