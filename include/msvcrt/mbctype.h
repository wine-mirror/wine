/*
 * Multibyte char definitions
 *
 * Copyright 2001 Francois Gouget.
 */
#ifndef __WINE_MBCTYPE_H
#define __WINE_MBCTYPE_H
#define __WINE_USE_MSVCRT


#ifdef __cplusplus
extern "C" {
#endif

unsigned char* __p__mbctype(void);
#define _mbctype                   (__p__mbctype())

int         _getmbcp(void);
int         _ismbbalnum(unsigned int);
int         _ismbbalpha(unsigned int);
int         _ismbbgraph(unsigned int);
int         _ismbbkalnum(unsigned int);
int         _ismbbkana(unsigned int);
int         _ismbbkprint(unsigned int);
int         _ismbbkpunct(unsigned int);
int         _ismbblead(unsigned int);
int         _ismbbprint(unsigned int);
int         _ismbbpunct(unsigned int);
int         _ismbbtrail(unsigned int);
int         _ismbslead(const unsigned char*,const unsigned char*);
int         _ismbstrail(const unsigned char*,const unsigned char*);
int         _setmbcp(int);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_MBCTYPE_H */
