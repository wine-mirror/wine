#ifndef __WINE_NE_IMAGE_H
#define __WINE_NE_IMAGE_H

extern int NE_FixupSegment(struct w_files *wpnt, int segment_num);
extern int NE_unloadImage(struct w_files *wpnt);
extern int NE_StartProgram(struct w_files *wpnt);
extern void NE_InitDLL(struct w_files *wpnt);
extern HINSTANCE NE_LoadImage(struct w_files *wpnt);

#endif /* __WINE_NE_IMAGE_H */
