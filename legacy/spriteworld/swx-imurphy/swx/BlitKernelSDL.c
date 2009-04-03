/*
    BlitKernel - SDL functions

    Copyright (C) 2005 Anders F Bjoerklund <afb@users.sourceforge.net>
    
    This software is provided 'as-is', without any express or implied
    warranty. In no event will the authors be held liable for any
    damages arising from the use of this software.
    
    Permission is granted to anyone to use this software for any
    purpose, including commercial applications, and to alter it and
    redistribute it freely, subject to the following restrictions:
    
    1. The origin of this software must not be misrepresented; you must
       not claim that you wrote the original software. If you use this
       software in a product, an acknowledgment in the product
       documentation would be appreciated but is not required.
    
    2. Altered source versions must be plainly marked as such, and must
       not be misrepresented as being the original software.
    
    3. This notice may not be removed or altered from any source
       distribution.
*/

#include <stdlib.h>
#include <string.h>

#include "BlitKernel.h"

#ifndef BK_ASSERT
#include <assert.h>
#define BK_ASSERT(c)    assert(c)
#endif

#if macintosh
#include "SDL_endian.h"
#else
#include <SDL/SDL_endian.h>
#endif  

#include "SDL_surface.h"

#ifdef HAVE_ZZIP
//#include "SDL_rwops_zzip.h"

extern DECLSPEC SDL_RWops *SDL_RWFromZZIP(const char* file, const char* mode);
#endif

#ifdef HAVE_SDL_IMAGE
//#include "SDL_image.h"

extern DECLSPEC SDL_Surface * SDLCALL IMG_Load_RW(SDL_RWops *src, int freesrc);
#endif // HAVE_SDL_IMAGE

//---------------------------------------------------------------------------------------
//  BKLoadSurface - transparent wrapper for zzip and SDL_image (if available)
//---------------------------------------------------------------------------------------

#ifdef HAVE_SDL_ZZIP
// Put the function in our own namespace
#define SDL_RWFromZZIP  BK_SDL_RWFromZZIP
// no library for this, just include the source directly
#include "SDL_rwops_zzip.c"
#endif

// DAVEPECK_CHANGED
/*
struct BK_GL_func *BKLoadGLfunctions(void) 
{
	return NULL;
}
 */

SDL_Surface *BKLoadSurface(const char *filename)
{
    SDL_Surface *tempSurfaceP = NULL;
    SDL_RWops *ops;

#ifdef HAVE_SDL_ZZIP
    ops = SDL_RWFromZZIP(filename, "rb");
#else
    ops = SDL_RWFromFile(filename, "rb");
#endif
    if (ops != NULL)
    {
        if ( strstr(filename, ".tga") )
        {
            tempSurfaceP = BKLoadTGA_RW(ops, 0);
            if (!tempSurfaceP) fprintf(stderr, "SDLLoadTGA %s\n", SDL_GetError());
        }
#ifdef HAVE_SDL_IMAGE
        else
        {
            tempSurfaceP = IMG_Load_RW(ops,0);
            if (!tempSurfaceP) fprintf(stderr, "IMG_Load %s\n", SDL_GetError());
        }
#else
        else if ( strstr(filename, ".bmp") )
        {
            tempSurfaceP = SDL_LoadBMP_RW(ops,0);
            if (!tempSurfaceP) fprintf(stderr, "SDL_LoadBMP %s\n", SDL_GetError());
        }
        else
        {
            SDL_SetError("Unknown file type");
        }
#endif // HAVE_SDL_IMAGE

    /*
        if (tempSurfaceP) fprintf(stderr, "loaded \"%s\" %dx%dx%d%s%s\n", filename,
            tempSurfaceP->w, tempSurfaceP->h,
            tempSurfaceP->format->BitsPerPixel,
            (tempSurfaceP->flags & SDL_SRCCOLORKEY) ? " colorkey" : "",
            (tempSurfaceP->flags & SDL_SRCALPHA) ? " alpha" : "");
    */
    
        SDL_FreeRW(ops);
    }
    
    return tempSurfaceP;
}

//---------------------------------------------------------------------------------------
//  BKLoadTGA - a more capable version of this function is available in SDL_image
//---------------------------------------------------------------------------------------

SDL_Surface *BKLoadTGA_RW(SDL_RWops *src, int freesrc)
{
    SDL_Surface *surface = NULL;
    Uint32 Rmask, Gmask, Bmask, Amask;
    Uint8 *row;
    long stride;
    int i, x, y;
 
    unsigned char   imageIDLength;
    unsigned char   imageTypeCode;
    unsigned char   imageColorMap;

    unsigned char   imageSpec[10];
    unsigned short  imageWidth;
    unsigned short  imageHeight;
    unsigned char   imageDepth;
    unsigned char   imageDescriptor;

    unsigned char   imageColorMapSpec[5];
    unsigned short  imageColorMapFirstEntry;
    unsigned short  imageColorMapLength;
    unsigned char   imageColorMapSize;

    // read the Targa header
    if (!SDL_RWread(src,&imageIDLength, sizeof(unsigned char), 1)) goto error;
    if (!SDL_RWread(src,&imageColorMap, sizeof(unsigned char), 1)) goto error;
    if (!SDL_RWread(src,&imageTypeCode, sizeof(unsigned char), 1)) goto error;
 
    if (!SDL_RWread(src,&imageColorMapSpec, sizeof(unsigned char), 5)) goto error;
    imageColorMapFirstEntry = SDL_SwapLE16(*((unsigned short *) &imageColorMapSpec[0]));
    imageColorMapLength = SDL_SwapLE16(*((unsigned short *) &imageColorMapSpec[2]));
    imageColorMapSize = imageColorMapSpec[4];

    if (!SDL_RWread(src,&imageSpec, sizeof(unsigned char), 10)) goto error;
    imageWidth = SDL_SwapLE16(*((unsigned short *) &imageSpec[4]));
    imageHeight = SDL_SwapLE16(*((unsigned short *) &imageSpec[6]));
    imageDepth = imageSpec[8];
    imageDescriptor = imageSpec[9];

    // Image Type Code: either 1 (colormap) 2 (truecolor) or 3 (greyscale)
    //                        plus the RLE versions thereof (with 8 added to it)
    if ((imageTypeCode != 1) && (imageTypeCode != 2) && (imageTypeCode != 3) &&
        (imageTypeCode != 9) && (imageTypeCode != 10) && (imageTypeCode != 11))
    {
        SDL_SetError("Unsupported Image Type");
        goto error;
    }

    // skip ImageID
    if (imageIDLength) SDL_RWseek(src, imageIDLength, SEEK_CUR);
 
    // (ARGB/RGB/palette)
    if (imageDepth == 32 || imageDepth == 24)
    {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        Rmask = 0x00FF0000;
        Gmask = 0x0000FF00;
        Bmask = 0x000000FF,
        Amask = (imageDepth == 32) ? 0xFF000000 : 0;
#else
        Rmask = 0x0000FF00;
        Gmask = 0x00FF0000;
        Bmask = 0xFF000000,
        Amask = (imageDepth == 32) ? 0x000000FF : 0;
#endif
    }
    else if (imageDepth == 8)
    {
        Rmask = Gmask = Bmask = Amask = 0;
    }
    else
    {
        SDL_SetError("Unsupported Image Depth");
        goto error;
    }
 
    // create surface
    surface = SDL_CreateRGBSurface( SDL_SWSURFACE,
        imageWidth, imageHeight, imageDepth,
        Rmask, Gmask, Bmask, Amask );
    if (surface == NULL)
    {
         SDL_SetError("Could not create surface");
        goto error;
    }
    
    // read palette
    if (imageColorMap && (imageColorMapSize == 24 || imageColorMapSize == 32))
    {
        SDL_Color *colors = surface->format->palette->colors;
        // read color map data
        for (i = 0; i < imageColorMapLength; i++)
        {
            Uint8 entry[4];
            if (!SDL_RWread(src,entry, imageColorMapSize / 8, sizeof(Uint8))) goto error;

            colors[i].b = entry[0];
            colors[i].g = entry[1];
            colors[i].r = entry[2];
            if (imageColorMapSize == 32)
                colors[i].unused = entry[3];
        }
        surface->format->palette->ncolors = imageColorMapLength;
    }
    else if (imageTypeCode == 1)
    {
        SDL_Color *colors = surface->format->palette->colors;
        // create greyscale palette
        for (i = 0; i < 256; i++)
        {
            colors[i].r = i;
            colors[i].g = i;
            colors[i].b = i;
        }
        surface->format->palette->ncolors = 256;
    }
 
    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);
    
    row = surface->pixels;
    stride = surface->pitch;
    
    // read pixels
    if (imageTypeCode & 8)
    {
            // Run-Length Encoding (RLE)
        for (y = 0; y < imageHeight; y++)
        {
            Uint8 *pixels = row;
            x = 0;
            while (x < imageWidth)
            {
                Uint8 packet;
                int packetCount;

                if (!SDL_RWread(src,&packet, 1, sizeof(packet))) goto error;
                packetCount = (packet & 0x7F) + 1;
                if (packet & 0x80) // rle packet
                {
                    Uint8 *pixel = pixels;
                    if (!SDL_RWread(src, pixel, 1, imageDepth / 8)) goto error;
                    pixels += imageDepth / 8;
                
                    for (i = 1; i < packetCount; i++)
                    {
                        memcpy(pixels, pixel, imageDepth / 8);
                        pixels += imageDepth / 8;
                    }
                }
                else // raw packet
                {
                    if (!SDL_RWread(src,pixels, packetCount, imageDepth / 8)) goto error;
                    pixels += packetCount * imageDepth / 8;
                }
                x += packetCount;
            }
            row += stride;
         }
     }
    else
    {
            // normal pixel data
        for (y = 0; y < imageHeight; y++)
        {
          if (!SDL_RWread(src, row, imageWidth, imageDepth / 8)) goto error;
          row += stride;
        }
    }

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
  
    if (imageDescriptor & (1<<4))
        BKFlipHorizontal(surface);
    if (!(imageDescriptor & (1<<5)))
        BKFlipVertical(surface);
 
    if (freesrc)
        SDL_RWclose(src);
    
    return surface;

error:
    if (surface != NULL)
        SDL_FreeSurface(surface);
    if (freesrc)
            SDL_RWclose(src);

    return NULL;
}

//---------------------------------------------------------------------------------------
//  BKCreateGLSurface
//---------------------------------------------------------------------------------------

SDL_Surface *BKCreateGLSurface(int w, int h, int bpp,
    SDL_bool power2, SDL_bool mask, SDL_bool alpha)
{
    SDL_Surface *surface;

    if (power2) // round up to a power of two
    {
        int x,y;
        
        x = 1;
        while ( x < w )
            x *= 2;
        w = x;
        
        y = 1;
        while ( y < h )
            y *= 2;
        h = y;
    }

    // TODO: Create 16-bit surfaces (RGBA4, RGB5_A1 and RGB5)
    (void) bpp;

    if (mask || alpha)  // create RGBA8
    {
        surface = SDL_CreateRGBSurface(
            SDL_SWSURFACE, w, h, 32,
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            0xFF000000, 0x00FF0000, 0x0000FF00,
            0x000000FF
        #else // SDL_BYTEORDER == SDL_LIL_ENDIAN
            0x000000FF, 0x0000FF00, 0x00FF0000,
            0xFF000000
        #endif
            );
    }
    else  // create RGB8
    {
        surface = SDL_CreateRGBSurface(
            SDL_SWSURFACE, w, h, 24,
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            0x00FF0000, 0x0000FF00, 0x000000FF,
        #else // SDL_BYTEORDER == SDL_LIL_ENDIAN
            0x000000FF, 0x0000FF00, 0x00FF0000,
        #endif
            0);
    }

    return surface;
}

//---------------------------------------------------------------------------------------
//  BKFlipHorizontal
//---------------------------------------------------------------------------------------

int BKFlipHorizontal(SDL_Surface *surface)
{
    int         x, y;
    Uint32      pixel1, pixel2;
    
    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);
    
    for (y = 0; y < surface->h; y++)
    {
        for (x = 0; x < (surface->w + 1) / 2; x++)
        {
            pixel1 = BKGetPixel(surface, x, y);
            pixel2 = BKGetPixel(surface, surface->w-1 - x, y);

            BKPutPixel(surface, x, y, pixel2);
            BKPutPixel(surface, surface->w-1 - x, y, pixel1);
        }
    }
    
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    return 0;
}

//---------------------------------------------------------------------------------------
//  BKFlipVertical
//---------------------------------------------------------------------------------------

int BKFlipVertical(SDL_Surface *surface)
{
    int         x, y;
    Uint32      pixel1, pixel2;
    
    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);
    
    for (x = 0; x < surface->w; x++)
    {
        for (y = 0; y < (surface->h + 1) / 2; y++)
        {
            pixel1 = BKGetPixel(surface, x, y);
            pixel2 = BKGetPixel(surface, x, surface->h-1 - y);

            BKPutPixel(surface, x, y, pixel2);
            BKPutPixel(surface, x, surface->h-1 - y, pixel1);
        }
    }
    
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    return 0;
}

//---------------------------------------------------------------------------------------
//  BKInvertAlphaChannel
//---------------------------------------------------------------------------------------

int BKInvertAlphaChannel(SDL_Surface *surface)
{
    int row, col;
    int offset;
    Uint8 *buf;

    // TODO: Support 16-bit surfaces (RGBA4, RGB5_A1)

    if ( (surface->format->BitsPerPixel != 32) ) {
        SDL_SetError("Unsupported surface bit depth");
        return -1;
    }

    if ( (surface->format->Amask != 0xFF000000) &&
         (surface->format->Amask != 0x000000FF) ) {
        SDL_SetError("Unsupported surface alpha mask format");
        return -1;
    }

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    if ( surface->format->Amask == 0xFF000000 ) {
            offset = 3;
    } else { // if (surface->format->Amask == 0x000000FF) 
            offset = 0;
    }
#else
    if ( surface->format->Amask == 0xFF000000 ) {
            offset = 0;
    } else { // if (surface->format->Amask == 0x000000FF)
            offset = 3;
    }
#endif /* Byte ordering */

    if ( SDL_MUSTLOCK(surface) )
        if ( SDL_LockSurface(surface) < 0 ) return -1;

    /* Quickly invert the alpha channel of an RGBA or ARGB surface */
    row = surface->h;
    while (row--) {
        col = surface->w;
        buf = (Uint8 *)surface->pixels + row * surface->pitch + offset;
        while(col--) {
            *buf = 255 - *buf;
            buf += 4;
        }
    }
    if ( SDL_MUSTLOCK(surface) )
        SDL_UnlockSurface(surface);

    return 0;
}

//---------------------------------------------------------------------------------------
//  DitherAlphaChannel - convert from grayscale to black-white
//---------------------------------------------------------------------------------------

int BKDitherAlphaChannel(SDL_Surface *surface)
{
    int         x,y;
    Uint32      amask;
    int         ashift;
    int         direction;
    int         errsum;
    int         val,alpha;
    int         *errors[2];
    void        *buffer;

    if (!(surface->flags & SDL_SRCALPHA))
    {
        SDL_SetError("Surface has no alpha channel");
        return -1;
    }

    if ( surface->format->BitsPerPixel != 32 )
    {
        SDL_SetError("Unsupported surface bit depth");
        return -1;
    }
    
    if (SDL_MUSTLOCK(surface))
        if ( SDL_LockSurface(surface) < 0)  return 1;

    buffer = calloc(2 * (surface->w + 2), sizeof(int));
    if ( buffer == NULL )
    {
        SDL_SetError("Could not allocate dithering error buffer");
        return -1;
    }

    errors[0] = (int *) buffer + 2;
    errors[1] = (int *) buffer + (surface->w + 2) + 2;

    direction = 0;

    amask = surface->format->Amask;
    ashift = surface->format->Ashift - surface->format->Aloss;

    if ( surface->format->BitsPerPixel == 32 )
    {
        Uint32 *row = (Uint32 *) surface->pixels;

        for (y = 0; y < surface->h; y++)
        {
            errsum = 0;
            
            for (x = 0; x < surface->w; x++)
            {
                val = 255 - ((row[x] & amask) >> ashift);
                val += errsum;

                // threshold :  128
                alpha = (val < 0x80) ? 0x00 : 0xFF;             
                
                // calculate initial error
                errsum = val - alpha;                   

                //  Floyd-Steinberg error distribution :
                errors[direction][x - 2] += (errsum * 3) >> 4;  // down-left :   3/16 of error
                errors[direction][x - 1] += (errsum * 5) >> 4;  // down :        5/16 of error
                errors[direction][x    ] = errsum >> 4;         // down-right :  1/16 of error
                errsum = (errsum * 7) >> 4;                     // right :       7/16 of error

                row[x] = (row[x] &~ amask) | (((255-alpha) << ashift) & amask);
                
                // add error from the row above
                errsum += errors[!direction][x];                
            }
        
            row = (Uint32 *) ((char *) row + surface->pitch);

            // alternate scan directions for better error distribution
            direction = !direction; 
        }
    }

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    free(buffer);

    return 0;
}

//---------------------------------------------------------------------------------------
//  BKConvertColorKeyToAlphaChannel - makes an alpha channel from the color key
//---------------------------------------------------------------------------------------

int BKConvertColorKeyToAlphaChannel(SDL_Surface *surface, SDL_Color *colorkey)
{
	/*
    int         x,y;
    Uint32      key;
    Uint32      mask;
    Uint32      amask;
    
    if (!(surface->flags & SDL_SRCALPHA))
    {
        SDL_SetError("Surface has no alpha channel");
        return -1;
    }
    
    if (SDL_MUSTLOCK(surface))
        if ( SDL_LockSurface(surface) < 0)  return 1;
    
    mask = surface->format->Rmask | surface->format->Gmask | surface->format->Bmask;
    amask = (SDL_ALPHA_OPAQUE << surface->format->Ashift) &  surface->format->Amask;

    if (colorkey != NULL)
        key = SDL_MapRGB(surface->format, colorkey->r, colorkey->g, colorkey->b);
    else
        key = surface->map->info.colorkey;
    
    key &= mask;
        
    if ( surface->format->BitsPerPixel == 16 )
    {
        Uint16 *row = (Uint16 *) surface->pixels;
        
        for (y = 0; y < surface->h; y++)
        {
            for (x = 0; x < surface->w; x++)
            {
                if ((row[x] & mask) == key)
                    row[x] = SDL_ALPHA_TRANSPARENT;
                else
                    row[x] |= amask;
            }
            
            row = (Uint16 *) ((char *) row + surface->pitch);
        }
    }
    else if ( surface->format->BitsPerPixel == 32 )
    {
        Uint32 *row = (Uint32 *) surface->pixels;
        
        for (y = 0; y < surface->h; y++)
        {
            for (x = 0; x < surface->w; x++)
            {
                if ((row[x] & mask) == key)
                    row[x] = SDL_ALPHA_TRANSPARENT;
                else
                    row[x] |= amask;
            }
        
            row = (Uint32 *) ((char *) row + surface->pitch);
        }
    }
    else
    {
        SDL_SetError("Unsupported surface bit depth");
        return -1;
    }
    
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
	 */
	
    return 0;
}

//---------------------------------------------------------------------------------------
//  BKConvertAlphaChannelToColorKey - makes color key from the alpha channel
//---------------------------------------------------------------------------------------

int BKConvertAlphaChannelToColorKey(SDL_Surface *surface, SDL_Color *colorkey)
{
	/*
    int         x,y;
    Uint32      key;
    Uint32      amask;
    int         ashift;
    int         alpha;

    if (!(surface->flags & SDL_SRCALPHA))
    {
        SDL_SetError("Surface has no alpha channel");
        return -1;
    }

    if ( surface->format->BitsPerPixel != 16 &&
         surface->format->BitsPerPixel != 32 )
    {
        SDL_SetError("Unsupported surface bit depth");
        return -1;
    }
    
    if (SDL_MUSTLOCK(surface))
        if ( SDL_LockSurface(surface) < 0)  return 1;

    amask = surface->format->Amask;
    ashift = surface->format->Ashift - surface->format->Aloss;

    if (colorkey != NULL)
        key = SDL_MapRGB(surface->format, colorkey->r, colorkey->g, colorkey->b);
    else
        key = surface->map->info.colorkey;
    
    if ( surface->format->BitsPerPixel == 16 )
    {
        Uint16 *row = (Uint16 *) surface->pixels;

        for (y = 0; y < surface->h; y++)
        {
            for (x = 0; x < surface->w; x++)
            {
                alpha = ((row[x] & amask) >> ashift);

                if (alpha == SDL_ALPHA_TRANSPARENT)
                    row[x] = key;
            }
        
            row = (Uint16 *) ((char *) row + surface->pitch);
        }
    }
    else if ( surface->format->BitsPerPixel == 32 )
    {
        Uint32 *row = (Uint32 *) surface->pixels;

        for (y = 0; y < surface->h; y++)
        {
            for (x = 0; x < surface->w; x++)
            {
                alpha = ((row[x] & amask) >> ashift);

                if (alpha == SDL_ALPHA_TRANSPARENT)
                    row[x] = key;
            }
        
            row = (Uint32 *) ((char *) row + surface->pitch);
        }
    }

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
    */
	
    return 0;
}

//---------------------------------------------------------------------------------------
//  BKAddColorKeyFromMask - mask should be a black&white picture (black is opaque)
//---------------------------------------------------------------------------------------

int BKAddColorKeyFromMask(SDL_Surface *surface, SDL_Surface *mask)
{
	/*
    int         x, y;
    SDL_Color   black = { 0x00, 0x00, 0x00 };
    SDL_Color   white = { 0xFF, 0xFF, 0xFF };
    Uint32      blackkey, whitekey, colorkey;
    
    BK_ASSERT(surface->flags & SDL_SRCCOLORKEY);
    BK_ASSERT(surface->w == mask->w);
    BK_ASSERT(surface->h == mask->h);
    
    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);
    if (SDL_MUSTLOCK(mask))
        SDL_LockSurface(mask);
    
    blackkey = SDL_MapRGB(mask->format, black.r, black.g, black.b);
    whitekey = SDL_MapRGB(mask->format, white.r, white.g, white.b);
    
    colorkey = surface->map->info.colorkey;
    
    for (y = 0; y < surface->h; y++)
    {
        for (x = 0; x < surface->w; x++)
        {
            if ( BKGetPixel(mask, x, y) != blackkey ) // == whitekey
                BKPutPixel(surface, x, y, colorkey);
        }
    }
    
    if (SDL_MUSTLOCK(mask))
        SDL_UnlockSurface(mask);
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
	*/
	
    return 0;
}

//---------------------------------------------------------------------------------------
//  BKAddAlphaChannelFromMask - mask should be a greyscale picture (black is opaque)
//---------------------------------------------------------------------------------------

int BKAddAlphaChannelFromMask(SDL_Surface *surface, SDL_Surface *mask)
{
    int         x, y;
    Uint32      pixel, alpha;
    SDL_Color   grey;
    
    BK_ASSERT(surface->flags & SDL_SRCALPHA);
    BK_ASSERT(surface->w == mask->w);
    BK_ASSERT(surface->h == mask->h);
    
    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);
    if (SDL_MUSTLOCK(mask))
        SDL_LockSurface(mask);
    
    for (y = 0; y < surface->h; y++)
    {
        for (x = 0; x < surface->w; x++)
        {
            alpha = BKGetPixel(mask, x, y);

            SDL_GetRGB(alpha, mask->format, &grey.r, &grey.g, &grey.b);
            BK_ASSERT(grey.r == grey.g && grey.g == grey.b);
                        
            pixel = BKGetPixel(surface, x, y);

            alpha = grey.g >> surface->format->Aloss;
            alpha <<= surface->format->Ashift;      
            
            pixel &= ~surface->format->Amask;
            pixel |= alpha & surface->format->Amask;
        
            BKPutPixel(surface, x, y, pixel);
        }
    }
    
    if (SDL_MUSTLOCK(mask))
        SDL_UnlockSurface(mask);
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    return 0;
}

//---------------------------------------------------------------------------------------
//  BKCollideBoundingBox
//---------------------------------------------------------------------------------------

SDL_bool BKCollideBoundingBox(SDL_Surface *sa , int ax , int ay ,
                               SDL_Surface *sb , int bx , int by)
{
    if(bx + sb->w < ax) return SDL_FALSE;
    if(bx > ax + sa->w) return SDL_FALSE;

    if(by + sb->h < ay) return SDL_FALSE;
    if(by > ay + sa->h) return SDL_FALSE;

    return SDL_TRUE; //bounding boxes intersect
}

//---------------------------------------------------------------------------------------
//  BKTransparentPixel
//---------------------------------------------------------------------------------------

SDL_bool BKTransparentPixel(SDL_Surface *surface , int x , int y)
{
    Uint32 pixel;
    
        // First check that the pixel is within surface bounds
    if ( (x < 0 || x >= surface->w) || (y < 0 || y >= surface->h ) )
        return SDL_FALSE;
   
    pixel = BKGetPixel(surface, x, y);
   
    if (surface->flags & SDL_SRCCOLORKEY)
        return pixel == 0; /* DAVEPECK_CHANGED surface->map->info.colorkey; */
    else if (surface->flags & SDL_SRCALPHA)
        return (pixel & surface->format->Amask) != SDL_ALPHA_TRANSPARENT;
    else
        return SDL_TRUE;
}

//---------------------------------------------------------------------------------------
//  BKCollidePixels
//---------------------------------------------------------------------------------------

SDL_bool BKCollidePixels(SDL_Surface *sa, int ax, int ay,
                         SDL_Surface *sb, int bx, int by)
{
    int ar,ab,br,bb;
    int l,t,r,b;
    int x,y;
    
    if (!BKCollideBoundingBox(sa, ax,ay, sb, bx,by))
        return SDL_FALSE;

    ar = ax + sa->w;
    ab = ay + sa->h;
    br = bx + sb->w;
    bb = by + sb->h;
    
    if ( SDL_MUSTLOCK(sa) )
        if ( SDL_LockSurface(sa) < 0 ) return SDL_FALSE;
    if ( SDL_MUSTLOCK(sb) )
        if ( SDL_LockSurface(sb) < 0 ) return SDL_FALSE;

        // compute intersection rect
    l = (ax < bx) ? ax : bx;
    t = (ax < bx) ? ax : bx;
    r = (ar > br) ? ar : br;
    b = (ab > bb) ? ab : bb;

    for (y = t ; y < b ; y++)
    {
        for (x = l ; x < r ; x++)
        {
            if((!BKTransparentPixel(sa , x-ax , y-ay))
            && (!BKTransparentPixel(sb , x-bx , y-by)))
                return SDL_TRUE;
        }
    }

    if ( SDL_MUSTLOCK(sa) )
        SDL_UnlockSurface(sa);
    if ( SDL_MUSTLOCK(sb) )
        SDL_UnlockSurface(sb);

    return SDL_FALSE;
}

//---------------------------------------------------------------------------------------
//  BKPutPixel -- clips to surface bounds
//---------------------------------------------------------------------------------------

void BKPutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    Uint8 *p;

        // First check that the pixel is within surface bounds
    if ( (x >= 0 || x < surface->w) || (y >= 0 || y < surface->h ) )
    {
        int bpp = surface->format->BytesPerPixel;
       
            // Here p is the address to the pixel we want to set
        p = ((Uint8 *) surface->pixels) + y * surface->pitch + x * bpp;
        
        switch(bpp)
        {
            case 1:
                *((Uint8 *) p) = pixel;
                break;
    
            case 2:
                *((Uint16 *) p) = pixel;
                break;
    
            case 3:
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
                p[0] = (pixel >> 16) & 0xFF;
                p[1] = (pixel >>  8) & 0xFF;
                p[2] = (pixel      ) & 0xFF;
        #else // SDL_BYTEORDER == SDL_LIL_ENDIAN
                p[0] = (pixel      ) & 0xFF;
                p[1] = (pixel >>  8) & 0xFF;
                p[2] = (pixel >> 16) & 0xFF;
        #endif
                break;
    
            case 4:
                *((Uint32 *) p) = pixel;
                break;
                
            default:
                // shouldn't happen, unknown depth
                BK_ASSERT(0);
        }
    }
}

//---------------------------------------------------------------------------------------
//  BKGetPixel -- returns 0 if outside bounds
//---------------------------------------------------------------------------------------

Uint32 BKGetPixel(SDL_Surface *surface, int x, int y)
{
    Uint8 *p;
    Uint32 pixel = 0;
    
        // First check that the pixel is within surface bounds
    if ( (x >= 0 || x < surface->w) || (y >= 0 || y < surface->h ) )
    {
        int bpp = surface->format->BytesPerPixel;

            // Here p is the address to the pixel we want to set
        p = ((Uint8 *) surface->pixels) + y * surface->pitch + x * bpp;
        
        switch(bpp)
        {
            case 1:
                pixel = *((Uint8 *) p);
                break;
    
            case 2:
                pixel = *((Uint16 *) p);
                break;
    
            case 3:
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
                pixel = p[0] << 16 | p[1] << 8 | p[2];
        #else // SDL_BYTEORDER == SDL_LIL_ENDIAN
                pixel = p[0] | p[1] << 8 | p[2] << 16;
        #endif
                break;
    
            case 4:
                pixel = *((Uint32 *) p);
                break;
               
           default:
                // shouldn't happen, unknown depth
                BK_ASSERT(0);
        }
    }
    
    return pixel;
}

//---------------------------------------------------------------------------------------
//  BKDrawLine - Bresenhams line drawing algorithm
//---------------------------------------------------------------------------------------

void BKDrawLine(SDL_Surface *surface, int x1, int y1, int x2, int y2, Uint32 pixel)
{
    int     deltax,deltay;
    int     x,dx1,dx2;
    int     y,dy1,dy2;
    int     d,dd1,dd2;
    int     i,len;
    
    // First check that the line is within surface bounds at all
   if ( (x1 < 0 && x2 < 0 ) || (x1 >= surface->w && x2 >= surface->w ) ||
        (y1 < 0 && y2 < 0 ) || (y1 >= surface->h && y2 >= surface->h ) )
        return;
   
    deltax = (x1 < x2) ? (x2 - x1) : (x1 - x2);
    deltay = (y1 < y2) ? (y2 - y1) : (y1 - y2);
    
    if (deltax >= deltay)
    {
        d = deltay * 2 - deltax;
        len = deltax + 1;
        
        dd1 = deltay * 2;
        dd2 = deltay * 2 - deltax * 2;
        
        dx1 = 1;
        dx2 = 1;
        
        dy1 = 0;
        dy2 = 1;
    }
    else // if (deltax < deltay)
    {
        d = deltax * 2 - deltay;
        len = deltay + 1;
        
        dd1 = deltax * 2;
        dd2 = deltax * 2 - deltay * 2;
        
        dx1 = 0;
        dx2 = 1;
        
        dy1 = 1;
        dy2 = 1;
    }   
        
    if (x1 > x2)
    {
        dx1 = -dx1;
        dx2 = -dx2;
    }

    if (y1 > y2)
    {
        dy1 = -dy1;
        dy2 = -dy2;
    }
    
    x = x1;
    y = y1;

    for (i = 0; i < len; i++)
    {
        // let putpixel handle clipping
        BKPutPixel(surface, x, y, pixel);

        if (d < 0)
        {
            d += dd1;
            x += dx1;
            y += dy1;
        }
        else
        {
            d += dd2;
            x += dx2;
            y += dy2;
        }
    }
 }
 
//---------------------------------------------------------------------------------------
//  BKDrawOval
//---------------------------------------------------------------------------------------

void BKDrawOval(SDL_Surface *surface, SDL_Rect *rect, Uint32 pixel, SDL_bool filled)
{
    int x1, x2, y1, y2;
    int xc, yc, a, b, a2, b2;
    register int x, y, sigma;
    
    if (rect == NULL || !rect->w || !rect->h)
        return;
    
    x1 = rect->x;
    y1 = rect->y;   
    x2 = x1 + rect->w - 1;
    y2 = y1 + rect->h - 1;

    // First check that the oval is within surface bounds at all
   if ( (x1 < 0 && x2 < 0 ) || (x1 >= surface->w && x2 >= surface->w ) ||
        (y1 < 0 && y2 < 0 ) || (y1 >= surface->h && y2 >= surface->h ) )
        return;

    xc = (x1 + x2)/2;
    yc = (y1 + y2)/2;

    a = (x1 < x2) ? (x2 - x1)/2 : (x1 - x2)/2;
    b = (y1 < y2) ? (y2 - y1)/2 : (y1 - y2)/2;

    a2 = a * a;
    b2 = b * b;

    // first half
    sigma = 2 * b2 + a2 * (1 -2*b);
    for (x = 0, y = b; x * b2 < y * a2; x++)
    {
        if (!filled)
        {
            BKPutPixel(surface, (xc+x), (yc+y), pixel);
            BKPutPixel(surface, (xc-x), (yc+y), pixel);
            BKPutPixel(surface, (xc+x), (yc-y), pixel);
            BKPutPixel(surface, (xc-x), (yc-y), pixel);
        }
        else
        {
            BKDrawLine(surface, (xc+x), (yc+y), (xc-x), (yc+y), pixel);
            BKDrawLine(surface, (xc+x), (yc-y), (xc-x), (yc-y), pixel);
        }
        
        if (sigma >= 0)
        {
            sigma += 4 * a2 - y * 4 * a2;
            y--;
        }
        sigma += 6 * b2 + x * 4 * b2;
    }

    // second half
    sigma = 2 * a2 + b2 * (1 - 2*a);
    for (y = 0, x = a; y * a2 <= x * b2; y++)
    {
        if (!filled)
        {
            BKPutPixel(surface, (xc+x), (yc+y), pixel);
            BKPutPixel(surface, (xc-x), (yc+y), pixel);
            BKPutPixel(surface, (xc+x), (yc-y), pixel);
            BKPutPixel(surface, (xc-x), (yc-y), pixel);
        }
        else
        {
            BKDrawLine(surface, (xc+x), (yc+y), (xc-x), (yc+y), pixel);
            BKDrawLine(surface, (xc+x), (yc-y), (xc-x), (yc-y), pixel);
        }
        
        if (sigma >= 0)
        {
            sigma += 4 * b2 - x * 4 * b2;
            x--;
        }
        sigma += 6 * a2 + y * 4 * a2;
    }
}

