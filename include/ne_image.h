#ifndef __WINE_NE_IMAGE_H
#define __WINE_NE_IMAGE_H

extern int NE_FixupSegment(struct w_files *wpnt, int segment_num);
extern int NE_unloadImage(struct w_files *wpnt);
extern int NE_StartProgram( HMODULE hModule );
extern BOOL NE_InitDLL( HMODULE hModule );
extern HINSTANCE NE_LoadImage(struct w_files *wpnt);

#endif /* __WINE_NE_IMAGE_H */
