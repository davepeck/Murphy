//---------------------------------------------------------------------------------------
//  SpriteFrame.c
//
//  Portions are copyright: � 1991-94 Tony Myles, All rights reserved worldwide.
//
//  Description:    implementation of the frame stuff
//---------------------------------------------------------------------------------------


#ifndef __SWCOMMON__
#include <SWCommonHeaders.h>
#endif

#ifndef __SPRITEFRAME__
#include <SpriteFrame.h>
#endif

#ifndef __BLITKERNEL__
#include <BlitKernel.h>
#endif

extern SWBoolean    gSWUseOpenGL;

//---------------------------------------------------------------------------------------
//  SWCreateFrame
//---------------------------------------------------------------------------------------

SWError SWCreateFrame (
    FramePtr* newFrameP )
{
    SWError err = kNoError;
    FramePtr tempFrameP = 0;
        
    tempFrameP = calloc(1,sizeof(FrameRec));
    
    if (!tempFrameP)
        err = kMemoryAllocationError;
    
    if (err == kNoError)
    {
                tempFrameP->frameSurfaceP = NULL;
                tempFrameP->originalSurfaceP = NULL;

                tempFrameP->glSurfaceP = NULL;
                
        *newFrameP = tempFrameP;
    }
        else
        {
            if ( tempFrameP )
            {
                free( tempFrameP );
            }
        }
    
    return err;
}

//---------------------------------------------------------------------------------------
//  SWCreateFrameFromSurface
//---------------------------------------------------------------------------------------

SWError SWCreateFrameFromSurface (
    FramePtr* newFrameP,
    SDL_Surface *surfaceP,
    int isVideoSurface )
{
    SWError err = kNoError;
    FramePtr tempFrameP = 0;
    
        
        err = SWCreateFrame( &tempFrameP );
    
    if (err == kNoError)
    {
        tempFrameP->frameSurfaceP = surfaceP;
        tempFrameP->isVideoSurface = isVideoSurface;
                
                err = SWUpdateFrame( tempFrameP );
    }

        if (err == kNoError)
    {
            *newFrameP = tempFrameP;
    }
        else
        {
            if ( tempFrameP )
            {
                free( tempFrameP );
            }
        }
        
    return err;
}

//---------------------------------------------------------------------------------------
//  SWCreateFrameFromSurfaceAndRect
//---------------------------------------------------------------------------------------

SWError SWCreateFrameFromSurfaceAndRect(
    FramePtr* newFrameP,
    SDL_Surface *surfaceP,
    SWRect* frameRectP )
{
    SWError     err = kNoError;
    FramePtr    tempFrameP;
    
    SW_ASSERT( surfaceP != 0 );
    SW_ASSERT( frameRectP->top >= 0 && frameRectP->left >= 0 &&
        frameRectP->right > frameRectP->left && frameRectP->bottom > frameRectP->top);
    
    tempFrameP = NULL;
    *newFrameP = NULL;
    
    err = SWCreateFrame( &tempFrameP );
    
    if( err == kNoError )
    {
        tempFrameP->frameSurfaceP = surfaceP;
        tempFrameP->frameRect = *frameRectP;
        tempFrameP->sharesSurface = true;
                
                err = SWUpdateFrame( tempFrameP );
    }
    
    if( err == kNoError )
        {
            *newFrameP = tempFrameP;
        }
        else
        {
            if ( tempFrameP )
            {
                free( tempFrameP );
            }
        }
    
    return err;
}

//---------------------------------------------------------------------------------------
//  SWCreateBlankFrame
//---------------------------------------------------------------------------------------

SWError SWCreateBlankFrame (
    FramePtr* newFrameP,
    int w, int h, unsigned char depth, SWBoolean createAlphaChannel )
{
    SWError err = kNoError;
    FramePtr tempFrameP = NULL;
    SDL_Surface *tempSurfaceP = NULL;
    Uint32 rmask, gmask, bmask, amask;
    
    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
     on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    
    err = SWCreateFrame( &tempFrameP );
        
    if (err == kNoError)
    {
        if ( !createAlphaChannel )
        {
            amask = 0;
        }

        /* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,
        as expected by OpenGL for textures */
        tempSurfaceP = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, depth,
                                            rmask, gmask, bmask, amask);
        if (tempSurfaceP != NULL && createAlphaChannel)
        {
            SDL_SetAlpha(tempSurfaceP, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
        }   

        if (!tempSurfaceP)
            err = kSDLCreateSurface;
    }
    
    if (err == kNoError)
    {       
        tempFrameP->originalSurfaceP = tempSurfaceP;

        err = SWUpdateFrame( tempFrameP );
    } 
    
    if (err == kNoError)
    {
        *newFrameP = tempFrameP;
    }
        else
        {
            if ( tempSurfaceP )
            {
                SDL_FreeSurface( tempSurfaceP );
            }
            if ( tempFrameP )
            {
                free( tempFrameP );
            }
        }
    
    return err;
}

//---------------------------------------------------------------------------------------
//  SWCreateFrameFromFile
//---------------------------------------------------------------------------------------

SWError SWCreateFrameFromFile (
    FramePtr* newFrameP,
    const char * filename)
{
    SWError err = kNoError;
    FramePtr tempFrameP = NULL;
    SDL_Surface *tempSurfaceP = NULL;
    
    err = SWCreateFrame( &tempFrameP );
    
    if (err == kNoError)
    {
        tempSurfaceP = BKLoadSurface(filename);
                
        if (!tempSurfaceP)
            err = kSDLCreateSurfaceFromFile;
    }
    
    if (err == kNoError)
    {
        tempFrameP->originalSurfaceP = tempSurfaceP;
        //tempFrameP->hasAlpha = true;
        
                err = SWUpdateFrame( tempFrameP );
    }
    
    if (err == kNoError)
    {
            *newFrameP = tempFrameP;
    }
        else
        {
            if ( tempSurfaceP )
            {
                SDL_FreeSurface( tempSurfaceP );
            }
            if ( tempFrameP )
            {
                    free( tempFrameP );
            }
        }
        
    return err;
}

//---------------------------------------------------------------------------------------
//  SWDisposeFrame
//---------------------------------------------------------------------------------------

int SWDisposeFrame(
    FramePtr *oldFramePP)
{
    int     frameDisposed = false;
    FramePtr    oldFrameP = *oldFramePP;


    if (oldFrameP != 0)
    {
            // is this frame still in use by another sprite?
        if (oldFrameP->useCount > 1)
        {
                // one less sprite is using it now!
            oldFrameP->useCount--;
        }
        else    // no more sprites are using this frame
        {
            frameDisposed = true;
            
                // If this is a video surface frame, don't dispose of its
                // surfaces, since the video surface is disposed of by SDL_Quit
            if(!oldFrameP->isVideoSurface)
            {
                if (oldFrameP->frameSurfaceP)
                {
                    if (!oldFrameP->sharesSurface)
                        SDL_FreeSurface(oldFrameP->frameSurfaceP);
                    oldFrameP->frameSurfaceP = 0;
                }
                
                if (oldFrameP->originalSurfaceP)
                {
                    if (!oldFrameP->sharesSurface)
                        SDL_FreeSurface(oldFrameP->originalSurfaceP);
                    oldFrameP->originalSurfaceP = 0;
                }
            }
            
            if (oldFrameP->glSurfaceP)
            {
				// DAVEPECK_CHANGED BKFreeGLSurface((BK_GL_Surface *) oldFrameP->glSurfaceP);
                oldFrameP->glSurfaceP = NULL;
            }
            
            free( oldFrameP );
            *oldFramePP = 0;    // Change the original pointer to 0
        }
    }

    return frameDisposed;
}

#if 0
#pragma mark -
#endif

//---------------------------------------------------------------------------------------
//  SWLockFrame
//---------------------------------------------------------------------------------------

void SWLockFrame(
    FramePtr srcFrameP)
{
    SW_ASSERT(srcFrameP != 0);

    if (!srcFrameP->isFrameLocked)
    {
        srcFrameP->isFrameLocked = true;
    }
}

//---------------------------------------------------------------------------------------
//  SWUnlockFrame
//---------------------------------------------------------------------------------------

void SWUnlockFrame(
    FramePtr srcFrameP)
{
    SW_ASSERT(srcFrameP != 0);

    if (srcFrameP->isFrameLocked)
    {
        srcFrameP->isFrameLocked = false;
    }
}

//---------------------------------------------------------------------------------------
//  SWCopyFrame - creates a new Frame and copies oldFrameP into it.
//---------------------------------------------------------------------------------------
/*
SWError SWCopyFrame(
    SpriteWorldPtr destSpriteWorldP,
    FramePtr oldFrameP,
    FramePtr *newFrameP,
    int copyMasks)
{
    SWError         err = kNoError;
        SDL_Surface     *tempSurfaceP = 0;
    FramePtr        tempFrameP;
    SWRect          frameRect;

    SW_ASSERT(newFrameP != NULL);
    SW_ASSERT(oldFrameP != NULL);
    SW_ASSERT(oldFrameP->frameSurfaceP != NULL);
    SW_ASSERT(destSpriteWorldP != NULL);

    *newFrameP = NULL;
    tempFrameP = NULL;

        // Get size of new frame
    frameRect = oldFrameP->frameRect;
    SW_OFFSET_RECT( frameRect, -frameRect.left, -frameRect.top );

        // Create the new frame
    err = SWCreateFrame( &tempFrameP );

        // Copy the image from the old frame into the new frame
    if ( err == kNoError )
    {
            
            if ( copyMasks && oldFrameP->maskType == kAlphaChannelMask )
            {
                tempSurfaceP = SDL_DisplayFormatAlpha( oldFrameP->frameSurfaceP );
            } else
            {
                tempSurfaceP = SDL_DisplayFormat( oldFrameP->frameSurfaceP );
            }
            if (!tempSurfaceP)
            {
                    err = kSDLSurfaceConversion;
            }
            
            if (err == kNoError)
            {
                tempFrameP->frameSurfaceP = tempSurfaceP;
                err = SWUpdateFrame( tempFrameP );
            }
    }

    if ((oldFrameP->isFrameLocked) && (err == kNoError))
    {
        SWLockFrame(tempFrameP);
    }

    if (err == kNoError)
    {
        *newFrameP = tempFrameP;
    }
    else
    {
                // an error occurred so dispose of anything we managed to create
                if ( tempSurfaceP )
                {
                    SDL_FreeSurface( tempSurfaceP );
                }                
        if (tempFrameP != NULL)
        {
                    SWDisposeFrame( &tempFrameP );
        }
    }

    return err;
}
*/

//---------------------------------------------------------------------------------------
//  SWUpdateFrame
//---------------------------------------------------------------------------------------

SWError SWUpdateFrame (
    FramePtr srcFrameP)
{
    SWError err = kNoError;
    SDL_Surface *tempSurfaceP = 0;
    //int hasAlpha;
    SDL_Surface *surf;
    SDL_PixelFormat *fmt;
  //  SDL_Rect rect;
    
    SW_ASSERT(srcFrameP != 0);

    if (srcFrameP->originalSurfaceP)
    {
        surf = srcFrameP->originalSurfaceP;
        
        if (SDL_GetVideoSurface() != NULL)
        {
               /* NOTE: If this is an offscreen frame (used for work area, etc...),
                        and it has an alpha channel this will use "SDL_DisplayFormatAlpha"
                        to create the formatted surface... which DOES NOT WORK */
            if ( surf->flags & SDL_SRCALPHA )
            {
                tempSurfaceP = SDL_DisplayFormatAlpha(surf);
            }
            else
            {
                tempSurfaceP = SDL_DisplayFormat(surf);
            }
        }
        else
        {
            fmt = surf->format; // should be the destination surface format

            tempSurfaceP = SDL_CreateRGBSurfaceFrom(
                surf->pixels, surf->w, surf->h, fmt->BitsPerPixel, surf->pitch,
                fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask );
        }
        
        if (!tempSurfaceP)
        {
            err = kSDLSurfaceConversion;
        }
        
        if (err == kNoError)
        {
            if( srcFrameP->frameSurfaceP )
            {
                SDL_FreeSurface(srcFrameP->frameSurfaceP);
            }
            
            srcFrameP->frameSurfaceP = tempSurfaceP;
        }
    }

#ifdef HAVE_OPENGL
    if (gSWUseOpenGL && err == kNoError && !srcFrameP->isVideoSurface)
    {
        if (srcFrameP->glSurfaceP != NULL)
        {
            BKFreeGLSurface( (BK_GL_Surface *) srcFrameP->glSurfaceP );
        }
        
        SW_CONVERT_SW_TO_SDL_RECT(srcFrameP->frameRect, rect);
        
        srcFrameP->glSurfaceP = BKCreateNewGLSurface( srcFrameP->frameSurfaceP, srcFrameP->sharesSurface ? &rect : NULL );
        if (srcFrameP->glSurfaceP == NULL)
        {
            fprintf(stderr, "BKCreateNewGLSurface: %s\n", SDL_GetError() );
            err =  kSDLSurfaceConversion;
        }
    }
#endif // OpenGL
        
        if ( err == kNoError )
        {
            // Update the frame rect if the frame surface isn't shared (it isn't a portion of a surface)
            if ( !srcFrameP->sharesSurface )
            {
                SW_SET_RECT( srcFrameP->frameRect, 0, 0, srcFrameP->frameSurfaceP->w, srcFrameP->frameSurfaceP->h );
            }
            
            // Set mask type
            if ( srcFrameP->frameSurfaceP->flags & SDL_SRCALPHA ) {
                srcFrameP->maskType = kAlphaChannelMask;
            } else {
                srcFrameP->maskType = kNoMask;
            }
        }
        else
        {
            if ( tempSurfaceP )
            {
                SDL_FreeSurface( tempSurfaceP );
            }
        }
        
    return err;
}
