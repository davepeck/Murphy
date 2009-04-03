//---------------------------------------------------------------------------------------
/// @file Scrolling.h
/// Constants, structures, and function prototypes for scrolling SpriteWorlds.
//
//  By: Vern Jensen 8/29/95  updated 6/21/03 for SWX
//---------------------------------------------------------------------------------------


#ifndef __SCROLLING__
#define __SCROLLING__

#ifndef __TILING__
#include <Tiling.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------
//  scrolling function prototypes
//---------------------------------------------------------------------------------------

void SWUpdateScrollingWindow(
    SpriteWorldPtr spriteWorldP);

void SWUpdateScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP,
    SWBoolean updateWindow);

void SWProcessScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP);

void SWAnimateScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP);

void SWFastAnimateScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP);

void SWUnwrapWorkAreaToBackground(
    SpriteWorldPtr spriteWorldP);

void SWDrawWrappedSprite(
    SpritePtr srcSpriteP,
    FramePtr dstFrameP,
    SWRect* srcRect,
    SWRect* dstRect);

void SWEraseWrappedSprite(
    SpriteWorldPtr spriteWorldP,
    SWRect* dstRect);

void SWWrapWorkAreaToFrame(
    SpriteWorldPtr  spriteWorldP,
    FramePtr dstFrameP,
    SWRect *dstRectP);

void SWWrapRectToScreen(
    SpriteWorldPtr  spriteWorldP,
    SWRect          *srcRectP);

void SWCheckWrappedIdleSpriteOverlap(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       idleSpriteP,
    SpritePtr       headActiveSpriteP);

void SWCheckWrappedIdleSpritesWithTiles(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       headIdleSpriteP);

void SWCheckWrappedIdleSpritesWithRects(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       headIdleSpriteP);

void SWSetScrollingWorldMoveBounds(
    SpriteWorldPtr  spriteWorldP,
    SWRect* scrollRectMoveBounds);

void SWSetScrollingWorldMoveProc(
    SpriteWorldPtr spriteWorldP,
    WorldMoveProcPtr worldMoveProcP,
    SpritePtr followSpriteP);

//void SWSetDoubleRectDrawProc(
    //SpriteWorldPtr        spriteWorldP,
    //DoubleDrawProcPtr doubleRectDrawProc);

void SWSetSpriteWorldScrollDelta(
    SpriteWorldPtr  spriteWorldP,
    short           horizDelta,
    short           vertDelta);

void SWMoveVisScrollRect(
    SpriteWorldPtr  spriteWorldP,
    short           horizPos,
    short           vertPos);

void SWOffsetVisScrollRect(
    SpriteWorldPtr  spriteWorldP,
    short           horizOffset,
    short           vertOffset);

void SWOffsetVisScrollRectBasedOnTime(
    SpriteWorldPtr spriteWorldP,
    SWMovementType horizOffset,
    SWMovementType vertOffset);

void SWResizeVisScrollRect(
    SpriteWorldPtr  spriteWorldP,
    short           horizSize,
    short           vertSize);

void SWCalculateOffscreenScrollRect(
    SpriteWorldPtr spriteWorldP);


#ifdef __cplusplus
}

SW_NAMESPACE_BEGIN

#include "Scrolling.hpp"

SW_NAMESPACE_END

#endif /* __cplusplus */

#endif /* __SCROLLING__ */


