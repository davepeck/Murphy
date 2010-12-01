//---------------------------------------------------------------------------------------
/// @file SWCommonHeaders.h
/// common macros, constants, and stuff, used throughout
//
//  Portions are copyright: (C) 1991-94 Tony Myles, All rights reserved worldwide.
//---------------------------------------------------------------------------------------

#ifndef __SWCOMMON__
#define __SWCOMMON__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

//--------------------------------------------------------------------------------------
//	sprite world options
//--------------------------------------------------------------------------------------

#ifndef SW_MOVEMENT_TYPE
    /// The precision of each sprite's movement. (Change to "short" if you
    /// don't need floating-point precision, and need the speed-up.)
typedef float                   SWMovementType;
#else
typedef SW_MOVEMENT_TYPE        SWMovementType;
#endif

#ifndef SW_DEBUG
    /// Change this to 1 for debugging builds. [use 1/0, not true/false]
#define SW_DEBUG        0
#endif

#ifndef SW_ASSERT_ON
    /// Change this to 0 to turn assertions off. [use 1/0, not true/false]
#define SW_ASSERT_ON    1
#endif

//--------------------------------------------------------------------------------------
//  standard library
//--------------------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

//--------------------------------------------------------------------------------------
//  SDL
//--------------------------------------------------------------------------------------

#ifndef _SDL_H
#if macintosh
#include "SDL.h"
#else
#include <SDL/SDL.h>
#endif
#endif // _SDL_H

//---------------------------------------------------------------------------------------
//  sprite world type definitions
//---------------------------------------------------------------------------------------

typedef struct SpriteWorldRec   SpriteWorldRec, *SpriteWorldPtr;

typedef struct TileMapStruct    TileMapStruct, *TileMapStructPtr;

typedef struct SpriteLayerRec   SpriteLayerRec, *SpriteLayerPtr;

typedef struct SpriteRec        SpriteRec, *SpritePtr;

typedef struct FrameRec         FrameRec, *FramePtr;

#ifdef __cplusplus

#ifdef SW_NAMESPACE
#define SW_NAMESPACE_BEGIN      namespace SW_NAMESPACE {
#define SW_NAMESPACE_END        }
#else
#define SW_NAMESPACE_BEGIN
#define SW_NAMESPACE_END
#endif

SW_NAMESPACE_BEGIN

class TSpriteWorld;

class TTileMap;
class TSpriteWorldTiling;

class TSpriteWorldScrolling;

class TSpriteLayer;

class TSprite;

class TFrame;

SW_NAMESPACE_END

#endif /* __cplusplus */

//--------------------------------------------------------------------------------------
//  Compatibility types    (for Mac compatibility, don't use these for SDL code)
//--------------------------------------------------------------------------------------

    /// Used for setting linking type
#define SW_FUNC

    /// A Boolean value, of either true or false
typedef unsigned char SWBoolean;

#ifndef __cplusplus

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#endif /* __cplusplus */

    /// a Point (h,v)
struct SWPoint {
  short               v;
  short               h;
};

typedef struct SWPoint          SWPoint, *SWPointPtr;

    /// a Rect (l,t,r,b)
struct SWRect {
  short               top;
  short               left;
  short               bottom;
  short               right;
};

typedef struct SWRect           SWRect, *SWRectPtr;

void SW_CONVERT_SW_TO_SDL_RECT(/*inout*/ SWRect swRect, /*inout*/ SDL_Rect sdlRect);
void SW_CONVERT_SDL_TO_SW_RECT(/*inout*/ SDL_Rect sdlRect, /*inout*/ SWRect swRect);

    /// a Color (r,g,b)
struct SWRGBColor {
  unsigned short      red;
  unsigned short      green;
  unsigned short      blue;
};

typedef struct SWRGBColor       SWRGBColor, *SWRGBColorPtr;

void SW_CONVERT_SW_TO_SDL_COLOR(/*inout*/ SWRGBColor swColor, /*inout*/ SDL_Color sdlColor);
void SW_CONVERT_SDL_TO_SW_COLOR(/*inout*/ SDL_Color sdlColor, /*inout*/ SWRGBColor swColor);

//---------------------------------------------------------------------------------------
/// Draw data structure
//---------------------------------------------------------------------------------------

struct DrawDataRec
{
    SpritePtr       parentSpriteP;

    SWMovementType  horizLoc;           ///< horizontal sprite position (hotspot)
    SWMovementType  vertLoc;            ///< vertical sprite position (hotspot)
    SWMovementType  horizMoveDelta;     ///< horizontal movement delta
    SWMovementType  vertMoveDelta;      ///< vertical movement delta

    unsigned long   translucencyLevel;  ///< used by translucent blitters
    float           rotation;           ///< used by rotation blitter
    float           rotationOffsetHoriz;///< used by rotation blitter
    float           rotationOffsetVert; ///< used by rotation blitter
    short           scaledWidth;        ///< used by scaling blitter
    short           scaledHeight;       ///< used by scaling blitter
    short           frameHotSpotH;      ///< horizontal hot point for this frame
    short           frameHotSpotV;      ///< vertical hot point for this frame
    SDL_Color       color;              ///< used by solid color blitter
    SWBoolean       flippedHorizontal;  ///< used by flipping DrawProcs
    SWBoolean       flippedVertical;    ///< used by flipping DrawProcs
};

typedef struct DrawDataRec DrawDataRec, *DrawDataPtr;

//---------------------------------------------------------------------------------------
/// sprite world error constants
//---------------------------------------------------------------------------------------

//i suggest leaving old codes for compatability sake - ben
enum SWError
{
    kNoError                    = 0,    ///< no error at all
    
        //compatability codes
    kSystemTooOldErr            = 100,  ///< < System 7.0
    kMaxFramesErr               = 101,  ///< attempt to exceed maximum number of frames for a sprite
    kInvalidFramesIndexErr      = 102,  ///< frame index out of range
    kNotCWindowErr              = 103,  ///< attempt to make a SpriteWorld from non-color WindowPtr
    kNilParameterErr            = 104,  ///< NULL SpritePtr, FramePtr, etc.
    kWrongDepthErr              = 105,  ///< invalid pixel size for attempted function
    kWrongMaskErr               = 106,  ///< invalid mask type for attempted function
    kOutOfRangeErr              = 107,  ///< tileID, tileMap, or other value out of bounds
    kTilingNotInitialized       = 108,  ///< tiling hasn't been initialized
    kTilingAlreadyInitialized   = 109,  ///< tiling already initialized; can't be initialized again
    kNullTileMapErr             = 110,  ///< no TileMap has ever been created/loaded
    kTileMapNotLockedErr        = 111,  ///< the TileMap is not locked and can't be used until it is
    kAlreadyCalledErr           = 112,  ///< the function was already called and can't be called again
    kSpriteNotCompiledErr       = 113,  ///< the sprite must be compiled before drawProc can be set
    kBadParameterErr            = 114,  ///< a parameter that was passed to the function is invalid
    kSpriteAlreadyInLayer       = 115,  ///< the Sprite is already in a Layer, and can't be added to another
    kNilFrameErr                = 116,  ///< the Frame this function acts on is NULL
    kNotLockedErr               = 117,  ///< a structure that must be locked is not
    k68kOnlyErr                 = 118,  ///< can only be used from 68k code
    kNotInCarbonErr             = 119,  ///< not supported for Carbon builds
    kWrongDrawProcTypeErr       = 120,  ///< hardware drawprocs can only be used for hardware
    
        //new codes
                
        kMemoryAllocationError          = 200,  ///< problem allocationg memory
        kSpriteCreationError            = 201,  ///< could not fully create the sprite
        kFrameCreationError         = 202,  ///< problem creating frame
        
        kFailedToInitSDL            = 300,  ///< could not init the sdl lib
        kSDLSetVideoMode            = 301,
        kSDLCreateSurface           = 302,  ///< could not create a surface
        kSDLCreateSurfaceFromFile       = 303,  ///< could not create surface from file
        kSDLBlitError               = 304,  ///< could not blit surfaces, maybe they are locked?
        KSDLBlitVideoMemoryError        = 305,  ///< sdl_blit returned -2 indicating texture is out of video memory
        kSDLSurfaceConversion           = 306,  ///< SDL_DisplayFormat failed to convert the surface
        kSDLSetAlpha            = 307,  ///< SDL_SetAlpha failed to set the alpha channel
        
        kUnknownError               = 999
};

/// Error value integer, zero if no error
typedef enum SWError            SWError;


#ifdef __cplusplus

#include <stdexcept>

SW_NAMESPACE_BEGIN

/// CLASS TError, wrapping SWError
/// @ingroup Classes
class TError : public std::runtime_error
{
protected:
    SWError mError;
    
public:
    TError(const char *what, SWError error) : std::runtime_error(what) { mError = error; }
    SWError error() { return mError; }
};

SW_NAMESPACE_END
#endif /* __cplusplus */

//---------------------------------------------------------------------------------------
//  debugging stuff
//---------------------------------------------------------------------------------------

typedef void (*AssertFailProcPtr)(
    char *filename,
    char *functionname,
    int lineNum);

extern AssertFailProcPtr    gSWAssertFailProc;

#if SW_ASSERT_ON
#if defined(__FUNCTION__)
#define SW_ASSERT(condition) \
    if ( !(condition) ) \
        gSWAssertFailProc( __FILE__, (char *)__FUNCTION__, __LINE__ )
#else
#define SW_ASSERT(condition) \
    if ( !(condition) ) \
        gSWAssertFailProc( __FILE__, "unknown", __LINE__ )
#endif
/*
#define SW_ASSERT(condition) \
    if ( !(condition) ) DebugStr("\pAssertion failed: " #condition)
*/
#else // !SW_ASSERT_ON
#define SW_ASSERT(condition)
#endif


// This is a workaround for #pragma unused,
// which should be portable to all compilers.
// (unlike "__attribute__((unused))", in GCC)

    /// Avoids warnings, for unused parameters
#define SW_UNUSED(var)  (void) var

//---------------------------------------------------------------------------------------
//  sprite world macros
//---------------------------------------------------------------------------------------

//mostly for compatability (avoid using these)
#define SW_ABS(x)    ((x) < (0) ? -(x) : (x))

#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef SW_MIN
#undef SW_MAX
#endif
// <Windows.h> redefines these macros
#define SW_MIN(a, b) ((a) < (b) ? (a) : (b))
#define SW_MAX(a, b) ((a) > (b) ? (a) : (b))

/*
 *  Rectangle structure macro functions.
 */

#define SW_RECT_WIDTH(r)    ((r).right - (r).left)
#define SW_RECT_HEIGHT(r)   ((r).bottom - (r).top)

#define SW_TOP_LEFT(r)      (((SWPoint *) &(r))[0])
#define SW_BOT_RIGHT(r)     (((SWPoint *) &(r))[1])

    /// Set the fields of the struct (beware of macro side effects)
#define SW_SET_RECT(theRect, myleft, mytop, myright, mybottom) \
{ \
    (theRect).top = mytop; \
    (theRect).left = myleft; \
    (theRect).bottom = mybottom; \
    (theRect).right = myright; \
}

    /// Convert SW to SDL rect (beware of macro side effects)
#define SW_CONVERT_SW_TO_SDL_RECT(swRect, sdlRect) \
{ \
    (sdlRect).x = (swRect).left; \
    (sdlRect).y = (swRect).top; \
    (sdlRect).w = (swRect).right - (swRect).left; \
    (sdlRect).h = (swRect).bottom - (swRect).top; \
}

    /// Convert SDL to SW rect (beware of macro side effects)
#define SW_CONVERT_SDL_TO_SW_RECT(sdlRect, swRect) \
{ \
    (swRect).left = (sdlRect).x; \
    (swRect).top = (sdlRect).y; \
    (swRect).right = (swRect).x + (sdlRect).w; \
    (swRect).bottom = (swRect).y + (sdlRect).h; \
}

    /// Offset the fields of the struct (beware of macro side effects)
#define SW_OFFSET_RECT(theRect, deltaH, deltaV) \
{ \
    (theRect).top += deltaV; \
    (theRect).left += deltaH; \
    (theRect).bottom += deltaV; \
    (theRect).right += deltaH; \
}

    /// Centers one rect inside another
// FIXME: davethebrv says this doesnt work due to macro side effects? (do not delete things that dont work)
#define SW_CENTER_RECT( centerThis, insideThis) \
{ \
    SW_OFFSET_RECT( centerThis, \
        (insideThis.left + insideThis.right - centerThis.left - centerThis.right) >> 1, \
        (insideThis.top + insideThis.bottom - centerThis.top - centerThis.bottom) >>1 ); \
}
              

    /// Offset the fields of the struct (beware of macro side effects)
#define SW_INSET_RECT(theRect, insetX, insetY) \
{ \
    (theRect).top += insetY; \
    (theRect).left += insetX; \
    (theRect).bottom -= insetY; \
    (theRect).right -= insetX; \
}

    /// Checks whether x,y is inside rect
#define SW_POINT_IS_IN_RECT( x, y, rect) \
    ( (x >= (rect).left) && \
      (x <= (rect).right) &&    \
      (y >= (rect).top) &&  \
      (y <= (rect).bottom) )

    /// Checks whether rectA is inside rectB
#define SW_RECT_IS_IN_RECT(rectA, rectB) \
    ( ((rectA).top < (rectB).bottom) && \
      ((rectA).bottom > (rectB).top) && \
      ((rectA).left < (rectB).right) && \
      ((rectA).right > (rectB).left) )

    /// Clips rectA with rectB
#define SW_CLIP_RECT(rectA, rectB) \
    if ((rectA).top < (rectB).top)  \
        (rectA).top = (rectB).top;  \
    if ((rectA).bottom > (rectB).bottom) \
        (rectA).bottom = (rectB).bottom; \
    if ((rectA).left < (rectB).left) \
        (rectA).left = (rectB).left; \
    if ((rectA).right > (rectB).right) \
        (rectA).right = (rectB).right;

    /// Clips dstRect and srcRect with clipRect.
#define SW_CLIP_DST_AND_SRC_RECT(dstRect, srcRect, clipRect) \
    if ((dstRect).top < (clipRect).top) \
    { \
        (srcRect).top += (clipRect).top - (dstRect).top; \
        (dstRect).top = (clipRect).top; \
    } \
    if ((dstRect).bottom > (clipRect).bottom) \
    { \
        (srcRect).bottom += (clipRect).bottom - (dstRect).bottom; \
        (dstRect).bottom = (clipRect).bottom; \
    } \
    if (dstRect.left < clipRect.left) \
    { \
        (srcRect).left += (clipRect).left - (dstRect).left; \
        (dstRect).left = (clipRect).left; \
    } \
    if ((dstRect).right > (clipRect).right) \
    { \
        (srcRect).right += (clipRect).right - (dstRect).right; \
        (dstRect).right = (clipRect).right; \
    }



#define SW_UNION_RECT(dstRect, srcRect) \
{   \
    (dstRect).top = SW_MIN((dstRect).top, (srcRect).top);   \
    (dstRect).left = SW_MIN((dstRect).left,(srcRect).left); \
    (dstRect).bottom = SW_MAX((dstRect).bottom, (srcRect).bottom);  \
    (dstRect).right = SW_MAX((dstRect).right, (srcRect).right); \
}


    /// Convert SW to SDL color (beware of macro side effects)
#define SW_CONVERT_SW_TO_SDL_COLOR(swColor, sdlColor) \
{ \
    (sdlColor).r = (swColor).red >> 8; \
    (sdlColor).g = (swColor).green >> 8; \
    (sdlColor).b = (swColor).blue >> 8; \
}

    /// Convert SDL to SW color (beware of macro side effects)
#define SW_CONVERT_SDL_TO_SW_COLOR(sdlColor, swColor) \
{ \
    (swColor).red = (sdlColor).r << 8; \
    (swColor).green = (sdlColor).g << 8; \
    (swColor).blue = (sdlColor).b << 8; \
}



//---------------------------------------------------------------------------------------
//  collision test macros
//---------------------------------------------------------------------------------------


#define SW_COLLISION_RECT_TOP(spriteP) \
    (spriteP->destFrameRect.top + spriteP->curFrameP->collisionInset.top)

#define SW_COLLISION_RECT_LEFT(spriteP) \
    (spriteP->destFrameRect.left + spriteP->curFrameP->collisionInset.left)

#define SW_COLLISION_RECT_BOTTOM(spriteP) \
    (spriteP->destFrameRect.bottom - spriteP->curFrameP->collisionInset.bottom)

#define SW_COLLISION_RECT_RIGHT(spriteP) \
    (spriteP->destFrameRect.right - spriteP->curFrameP->collisionInset.right)

    
#endif /*__SWCOMMON__*/

