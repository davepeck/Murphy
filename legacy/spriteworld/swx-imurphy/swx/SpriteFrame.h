//---------------------------------------------------------------------------------------
/// @file SpriteFrame.h
/// constants, structures, and function prototypes for sprite frames
//
//  Portions are copyright: (C) 1991-94 Tony Myles, All rights reserved worldwide.
//---------------------------------------------------------------------------------------


#ifndef __SPRITEFRAME__
#define __SPRITEFRAME__

#ifndef __SWCOMMON__
#include <SWCommonHeaders.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------
/// frame mask type
//---------------------------------------------------------------------------------------

typedef enum MaskType
{
    kNoMask             = 0,
    kAlphaChannelMask,
    kColorKeyMask
    
} MaskType;


//---------------------------------------------------------------------------------------
/// frame data structure
//---------------------------------------------------------------------------------------

struct FrameRec
{
    SDL_Surface *frameSurfaceP;         ///< pointer to screen optimized SDL_Surface for the frame
    SDL_Surface *originalSurfaceP;      ///< SDL_Surface before optimizations
    SDL_bool    isVideoSurface;         ///< is the frame's surface the SDL video surface (mapped directly to screen)
    SDL_bool    sharesSurface;          ///< should we free the surface when disposing the frame?
    
    SWBoolean   isFrameLocked;          ///< has the frame been locked?
    MaskType    maskType;           ///< frame mask type
        //int       hasAlpha;
        
    SWRect      frameRect;          ///< source image rectangle
    int     hotSpotH;           ///< horizontal hot point for this frame
    int     hotSpotV;           ///< vertical hot point for this frame
    SWRect      collisionInset;         ///< if used, makes the collision rect smaller/large for this frame
    
    unsigned short  useCount;           ///< number of sprites using this frame
    
    void*       glSurfaceP;         ///< used with OpenGL (internally)
    
    long        userData;           ///< reserved for user
};



SWError SWCreateFrame (FramePtr* newFrameP);
int SWDisposeFrame(FramePtr *oldFramePP);
SWError SWCreateFrameFromSurfaceAndRect(FramePtr* newFrameP,
    SDL_Surface *surface,SWRect* frameRect );
SWError SWCreateFrameFromSurface (FramePtr* newFrameP, SDL_Surface *theSurface, int isVideoSurface );
SWError SWCreateBlankFrame (
    FramePtr* newFrameP,
    int w, int h, unsigned char depth, SWBoolean createAlphaChannel );
SWError SWCreateFrameFromFile (FramePtr* newFrameP, const char * filename);

void SWLockFrame(FramePtr srcFrameP);
void SWUnlockFrame(FramePtr srcFrameP);
SWError SWUpdateFrame ( FramePtr srcFrameP );

#ifdef __cplusplus
}

SW_NAMESPACE_BEGIN

#include "SpriteFrame.hpp"

SW_NAMESPACE_END

#endif /* __cplusplus */

#endif  /* __SPRITEFRAME__ */

