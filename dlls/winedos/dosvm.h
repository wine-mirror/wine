/*
 * DOS virtual machine
 *
 * Copyright 2000 Ove Kåven
 */

#ifndef __WINE_DOSVM_H
#define __WINE_DOSVM_H

#include <sys/types.h> /* pid_t */

typedef struct {
  PAPCFUNC proc;
  ULONG_PTR arg;
} DOS_SPC;

extern pid_t dosvm_pid;

#endif /* __WINE_DOSVM_H */
