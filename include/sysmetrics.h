/*
 * System metrics definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef SYSMETRICS_H
#define SYSMETRICS_H

#include "windows.h"


  /* Constant system metrics */
#define SYSMETRICS_CXVSCROLL         16
#define SYSMETRICS_CYHSCROLL         16
#define SYSMETRICS_CYCAPTION         20
#define SYSMETRICS_CXBORDER           1
#define SYSMETRICS_CYBORDER           1
#define SYSMETRICS_CXDLGFRAME         4
#define SYSMETRICS_CYDLGFRAME         4
#define SYSMETRICS_CYVTHUMB          16
#define SYSMETRICS_CXHTHUMB          16
#define SYSMETRICS_CXICON            32
#define SYSMETRICS_CYICON            32
#define SYSMETRICS_CXCURSOR          32
#define SYSMETRICS_CYCURSOR          32
#define SYSMETRICS_CYMENU            18
#define SYSMETRICS_CYVSCROLL         16
#define SYSMETRICS_CXHSCROLL         16
#define SYSMETRICS_CXMIN            100
#define SYSMETRICS_CYMIN             28
#define SYSMETRICS_CXSIZE            18
#define SYSMETRICS_CYSIZE            18
#define SYSMETRICS_CXMINTRACK       100
#define SYSMETRICS_CYMINTRACK        28
#define SYSMETRICS_CXICONSPACING     20
#define SYSMETRICS_CYICONSPACING     20

  /* Some non-constant system metrics */
#define SYSMETRICS_CXSCREEN            sysMetrics[SM_CXSCREEN]
#define SYSMETRICS_CYSCREEN            sysMetrics[SM_CYSCREEN]
#define SYSMETRICS_CXFULLSCREEN        sysMetrics[SM_CXFULLSCREEN]
#define SYSMETRICS_CYFULLSCREEN        sysMetrics[SM_CYFULLSCREEN]
#define SYSMETRICS_SWAPBUTTON          sysMetrics[SM_SWAPBUTTON]
#define SYSMETRICS_CXFRAME             sysMetrics[SM_CXFRAME]
#define SYSMETRICS_CYFRAME             sysMetrics[SM_CYFRAME]
#define SYSMETRICS_CXDOUBLECLK         sysMetrics[SM_CXDOUBLECLK]
#define SYSMETRICS_CYDOUBLECLK         sysMetrics[SM_CYDOUBLECLK]
#define SYSMETRICS_MENUDROPALIGNMENT   sysMetrics[SM_MENUDROPALIGNMENT]


extern short sysMetrics[SM_CMETRICS];


#endif
