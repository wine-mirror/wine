#ifndef __WINE_WINVERSION_H
#define __WINE_WINVERSION_H

typedef enum
{
    WIN31, /* Windows 3.1 */
    WIN95,   /* Windows 95 */
    NT351,   /* Windows NT 3.51 */
    NT40,    /* Windows NT 4.0 */  
    NB_WINDOWS_VERSIONS
} WINDOWS_VERSION;

extern WINDOWS_VERSION VERSION_GetVersion(void); 
extern char *VERSION_GetVersionName(void); 
extern BOOL VERSION_OsIsUnicode(void);

#endif  /* __WINE_WINVERSION_H */
