#ifndef MICROPY_INCLUDED_DRIVERS_I2S_MP3_DECODER_H
#define MICROPY_INCLUDED_DRIVERS_I2S_MP3_DECODER_H

#include "stdbool.h"

enum {
    ERR_MP3_NONE =                  0,
    ERR_MP3_INDATA_UNDERFLOW =     -1,
    ERR_MP3_MAINDATA_UNDERFLOW =   -2,
    ERR_MP3_FREE_BITRATE_SYNC =    -3,
    ERR_MP3_OUT_OF_MEMORY =        -4,
    ERR_MP3_NULL_POINTER =         -5,
    ERR_MP3_INVALID_FRAMEHEADER =  -6,
    ERR_MP3_INVALID_SIDEINFO =     -7,
    ERR_MP3_INVALID_SCALEFACT =    -8,
    ERR_MP3_INVALID_HUFFCODES =    -9,
    ERR_MP3_INVALID_DEQUANTIZE =   -10,
    ERR_MP3_INVALID_IMDCT =        -11,
    ERR_MP3_INVALID_SUBBAND =      -12,

    ERR_UNKNOWN =                  -9999
};

// prototypes
bool MP3Decoder_AllocateBuffers(void);
void MP3Decoder_FreeBuffers();
int  MP3Decode( unsigned char *inbuf, int *bytesLeft, short *outbuf, int useSize);
void MP3GetLastFrameInfo();
int  MP3GetNextFrameInfo(unsigned char *buf);
int  MP3FindSyncWord(unsigned char *buf, int nBytes);
int  MP3GetSampRate();
int  MP3GetChannels();
int  MP3GetBitsPerSample();
int  MP3GetBitrate();
int  MP3GetOutputSamps();
int  MP3FrameLength();

#endif // MICROPY_INCLUDED_DRIVERS_I2S_MP3_DECODER_H
