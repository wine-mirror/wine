#ifndef QUARTZ_AUDIOUTL_H
#define QUARTZ_AUDIOUTL_H


void AUDIOUTL_ChangeSign8( BYTE* pbData, DWORD cbData );
void AUDIOUTL_ChangeSign16LE( BYTE* pbData, DWORD cbData );
void AUDIOUTL_ChangeSign16BE( BYTE* pbData, DWORD cbData );
void AUDIOUTL_ByteSwap( BYTE* pbData, DWORD cbData );


#endif  /* QUARTZ_AUDIOUTL_H */
