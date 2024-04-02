#ifndef OPENGL32_TEXCONV
#define OPENGL32_TEXCONV

#ifdef _WIN64
#define BCDEC_IMPLEMENTATION
#include "bcdec.h"
#undef BCDEC_IMPLEMENTATION
#endif

unsigned char get_btpc_convfmt( void );
void * decode_bptc_unorm_to_rgba( const void *data, int width, int height, int depth );

#ifdef OPENGL32_TEXCONV_IMPLEMENTATION
unsigned char get_btpc_convfmt()
{
    const char * const HOSTPTR restrict winegl_bptc = getenv( "WINEGL_BPTC" );

    if (winegl_bptc != 0)
    {
        if ( !strcasecmp( winegl_bptc, "NONE" ) || !strcasecmp( winegl_bptc, "FALSE") || !strcmp( winegl_bptc, "0" ) )
        {
            return 0;
        }
#ifdef _WIN64
        else if ( !strcasecmp( winegl_bptc, "RGBA" ) || !strcmp( winegl_bptc, "1" ) )
        {
            return 1;
        }
#endif
        else if ( !strcasecmp( winegl_bptc, "S3TC" ) || !strcmp( winegl_bptc, "2" ) )
        {
            return 2;
        }
    }

#if defined(__APPLE__) && defined(_WIN64)
    return 1;
#else
    return 0;
#endif
}

void * decode_bptc_unorm_to_rgba( const void *data, int width, int height, int depth )
{
#ifdef _WIN64
    if ( data == 0 )
    {
        return 0;
    }

    const int numBlocksX = (width + 3) / 4;
    const int numBlocksY = (height + 3) / 4;
    const int numBlocksZ = (depth + 3) / 4;

    char * restrict pixels = malloc( width * height * depth * 4 );

    for ( int z = 0; z < numBlocksZ; z++ )
    {
        for ( int y = 0; y < numBlocksY; y++ )
        {
            for ( int x = 0; x < numBlocksX; x++ )
            {
                const int blockOffsetX = x * 4;
                const int blockOffsetY = y * 4;
                const int blockOffsetZ = z;
                const int blockOffset = blockOffsetZ * numBlocksX * numBlocksY * 4 + blockOffsetY * numBlocksX * 4 + blockOffsetX * 4;

                const int pixelOffsetX = x * 4;
                const int pixelOffsetY = y * 4;
                const int pixelOffsetZ = z * width * height * 4;
                const int pixelOffset = pixelOffsetZ + pixelOffsetY * width * 4 + pixelOffsetX * 4;

                bcdec_bc7( (const void*)((const char*)data + blockOffset), (void *)(pixels + pixelOffset), width * 4 );
            }
        }
    }

    return pixels;
#else
    return 0; // wine32on64 not supported
#endif
}
#endif // OPENGL32_TEXCONV_IMPLEMENTATION

#endif // OPENGL32_TEXCONV
