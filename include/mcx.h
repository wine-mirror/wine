#ifndef __WINE_MCX_H
#define __WINE_MCX_H

typedef struct tagMODEMDEVCAPS {
       DWORD dwActualSize;
       DWORD dwRequiredSize;
       DWORD dwDevSpecificOffset;
       DWORD dwDevSpecificSize;
       DWORD dwModemProviderVersion;
       DWORD dwModemManufacturerOffset;
       DWORD dwModemManufacturerSize;
       DWORD dwModemModelOffset;
       DWORD dwModemModelSize;
       DWORD dwModemVersionOffset;
       DWORD dwModemVersionSize;
       DWORD dwDialOptions;
       DWORD dwCallSetupFailTimer;
       DWORD dwInactivityTimeout;
       DWORD dwSpeakerVolume;
       DWORD dwSpeakerMode;
       DWORD dwModemoptions;
       DWORD dwMaxDTERate;
       DWORD dwMaxDCERate;
       BYTE  abVariablePortion[1];
} MODEMDEVCAPS, *LPMODEMDEVCAPS;

typedef struct tagMODEMSETTINGS {
       DWORD dwActualSize;
       DWORD dwRequiredSize;
       DWORD dwDevSpecificOffset;
       DWORD dwDevSpecificSize;
       DWORD dwCallSetupFailTimer;
       DWORD dwInactivityTimeout;
       DWORD dwSpeakerVolume;
       DWORD dwSpeakerMode;
       DWORD dwPreferedModemOptions;
       DWORD dwNegotiatedModemOptions;
       DWORD dwNegotiatedDCERate;
       BYTE  abVariablePortion[1];
} MODEMSETTINGS, *LPMODEMSETTINGS;

#endif  /* __WINE_MCX_H */
