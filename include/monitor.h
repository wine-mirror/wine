/*
 * Monitor definitions
 *
 */

#ifndef __WINE_MONITOR_H
#define __WINE_MONITOR_H

struct tagMONITOR_DRIVER;

typedef struct tagMONITOR 
{
  struct tagMONITOR_DRIVER *pDriver;
  void                     *pDriverData;
} MONITOR;

typedef struct tagMONITOR_DRIVER {
  void (*pInitialize)(MONITOR *);
  void (*pFinalize)(MONITOR *);
  int  (*pGetWidth)(MONITOR *);
  int  (*pGetHeight)(MONITOR *);
  int  (*pGetDepth)(MONITOR *);
} MONITOR_DRIVER;

extern MONITOR MONITOR_PrimaryMonitor;

extern void MONITOR_Initialize(MONITOR *pMonitor);
extern void MONITOR_Finalize(MONITOR *pMonitor);
extern int MONITOR_GetWidth(MONITOR *pMonitor);
extern int MONITOR_GetHeight(MONITOR *pMonitor);
extern int MONITOR_GetDepth(MONITOR *pMonitor);

#endif /* __WINE_MONITOR_H */
