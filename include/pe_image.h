#ifndef __WINE_PE_IMAGE_H
#define __WINE_PE_IMAGE_H

extern int PE_unloadImage(struct w_files *wpnt);
extern int PE_StartProgram(struct w_files *wpnt);
extern void PE_InitDLL(struct w_files *wpnt);
extern HINSTANCE PE_LoadImage(struct w_files *wpnt);
extern void my_wcstombs(char * result, u_short * source, int len);

#endif /* __WINE_PE_IMAGE_H */
