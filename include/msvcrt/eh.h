/*
 * C++ exception handling facility
 *
 * Copyright 2000 Francois Gouget.
 */
#ifndef __WINE_EH_H
#define __WINE_EH_H
#define __WINE_USE_MSVCRT

#if !defined(__cplusplus) && !defined(__WINE__)
#error "eh.h is meant only for C++ applications"
#endif

#ifdef USE_MSVCRT_PREFIX
#define MSVCRT(x)    MSVCRT_##x
#else
#define MSVCRT(x)    x
#endif


typedef void (*terminate_handler)();
typedef void (*terminate_function)();
typedef void (*unexpected_handler)();
typedef void (*unexpected_function)();


terminate_function MSVCRT(set_terminate)(terminate_function func);
unexpected_function MSVCRT(set_unexpected)(unexpected_function func);
void        MSVCRT(terminate)();
void        MSVCRT(unexpected)();

#endif /* __WINE_EH_H */
