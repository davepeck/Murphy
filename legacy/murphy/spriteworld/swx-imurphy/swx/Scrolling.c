//---------------------------------------------------------------------------------------
//  Scrolling.c
//
//  By: Vern Jensen
//
//  Created: 8/29/95 updated 6/21/03 for SWX
//
//  Description:    Routines for making fast scrolling games with SpriteWorld
//---------------------------------------------------------------------------------------

#define SW_PRIVATE_STRUCTURES

#ifndef __SWCOMMON__
#include <SWCommonHeaders.h>
#endif

#ifndef __SPRITEWORLD__
#include <SpriteWorld.h>
#endif

//#ifndef __SPRITEWORLDUTILS__
//#include <SpriteWorldUtils.h>
//#endif

#ifndef __SCROLLING__
#include <Scrolling.h>
#endif

#ifndef __TILING__
#include <Tiling.h>
#endif

#ifndef __BLITKERNEL__
#include <BlitKernel.h>
#endif

    extern SWBoolean gSWUseOpenGL;

//---------------------------------------------------------------------------------------
//  SWUpdateScrollingWindow
//---------------------------------------------------------------------------------------

void SWUpdateScrollingWindow(
    SpriteWorldPtr spriteWorldP)
{
    SW_ASSERT(spriteWorldP != NULL);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked);
    SW_ASSERT(spriteWorldP->screenFrameP->isFrameLocked);

    //SWWaitForVBL( spriteWorldP );

    gSWCurrentSpriteWorld = spriteWorldP;


#ifdef HAVE_OPENGL
    if (spriteWorldP->useOpenGL)
        SDL_GL_SwapBuffers();
    else
#endif
    {
        // Copy visScrollRect to window
    SWWrapWorkAreaToFrame(spriteWorldP, spriteWorldP->screenFrameP, &spriteWorldP->screenRect);

    }

    gSWCurrentSpriteWorld = NULL;
}


//---------------------------------------------------------------------------------------
//  SWUpdateScrollingSpriteWorld - should be called at the beginning of
//  a scrolling animation, so that idle sprites are set up correctly.
//---------------------------------------------------------------------------------------

void SWUpdateScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP,
    SWBoolean updateWindow)
{
    UpdateRectStructPtr     curRectStructP, nextRectStructP;
    register SpriteLayerPtr     curSpriteLayerP;
    register SpritePtr      curSpriteP;
    short               curTileLayer;
    SWRect              *visScrollRectP = &spriteWorldP->visScrollRect;
    SWRect              srcFrameRect, tempDstRect;

    SW_ASSERT(spriteWorldP != NULL);
//  SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked);
//  SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked);
//  SW_ASSERT(spriteWorldP->screenFrameP->isFrameLocked);

    gSWCurrentSpriteWorld = spriteWorldP;

#ifdef HAVE_OPENGL
    if (spriteWorldP->useOpenGL)
    {
        struct BK_GL_func *gl = BKLoadGLfunctions();
        
        gl->glClearColor(
            spriteWorldP->backgroundColor.r * (1.0f/255.0f),
            spriteWorldP->backgroundColor.g * (1.0f/255.0f),
            spriteWorldP->backgroundColor.b * (1.0f/255.0f),
            1.0f);
        gl->glClear(GL_COLOR_BUFFER_BIT);

        SWDrawTileLayersInWorkArea(spriteWorldP);
    }
    else
#endif
    if (spriteWorldP->backFrameP != NULL)
    {

        // Copy the background into the work area
    (*spriteWorldP->offscreenDrawProc)(spriteWorldP->backFrameP,
                        spriteWorldP->workFrameP,
                        &spriteWorldP->backFrameP->frameRect,
                        &spriteWorldP->workFrameP->frameRect);

    }

            // Call the postEraseCallBack
    if (spriteWorldP->postEraseCallBack != NULL)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);


        // Build the current frame of the animation in the work area

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

        // iterate through the layers in this world
    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

            // iterate through the sprites in this layer
        while (curSpriteP != NULL)
        {
                // it's possible to have frameless sprites
            if (curSpriteP->curFrameP != NULL)
                SW_ASSERT(curSpriteP->curFrameP->isFrameLocked);

            curSpriteP->tileDepth = curTileLayer;

            if (curSpriteP->isVisible)
            {
                curSpriteP->destOffscreenRect = curSpriteP->destFrameRect;
                srcFrameRect = curSpriteP->curFrameP->frameRect;


                    // Clip the sprite's destOffscreenRect with visScrollRect
                    // We use the destOffscreenRect both for drawing and erasing later
                if (curSpriteP->destOffscreenRect.top < visScrollRectP->top)
                {
                    srcFrameRect.top += visScrollRectP->top -
                            curSpriteP->destOffscreenRect.top;
                    curSpriteP->destOffscreenRect.top =  visScrollRectP->top;
                }

                if (curSpriteP->destOffscreenRect.bottom > visScrollRectP->bottom)
                {
                    srcFrameRect.bottom += visScrollRectP->bottom -
                            curSpriteP->destOffscreenRect.bottom;
                    curSpriteP->destOffscreenRect.bottom = visScrollRectP->bottom;
                }

                if (curSpriteP->destOffscreenRect.left < visScrollRectP->left)
                {
                    srcFrameRect.left += visScrollRectP->left -
                            curSpriteP->destOffscreenRect.left;
                    curSpriteP->destOffscreenRect.left = visScrollRectP->left;
                }

                if (curSpriteP->destOffscreenRect.right > visScrollRectP->right)
                {
                    srcFrameRect.right += visScrollRectP->right -
                            curSpriteP->destOffscreenRect.right;
                    curSpriteP->destOffscreenRect.right = visScrollRectP->right;
                }

                curSpriteP->destRectIsVisible =
                        ( (curSpriteP->destOffscreenRect.right >
                        curSpriteP->destOffscreenRect.left) &&
                        (curSpriteP->destOffscreenRect.bottom >
                        curSpriteP->destOffscreenRect.top) );


                    // Is sprite visible on the screen?
                if (curSpriteP->destRectIsVisible)
                {
                    if (spriteWorldP->useOpenGL)
                    {
                    
                        // Make the sprite's rect local to the offscreen area
                    tempDstRect.top = curSpriteP->destFrameRect.top - spriteWorldP->visScrollRect.top;
                    tempDstRect.left = curSpriteP->destFrameRect.left - spriteWorldP->visScrollRect.left;
                    tempDstRect.bottom = curSpriteP->destFrameRect.bottom - spriteWorldP->visScrollRect.top;
                    tempDstRect.right = curSpriteP->destFrameRect.right - spriteWorldP->visScrollRect.left;

                    gSWCurrentElementDrawData = curSpriteP->drawData;

                        // Draw the sprite in the work area
                    SWDrawWrappedSprite(curSpriteP, spriteWorldP->workFrameP,
                        &srcFrameRect, &tempDstRect);
                    
                                        gSWCurrentElementDrawData = NULL;

                        // Draw tiles above sprite
                    if (spriteWorldP->tilingIsOn &&
                        curSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                    {
                        tempDstRect = curSpriteP->destFrameRect;

                        SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curSpriteP->tileDepth);
                    }
                        
                    }
                    else // if (!spriteWorldP->useOpenGL)
                    {       
                        // Make the sprite's rect local to the offscreen area
                    curSpriteP->destOffscreenRect.top -= spriteWorldP->vertScrollRectOffset;
                    curSpriteP->destOffscreenRect.bottom -= spriteWorldP->vertScrollRectOffset;
                    curSpriteP->destOffscreenRect.left -= spriteWorldP->horizScrollRectOffset;
                    curSpriteP->destOffscreenRect.right -= spriteWorldP->horizScrollRectOffset;

                    gSWCurrentElementDrawData = curSpriteP->drawData;

                        // Draw the sprite in the work area
                    SWDrawWrappedSprite(curSpriteP, spriteWorldP->workFrameP,
                        &srcFrameRect, &curSpriteP->destOffscreenRect);
                    
                                        gSWCurrentElementDrawData = NULL;

                        // Draw tiles above sprite
                    if (spriteWorldP->tilingIsOn &&
                        curSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                    {
                        tempDstRect = curSpriteP->destOffscreenRect;
                        tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                        tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                        tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                        tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                        SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curSpriteP->tileDepth);
                    }

                    } // OpenGL
                }
            }

                // Set last rect to current rect
            curSpriteP->oldFrameRect = curSpriteP->destFrameRect;
            curSpriteP->oldRectIsVisible = curSpriteP->destRectIsVisible;

            curSpriteP->needsToBeDrawn = false;
            curSpriteP->needsToBeErased = false;

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    spriteWorldP->oldVisScrollRect = spriteWorldP->visScrollRect;


            // Call the postDrawCallBack
    if (spriteWorldP->postDrawCallBack != NULL)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);


        // Copy the work area to the window
    if (updateWindow)
    {
        //SWWaitForVBL(spriteWorldP);

#ifdef HAVE_OPENGL
    if (spriteWorldP->useOpenGL)
        SDL_GL_SwapBuffers();
    else
#endif
        {

            // Copy visScrollRect to window
        SWWrapWorkAreaToFrame(spriteWorldP, spriteWorldP->screenFrameP, &spriteWorldP->screenRect);


    //todo justin update screen
        }
    
    }

        // dispose of flagged background rects
    nextRectStructP = spriteWorldP->headUpdateRectP;
    while ( nextRectStructP != NULL )
    {
        curRectStructP = nextRectStructP;
        nextRectStructP = curRectStructP->nextRectStructP;
        free( curRectStructP );
    }
    spriteWorldP->headUpdateRectP = NULL;

    spriteWorldP->numTilesChanged = 0;

        // This is so time-based animations work correctly.
    SWResetMovementTimer(spriteWorldP);

    gSWCurrentSpriteWorld = NULL;
}


//---------------------------------------------------------------------------------------
//  SWProcessScrollingSpriteWorld
//---------------------------------------------------------------------------------------

void SWProcessScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP)
{
        // Process the sprites
    SWProcessSpriteWorld(spriteWorldP);
    if ( !spriteWorldP->frameHasOccurred )
    {
        return;
    }

    gSWCurrentSpriteWorld = spriteWorldP;


        // Call the scrolling world move proc
    if (spriteWorldP->worldMoveProc != NULL)
    {
        (*spriteWorldP->worldMoveProc)(spriteWorldP, spriteWorldP->followSpriteP);
    }


        // Move visScrollRect
    if (spriteWorldP->horizScrollDelta || spriteWorldP->vertScrollDelta)
    {
        SWOffsetVisScrollRect(spriteWorldP,
            spriteWorldP->horizScrollDelta,
            spriteWorldP->vertScrollDelta);
    }

    SWProcessNonScrollingLayers(spriteWorldP);

    gSWCurrentSpriteWorld = NULL;
}


//---------------------------------------------------------------------------------------
//  SWAnimateScrollingSpriteWorld
//---------------------------------------------------------------------------------------

void SWAnimateScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP)
{
    UpdateRectStructPtr     curRectStructP, nextRectStructP;
    register SpriteLayerPtr     curSpriteLayerP;
    register SpritePtr      curSpriteP;
    SpritePtr           headActiveSpriteP = NULL;   // Tail of active sprite list
    SpritePtr           headIdleSpriteP = NULL;     // Tail of idle sprite list
    SpritePtr           curActiveSpriteP = NULL;
    SpritePtr           curIdleSpriteP = NULL;
    SWRect              *visScrollRectP = &spriteWorldP->visScrollRect;
    SWRect              tempDstRect, tempSrcRect;
    short               hScrollDelta, vScrollDelta, curTileLayer;
    short               oldVertScrollRectOffset, oldHorizScrollRectOffset;

    SW_ASSERT(spriteWorldP != NULL);
//  SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked);
//  SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked);
//  SW_ASSERT(spriteWorldP->screenFrameP->isFrameLocked);

    if (!spriteWorldP->frameHasOccurred)
        return;

    if (spriteWorldP->useOpenGL)
    {
        SWUpdateScrollingSpriteWorld(spriteWorldP, true );
        return;
    }
    
        gSWCurrentSpriteWorld = spriteWorldP;
        
    oldVertScrollRectOffset = spriteWorldP->backRect.bottom *
        (spriteWorldP->oldVisScrollRect.top / spriteWorldP->backRect.bottom);

    oldHorizScrollRectOffset = spriteWorldP->backRect.right *
        (spriteWorldP->oldVisScrollRect.left / spriteWorldP->backRect.right);

    hScrollDelta = visScrollRectP->left - spriteWorldP->oldVisScrollRect.left;
    vScrollDelta = visScrollRectP->top - spriteWorldP->oldVisScrollRect.top;


            // Add the deadSpriteLayer if there are any Sprites in it.
    if ( spriteWorldP->deadSpriteLayerP->headSpriteP != NULL )
    {
        SWAddSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);
    }


        // Update tiles as we scroll if tiling is turned on
    if (spriteWorldP->tilingIsOn)
    {
            // VisScrollRect moved horizontally
        if (hScrollDelta)
        {
                // Get rect of new vertical section to update
            tempDstRect = *visScrollRectP;

                // Moved left
            if (hScrollDelta < 0)
            {
                if (tempDstRect.right > spriteWorldP->oldVisScrollRect.left)
                    tempDstRect.right = spriteWorldP->oldVisScrollRect.left;
            }
            else    // Moved right
            {
                if (tempDstRect.left < spriteWorldP->oldVisScrollRect.right)
                    tempDstRect.left = spriteWorldP->oldVisScrollRect.right;
            }

            (*spriteWorldP->tileRectDrawProc)(spriteWorldP, &tempDstRect, true);
            SWWrapRectToWorkArea(spriteWorldP, &tempDstRect);


                // Did VisScrollRect moved diagonally?
            if (vScrollDelta)
            {
                    // Get rect of new horizontal section to update
                tempDstRect = spriteWorldP->visScrollRect;

                    // Moved up
                if (vScrollDelta < 0)
                {
                    if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.top)
                        tempDstRect.bottom = spriteWorldP->oldVisScrollRect.top;
                }
                else    // Moved down
                {
                    if (tempDstRect.top < spriteWorldP->oldVisScrollRect.bottom)
                        tempDstRect.top = spriteWorldP->oldVisScrollRect.bottom;
                }

                    // Clip off the part we've already updated
                if (hScrollDelta < 0)
                {
                    if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                        tempDstRect.left = spriteWorldP->oldVisScrollRect.left;
                }
                else
                {
                    if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                        tempDstRect.right = spriteWorldP->oldVisScrollRect.right;
                }

                    // We pass false here to avoid a bug which occured in the
                    // tile optimizing code when updating tiles twice in one frame
                if (tempDstRect.right > tempDstRect.left)
                {
                    (*spriteWorldP->tileRectDrawProc)(spriteWorldP, &tempDstRect, false);
                    SWWrapRectToWorkArea(spriteWorldP, &tempDstRect);
                }
            }
        }       // VisScrollRect moved vertically only
        else if (vScrollDelta)
        {
                // Get rect of new horizontal section to update
            tempDstRect = *visScrollRectP;

                // Moved up
            if (vScrollDelta < 0)
            {
                if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.top)
                    tempDstRect.bottom = spriteWorldP->oldVisScrollRect.top;
            }
            else    // Moved down
            {
                if (tempDstRect.top < spriteWorldP->oldVisScrollRect.bottom)
                    tempDstRect.top = spriteWorldP->oldVisScrollRect.bottom;
            }

            (*spriteWorldP->tileRectDrawProc)(spriteWorldP, &tempDstRect, true);
            SWWrapRectToWorkArea(spriteWorldP, &tempDstRect);
        }
    }

    //-----------------erase the sprites--------------------

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

        // iterate through the layers in this world
    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

            // iterate through the sprites in this layer
        while (curSpriteP != NULL)
        {
                // it's possible to have frameless sprites
            if (curSpriteP->curFrameP != NULL)
                SW_ASSERT(curSpriteP->curFrameP->isFrameLocked);

            curSpriteP->tileDepth = curTileLayer;

                // Clip the sprite's destOffscreenRect with visScrollRect.
            if (curSpriteP->isVisible)
            {
                curSpriteP->destOffscreenRect = curSpriteP->destFrameRect;
                curSpriteP->clippedSourceRect = curSpriteP->curFrameP->frameRect;


                if (curSpriteP->destOffscreenRect.top < visScrollRectP->top)
                {
                    curSpriteP->clippedSourceRect.top += visScrollRectP->top -
                            curSpriteP->destOffscreenRect.top;
                    curSpriteP->destOffscreenRect.top = visScrollRectP->top;
                }

                if (curSpriteP->destOffscreenRect.bottom > visScrollRectP->bottom)
                {
                    curSpriteP->clippedSourceRect.bottom += visScrollRectP->bottom -
                            curSpriteP->destOffscreenRect.bottom;
                    curSpriteP->destOffscreenRect.bottom = visScrollRectP->bottom;
                }

                if (curSpriteP->destOffscreenRect.left < visScrollRectP->left)
                {
                    curSpriteP->clippedSourceRect.left += visScrollRectP->left -
                            curSpriteP->destOffscreenRect.left;
                    curSpriteP->destOffscreenRect.left = visScrollRectP->left;
                }

                if (curSpriteP->destOffscreenRect.right > visScrollRectP->right)
                {
                    curSpriteP->clippedSourceRect.right += visScrollRectP->right -
                            curSpriteP->destOffscreenRect.right;
                    curSpriteP->destOffscreenRect.right = visScrollRectP->right;
                }

                curSpriteP->destRectIsVisible =
                        (curSpriteP->destOffscreenRect.right >
                        curSpriteP->destOffscreenRect.left &&
                        curSpriteP->destOffscreenRect.bottom >
                        curSpriteP->destOffscreenRect.top);
            }


                // Erase the sprites
            if ((curSpriteP->needsToBeDrawn && curSpriteP->isVisible) ||
                (curSpriteP->needsToBeErased && !curSpriteP->isVisible))
            {
                // Recalculate the oldOffscreenRect (tempDstRect) from scratch. We can't save the
                // destOffscreenRect from the previous frame, since an invisible Sprite that
                // is made visible many frames later could have an incorrect oldOffscreenRect,
                // meaning the Sprite would be erased even if it's not currently on the screen.

                tempDstRect = curSpriteP->oldFrameRect;

                if (tempDstRect.top < spriteWorldP->oldVisScrollRect.top)
                    tempDstRect.top = spriteWorldP->oldVisScrollRect.top;

                if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.bottom)
                    tempDstRect.bottom = spriteWorldP->oldVisScrollRect.bottom;

                if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                    tempDstRect.left = spriteWorldP->oldVisScrollRect.left;

                if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                    tempDstRect.right = spriteWorldP->oldVisScrollRect.right;

                    // Was the sprite visible on the screen last frame?
                if (tempDstRect.right > tempDstRect.left &&
                    tempDstRect.bottom > tempDstRect.top)
                {
                        // Make the tempDstRect local to the offscreen area
                    tempDstRect.top -= oldVertScrollRectOffset;
                    tempDstRect.bottom -= oldVertScrollRectOffset;
                    tempDstRect.left -= oldHorizScrollRectOffset;
                    tempDstRect.right -= oldHorizScrollRectOffset;

                        // Add sprite to active sprite list
                    if (headActiveSpriteP == NULL)
                        headActiveSpriteP = curSpriteP;

                    if (curActiveSpriteP != NULL)
                        curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                    curActiveSpriteP = curSpriteP;

                                        /*
                    {
                        short temp;

                            // align left edge of tempDstRect for erasing
                        tempDstRect.left &=
                            (spriteWorldP->workFrameP->leftAlignFactor);

                            // align the right edge to long word boundary
                        temp = tempDstRect.right &
                                spriteWorldP->workFrameP->rightAlignFactor;
                        if (temp != 0)
                        {
                            tempDstRect.right +=
                                (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                        }

                            // align left edge of oldFrameRect - necessary for
                            // deltaFrameRect below, used by idle sprite collision
                        curSpriteP->oldFrameRect.left &=
                            (spriteWorldP->workFrameP->leftAlignFactor);

                            // align the right edge to long word boundary
                        temp = curSpriteP->oldFrameRect.right &
                            spriteWorldP->workFrameP->rightAlignFactor;
                        if (temp != 0)
                        {
                            curSpriteP->oldFrameRect.right +=
                                (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                        }
                    }
                                        */
                                        
                        // union last rect and current rect - this is necessary for
                        // the proper redrawing of idle sprites
                    curSpriteP->deltaFrameRect.top =
                        SW_MIN(curSpriteP->oldFrameRect.top, curSpriteP->destFrameRect.top);
                    curSpriteP->deltaFrameRect.left =
                        SW_MIN(curSpriteP->oldFrameRect.left, curSpriteP->destFrameRect.left);
                    curSpriteP->deltaFrameRect.bottom =
                        SW_MAX(curSpriteP->oldFrameRect.bottom, curSpriteP->destFrameRect.bottom);
                    curSpriteP->deltaFrameRect.right =
                        SW_MAX(curSpriteP->oldFrameRect.right, curSpriteP->destFrameRect.right);


                        // Erase the sprite from the work area
                    SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
                }
                else if (curSpriteP->destRectIsVisible) // Sprite will be drawn
                {
                        // Add sprite to active sprite list
                    if (headActiveSpriteP == NULL)
                        headActiveSpriteP = curSpriteP;

                    if (curActiveSpriteP != NULL)
                            curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                    curActiveSpriteP = curSpriteP;
                }
            }
            else if (curSpriteP->isVisible)     // Visible, idle sprites
            {
                if (curSpriteP->oldRectIsVisible)
                {
                        // Is idle sprite moving outside the visScrollRect?
                    if ((hScrollDelta > 0 &&
                        (curSpriteP->destFrameRect.left < visScrollRectP->left) &&
                        (curSpriteP->destFrameRect.right > spriteWorldP->oldVisScrollRect.left)) ||
                        (hScrollDelta < 0 &&
                        (curSpriteP->destFrameRect.left < spriteWorldP->oldVisScrollRect.right) &&
                        (curSpriteP->destFrameRect.right > visScrollRectP->right)) )
                    {
                            // Erase piece of idle sprite outside of visScrollRect
                        tempDstRect = curSpriteP->oldFrameRect;

                            // Get section of sprite outside visScrollRect
                        if (hScrollDelta > 0)
                        {
                            if (tempDstRect.right > visScrollRectP->left)
                                tempDstRect.right = visScrollRectP->left;
                        }
                        else
                        {
                            if (tempDstRect.left < visScrollRectP->right)
                                tempDstRect.left = visScrollRectP->right;
                        }

                            // Clip tempDstRect with oldVisScrollRect
                        if (tempDstRect.top < spriteWorldP->oldVisScrollRect.top)
                            tempDstRect.top = spriteWorldP->oldVisScrollRect.top;
                        if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.bottom)
                            tempDstRect.bottom = spriteWorldP->oldVisScrollRect.bottom;
                        if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                            tempDstRect.left = spriteWorldP->oldVisScrollRect.left;
                        if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                            tempDstRect.right = spriteWorldP->oldVisScrollRect.right;

                            // Make the rect local to the offscreen area
                        tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
                        tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                        tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
                        tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

                        SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
                    }

                        // Is idle sprite moving outside the visScrollRect?
                    if ((vScrollDelta > 0 &&
                        (curSpriteP->destFrameRect.top < visScrollRectP->top) &&
                        (curSpriteP->destFrameRect.bottom > spriteWorldP->oldVisScrollRect.top)) ||
                        (vScrollDelta < 0 &&
                        (curSpriteP->destFrameRect.top < spriteWorldP->oldVisScrollRect.bottom) &&
                        (curSpriteP->destFrameRect.bottom > visScrollRectP->bottom)) )
                    {
                            // Erase piece of idle sprite outside of visScrollRect
                        tempDstRect = curSpriteP->oldFrameRect;

                            // Get section of sprite outside visScrollRect
                        if (vScrollDelta > 0)
                        {
                            if (tempDstRect.bottom > visScrollRectP->top)
                                tempDstRect.bottom = visScrollRectP->top;
                        }
                        else
                        {
                            if (tempDstRect.top < visScrollRectP->bottom)
                                tempDstRect.top = visScrollRectP->bottom;
                        }

                            // Clip tempDstRect with oldVisScrollRect
                        if (tempDstRect.top < spriteWorldP->oldVisScrollRect.top)
                            tempDstRect.top = spriteWorldP->oldVisScrollRect.top;
                        if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.bottom)
                            tempDstRect.bottom = spriteWorldP->oldVisScrollRect.bottom;
                        if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                            tempDstRect.left = spriteWorldP->oldVisScrollRect.left;
                        if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                            tempDstRect.right = spriteWorldP->oldVisScrollRect.right;

                            // Make the rect local to the offscreen area
                        tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
                        tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                        tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
                        tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

                        SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
                    }
                }


                    // Is the idle sprite visible on the screen?
                if (curSpriteP->destRectIsVisible)
                {
                        // Add sprite to idle sprite list
                    if (headIdleSpriteP == NULL)
                        headIdleSpriteP = curSpriteP;

                    if (curIdleSpriteP != NULL)
                        curIdleSpriteP->nextIdleSpriteP = curSpriteP;

                    curIdleSpriteP = curSpriteP;
                }
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    if (curActiveSpriteP != NULL)
        curActiveSpriteP->nextActiveSpriteP = NULL;

    if (curIdleSpriteP != NULL)
        curIdleSpriteP->nextIdleSpriteP = NULL;



        // This section of code iterates through the idle sprite list, drawing the tiny
        // sliver of any idle sprites that have just entered the visScrollRect.
    curIdleSpriteP = headIdleSpriteP;
    while (curIdleSpriteP != NULL)
    {
            // Draw vertical piece of idle sprite if it is coming into the visScrollRect.
        if ((vScrollDelta > 0 &&
            (curIdleSpriteP->destFrameRect.top < visScrollRectP->bottom) &&
            (curIdleSpriteP->destFrameRect.bottom > spriteWorldP->oldVisScrollRect.bottom) ) ||
            (vScrollDelta < 0 &&
            (curIdleSpriteP->destFrameRect.top < spriteWorldP->oldVisScrollRect.top) &&
            (curIdleSpriteP->destFrameRect.bottom > visScrollRectP->top)) )
        {
            tempDstRect = curIdleSpriteP->destOffscreenRect;
            tempSrcRect = curIdleSpriteP->clippedSourceRect;

                // Determine whether scrolling up or down, then get
                // section of sprite outside oldVisScrollRect.
            if (vScrollDelta < 0)
            {
                    // Scrolling up, so get section above oldVisScrollRect
                if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.top)
                {
                    tempSrcRect.bottom += spriteWorldP->oldVisScrollRect.top -
                        tempDstRect.bottom;
                    tempDstRect.bottom = spriteWorldP->oldVisScrollRect.top;
                }
            }
            else
            {
                    // Scrolling down, so get section below oldVisScrollRect
                if (tempDstRect.top < spriteWorldP->oldVisScrollRect.bottom)
                {
                    tempSrcRect.top += spriteWorldP->oldVisScrollRect.bottom -
                        tempDstRect.top;
                    tempDstRect.top = spriteWorldP->oldVisScrollRect.bottom;
                }
            }

                // Make the sprite's rect local to the offscreen area
            tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
            tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
            tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
            tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

                        gSWCurrentElementDrawData = curIdleSpriteP->drawData;

                // Draw the sprite in the work area
            SWDrawWrappedSprite(curIdleSpriteP, spriteWorldP->workFrameP,
                    &tempSrcRect, &tempDstRect);
                        
                        gSWCurrentElementDrawData = NULL;
                        
                // Draw tiles above sprite
            if (spriteWorldP->tilingIsOn &&
                curIdleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
            {
                tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curIdleSpriteP->tileDepth);
            }
        }


            // Draw horizontal piece of idle sprite if it is coming into the visScrollRect.
        if ((hScrollDelta > 0 &&
            (curIdleSpriteP->destFrameRect.left < visScrollRectP->right) &&
            (curIdleSpriteP->destFrameRect.right > spriteWorldP->oldVisScrollRect.right) ) ||
            (hScrollDelta < 0 &&
            (curIdleSpriteP->destFrameRect.left < spriteWorldP->oldVisScrollRect.left) &&
            (curIdleSpriteP->destFrameRect.right > visScrollRectP->left)) )
        {
            tempDstRect = curIdleSpriteP->destOffscreenRect;
            tempSrcRect = curIdleSpriteP->clippedSourceRect;

                // Determine whether scrolling left or right, then get
                // section of sprite outside oldVisScrollRect.
            if (hScrollDelta < 0)
            {
                    // Scrolling left, so get section to the left of oldVisScrollRect
                if (tempDstRect.right > spriteWorldP->oldVisScrollRect.left)
                {
                    tempSrcRect.right += spriteWorldP->oldVisScrollRect.left -
                        tempDstRect.right;
                    tempDstRect.right = spriteWorldP->oldVisScrollRect.left;
                }
            }
            else
            {
                    // Scrolling right, so get section to the right of oldVisScrollRect
                if (tempDstRect.left < spriteWorldP->oldVisScrollRect.right)
                {
                    tempSrcRect.left += spriteWorldP->oldVisScrollRect.right -
                        tempDstRect.left;
                    tempDstRect.left = spriteWorldP->oldVisScrollRect.right;
                }
            }


                // Make the sprite's rect local to the offscreen area
            tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
            tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
            tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
            tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

                        gSWCurrentElementDrawData = curIdleSpriteP->drawData;
                        
                // Draw the sprite in the work area
            SWDrawWrappedSprite(curIdleSpriteP, spriteWorldP->workFrameP,
                    &tempSrcRect, &tempDstRect);

                        gSWCurrentElementDrawData = NULL;
                        
                // Draw tiles above sprite
            if (spriteWorldP->tilingIsOn &&
                curIdleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
            {
                tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curIdleSpriteP->tileDepth);
            }
        }

        curIdleSpriteP = curIdleSpriteP->nextIdleSpriteP;
    }


        // update flagged background rects
    curRectStructP = spriteWorldP->headUpdateRectP;
    while ( curRectStructP != NULL )
    {
        tempDstRect = curRectStructP->updateRect;

            // Make the rect local to the offscreen area
        tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
        tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
        tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
        tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

            // We're not really erasing a sprite, just copying while wrapping
        SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
        curRectStructP = curRectStructP->nextRectStructP;
    }

        // Redraw idle sprites that were erased by a tile
    if (spriteWorldP->numTilesChanged > 0)
        SWCheckWrappedIdleSpritesWithTiles(spriteWorldP, headIdleSpriteP);

        // Redraw idle sprites that were erased by an updateRect
    if (spriteWorldP->headUpdateRectP != NULL)
        SWCheckWrappedIdleSpritesWithRects(spriteWorldP, headIdleSpriteP);

        // Call the postEraseCallBack
    if (spriteWorldP->postEraseCallBack != NULL)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);


    //-----------------draw the sprites-------------------

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

        // iterate through the layers in this world
    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

            // iterate through the sprites in this layer
        while (curSpriteP != NULL)
        {
            if (curSpriteP->isVisible)
            {
                    // Make the sprite's rect local to the offscreen area
                curSpriteP->destOffscreenRect.top -= spriteWorldP->vertScrollRectOffset;
                curSpriteP->destOffscreenRect.bottom -= spriteWorldP->vertScrollRectOffset;
                curSpriteP->destOffscreenRect.left -= spriteWorldP->horizScrollRectOffset;
                curSpriteP->destOffscreenRect.right -= spriteWorldP->horizScrollRectOffset;

                if (curSpriteP->needsToBeDrawn)
                {
                        // Is the sprite visible on the screen?
                    if (curSpriteP->destRectIsVisible)
                    {
                        gSWCurrentElementDrawData = curSpriteP->drawData;

                            // Draw the sprite in the work area
                        SWDrawWrappedSprite(curSpriteP, spriteWorldP->workFrameP,
                            &curSpriteP->clippedSourceRect,
                            &curSpriteP->destOffscreenRect);

                        gSWCurrentElementDrawData = NULL;

                            // Draw tiles above sprite
                        if (spriteWorldP->tilingIsOn &&
                            curSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                        {
                            tempDstRect = curSpriteP->destOffscreenRect;
                            tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                            tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                            tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                            tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                            SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curSpriteP->tileDepth);
                        }
                    }
                }
                else
                {
                        // Is the idle sprite visible on the screen?
                    if (curSpriteP->destRectIsVisible)
                    {
                        SWCheckWrappedIdleSpriteOverlap(spriteWorldP,
                            curSpriteP, headActiveSpriteP);
                    }
                }
            }

                // Set last rect to current rect
            curSpriteP->oldFrameRect = curSpriteP->destFrameRect;
            curSpriteP->oldRectIsVisible = curSpriteP->destRectIsVisible;

            curSpriteP->needsToBeDrawn = false;
            curSpriteP->needsToBeErased = false;

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    spriteWorldP->oldVisScrollRect = spriteWorldP->visScrollRect;


            // Call the postDrawCallBack
    if (spriteWorldP->postDrawCallBack != NULL)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);


    //-----------------update the screen--------------------

    //SWWaitForVBL(spriteWorldP);

        // Copy offscreen area to screen while wrapping
    SWWrapWorkAreaToFrame(spriteWorldP, spriteWorldP->screenFrameP, &spriteWorldP->screenRect);



            // dispose of flagged background rects
    nextRectStructP = spriteWorldP->headUpdateRectP;
    while ( nextRectStructP != NULL )
    {
        curRectStructP = nextRectStructP;
        nextRectStructP = curRectStructP->nextRectStructP;
        free( curRectStructP );
    }
    spriteWorldP->headUpdateRectP = NULL;

    spriteWorldP->numTilesChanged = 0;

    gSWCurrentSpriteWorld = NULL;

        // Remove the deadSpriteLayer if we added it earlier.
    if ( spriteWorldP->deadSpriteLayerP->headSpriteP != NULL )
    {
        SWRemoveSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);
    }

    //SetGWorld( saveGWorld, saveGDH );
}


//---------------------------------------------------------------------------------------
//  SWFastAnimateScrollingSpriteWorld - essentially the same as SWAnimateScrollingSpriteWorld,
//  except that this only updates that sprites/tiles that have moved or changed, by calling
//  SWWrapRectToScreen. All code that handles updating tiles/sprites while scrolling has
//  been removed. (Three parts: the part that draws new tiles, the part that erases idle
//  sprites leaving the visScrollRect, and the part that draws idle sprites entering it.)
//---------------------------------------------------------------------------------------

void SWFastAnimateScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP)
{
    UpdateRectStructPtr     curRectStructP, nextRectStructP;
    register SpriteLayerPtr     curSpriteLayerP;
    register SpritePtr      curSpriteP;
    SpritePtr           headActiveSpriteP = NULL;   // Tail of active sprite list
    SpritePtr           headIdleSpriteP = NULL;     // Tail of idle sprite list
    SpritePtr           curActiveSpriteP = NULL;
    SpritePtr           curIdleSpriteP = NULL;
    SWRect              *visScrollRectP = &spriteWorldP->visScrollRect;
    SWRect              tempDstRect;
    SWRect              *changedRectP;
    short               hScrollDelta, vScrollDelta, curTileLayer, index;
    short               oldVertScrollRectOffset, oldHorizScrollRectOffset;

    SW_ASSERT(spriteWorldP != NULL);
//  SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked);
//  SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked);
//  SW_ASSERT(spriteWorldP->screenFrameP->isFrameLocked);

    if (!spriteWorldP->frameHasOccurred)
        return;

    if (spriteWorldP->useOpenGL)
    {
        SWUpdateScrollingSpriteWorld(spriteWorldP, true );
        return;
    }

    gSWCurrentSpriteWorld = spriteWorldP;

    oldVertScrollRectOffset = spriteWorldP->backRect.bottom *
        (spriteWorldP->oldVisScrollRect.top / spriteWorldP->backRect.bottom);

    oldHorizScrollRectOffset = spriteWorldP->backRect.right *
        (spriteWorldP->oldVisScrollRect.left / spriteWorldP->backRect.right);

    hScrollDelta = visScrollRectP->left - spriteWorldP->oldVisScrollRect.left;
    vScrollDelta = visScrollRectP->top - spriteWorldP->oldVisScrollRect.top;
    if (hScrollDelta || vScrollDelta)
    {
            // We let this function do everything when scrolling
        SWAnimateScrollingSpriteWorld(spriteWorldP);
        return;
    }


        // Add the deadSpriteLayer if there are any Sprites in it.
    if ( spriteWorldP->deadSpriteLayerP->headSpriteP != NULL )
    {
        SWAddSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);
    }


    //-----------------erase the sprites--------------------

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

        // iterate through the layers in this world
    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

            // iterate through the sprites in this layer
        while (curSpriteP != NULL)
        {
                // it's possible to have frameless sprites
            if (curSpriteP->curFrameP != NULL)
                SW_ASSERT(curSpriteP->curFrameP->isFrameLocked);

            curSpriteP->tileDepth = curTileLayer;

                // Clip the sprite's destOffscreenRect with visScrollRect.
            if (curSpriteP->isVisible)
            {
                curSpriteP->destOffscreenRect = curSpriteP->destFrameRect;
                curSpriteP->clippedSourceRect = curSpriteP->curFrameP->frameRect;


                if (curSpriteP->destOffscreenRect.top < visScrollRectP->top)
                {
                    curSpriteP->clippedSourceRect.top += visScrollRectP->top -
                            curSpriteP->destOffscreenRect.top;
                    curSpriteP->destOffscreenRect.top = visScrollRectP->top;
                }

                if (curSpriteP->destOffscreenRect.bottom > visScrollRectP->bottom)
                {
                    curSpriteP->clippedSourceRect.bottom += visScrollRectP->bottom -
                            curSpriteP->destOffscreenRect.bottom;
                    curSpriteP->destOffscreenRect.bottom = visScrollRectP->bottom;
                }

                if (curSpriteP->destOffscreenRect.left < visScrollRectP->left)
                {
                    curSpriteP->clippedSourceRect.left += visScrollRectP->left -
                            curSpriteP->destOffscreenRect.left;
                    curSpriteP->destOffscreenRect.left = visScrollRectP->left;
                }

                if (curSpriteP->destOffscreenRect.right > visScrollRectP->right)
                {
                    curSpriteP->clippedSourceRect.right += visScrollRectP->right -
                            curSpriteP->destOffscreenRect.right;
                    curSpriteP->destOffscreenRect.right = visScrollRectP->right;
                }

                curSpriteP->destRectIsVisible =
                        (curSpriteP->destOffscreenRect.right >
                        curSpriteP->destOffscreenRect.left &&
                        curSpriteP->destOffscreenRect.bottom >
                        curSpriteP->destOffscreenRect.top);
            }


                // Erase the sprites
            if ((curSpriteP->needsToBeDrawn && curSpriteP->isVisible) ||
                (curSpriteP->needsToBeErased && !curSpriteP->isVisible))
            {
                // Recalculate the oldOffscreenRect (tempDstRect) from scratch. We can't save the
                // destOffscreenRect from the previous frame, since an invisible Sprite that
                // is made visible many frames later could have an incorrect oldOffscreenRect,
                // meaning the Sprite would be erased even if it's not currently on the screen.

                tempDstRect = curSpriteP->oldFrameRect;

                if (tempDstRect.top < spriteWorldP->oldVisScrollRect.top)
                    tempDstRect.top = spriteWorldP->oldVisScrollRect.top;

                if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.bottom)
                    tempDstRect.bottom = spriteWorldP->oldVisScrollRect.bottom;

                if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                    tempDstRect.left = spriteWorldP->oldVisScrollRect.left;

                if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                    tempDstRect.right = spriteWorldP->oldVisScrollRect.right;

                    // Was the sprite visible on the screen last frame?
                if (tempDstRect.right > tempDstRect.left &&
                    tempDstRect.bottom > tempDstRect.top)
                {
                        // Make the tempDstRect local to the offscreen area
                    tempDstRect.top -= oldVertScrollRectOffset;
                    tempDstRect.bottom -= oldVertScrollRectOffset;
                    tempDstRect.left -= oldHorizScrollRectOffset;
                    tempDstRect.right -= oldHorizScrollRectOffset;

                        // Add sprite to active sprite list
                    if (headActiveSpriteP == NULL)
                        headActiveSpriteP = curSpriteP;

                    if (curActiveSpriteP != NULL)
                            curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                    curActiveSpriteP = curSpriteP;

                                        /*
                    {
                        short temp;

                            // align left edge of tempDstRect for erasing
                        tempDstRect.left &=
                            (spriteWorldP->workFrameP->leftAlignFactor);

                            // align the right edge to long word boundary
                        temp = tempDstRect.right &
                            spriteWorldP->workFrameP->rightAlignFactor;
                        if (temp != 0)
                        {
                            tempDstRect.right +=
                                (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                        }

                            // align left edge of oldFrameRect - necessary for
                            // deltaFrameRect below, used by idle sprite collision
                        curSpriteP->oldFrameRect.left &=
                            (spriteWorldP->workFrameP->leftAlignFactor);

                            // align the right edge to long word boundary
                        temp = curSpriteP->oldFrameRect.right &
                            spriteWorldP->workFrameP->rightAlignFactor;
                        if (temp != 0)
                        {
                            curSpriteP->oldFrameRect.right +=
                                (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                        }
                    }
                                        */
                                        
                        // union last rect and current rect - this is necessary for
                        // the proper redrawing of idle sprites
                    curSpriteP->deltaFrameRect.top =
                        SW_MIN(curSpriteP->oldFrameRect.top, curSpriteP->destFrameRect.top);
                    curSpriteP->deltaFrameRect.left =
                        SW_MIN(curSpriteP->oldFrameRect.left, curSpriteP->destFrameRect.left);
                    curSpriteP->deltaFrameRect.bottom =
                        SW_MAX(curSpriteP->oldFrameRect.bottom, curSpriteP->destFrameRect.bottom);
                    curSpriteP->deltaFrameRect.right =
                        SW_MAX(curSpriteP->oldFrameRect.right, curSpriteP->destFrameRect.right);


                        // Erase the sprite from the work area
                    SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
                }
                else if (curSpriteP->destRectIsVisible) // Sprite will be drawn
                {
                        // Add sprite to active sprite list
                    if (headActiveSpriteP == NULL)
                        headActiveSpriteP = curSpriteP;

                    if (curActiveSpriteP != NULL)
                            curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                    curActiveSpriteP = curSpriteP;
                }
            }
            else if (curSpriteP->isVisible)     // Visible, idle sprites
            {
                    // Is the idle sprite visible on the screen?
                if (curSpriteP->destRectIsVisible)
                {
                        // Add sprite to idle sprite list
                    if (headIdleSpriteP == NULL)
                        headIdleSpriteP = curSpriteP;

                    if (curIdleSpriteP != NULL)
                        curIdleSpriteP->nextIdleSpriteP = curSpriteP;

                    curIdleSpriteP = curSpriteP;
                }
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    if (curActiveSpriteP != NULL)
        curActiveSpriteP->nextActiveSpriteP = NULL;

    if (curIdleSpriteP != NULL)
        curIdleSpriteP->nextIdleSpriteP = NULL;


        // update flagged background rects
    curRectStructP = spriteWorldP->headUpdateRectP;
    while ( curRectStructP != NULL )
    {
        tempDstRect = curRectStructP->updateRect;

            // Make the rect local to the offscreen area
        tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
        tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
        tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
        tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

            // We're not really erasing a sprite, just copying while wrapping
        SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
        curRectStructP = curRectStructP->nextRectStructP;
    }

        // Redraw idle sprites that were erased by a tile
    if (spriteWorldP->numTilesChanged > 0)
        SWCheckWrappedIdleSpritesWithTiles(spriteWorldP, headIdleSpriteP);

        // Redraw idle sprites that were erased by an updateRect
    if (spriteWorldP->headUpdateRectP != NULL)
        SWCheckWrappedIdleSpritesWithRects(spriteWorldP, headIdleSpriteP);

        // Call the postEraseCallBack
    if (spriteWorldP->postEraseCallBack != NULL)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);


    //-----------------draw the sprites-------------------

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

        // iterate through the layers in this world
    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

            // iterate through the sprites in this layer
        while (curSpriteP != NULL)
        {
            if (curSpriteP->isVisible)
            {
                    // Make the sprite's rect local to the offscreen area
                curSpriteP->destOffscreenRect.top -= spriteWorldP->vertScrollRectOffset;
                curSpriteP->destOffscreenRect.bottom -= spriteWorldP->vertScrollRectOffset;
                curSpriteP->destOffscreenRect.left -= spriteWorldP->horizScrollRectOffset;
                curSpriteP->destOffscreenRect.right -= spriteWorldP->horizScrollRectOffset;

                if (curSpriteP->needsToBeDrawn)
                {
                        // Is the sprite visible on the screen?
                    if (curSpriteP->destRectIsVisible)
                    {
                        gSWCurrentElementDrawData = curSpriteP->drawData;

                            // Draw the sprite in the work area
                        SWDrawWrappedSprite(curSpriteP, spriteWorldP->workFrameP,
                            &curSpriteP->clippedSourceRect,
                            &curSpriteP->destOffscreenRect);

                        gSWCurrentElementDrawData = NULL;

                            // Draw tiles above sprite
                        if (spriteWorldP->tilingIsOn &&
                            curSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                        {
                            tempDstRect = curSpriteP->destOffscreenRect;
                            tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                            tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                            tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                            tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                            SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curSpriteP->tileDepth);
                        }
                    }
                }
                else
                {
                        // Is the idle sprite visible on the screen?
                    if (curSpriteP->destRectIsVisible)
                    {
                        SWCheckWrappedIdleSpriteOverlap(spriteWorldP,
                            curSpriteP, headActiveSpriteP);
                    }
                }
            }

                // Set last rect to current rect
            curSpriteP->oldFrameRect = curSpriteP->destFrameRect;
            curSpriteP->oldRectIsVisible = curSpriteP->destRectIsVisible;

            curSpriteP->needsToBeDrawn = false;
            curSpriteP->needsToBeErased = false;

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    spriteWorldP->oldVisScrollRect = spriteWorldP->visScrollRect;


            // Call the postDrawCallBack
    if (spriteWorldP->postDrawCallBack != NULL)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);


    //-----------------update the screen--------------------

    //SWWaitForVBL(spriteWorldP);


        // update flagged background rects
    curRectStructP = spriteWorldP->headUpdateRectP;
    while ( curRectStructP != NULL )
    {
        SWWrapRectToScreen(spriteWorldP, &curRectStructP->updateRect);
        curRectStructP = curRectStructP->nextRectStructP;
    }

        // Update on screen the tiles that have changed
    changedRectP = spriteWorldP->changedTiles;
    for (index = 0; index < spriteWorldP->numTilesChanged; index++, changedRectP++)
    {
        SWWrapRectToScreen(spriteWorldP, changedRectP);
    }


        // update the sprites on the screen
    curSpriteP = headActiveSpriteP;
    while (curSpriteP != NULL)
    {
        SWWrapRectToScreen(spriteWorldP, &curSpriteP->deltaFrameRect);
        curSpriteP->deltaFrameRect = curSpriteP->destFrameRect;
        curSpriteP = curSpriteP->nextActiveSpriteP;
    }

            // dispose of flagged background rects
    nextRectStructP = spriteWorldP->headUpdateRectP;
    while ( nextRectStructP != NULL )
    {
        curRectStructP = nextRectStructP;
        nextRectStructP = curRectStructP->nextRectStructP;
        free( curRectStructP );
    }
    spriteWorldP->headUpdateRectP = NULL;

    spriteWorldP->numTilesChanged = 0;

    gSWCurrentSpriteWorld = NULL;

        // Remove the deadSpriteLayer if we added it earlier.
    if ( spriteWorldP->deadSpriteLayerP->headSpriteP != NULL )
    {
        SWRemoveSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);
    }
}


//---------------------------------------------------------------------------------------
//  SWUnwrapWorkAreaToBackground - copies the work area to the background while "unwrapping"
//  it, so the user can do special screen wipes or whatever to update the screen while copying
//  from the background.
//---------------------------------------------------------------------------------------

void SWUnwrapWorkAreaToBackground(
    SpriteWorldPtr spriteWorldP)
{
    SWRect  destRect;

    destRect = spriteWorldP->offscreenScrollRect;
    SW_OFFSET_RECT(destRect, -destRect.left, -destRect.top);

    SWWrapWorkAreaToFrame(spriteWorldP, spriteWorldP->backFrameP, &destRect);
}

#if 0
#pragma mark -
#endif

//---------------------------------------------------------------------------------------
//  SWDrawWrappedSprite - wraps image in dest area. For drawing sprites, not erasing,
//  since source rect stays the same.
//---------------------------------------------------------------------------------------

void SWDrawWrappedSprite(
    SpritePtr srcSpriteP,
    FramePtr dstFrameP,
    SWRect* srcRect,
    SWRect* dstRect)
{
    SWRect      tempDstRect;

    SW_ASSERT(srcSpriteP->frameDrawProc != NULL );
    SW_ASSERT(srcSpriteP->curFrameP != NULL );

        // Draw main image //
    (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP, srcRect, dstRect);

    if (!gSWUseOpenGL)
    {

        // Wrap to top //
    if (dstRect->bottom > dstFrameP->frameRect.bottom)
    {
        tempDstRect.top = dstRect->top - dstFrameP->frameRect.bottom;
        tempDstRect.bottom = dstRect->bottom - dstFrameP->frameRect.bottom;
        tempDstRect.left = dstRect->left;
        tempDstRect.right = dstRect->right;

        (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
            srcRect, &tempDstRect);


            // Wrap to upper left or right corner //
        if (dstRect->right > dstFrameP->frameRect.right)
        {
            tempDstRect.left -= dstFrameP->frameRect.right;
            tempDstRect.right -= dstFrameP->frameRect.right;

            (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
                srcRect, &tempDstRect);
        }
        else if (dstRect->left < dstFrameP->frameRect.left)
        {
            tempDstRect.left += dstFrameP->frameRect.right;
            tempDstRect.right += dstFrameP->frameRect.right;

            (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
                srcRect, &tempDstRect);
        }
    }

            // Wrap to left or right side //
    if (dstRect->right > dstFrameP->frameRect.right)
    {
        tempDstRect.top = dstRect->top;
        tempDstRect.bottom = dstRect->bottom;
        tempDstRect.left = dstRect->left - dstFrameP->frameRect.right;
        tempDstRect.right = dstRect->right - dstFrameP->frameRect.right;

        (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
            srcRect, &tempDstRect);
    }
    else if (dstRect->left < dstFrameP->frameRect.left)
    {
        tempDstRect.top = dstRect->top;
        tempDstRect.bottom = dstRect->bottom;
        tempDstRect.left = dstRect->left + dstFrameP->frameRect.right;
        tempDstRect.right = dstRect->right + dstFrameP->frameRect.right;

        (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
            srcRect, &tempDstRect);
    }


            // Wrap to bottom //
    if (dstRect->top < dstFrameP->frameRect.top)
    {
        tempDstRect.top = dstRect->top + dstFrameP->frameRect.bottom;
        tempDstRect.bottom = dstRect->bottom + dstFrameP->frameRect.bottom;
        tempDstRect.left = dstRect->left;
        tempDstRect.right = dstRect->right;

        (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
            srcRect, &tempDstRect);

            // Wrap to lower left or right corner //
        if (dstRect->right > dstFrameP->frameRect.right)
        {
            tempDstRect.left -= dstFrameP->frameRect.right;
            tempDstRect.right -= dstFrameP->frameRect.right;

            (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
                srcRect, &tempDstRect);
        }
        else if (dstRect->left < dstFrameP->frameRect.left)
        {
            tempDstRect.left += dstFrameP->frameRect.right;
            tempDstRect.right += dstFrameP->frameRect.right;

            (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
                srcRect, &tempDstRect);
        }
    }
    
    } // OpenGL
}


//---------------------------------------------------------------------------------------
//  SWEraseWrappedSprite - erases a wrapped sprite from the work area
//---------------------------------------------------------------------------------------

void SWEraseWrappedSprite(
    SpriteWorldPtr spriteWorldP,
    SWRect* dstRect)
{
    SWRect      tempDstRect;
    FramePtr    srcFrameP = spriteWorldP->backFrameP;
    FramePtr    dstFrameP = spriteWorldP->workFrameP;


        // Draw main image //
    (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP, dstRect, dstRect);


        // Wrap to top //
    if (dstRect->bottom > dstFrameP->frameRect.bottom)
    {
        tempDstRect.top = dstRect->top - dstFrameP->frameRect.bottom;
        tempDstRect.bottom = dstRect->bottom - dstFrameP->frameRect.bottom;
        tempDstRect.left = dstRect->left;
        tempDstRect.right = dstRect->right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDstRect, &tempDstRect);

            // Wrap to upper left or right corner //
        if (dstRect->right > dstFrameP->frameRect.right)
        {
            tempDstRect.left -= dstFrameP->frameRect.right;
            tempDstRect.right -= dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDstRect, &tempDstRect);
        }
        else if (dstRect->left < dstFrameP->frameRect.left)
        {
            tempDstRect.left += dstFrameP->frameRect.right;
            tempDstRect.right += dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDstRect, &tempDstRect);
        }
    }

            // Wrap to left or right side //
    if (dstRect->right > dstFrameP->frameRect.right)
    {
        tempDstRect.top = dstRect->top;
        tempDstRect.bottom = dstRect->bottom;
        tempDstRect.left = dstRect->left - dstFrameP->frameRect.right;
        tempDstRect.right = dstRect->right - dstFrameP->frameRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDstRect, &tempDstRect);
    }
    else if (dstRect->left < dstFrameP->frameRect.left)
    {
        tempDstRect.top = dstRect->top;
        tempDstRect.bottom = dstRect->bottom;
        tempDstRect.left = dstRect->left + dstFrameP->frameRect.right;
        tempDstRect.right = dstRect->right + dstFrameP->frameRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDstRect, &tempDstRect);
    }


            // Wrap to bottom //
    if (dstRect->top < dstFrameP->frameRect.top)
    {
        tempDstRect.top = dstRect->top + dstFrameP->frameRect.bottom;
        tempDstRect.bottom = dstRect->bottom + dstFrameP->frameRect.bottom;
        tempDstRect.left = dstRect->left;
        tempDstRect.right = dstRect->right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDstRect, &tempDstRect);

            // Wrap to lower left or right corner //
        if (dstRect->right > dstFrameP->frameRect.right)
        {
            tempDstRect.left -= dstFrameP->frameRect.right;
            tempDstRect.right -= dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDstRect, &tempDstRect);
        }
        else if (dstRect->left < dstFrameP->frameRect.left)
        {
            tempDstRect.left += dstFrameP->frameRect.right;
            tempDstRect.right += dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDstRect, &tempDstRect);
        }
    }
}


//---------------------------------------------------------------------------------------
//  SWWrapWorkAreaToFrame - copy source rect to dest rect while wrapping soruce. Assumes they
//  are the same size, and that dstRect will fit within the bounds of the dstFrameP. Used
//  for copying the work area to the screen, with the ability of "wrapping" the source
//  rect around the work area. Also used by SWUnwrapWorkAreaToBackground.
//---------------------------------------------------------------------------------------

void SWWrapWorkAreaToFrame(
    SpriteWorldPtr  spriteWorldP,
    FramePtr dstFrameP,
    SWRect *dstRectP)
{
    FramePtr    srcFrameP = spriteWorldP->workFrameP;

        // We can always add code to clip dstRect later if we want.
        // Just make sure to clip it before clipping srcRect.
    SWRect  srcRect = spriteWorldP->offscreenScrollRect;
    SWRect  dstRect = *dstRectP;
    SWRect  srcRectA, srcRectB, dstRectA, dstRectB;

                // Size of area that was clipped in source rect
    short   topClip=0, rightClip=0, bottomClip=0, leftClip=0;


        // Clip the source rect, and save what we clipped for wrapping later //

        // clip off the top
    if (srcRect.top < srcFrameP->frameRect.top)
    {
        topClip = srcFrameP->frameRect.top - srcRect.top;
        srcRect.top += topClip;
    }

        // clip off the bottom
    if (srcRect.bottom > srcFrameP->frameRect.bottom)
    {
        bottomClip = srcRect.bottom - srcFrameP->frameRect.bottom;
        srcRect.bottom -= bottomClip;
    }

        // clip off the left
    if (srcRect.left < srcFrameP->frameRect.left)
    {
        leftClip = srcFrameP->frameRect.left - srcRect.left;
        srcRect.left += leftClip;
    }

        // clip off the right
    if (srcRect.right > srcFrameP->frameRect.right)
    {
        rightClip = srcRect.right - srcFrameP->frameRect.right;
        srcRect.right -= rightClip;
    }


                    // Here we do the wrapping and drawing //

        // Draw top section //

    if (topClip)
    {
                // Calculate top piece //

            // Wrap source rect to bottom side
        srcRectA.right = srcRect.right;                 // Copy clipped source rect
        srcRectA.left = srcRect.left;
        srcRectA.bottom = srcFrameP->frameRect.bottom;
        srcRectA.top = srcFrameP->frameRect.bottom - topClip;

            // Position dest rect at top side
        dstRectA.top = dstRect.top;
        dstRectA.bottom = dstRect.top + topClip;
        dstRectA.left = dstRect.left + leftClip;
        dstRectA.right = dstRect.right - rightClip;


        if (leftClip)   // Calculate top-left piece
        {
                // Wrap source rect to lower-right corner
            srcRectB.bottom = srcFrameP->frameRect.bottom;
            srcRectB.right = srcFrameP->frameRect.right;
            srcRectB.top = srcFrameP->frameRect.bottom - topClip;
            srcRectB.left = srcFrameP->frameRect.right - leftClip;

                // Position dest rect at top-left corner
            dstRectB.left = dstRect.left;
            dstRectB.top = dstRect.top;
            dstRectB.right = dstRect.left + leftClip;
            dstRectB.bottom = dstRect.top + topClip;

            /*if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                    // RectB is passed first, since it is to the left of rectA
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectB, &dstRectB, &srcRectA, &dstRectA);
            }
            else*/
            //{
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
            //}
        }
        else if (rightClip)     // Calculate top-right piece
        {
                // Wrap source rect to lower-left corner
            srcRectB.bottom = srcFrameP->frameRect.bottom;
            srcRectB.left = srcFrameP->frameRect.left;
            srcRectB.right = srcFrameP->frameRect.left + rightClip;
            srcRectB.top = srcFrameP->frameRect.bottom - topClip;

                // Position dest rect at top-right corner
            dstRectB.top = dstRect.top;
            dstRectB.right = dstRect.right;
            dstRectB.bottom = dstRect.top + topClip;
            dstRectB.left = dstRect.right - rightClip;

            /*if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                     &srcRectA, &dstRectA, &srcRectB, &dstRectB);
            }
            else*/
            //{
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            //}
        }
        else
        {
                // Draw just the top rect
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
        }
    }


            // Draw middle section //

        // Calculate main middle piece (not wrapped)
    dstRectA.left = dstRect.left + leftClip;
    dstRectA.top = dstRect.top + topClip;
    dstRectA.right = dstRect.right - rightClip;
    dstRectA.bottom = dstRect.bottom - bottomClip;


    if (leftClip)   // Draw left piece
    {
            // Wrap source rect to right side
        srcRectB.top = srcRect.top;             // Copy clipped source rect
        srcRectB.bottom = srcRect.bottom;
        srcRectB.right = srcFrameP->frameRect.right;
        srcRectB.left = srcFrameP->frameRect.right - leftClip;

            // Position dest rect at left side
        dstRectB.left = dstRect.left;
        dstRectB.right = dstRect.left + leftClip;
        dstRectB.top = dstRect.top + topClip;
        dstRectB.bottom = dstRect.bottom - bottomClip;

        /*if (spriteWorldP->doubleRectDrawProc != NULL)
        {
                // RectB is passed first, since it is to the left of rectA
            (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                &srcRectB, &dstRectB, &srcRect, &dstRectA);
        }
        else*/
        //{
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
        //}
    }
    else if (rightClip)     // Draw right piece
    {
            // Wrap source rect to left side
        srcRectB.top = srcRect.top;             // Copy clipped source rect
        srcRectB.bottom = srcRect.bottom;
        srcRectB.left = srcFrameP->frameRect.left;
        srcRectB.right = srcFrameP->frameRect.left + rightClip;

            // Position dest rect at right side
        dstRectB.right = dstRect.right;
        dstRectB.left = dstRect.right - rightClip;
        dstRectB.top = dstRect.top + topClip;
        dstRectB.bottom = dstRect.bottom - bottomClip;

        /*if (spriteWorldP->doubleRectDrawProc != NULL)
        {
            (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                &srcRect, &dstRectA, &srcRectB, &dstRectB);
        }
        else*/
        //{
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
        //}
    }
    else
    {
            // Draw just the middle piece
        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
    }


        // Draw bottom section //

    if (bottomClip)
    {
            // Calculate bottom piece //

            // Wrap source rect to top side
        srcRectA.right = srcRect.right;             // Copy clipped source rect
        srcRectA.left = srcRect.left;
        srcRectA.top = srcFrameP->frameRect.top;
        srcRectA.bottom = srcFrameP->frameRect.top + bottomClip;

            // Position dest rect at bottom side
        dstRectA.bottom = dstRect.bottom;
        dstRectA.top = dstRect.bottom - bottomClip;
        dstRectA.left = dstRect.left + leftClip;
        dstRectA.right = dstRect.right - rightClip;


        if (leftClip)   // Draw bottom-left piece
        {
                // Wrap source rect to upper-right corner
            srcRectB.top = srcFrameP->frameRect.top;
            srcRectB.right = srcFrameP->frameRect.right;
            srcRectB.bottom = srcFrameP->frameRect.top + bottomClip;
            srcRectB.left = srcFrameP->frameRect.right - leftClip;

                // Position dest rect at bottom-left corner
            dstRectB.bottom = dstRect.bottom;
            dstRectB.left = dstRect.left;
            dstRectB.top = dstRect.bottom - bottomClip;
            dstRectB.right = dstRect.left + leftClip;

            /*if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                    // RectB is passed first, since it is to the left of rectA
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectB, &dstRectB, &srcRectA, &dstRectA);
            }
            else*/
            //{
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
            //}
        }
        else if (rightClip)     // Draw bottom-right piece
        {
                // Wrap source rect to upper-left corner
            srcRectB.top = srcFrameP->frameRect.top;
            srcRectB.left = srcFrameP->frameRect.left;
            srcRectB.bottom = srcFrameP->frameRect.top + bottomClip;
            srcRectB.right = srcFrameP->frameRect.left + rightClip;

                // Position dest rect at bottom-right corner
            dstRectB.bottom = dstRect.bottom;
            dstRectB.right = dstRect.right;
            dstRectB.top = dstRect.bottom - bottomClip;
            dstRectB.left = dstRect.right - rightClip;

            /*if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectA, &dstRectA, &srcRectB, &dstRectB);
            }
            else*/
            //{
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            //}
        }
        else
        {
                // Draw just the bottom clip
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
        }
    }
}


//---------------------------------------------------------------------------------------
//  SWWrapRectToScreen - essentially the same as SWWrapWorkAreaToFrame, except with some
//  extra code at the beginning to calculate the dstRect based on the srcRect, and clip it.
//  Used by SWFastAnimateScrollingSpriteWorld.
//---------------------------------------------------------------------------------------

void SWWrapRectToScreen(
    SpriteWorldPtr  spriteWorldP,
    SWRect          *srcRectP)
{
    FramePtr    srcFrameP = spriteWorldP->workFrameP;
    FramePtr    dstFrameP = spriteWorldP->screenFrameP;
    SWRect      srcRect, dstRect, srcRectA, srcRectB, dstRectA, dstRectB;
    short       topClip=0, rightClip=0, bottomClip=0, leftClip=0;

    srcRect = *srcRectP;

        // dstRect = srcRect - spriteWorldP->visScrollRect + spriteWorldP->windRect;
    dstRect = srcRect;
    dstRect.top -= spriteWorldP->visScrollRect.top;
    dstRect.bottom -= spriteWorldP->visScrollRect.top;
    dstRect.left -= spriteWorldP->visScrollRect.left;
    dstRect.right -= spriteWorldP->visScrollRect.left;
    dstRect.top += spriteWorldP->screenRect.top;
    dstRect.bottom += spriteWorldP->screenRect.top;
    dstRect.left += spriteWorldP->screenRect.left;
    dstRect.right += spriteWorldP->screenRect.left;

        // Clip dstRect
    if (dstRect.top < spriteWorldP->screenRect.top)
    {
        srcRect.top += spriteWorldP->screenRect.top - dstRect.top;
        dstRect.top = spriteWorldP->screenRect.top;
    }

    if (dstRect.bottom > spriteWorldP->screenRect.bottom)
    {
        srcRect.bottom += spriteWorldP->screenRect.bottom - dstRect.bottom;
        dstRect.bottom = spriteWorldP->screenRect.bottom;
    }

    if (dstRect.left < spriteWorldP->screenRect.left)
    {
        srcRect.left += spriteWorldP->screenRect.left - dstRect.left;
        dstRect.left = spriteWorldP->screenRect.left;
    }

    if (dstRect.right > spriteWorldP->screenRect.right)
    {
        srcRect.right += spriteWorldP->screenRect.right - dstRect.right;
        dstRect.right = spriteWorldP->screenRect.right;
    }

        // Make sure rect is visible
    if ( (dstRect.right <= dstRect.left) || (dstRect.bottom <= dstRect.top) )
        return;

        // Make the srcRect local to the offscreen area
    srcRect.top -= spriteWorldP->vertScrollRectOffset;
    srcRect.bottom -= spriteWorldP->vertScrollRectOffset;
    srcRect.left -= spriteWorldP->horizScrollRectOffset;
    srcRect.right -= spriteWorldP->horizScrollRectOffset;


        // Clip the source rect, and save what we clipped for wrapping later //

        // clip off the top
    if (srcRect.top < srcFrameP->frameRect.top)
    {
        topClip = srcFrameP->frameRect.top - srcRect.top;
        srcRect.top += topClip;
    }

        // clip off the bottom
    if (srcRect.bottom > srcFrameP->frameRect.bottom)
    {
        bottomClip = srcRect.bottom - srcFrameP->frameRect.bottom;
        srcRect.bottom -= bottomClip;
    }

        // clip off the left
    if (srcRect.left < srcFrameP->frameRect.left)
    {
        leftClip = srcFrameP->frameRect.left - srcRect.left;
        srcRect.left += leftClip;
    }

        // clip off the right
    if (srcRect.right > srcFrameP->frameRect.right)
    {
        rightClip = srcRect.right - srcFrameP->frameRect.right;
        srcRect.right -= rightClip;
    }


                    // Here we do the wrapping and drawing //

        // Draw top section //

    if (topClip)
    {
                // Calculate top piece //

            // Wrap source rect to bottom side
        srcRectA.right = srcRect.right;                 // Copy clipped source rect
        srcRectA.left = srcRect.left;
        srcRectA.bottom = srcFrameP->frameRect.bottom;
        srcRectA.top = srcFrameP->frameRect.bottom - topClip;

            // Position dest rect at top side
        dstRectA.top = dstRect.top;
        dstRectA.bottom = dstRect.top + topClip;
        dstRectA.left = dstRect.left + leftClip;
        dstRectA.right = dstRect.right - rightClip;


        if (leftClip)   // Calculate top-left piece
        {
                // Wrap source rect to lower-right corner
            srcRectB.bottom = srcFrameP->frameRect.bottom;
            srcRectB.right = srcFrameP->frameRect.right;
            srcRectB.top = srcFrameP->frameRect.bottom - topClip;
            srcRectB.left = srcFrameP->frameRect.right - leftClip;

                // Position dest rect at top-left corner
            dstRectB.left = dstRect.left;
            dstRectB.top = dstRect.top;
            dstRectB.right = dstRect.left + leftClip;
            dstRectB.bottom = dstRect.top + topClip;

            /*if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                    // RectB is passed first, since it is to the left of rectA
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectB, &dstRectB, &srcRectA, &dstRectA);
            }
            else*/
            //{
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
            //}
        }
        else if (rightClip)     // Calculate top-right piece
        {
                // Wrap source rect to lower-left corner
            srcRectB.bottom = srcFrameP->frameRect.bottom;
            srcRectB.left = srcFrameP->frameRect.left;
            srcRectB.right = srcFrameP->frameRect.left + rightClip;
            srcRectB.top = srcFrameP->frameRect.bottom - topClip;

                // Position dest rect at top-right corner
            dstRectB.top = dstRect.top;
            dstRectB.right = dstRect.right;
            dstRectB.bottom = dstRect.top + topClip;
            dstRectB.left = dstRect.right - rightClip;

            /*if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                     &srcRectA, &dstRectA, &srcRectB, &dstRectB);
            }
            else*/
            //{
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            //}
        }
        else
        {
                // Draw just the top rect
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
        }
    }


            // Draw middle section //

        // Calculate main middle piece (not wrapped)
    dstRectA.left = dstRect.left + leftClip;
    dstRectA.top = dstRect.top + topClip;
    dstRectA.right = dstRect.right - rightClip;
    dstRectA.bottom = dstRect.bottom - bottomClip;


    if (leftClip)   // Draw left piece
    {
            // Wrap source rect to right side
        srcRectB.top = srcRect.top;             // Copy clipped source rect
        srcRectB.bottom = srcRect.bottom;
        srcRectB.right = srcFrameP->frameRect.right;
        srcRectB.left = srcFrameP->frameRect.right - leftClip;

            // Position dest rect at left side
        dstRectB.left = dstRect.left;
        dstRectB.right = dstRect.left + leftClip;
        dstRectB.top = dstRect.top + topClip;
        dstRectB.bottom = dstRect.bottom - bottomClip;

        /*if (spriteWorldP->doubleRectDrawProc != NULL)
        {
                // RectB is passed first, since it is to the left of rectA
            (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                &srcRectB, &dstRectB, &srcRect, &dstRectA);
        }
        else*/
        //{
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
        //}
    }
    else if (rightClip)     // Draw right piece
    {
            // Wrap source rect to left side
        srcRectB.top = srcRect.top;             // Copy clipped source rect
        srcRectB.bottom = srcRect.bottom;
        srcRectB.left = srcFrameP->frameRect.left;
        srcRectB.right = srcFrameP->frameRect.left + rightClip;

            // Position dest rect at right side
        dstRectB.right = dstRect.right;
        dstRectB.left = dstRect.right - rightClip;
        dstRectB.top = dstRect.top + topClip;
        dstRectB.bottom = dstRect.bottom - bottomClip;

        /*if (spriteWorldP->doubleRectDrawProc != NULL)
        {
            (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                &srcRect, &dstRectA, &srcRectB, &dstRectB);
        }
        else*/
        //{
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
        //}
    }
    else
    {
            // Draw just the middle piece
        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
    }


        // Draw bottom section //

    if (bottomClip)
    {
            // Calculate bottom piece //

            // Wrap source rect to top side
        srcRectA.right = srcRect.right;             // Copy clipped source rect
        srcRectA.left = srcRect.left;
        srcRectA.top = srcFrameP->frameRect.top;
        srcRectA.bottom = srcFrameP->frameRect.top + bottomClip;

            // Position dest rect at bottom side
        dstRectA.bottom = dstRect.bottom;
        dstRectA.top = dstRect.bottom - bottomClip;
        dstRectA.left = dstRect.left + leftClip;
        dstRectA.right = dstRect.right - rightClip;


        if (leftClip)   // Draw bottom-left piece
        {
                // Wrap source rect to upper-right corner
            srcRectB.top = srcFrameP->frameRect.top;
            srcRectB.right = srcFrameP->frameRect.right;
            srcRectB.bottom = srcFrameP->frameRect.top + bottomClip;
            srcRectB.left = srcFrameP->frameRect.right - leftClip;

                // Position dest rect at bottom-left corner
            dstRectB.bottom = dstRect.bottom;
            dstRectB.left = dstRect.left;
            dstRectB.top = dstRect.bottom - bottomClip;
            dstRectB.right = dstRect.left + leftClip;

            /*if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                    // RectB is passed first, since it is to the left of rectA
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectB, &dstRectB, &srcRectA, &dstRectA);
            }
            else*/
            //{
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
            //}
        }
        else if (rightClip)     // Draw bottom-right piece
        {
                // Wrap source rect to upper-left corner
            srcRectB.top = srcFrameP->frameRect.top;
            srcRectB.left = srcFrameP->frameRect.left;
            srcRectB.bottom = srcFrameP->frameRect.top + bottomClip;
            srcRectB.right = srcFrameP->frameRect.left + rightClip;

                // Position dest rect at bottom-right corner
            dstRectB.bottom = dstRect.bottom;
            dstRectB.right = dstRect.right;
            dstRectB.top = dstRect.bottom - bottomClip;
            dstRectB.left = dstRect.right - rightClip;

            /*if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectA, &dstRectA, &srcRectB, &dstRectB);
            }
            else*/
            //{
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            //}
        }
        else
        {
                // Draw just the bottom clip
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
        }
    }


}


//---------------------------------------------------------------------------------------
//  SWCheckWrappedIdleSpriteOverlap - same as SWCheckIdleSpriteOverlap,
//  but for scrolling SpriteWorlds.
//---------------------------------------------------------------------------------------

void SWCheckWrappedIdleSpriteOverlap(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       idleSpriteP,
    SpritePtr       headActiveSpriteP)
{
    SWRect              *visScrollRectP = &spriteWorldP->visScrollRect;
    register SpritePtr  activeSpriteP = headActiveSpriteP;
    SWRect              srcSectRect,
                        dstSectRect;


        // iterate through the active sprites
    while (activeSpriteP != NULL)
    {
            // do the sprites overlap?
        if ((idleSpriteP->oldFrameRect.top < activeSpriteP->deltaFrameRect.bottom) &&
             (idleSpriteP->oldFrameRect.bottom > activeSpriteP->deltaFrameRect.top) &&
             (idleSpriteP->oldFrameRect.left < activeSpriteP->deltaFrameRect.right) &&
             (idleSpriteP->oldFrameRect.right > activeSpriteP->deltaFrameRect.left))
        {
                // calculate the intersection between the idle sprite's destination
                // rect, and the active sprite's delta rect
            dstSectRect.left =
                SW_MAX(idleSpriteP->destFrameRect.left, activeSpriteP->deltaFrameRect.left);
            dstSectRect.top =
                SW_MAX(idleSpriteP->destFrameRect.top, activeSpriteP->deltaFrameRect.top);
            dstSectRect.right =
                SW_MIN(idleSpriteP->destFrameRect.right, activeSpriteP->deltaFrameRect.right);
            dstSectRect.bottom =
                SW_MIN(idleSpriteP->destFrameRect.bottom, activeSpriteP->deltaFrameRect.bottom);

                // Clip the sprite's dstSectRect with visScrollRect
            if (dstSectRect.top < visScrollRectP->top)
                dstSectRect.top = visScrollRectP->top;
            if (dstSectRect.bottom > visScrollRectP->bottom)
                dstSectRect.bottom = visScrollRectP->bottom;
            if (dstSectRect.left < visScrollRectP->left)
                dstSectRect.left = visScrollRectP->left;
            if (dstSectRect.right > visScrollRectP->right)
                dstSectRect.right = visScrollRectP->right;

                // Calculate the source rect
            srcSectRect = idleSpriteP->curFrameP->frameRect;

            srcSectRect.left += (dstSectRect.left - idleSpriteP->destFrameRect.left);
            srcSectRect.top += (dstSectRect.top - idleSpriteP->destFrameRect.top);
            srcSectRect.right -= (idleSpriteP->destFrameRect.right - dstSectRect.right);
            srcSectRect.bottom -= (idleSpriteP->destFrameRect.bottom - dstSectRect.bottom);


                // Make the sprite's dest rect local to the offscreen area
            dstSectRect.top -= spriteWorldP->vertScrollRectOffset;
            dstSectRect.bottom -= spriteWorldP->vertScrollRectOffset;
            dstSectRect.left -= spriteWorldP->horizScrollRectOffset;
            dstSectRect.right -= spriteWorldP->horizScrollRectOffset;

            gSWCurrentElementDrawData = idleSpriteP->drawData;

                // Copy a piece of the sprite image onto the back drop piece
            SWDrawWrappedSprite(idleSpriteP, spriteWorldP->workFrameP,
                &srcSectRect, &dstSectRect);

            gSWCurrentElementDrawData = NULL;
        
            if (spriteWorldP->tilingIsOn &&
                idleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
            {
                dstSectRect.top += spriteWorldP->vertScrollRectOffset;
                dstSectRect.bottom += spriteWorldP->vertScrollRectOffset;
                dstSectRect.left += spriteWorldP->horizScrollRectOffset;
                dstSectRect.right += spriteWorldP->horizScrollRectOffset;
                SWDrawTilesAboveSprite(spriteWorldP, &dstSectRect, idleSpriteP->tileDepth);
            }
        }

        activeSpriteP = activeSpriteP->nextActiveSpriteP;
    }
}


//---------------------------------------------------------------------------------------
//  SWCheckWrappedIdleSpritesWithTiles - redraw sprites erased by tiles that changed
//---------------------------------------------------------------------------------------

void SWCheckWrappedIdleSpritesWithTiles(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       headIdleSpriteP)
{
    SWRect      *visScrollRectP = &spriteWorldP->visScrollRect;
    SWRect      srcSectRect, dstSectRect;
    register    SpritePtr idleSpriteP;
    SWRect      *changedRectP;
    short       index;


        // Cycle through the changedTiles array of rects
    changedRectP = spriteWorldP->changedTiles;
    for (index = 0; index < spriteWorldP->numTilesChanged; index++, changedRectP++)
    {
        idleSpriteP = headIdleSpriteP;

            // iterate through the idle sprites
        while (idleSpriteP != NULL)
        {
                // does the idle sprite overlap the changedRect?
            if ((idleSpriteP->oldFrameRect.top < changedRectP->bottom) &&
                 (idleSpriteP->oldFrameRect.bottom > changedRectP->top) &&
                 (idleSpriteP->oldFrameRect.left < changedRectP->right) &&
                 (idleSpriteP->oldFrameRect.right > changedRectP->left))
            {
                    // calculate the intersection between the idle sprite's old
                    // rect and the changedRectP
                dstSectRect.left = SW_MAX(idleSpriteP->oldFrameRect.left,
                                        changedRectP->left);
                dstSectRect.top = SW_MAX(idleSpriteP->oldFrameRect.top,
                                        changedRectP->top);
                dstSectRect.right = SW_MIN(idleSpriteP->oldFrameRect.right,
                                        changedRectP->right);
                dstSectRect.bottom = SW_MIN(idleSpriteP->oldFrameRect.bottom,
                                        changedRectP->bottom);

                    // Clip the sprite's dstSectRect with visScrollRect
                if (dstSectRect.top < visScrollRectP->top)
                    dstSectRect.top = visScrollRectP->top;
                if (dstSectRect.bottom > visScrollRectP->bottom)
                    dstSectRect.bottom = visScrollRectP->bottom;
                if (dstSectRect.left < visScrollRectP->left)
                    dstSectRect.left = visScrollRectP->left;
                if (dstSectRect.right > visScrollRectP->right)
                    dstSectRect.right = visScrollRectP->right;

                    // Calculate the source rect
                srcSectRect = idleSpriteP->curFrameP->frameRect;

                srcSectRect.left += (dstSectRect.left - idleSpriteP->oldFrameRect.left);
                srcSectRect.top += (dstSectRect.top - idleSpriteP->oldFrameRect.top);
                srcSectRect.right -= (idleSpriteP->oldFrameRect.right - dstSectRect.right);
                srcSectRect.bottom -= (idleSpriteP->oldFrameRect.bottom - dstSectRect.bottom);


                    // Make the sprite's dest rect local to the offscreen area
                dstSectRect.top -= spriteWorldP->vertScrollRectOffset;
                dstSectRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstSectRect.left -= spriteWorldP->horizScrollRectOffset;
                dstSectRect.right -= spriteWorldP->horizScrollRectOffset;

                gSWCurrentElementDrawData = idleSpriteP->drawData;

                    // Copy a piece of the sprite image onto the back drop piece
                SWDrawWrappedSprite(idleSpriteP, spriteWorldP->workFrameP,
                    &srcSectRect, &dstSectRect);

                                gSWCurrentElementDrawData = NULL;

                if (spriteWorldP->tilingIsOn &&
                    idleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                {
                    dstSectRect.top += spriteWorldP->vertScrollRectOffset;
                    dstSectRect.bottom += spriteWorldP->vertScrollRectOffset;
                    dstSectRect.left += spriteWorldP->horizScrollRectOffset;
                    dstSectRect.right += spriteWorldP->horizScrollRectOffset;
                    SWDrawTilesAboveSprite(spriteWorldP, &dstSectRect, idleSpriteP->tileDepth);
                }
            }

            idleSpriteP = idleSpriteP->nextIdleSpriteP;
        }
    }
}


//---------------------------------------------------------------------------------------
//  SWCheckWrappedIdleSpritesWithRects - redraw sprites erased by updateRects
//---------------------------------------------------------------------------------------
void SWCheckWrappedIdleSpritesWithRects(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       headIdleSpriteP)
{
    UpdateRectStructPtr curRectStructP;
    register SpritePtr  idleSpriteP;
    SWRect          *visScrollRectP = &spriteWorldP->visScrollRect;
    SWRect          srcSectRect, dstSectRect;
    SWRect          *changedRectP;


    curRectStructP = spriteWorldP->headUpdateRectP;

    while (curRectStructP != NULL)
    {
        changedRectP = &curRectStructP->updateRect;
        idleSpriteP = headIdleSpriteP;

            // iterate through the idle sprites
        while (idleSpriteP != NULL)
        {
                // does the idle sprite overlap the changedRect?
            if ((idleSpriteP->oldFrameRect.top < changedRectP->bottom) &&
                 (idleSpriteP->oldFrameRect.bottom > changedRectP->top) &&
                 (idleSpriteP->oldFrameRect.left < changedRectP->right) &&
                 (idleSpriteP->oldFrameRect.right > changedRectP->left))
            {
                    // calculate the intersection between the idle sprite's old
                    // rect and the changedRectP
                dstSectRect.left = SW_MAX(idleSpriteP->oldFrameRect.left,
                                        changedRectP->left);
                dstSectRect.top = SW_MAX(idleSpriteP->oldFrameRect.top,
                                        changedRectP->top);
                dstSectRect.right = SW_MIN(idleSpriteP->oldFrameRect.right,
                                        changedRectP->right);
                dstSectRect.bottom = SW_MIN(idleSpriteP->oldFrameRect.bottom,
                                        changedRectP->bottom);

                    // Clip the sprite's dstSectRect with visScrollRect
                if (dstSectRect.top < visScrollRectP->top)
                    dstSectRect.top = visScrollRectP->top;
                if (dstSectRect.bottom > visScrollRectP->bottom)
                    dstSectRect.bottom = visScrollRectP->bottom;
                if (dstSectRect.left < visScrollRectP->left)
                    dstSectRect.left = visScrollRectP->left;
                if (dstSectRect.right > visScrollRectP->right)
                    dstSectRect.right = visScrollRectP->right;

                    // Calculate the source rect
                srcSectRect = idleSpriteP->curFrameP->frameRect;

                srcSectRect.left += (dstSectRect.left - idleSpriteP->oldFrameRect.left);
                srcSectRect.top += (dstSectRect.top - idleSpriteP->oldFrameRect.top);
                srcSectRect.right -= (idleSpriteP->oldFrameRect.right - dstSectRect.right);
                srcSectRect.bottom -= (idleSpriteP->oldFrameRect.bottom - dstSectRect.bottom);


                    // Make the sprite's dest rect local to the offscreen area
                dstSectRect.top -= spriteWorldP->vertScrollRectOffset;
                dstSectRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstSectRect.left -= spriteWorldP->horizScrollRectOffset;
                dstSectRect.right -= spriteWorldP->horizScrollRectOffset;

                gSWCurrentElementDrawData = idleSpriteP->drawData;

                    // Copy a piece of the sprite image onto the back drop piece
                SWDrawWrappedSprite(idleSpriteP, spriteWorldP->workFrameP,
                    &srcSectRect, &dstSectRect);

                gSWCurrentElementDrawData = NULL;

                if (spriteWorldP->tilingIsOn &&
                    idleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                {
                    dstSectRect.top += spriteWorldP->vertScrollRectOffset;
                    dstSectRect.bottom += spriteWorldP->vertScrollRectOffset;
                    dstSectRect.left += spriteWorldP->horizScrollRectOffset;
                    dstSectRect.right += spriteWorldP->horizScrollRectOffset;
                    SWDrawTilesAboveSprite(spriteWorldP, &dstSectRect, idleSpriteP->tileDepth);
                }
            }

            idleSpriteP = idleSpriteP->nextIdleSpriteP;
        }

        curRectStructP = curRectStructP->nextRectStructP;
    }
}

#if 0
#pragma mark -
#endif

//---------------------------------------------------------------------------------------
//  SWSetScrollingWorldMoveBounds
//---------------------------------------------------------------------------------------

void SWSetScrollingWorldMoveBounds(
    SpriteWorldPtr  spriteWorldP,
    SWRect* scrollRectMoveBounds)
{
    spriteWorldP->scrollRectMoveBounds = *scrollRectMoveBounds;

        // Make sure the visScrollRect is still within its bounds
    SWMoveVisScrollRect(spriteWorldP,
            spriteWorldP->visScrollRect.left,
            spriteWorldP->visScrollRect.top);
}


//---------------------------------------------------------------------------------------
//  SWSetScrollingWorldMoveProc
//---------------------------------------------------------------------------------------

void SWSetScrollingWorldMoveProc(
    SpriteWorldPtr spriteWorldP,
    WorldMoveProcPtr worldMoveProcP,
    SpritePtr followSpriteP)
{
    spriteWorldP->worldMoveProc = worldMoveProcP;
    spriteWorldP->followSpriteP = followSpriteP;
}


//---------------------------------------------------------------------------------------
//  SWSetDoubleRectDrawProc
//---------------------------------------------------------------------------------------

//void SWSetDoubleRectDrawProc(
    //SpriteWorldPtr        spriteWorldP,
    //DoubleDrawProcPtr drawProc)
//{
    //spriteWorldP->doubleRectDrawProc = drawProc;
//}


//---------------------------------------------------------------------------------------
//  SWSetSpriteWorldScrollDelta
//---------------------------------------------------------------------------------------

void SWSetSpriteWorldScrollDelta(
    SpriteWorldPtr  spriteWorldP,
    short           horizDelta,
    short           vertDelta)
{
    spriteWorldP->horizScrollDelta = horizDelta;
    spriteWorldP->vertScrollDelta = vertDelta;
}


//---------------------------------------------------------------------------------------
//  SWMoveVisScrollRect - move visScrollRect to an absolute vertical and
//  horizontal location
//---------------------------------------------------------------------------------------

void SWMoveVisScrollRect(
    SpriteWorldPtr  spriteWorldP,
    short           horizPos,
    short           vertPos)
{
    short   width, height;

    height = spriteWorldP->visScrollRect.bottom - spriteWorldP->visScrollRect.top;
    width = spriteWorldP->visScrollRect.right - spriteWorldP->visScrollRect.left;


            // Move visScrollRect and keep within moveBounds //


        // Move vertically
    if ((long)vertPos < spriteWorldP->scrollRectMoveBounds.top)
    {
        spriteWorldP->visScrollRect.top = spriteWorldP->scrollRectMoveBounds.top;
        spriteWorldP->visScrollRect.bottom = spriteWorldP->visScrollRect.top + height;
    }
    else if ((long)vertPos + height > spriteWorldP->scrollRectMoveBounds.bottom)
    {
        spriteWorldP->visScrollRect.bottom = spriteWorldP->scrollRectMoveBounds.bottom;
        spriteWorldP->visScrollRect.top = spriteWorldP->visScrollRect.bottom - height;
    }
    else
    {
        spriteWorldP->visScrollRect.top = vertPos;
        spriteWorldP->visScrollRect.bottom = vertPos + height;
    }


        // Move horizontally
    if ((long)horizPos < spriteWorldP->scrollRectMoveBounds.left)
    {
        spriteWorldP->visScrollRect.left = spriteWorldP->scrollRectMoveBounds.left;
        spriteWorldP->visScrollRect.right = spriteWorldP->visScrollRect.left + width;
    }
    else if ((long)horizPos + width > spriteWorldP->scrollRectMoveBounds.right)
    {
        spriteWorldP->visScrollRect.right = spriteWorldP->scrollRectMoveBounds.right;
        spriteWorldP->visScrollRect.left = spriteWorldP->visScrollRect.right - width;
    }
    else
    {
        spriteWorldP->visScrollRect.left = horizPos;
        spriteWorldP->visScrollRect.right = horizPos + width;
    }


    SWCalculateOffscreenScrollRect(spriteWorldP);
}


//---------------------------------------------------------------------------------------
//  SWOffsetVisScrollRect - move visScrollRect to an absolute vertical and
//  horizontal location, while keeping within the bounds of scrollRectMoveBounds
//---------------------------------------------------------------------------------------

void SWOffsetVisScrollRect(
    SpriteWorldPtr  spriteWorldP,
    short           horizOffset,
    short           vertOffset)
{
            // Move visScrollRect and keep within moveBounds //

        // Move vertically
    if ((long)spriteWorldP->visScrollRect.top + vertOffset <
            spriteWorldP->scrollRectMoveBounds.top)
    {
        spriteWorldP->visScrollRect.bottom -= spriteWorldP->visScrollRect.top -
            spriteWorldP->scrollRectMoveBounds.top;
        spriteWorldP->visScrollRect.top = spriteWorldP->scrollRectMoveBounds.top;
    }
    else if ((long)spriteWorldP->visScrollRect.bottom + vertOffset >
            spriteWorldP->scrollRectMoveBounds.bottom)
    {
        spriteWorldP->visScrollRect.top -= spriteWorldP->visScrollRect.bottom -
            spriteWorldP->scrollRectMoveBounds.bottom;
        spriteWorldP->visScrollRect.bottom = spriteWorldP->scrollRectMoveBounds.bottom;
    }
    else
    {
        spriteWorldP->visScrollRect.top += vertOffset;
        spriteWorldP->visScrollRect.bottom += vertOffset;
    }


        // Move horizontally
    if ((long)spriteWorldP->visScrollRect.left + horizOffset <
            spriteWorldP->scrollRectMoveBounds.left)
    {
        spriteWorldP->visScrollRect.right -= spriteWorldP->visScrollRect.left -
            spriteWorldP->scrollRectMoveBounds.left;
        spriteWorldP->visScrollRect.left = spriteWorldP->scrollRectMoveBounds.left;
    }
    else if ((long)spriteWorldP->visScrollRect.right + horizOffset >
            spriteWorldP->scrollRectMoveBounds.right)
    {
        spriteWorldP->visScrollRect.left -= spriteWorldP->visScrollRect.right -
            spriteWorldP->scrollRectMoveBounds.right;
        spriteWorldP->visScrollRect.right = spriteWorldP->scrollRectMoveBounds.right;
    }
    else
    {
        spriteWorldP->visScrollRect.left += horizOffset;
        spriteWorldP->visScrollRect.right += horizOffset;
    }


    SWCalculateOffscreenScrollRect(spriteWorldP);
}


//---------------------------------------------------------------------------------------
//  SWOffsetVisScrollRectBasedOnTime - use this with time-based animations.
//---------------------------------------------------------------------------------------

void SWOffsetVisScrollRectBasedOnTime(
    SpriteWorldPtr spriteWorldP,
    SWMovementType horizOffset,
    SWMovementType vertOffset)
{
    double          vertMoveDelta, horizMoveDelta;
    double          frameRate;

    SW_ASSERT(spriteWorldP != NULL);

    frameRate = spriteWorldP->pseudoFPS;
    horizMoveDelta = SWConvertOffsetBasedOnTime(spriteWorldP, horizOffset, frameRate);
    vertMoveDelta = SWConvertOffsetBasedOnTime(spriteWorldP, vertOffset, frameRate);

    SWOffsetVisScrollRect(spriteWorldP, (short)horizMoveDelta, (short)vertMoveDelta);
}


//---------------------------------------------------------------------------------------
//  SWResizeVisScrollRect - make the visScrollRect a new width and height.
//  Used by SWChangeWorldRect. Make sure to call SWUpdateScrollingSpriteWorld after
//  calling this.
//---------------------------------------------------------------------------------------

void SWResizeVisScrollRect(
    SpriteWorldPtr  spriteWorldP,
    short           width,
    short           height)
{
    short   offset;

        // Will expanding it vertically move it past its bounds?
    if ((long)spriteWorldP->visScrollRect.top + height >
        spriteWorldP->scrollRectMoveBounds.bottom)
    {
        spriteWorldP->visScrollRect.bottom = spriteWorldP->scrollRectMoveBounds.bottom;
        spriteWorldP->visScrollRect.top = spriteWorldP->visScrollRect.bottom - height;

            // Make sure we didn't push it too far up, into negative values.
        offset = spriteWorldP->scrollRectMoveBounds.top - spriteWorldP->visScrollRect.top;
        if (offset > 0)
        {
            spriteWorldP->visScrollRect.top += offset;
            spriteWorldP->visScrollRect.bottom += offset;
        }
    }
    else
    {
        spriteWorldP->visScrollRect.bottom = spriteWorldP->visScrollRect.top + height;
    }


        // Will expanding it horizontally move it past its bounds?
    if ((long)spriteWorldP->visScrollRect.left + width >
        spriteWorldP->scrollRectMoveBounds.right)
    {
        spriteWorldP->visScrollRect.right = spriteWorldP->scrollRectMoveBounds.right;
        spriteWorldP->visScrollRect.left = spriteWorldP->visScrollRect.right - width;

            // Make sure we didn't push it too far left, into negative values.
        offset = spriteWorldP->scrollRectMoveBounds.left - spriteWorldP->visScrollRect.left;
        if (offset > 0)
        {
            spriteWorldP->visScrollRect.left += offset;
            spriteWorldP->visScrollRect.right += offset;
        }
    }
    else
    {
        spriteWorldP->visScrollRect.right = spriteWorldP->visScrollRect.left + width;
    }

    spriteWorldP->oldVisScrollRect = spriteWorldP->visScrollRect;
    SWCalculateOffscreenScrollRect(spriteWorldP);
}

//---------------------------------------------------------------------------------------
//  SWCalculateOffscreenScrollRect (wrap offscreenScrollRect in the work area)
//---------------------------------------------------------------------------------------

void SWCalculateOffscreenScrollRect(
    SpriteWorldPtr spriteWorldP)
{
    SWRect *offscreenScrollRectP = &spriteWorldP->offscreenScrollRect;
    SWRect *visScrollRectP = &spriteWorldP->visScrollRect;


    spriteWorldP->vertScrollRectOffset = spriteWorldP->backRect.bottom *
        (visScrollRectP->top / spriteWorldP->backRect.bottom);

    spriteWorldP->horizScrollRectOffset = spriteWorldP->backRect.right *
        (visScrollRectP->left / spriteWorldP->backRect.right);


    offscreenScrollRectP->top = visScrollRectP->top -
            spriteWorldP->vertScrollRectOffset;
    offscreenScrollRectP->bottom = visScrollRectP->bottom -
            spriteWorldP->vertScrollRectOffset;
    offscreenScrollRectP->left = visScrollRectP->left -
            spriteWorldP->horizScrollRectOffset;
    offscreenScrollRectP->right = visScrollRectP->right -
            spriteWorldP->horizScrollRectOffset;
}


