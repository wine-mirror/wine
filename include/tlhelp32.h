#ifndef __WINE_TLHELP32_H
#define __WINE_TLHELP32_H

#include "windef.h"

#ifdef __cplusplus
extern "C" {
#endif

  /*===================================================================
   *  Arguments for Toolhelp routines
   */

  /*
   * CreateToolhelp32Snapshot
   */

#define TH32CS_SNAPHEAPLIST 0x00000001
#define TH32CS_SNAPPROCESS  0x00000002
#define TH32CS_SNAPTHREAD   0x00000004
#define TH32CS_SNAPMODULE   0x00000008
#define TH32CS_SNAPALL     (TH32CS_SNAPHEAPLIST | TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD | TH32CS_SNAPMODULE)
#define TH32CS_INHERIT     0x80000000

#ifdef __cplusplus
extern "C" {
#endif

#endif /* __WINE_TLHELP32_H */
