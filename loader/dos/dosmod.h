#ifndef __WINE_DOSMOD_H
#define __WINE_DOSMOD_H

#define DOSMOD_ENTER     0x01 /* VM86_ENTER */
#define DOSMOD_SET_TIMER 0x10
#define DOSMOD_GET_TIMER 0x11
#define DOSMOD_MPROTECT  0x12
#define DOSMOD_ENTERIDLE 0x13
#define DOSMOD_LEAVEIDLE 0x14

#define DOSMOD_SIGNAL    0x00 /* VM86_SIGNAL */
#define DOSMOD_LEFTIDLE  0x10

typedef struct {
  void *addr;
  size_t len;
  int prot;
} mprot_info;

#endif
