//---------------------------------------------------------------------------------------
/// @file SpriteWorld.h
/// constants, structures, and function prototypes for sprite worlds
//
//  Portions are copyright: (C) 1991-94 Tony Myles, All rights reserved worldwide.
//---------------------------------------------------------------------------------------


#ifndef __SPRITEWORLD__
#define __SPRITEWORLD__

#ifndef __SWCOMMON__
#include <SWCommonHeaders.h>
#endif

#ifndef __SPRITELAYER__
#include <SpriteLayer.h>
#endif

#ifndef __SPRITEFRAME__
#include <SpriteFrame.h>
#endif

#ifndef __SPRITE__
#include <Sprite.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


//---------------------------------------------------------------------------------------
//  data structures
//---------------------------------------------------------------------------------------

typedef struct UpdateRectStruct UpdateRectStruct;
typedef UpdateRectStruct *UpdateRectStructPtr;

    /// delta update rect
struct UpdateRectStruct
{
    SWRect              updateRect;         ///< this SWRect
    UpdateRectStructPtr nextRectStructP;    ///< next SWRect struct
};

//---------------------------------------------------------------------------------------
//  sprite world type definitions
//---------------------------------------------------------------------------------------

typedef void (*WorldMoveProcPtr)(
    SpriteWorldPtr spriteWorldP,
    SpritePtr followSpriteP);

typedef void (*CallBackPtr)(
    SpriteWorldPtr spriteWorldP);

typedef void (*GLCallBackPtr)(
    void *glContext);

typedef void (*CleanUpCallBackPtr)( void );

typedef void (*TileChangeProcPtr)(
    SpriteWorldPtr spriteWorldP);

typedef void (*TileRectDrawProcPtr)(
    SpriteWorldPtr  spriteWorldP,
    SWRect*         updateRectP,
    SWBoolean       optimizingOn);

typedef void (*CustomTileDrawProcPtr)(
    SpriteWorldPtr srcSpriteWorldP,
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    SWRect *srcRectP,
    SWRect *dstRectP,
    short tileLayer,
    short tileID,
    short curTileImage,
        SWBoolean useMask );

//---------------------------------------------------------------------------------------
/// sprite world data structure
//---------------------------------------------------------------------------------------


struct SpriteWorldRec
{
    SpriteLayerPtr headSpriteLayerP;    ///< head of the sprite layer linked list
    SpriteLayerPtr tailSpriteLayerP;    ///< tail of the sprite layer linked list
    SpriteLayerPtr deadSpriteLayerP;    ///< where SWRemoveSpriteFromAnimation puts Sprites
    
    UpdateRectStructPtr headUpdateRectP; ///< used by SWFlagRectAsChanged
    SWBoolean   useUpdateRectOptimizations; ///< should try to merge update rects 

    FramePtr extraBackFrameP;           ///< used when adding a background behind tiles
    FramePtr backFrameP;                ///< frame for the background
    FramePtr workFrameP;                ///< work, or "mixing" frame
    FramePtr screenFrameP;              ///< frame for drawing to the screen

    DrawProcPtr offscreenDrawProc;      ///< callback for erasing sprites offscreen normally
    DrawProcPtr screenDrawProc;         ///< callback for drawing sprite pieces onscreen normally

    CallBackPtr postEraseCallBack;      ///< called after erasing sprites
    CallBackPtr postDrawCallBack;       ///< called after drawing sprites
    
    SWRect  screenRect;                 ///< holds windowFrameP->frameRect for easier access
    SWRect  backRect;                   ///< holds backFrameP->frameRect for easier access

    SWRect  visScrollRect;              ///< SWRect that is copied to screen when scrolling
    SWRect  oldVisScrollRect;           ///< visScrollRect from last frame
    SWRect  offscreenScrollRect;        ///< same as visScrollRect, but local to offscreen
    short   horizScrollRectOffset;      ///< offset from offscreenScrollRect to visScrollRect
    short   vertScrollRectOffset;       ///< offset from offscreenScrollRect to visScrollRect
    short   horizScrollDelta;           ///< horizontal scrolling delta
    short   vertScrollDelta;            ///< vertical scrolling delta
    SWRect  scrollRectMoveBounds;       ///< move bounds for visScrollRect

    WorldMoveProcPtr worldMoveProc;     ///< pointer to the scrolling world move procedure
    SpritePtr followSpriteP;            ///< pointer to the "follow sprite", or NULL
    
        
        
        TileMapStructPtr *tileLayerArray;   ///< an array of all the tileMap layers
    SWBoolean       tilingIsInitialized;    ///< has the tiling been initialized yet?
    SWBoolean       tilingIsOn;             ///< are the tiling routines turned on?
    short       lastActiveTileLayer;    ///< the last active tile layer
    short       **tilingCache;          ///< two-dimensional tiling cache
    short       numTilingCacheRows;     ///< number of rows in tilingCache array
    short       numTilingCacheCols;     ///< number of cols in tilingCache array
    FramePtr    *tileFrameArray;        ///< array of tile framePtrs (the part below sprites)
    //FramePtr  *tilePartialFrameArray; ///< array of tile framePtrs (the part above sprites)
    short       *curTileImage;          ///< array specifying the current frame of each tile
    short       maxNumTiles;            ///< number of elements in tileFrameArray
    short       tileWidth;              ///< width of each tile
    short       tileHeight;             ///< height of each tile
    long        numTilesChanged;        ///< number of rects in changedTiles array to update
    SWRect      *changedTiles;          ///< array of rects of tiles that changed
    long        changedTilesArraySize;  ///< number of elements in changedTiles array
    TileChangeProcPtr tileChangeProc;   ///< pointer to tile frame changing procedure
    TileRectDrawProcPtr tileRectDrawProc; ///< pointer to the function that draws tiles in a rect
    DrawProcPtr tileMaskDrawProc;       ///< drawProc for drawing masked tiles in tile layers
    //DrawProcPtr tileMaskHardwareDrawProc;     ///< drawProc for drawing masked tiles in tile layers in hardware
    //DrawProcPtr partialMaskDrawProc;  ///< drawProc for drawing partialMask tiles above sprites
    //DrawProcPtr partialMaskHardwareDrawProc;  ///< drawProc for drawing partialMask tiles above sprites in hardware
    CustomTileDrawProcPtr customTileDrawProc;   ///< DrawProc that is also passed the tileID and tileLayer
        
    
    unsigned char   pixelDepth;             ///< SpriteWorld's depth

    short           fpsTimeInterval;    ///< milliseconds per frame of animation (1000/fps)
    double          pseudoFPS;          ///< milliseconds per pseudo frame of animation (for time-based animations)
    unsigned long   runningTimeCount;   ///< running total time in milliseconds
    unsigned long   timeOfLastFrame;    ///< time (from runningTimeCount) of last frame
    
            // For time-based animation
    unsigned long   minimumFrameRate;   ///< maximum number of microseconds that can go by in a frame (used only for time-based movement)
    unsigned long   microsecsSinceLastFrame; ///< # microseconds passed since last frame
    //UnsignedWide  lastMicrosecondsB;  ///< used to compute the microsecsSinceLastFrame
    double          pseudoFrameAccum;   ///< used to set the pseudoFrameHasFired variable.
    SWBoolean       pseudoFrameHasFired;///< used in time-based animations.

    SWBoolean       frameHasOccurred;   ///< Has the SpriteWorld been processed?
    SWBoolean       thereAreNonScrollingLayers; ///< used internally by SWProcessSpriteWorld

    SWBoolean       makeTranslucencyAdjustments;
    SWBoolean       doFastMovingSpritesChecks;

    SDL_Color       backgroundColor;    ///< background color for clear drawproc
    SDL_bool        useOpenGL;
    
    void*       glContext;          ///< passed to the OpenGL callbacks
    GLCallBackPtr glMakeCurrent;      ///< called before OpenGL drawing
    GLCallBackPtr glSwapBuffers;      ///< called after OpenGL drawing
    
    long userData;                      ///< reserved for user
};

//---------------------------------------------------------------------------------------
//  sprite world globals
//---------------------------------------------------------------------------------------

extern DrawDataPtr      gSWCurrentElementDrawData;
extern SpriteWorldPtr       gSWCurrentSpriteWorld;

extern CleanUpCallBackPtr   gSWCleanUpCallBackP;
extern SpriteWorldPtr       gSWCleanUpSpriteWorldP;

//---------------------------------------------------------------------------------------
//  sprite world function prototypes
//---------------------------------------------------------------------------------------


SWError SWEnterSpriteWorld(SWBoolean shouldInitSDL);

void SWExitSpriteWorld(void);

SWError SWCreateSpriteWorld(
    SpriteWorldPtr  *spriteWorldPP,
    int             w,
    int             h,
    int             depth,
    SWBoolean       makeFullScreen,
    SWBoolean       wantOpenGL );

SWError SWCreateSpriteWorldFromVideoSurface(
    SpriteWorldPtr      *spriteWorldPP,
    SDL_Surface         *videoSurfaceP,
    SWRect              *worldRectP,
    SWRect              *backRectP,
    int                 maxDepth );

SWError SWCreateSpriteWorldFromSWSurface(
    SpriteWorldPtr      *spriteWorldPP,
    SDL_Surface         *surfaceP,
    SWRect              *worldRectP,
    SWRect              *backRectP,
    int                 maxDepth );

SWError SWCreateSpriteWorldFromGLContext(
    SpriteWorldPtr      *spriteWorldPP,
    void*               glContext,
    SWRect              *worldRectP,
    GLCallBackPtr       glMakeCurrent,
    GLCallBackPtr       glSwapBuffers);

SWError SWCreateSpriteWorldFromFrames(
    SpriteWorldPtr      *spriteWorldP,
    FramePtr            screenFrameP,
    FramePtr            backFrameP,
    FramePtr            workFrameP );

void SWDisposeSpriteWorld(
    SpriteWorldPtr  *spriteWorldPP);
    
void SWAddSpriteLayer(
    SpriteWorldPtr      spriteWorldP,
    SpriteLayerPtr      newSpriteLayerP);

void SWRemoveSpriteLayer(
    SpriteWorldPtr  spriteWorldP,
    SpriteLayerPtr  oldSpriteLayerP);

void SWSwapSpriteLayer(
    SpriteWorldPtr  spriteWorldP,
    SpriteLayerPtr  srcSpriteLayerP,
    SpriteLayerPtr  dstSpriteLayerP);

SpriteLayerPtr SWGetNextSpriteLayer(
    SpriteWorldPtr  spriteWorldP,
    SpriteLayerPtr  curSpriteLayerP);

DrawDataPtr SWCreateDrawData (void);

void SWLockSpriteWorld(
    SpriteWorldPtr  spriteWorldP);

void SWUnlockSpriteWorld(
    SpriteWorldPtr  spriteWorldP);

void SWSetPostEraseCallBack(
    SpriteWorldPtr  spriteWorldP,
    CallBackPtr     callBack);

void SWSetPostDrawCallBack(
    SpriteWorldPtr  spriteWorldP,
    CallBackPtr     callBack);

void SWSetSpriteWorldMaxFPS(
    SpriteWorldPtr spriteWorldP,
    short framesPerSec);

void SWSetSpriteWorldPseudoFPS(
    SpriteWorldPtr spriteWorldP,
    short framesPerSec);

void SWSetMinimumFrameRate(
    SpriteWorldPtr spriteWorldP,
    long minimumFrameRate);

void SWSetBackgroundColor(
    SpriteWorldPtr spriteWorldP,
    SDL_Color color);

void SWStdOffscreenDrawProc(
    FramePtr    srcFrameP,
    FramePtr    dstFrameP,
    SWRect*     srcRect,
    SWRect*     dstRect);
    
void SWStdScreenDrawProc(
    FramePtr    srcFrameP,
    FramePtr    dstFrameP,
    SWRect*     srcRect,
    SWRect*     dstRect);

void SWCopyBackgroundToWorkArea(
    SpriteWorldPtr spriteWorldP);

void SWUpdateSpriteWorld(
    SpriteWorldPtr spriteWorldP,
    int updateWindow);

void SWProcessSpriteWorld(
    SpriteWorldPtr spriteWorldP);

void SWProcessSpriteLayer(
    SpriteWorldPtr spriteWorldP,
    SpriteLayerPtr curSpriteLayerP);

void SWProcessNonScrollingLayers(
    SpriteWorldPtr spriteWorldP);

void SWAnimateSpriteWorld(
    SpriteWorldPtr spriteWorldP);

void SWCheckIdleSpriteOverlap(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       idleSpriteP,
    SpritePtr       headActiveSpriteP);

void SWCollectIdleSpriteIntersectingRects(
    SWRect checkRect1,
    SWRect checkRect2,
    SWRect *enclosingRect);

void SWFindSpritesToBeRemoved(
    SpriteWorldPtr spriteWorldP );

SWError SWFlagRectAsChanged(
    SpriteWorldPtr spriteWorldP,
    SWRect* theChangedRect);


SWError SWMergeUpdateRect(
    SpriteWorldPtr spriteWorldP,
    SWRect  theChangedRect,
    int *countP);

void SWRemoveUpdateRectStruct(
    SpriteWorldPtr spriteWorldP,
    UpdateRectStructPtr removeRect);


void SWResetMovementTimer(
    SpriteWorldPtr spriteWorldP);

short SWHasPseudoFrameFired(
    SpriteWorldPtr spriteWorldP);

float SWGetCurrentFPS(SpriteWorldPtr spriteWorldP);

void SWSetCleanUpSpriteWorld(
    SpriteWorldPtr spriteWorldP);

void SWSetCleanUpFunction(
    CleanUpCallBackPtr callBackP);

unsigned long SWGetSpriteWorldVersion(void);

short SWGetSpriteWorldPixelDepth( SpriteWorldPtr spriteWorldP );

unsigned long SWGetMilliseconds(SpriteWorldPtr spriteWorldP);

void SWClearStickyError(void);
SWError SWStickyError(void);
void SWSetStickyIfError(SWError errNum);
void SWAssertFail(char *filename, char *functionName, int lineNum);

#ifdef __cplusplus
}

SW_NAMESPACE_BEGIN

#include "SpriteWorld.hpp"

SW_NAMESPACE_END

#endif /* __cplusplus */

#endif /* __SPRITEWORLD__ */

