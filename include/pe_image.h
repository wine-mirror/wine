#ifndef __WINE_PE_IMAGE_H
#define __WINE_PE_IMAGE_H

extern int PE_unloadImage(struct w_files *wpnt);
extern int PE_StartProgram(struct w_files *wpnt);
extern void PE_InitDLL(struct w_files *wpnt);
extern HINSTANCE PE_LoadImage(struct w_files *wpnt);
extern void my_wcstombs(char * result, u_short * source, int len);

typedef struct _WIN32_function{
    char *name;
    void *definition;
} WIN32_function;

typedef struct _WIN32_builtin{
    char *name;
    WIN32_function *functions;
    int size;
    struct _WIN32_builtin *next;
} WIN32_builtin;

extern WIN32_builtin *WIN32_builtin_list;

#endif /* __WINE_PE_IMAGE_H */
