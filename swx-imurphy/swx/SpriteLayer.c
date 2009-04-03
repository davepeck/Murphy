//---------------------------------------------------------------------------------------
//	SpriteLayer.c
//
//	Portions are copyright: © 1991-94 Tony Myles, All rights reserved worldwide.
//
//	Description:	implementation of the sprite layers
//---------------------------------------------------------------------------------------

#define SW_PRIVATE_STRUCTURES

#ifndef __SWCOMMON__
#include <SWCommonHeaders.h>
#endif

#ifndef __SPRITEWORLD__
#include <SpriteWorld.h>
#endif

#ifndef __SPRITELAYER__
#include <SpriteLayer.h>
#endif

#ifndef __SPRITE__
#include <Sprite.h>
#endif


#ifndef __TILING__
#include <Tiling.h>
#endif


//---------------------------------------------------------------------------------------
//	SWCreateSpriteLayer
//---------------------------------------------------------------------------------------

SWError SWCreateSpriteLayer(
	SpriteLayerPtr *spriteLayerP)
{
	SWError err;
	SpriteLayerPtr tempSpriteLayerP;

	err = kNoError;
	*spriteLayerP = NULL;

	tempSpriteLayerP = (SpriteLayerPtr) calloc(1,sizeof(SpriteLayerRec));

	if (tempSpriteLayerP != NULL)
	{
		tempSpriteLayerP->parentSpriteWorldP = NULL;

                
		tempSpriteLayerP->tileLayer = 10;
                
		tempSpriteLayerP->isPaused = false;
		tempSpriteLayerP->layerIsNonScrolling = false;
		tempSpriteLayerP->headSpriteP = NULL;

		*spriteLayerP = tempSpriteLayerP;
	}
	else
	{
		err = kMemoryAllocationError;
	}

	SWSetStickyIfError( err );
	return err;
}


//---------------------------------------------------------------------------------------
//	SWDisposeSpriteLayer
//---------------------------------------------------------------------------------------

void SWDisposeSpriteLayer(
	SpriteLayerPtr *spriteLayerPP)
{
	SpriteLayerPtr	spriteLayerP = *spriteLayerPP;

	if (spriteLayerP != NULL)
	{
		free(spriteLayerP);
		*spriteLayerPP = NULL;
	}
}


//---------------------------------------------------------------------------------------
//	SWAddSprite
//---------------------------------------------------------------------------------------

SWError SWAddSprite(
	SpriteLayerPtr spriteLayerP,
	SpritePtr newSpriteP)
{
	SpritePtr		tailSpriteP = spriteLayerP->tailSpriteP;
	SWError			err = kNoError;

	SW_ASSERT(spriteLayerP != NULL);
	SW_ASSERT(newSpriteP != NULL);

	if (newSpriteP->parentSpriteLayerP != NULL)
		err = kSpriteAlreadyInLayer;

	if (err == kNoError)
	{
			// attach the new sprite to the end of the list
			// or make it the head if the list is empty.
		if (tailSpriteP != NULL)
			tailSpriteP->nextSpriteP = newSpriteP;
		else
			spriteLayerP->headSpriteP = newSpriteP;

			// link up the new sprite in both directions
		newSpriteP->prevSpriteP = tailSpriteP;
		newSpriteP->nextSpriteP = NULL;

			// make the new sprite the tail
		spriteLayerP->tailSpriteP = newSpriteP;

			// mark the sprite to be drawn, in case the sprite changed tile layers
		newSpriteP->needsToBeDrawn = true;

			// Store the parentSpriteLayer in the Sprite
		newSpriteP->parentSpriteLayerP = spriteLayerP;
	}

	SWSetStickyIfError( err );
	return err;
}


//---------------------------------------------------------------------------------------
//	SWRemoveSprite
//---------------------------------------------------------------------------------------

SWError SWRemoveSprite(
	SpritePtr oldSpriteP)
{
	SpriteLayerPtr	spriteLayerP;
	SWError			err = kNoError;

	SW_ASSERT(oldSpriteP != NULL);

	spriteLayerP = oldSpriteP->parentSpriteLayerP;
	if (spriteLayerP == NULL)
		err = kBadParameterErr;

	if (err == kNoError)
	{
			// is this not the tail sprite?
		if (oldSpriteP->nextSpriteP != NULL)
		{
				// link the next sprite to the prev sprite
			oldSpriteP->nextSpriteP->prevSpriteP = oldSpriteP->prevSpriteP;
		}
		else
		{
				// make the prev sprite the tail
			spriteLayerP->tailSpriteP = oldSpriteP->prevSpriteP;
		}

			// is this not the head sprite?
		if (oldSpriteP->prevSpriteP != NULL)
		{
				// link the prev sprite to the next sprite
			oldSpriteP->prevSpriteP->nextSpriteP = oldSpriteP->nextSpriteP;
		}
		else
		{
				// make the next sprite the first sprite
			spriteLayerP->headSpriteP = oldSpriteP->nextSpriteP;
		}

		oldSpriteP->parentSpriteLayerP = NULL;
	}

	SWSetStickyIfError( err );
	return err;
}


//---------------------------------------------------------------------------------------
//	SWRemoveAllSpritesFromLayer
//---------------------------------------------------------------------------------------

void SWRemoveAllSpritesFromLayer(
	SpriteLayerPtr srcSpriteLayerP)
{
	SpritePtr curSpriteP;

	SW_ASSERT(srcSpriteLayerP != NULL);

	while ((curSpriteP = SWGetNextSprite(srcSpriteLayerP, NULL)) != NULL)
	{
		SWRemoveSprite(curSpriteP);
	}
}


//---------------------------------------------------------------------------------------
//	SWDisposeAllSpritesInLayer
//---------------------------------------------------------------------------------------

void SWDisposeAllSpritesInLayer(
	SpriteLayerPtr spriteLayerP)
{
	SpritePtr	curSpriteP, nextSpriteP;

	SW_ASSERT(spriteLayerP != NULL);

	curSpriteP = spriteLayerP->headSpriteP;

		// iterate through the sprites in this layer
	while (curSpriteP != NULL)
	{
		nextSpriteP = curSpriteP->nextSpriteP;

		SWRemoveSprite(curSpriteP);
		SWDisposeSprite(&curSpriteP);

		curSpriteP = nextSpriteP;
	}
}


//---------------------------------------------------------------------------------------
// SWCountNumSpritesInLayer
//---------------------------------------------------------------------------------------

unsigned long SWCountNumSpritesInLayer(
	SpriteLayerPtr spriteLayerP)
{
	SpritePtr		curSpriteP;
	unsigned long	numSprites = 0;

	SW_ASSERT(spriteLayerP != NULL);

	curSpriteP = spriteLayerP->headSpriteP;

		// iterate through the sprites in this layer
	while (curSpriteP != NULL)
	{
		numSprites++;
		curSpriteP = curSpriteP->nextSpriteP;
	}

	return numSprites;
}

#if 0
#pragma mark -
#endif

//---------------------------------------------------------------------------------------
//	SWSwapSprite - both Sprites must be in the same Layer. Useful for sorting Sprites
//  according to depth, among other things.
//---------------------------------------------------------------------------------------

SWError SWSwapSprite(
	SpritePtr srcSpriteP,
	SpritePtr dstSpriteP)
{
	SpriteLayerPtr		spriteLayerP;
	register SpritePtr	swapSpriteP;
	SWError				err = kNoError;

	SW_ASSERT(srcSpriteP != NULL && dstSpriteP != NULL);

		// Do nothing if the sprites are the same
	if (srcSpriteP == dstSpriteP)
		return kNoError;

	spriteLayerP = srcSpriteP->parentSpriteLayerP;
	if (spriteLayerP != dstSpriteP->parentSpriteLayerP || spriteLayerP == NULL)
	{
		err = kBadParameterErr;
		SWSetStickyIfError( err );
		return err;
	}

		// adjacent Sprites are a special case
	if ( srcSpriteP->nextSpriteP == dstSpriteP ||
		dstSpriteP->nextSpriteP == srcSpriteP )
	{
		if ( srcSpriteP->nextSpriteP == dstSpriteP )
		{
			if ( srcSpriteP->prevSpriteP != NULL )
				(srcSpriteP->prevSpriteP)->nextSpriteP = dstSpriteP;
			if ( dstSpriteP->nextSpriteP != NULL )
				(dstSpriteP->nextSpriteP)->prevSpriteP = srcSpriteP;

			dstSpriteP->prevSpriteP = srcSpriteP->prevSpriteP;
			srcSpriteP->nextSpriteP = dstSpriteP->nextSpriteP;

			dstSpriteP->nextSpriteP = srcSpriteP;
			srcSpriteP->prevSpriteP = dstSpriteP;
		}
		else
		{
			if ( dstSpriteP->prevSpriteP != NULL )
				(dstSpriteP->prevSpriteP)->nextSpriteP = srcSpriteP;
			if ( srcSpriteP->nextSpriteP != NULL )
				(srcSpriteP->nextSpriteP)->prevSpriteP = dstSpriteP;

			srcSpriteP->prevSpriteP = dstSpriteP->prevSpriteP;
			dstSpriteP->nextSpriteP = srcSpriteP->nextSpriteP;

			srcSpriteP->nextSpriteP = dstSpriteP;
			dstSpriteP->prevSpriteP = srcSpriteP;
		}
	}
	else
	{
		if (srcSpriteP->prevSpriteP != NULL)
			srcSpriteP->prevSpriteP->nextSpriteP = dstSpriteP;
		if (dstSpriteP->prevSpriteP != NULL)
			dstSpriteP->prevSpriteP->nextSpriteP = srcSpriteP;
		if (srcSpriteP->nextSpriteP != NULL)
			srcSpriteP->nextSpriteP->prevSpriteP = dstSpriteP;
		if (dstSpriteP->nextSpriteP != NULL)
			dstSpriteP->nextSpriteP->prevSpriteP = srcSpriteP;

		swapSpriteP = srcSpriteP->nextSpriteP;
		srcSpriteP->nextSpriteP = dstSpriteP->nextSpriteP;
		dstSpriteP->nextSpriteP = swapSpriteP;

		swapSpriteP = srcSpriteP->prevSpriteP;
		srcSpriteP->prevSpriteP = dstSpriteP->prevSpriteP;
		dstSpriteP->prevSpriteP = swapSpriteP;
	}


	if (srcSpriteP->nextSpriteP == NULL)
	{
		spriteLayerP->tailSpriteP = srcSpriteP;
	}
	else if (srcSpriteP->prevSpriteP == NULL)
	{
		spriteLayerP->headSpriteP = srcSpriteP;
	}

	if (dstSpriteP->nextSpriteP == NULL)
	{
		spriteLayerP->tailSpriteP = dstSpriteP;
	}
	else if (dstSpriteP->prevSpriteP == NULL)
	{
		spriteLayerP->headSpriteP = dstSpriteP;
	}

	return kNoError;
}


//---------------------------------------------------------------------------------------
//	SWInsertSpriteAfterSprite
//---------------------------------------------------------------------------------------

SWError SWInsertSpriteAfterSprite(
	SpritePtr newSpriteP,
	SpritePtr dstSpriteP)
{
	SpriteLayerPtr	spriteLayerP;
	SWError			err = kNoError;

	SW_ASSERT(newSpriteP != NULL && dstSpriteP != NULL);

	spriteLayerP = dstSpriteP->parentSpriteLayerP;
	if (newSpriteP->parentSpriteLayerP != NULL)
		err = kSpriteAlreadyInLayer;

	if (err == kNoError)
	{
		if ( dstSpriteP->nextSpriteP != NULL )
			(dstSpriteP->nextSpriteP)->prevSpriteP = newSpriteP;

		newSpriteP->nextSpriteP = dstSpriteP->nextSpriteP;
		dstSpriteP->nextSpriteP = newSpriteP;
		newSpriteP->prevSpriteP = dstSpriteP;

		if (newSpriteP->nextSpriteP == NULL)
		{
			spriteLayerP->tailSpriteP = newSpriteP;
		}

			// mark the sprite to be drawn, in case the sprite changed tile layers
		newSpriteP->needsToBeDrawn = true;

		newSpriteP->parentSpriteLayerP = spriteLayerP;
	}

	SWSetStickyIfError( err );
	return err;
}


//---------------------------------------------------------------------------------------
//	SWInsertSpriteBeforeSprite
//---------------------------------------------------------------------------------------

SWError SWInsertSpriteBeforeSprite(
	SpritePtr newSpriteP,
	SpritePtr dstSpriteP)
{
	SpriteLayerPtr	spriteLayerP;
	SWError			err = kNoError;

	SW_ASSERT(newSpriteP != NULL && dstSpriteP != NULL);

	spriteLayerP = dstSpriteP->parentSpriteLayerP;
	if (newSpriteP->parentSpriteLayerP != NULL)
		err = kSpriteAlreadyInLayer;

	if (err == kNoError)
	{
		if ( dstSpriteP->prevSpriteP != NULL )
			(dstSpriteP->prevSpriteP)->nextSpriteP = newSpriteP;

		newSpriteP->prevSpriteP = dstSpriteP->prevSpriteP;
		dstSpriteP->prevSpriteP = newSpriteP;
		newSpriteP->nextSpriteP = dstSpriteP;

		if (newSpriteP->prevSpriteP == NULL)
		{
			spriteLayerP->headSpriteP = newSpriteP;
		}

			// mark the sprite to be drawn, in case the sprite changed tile layers
		newSpriteP->needsToBeDrawn = true;

		newSpriteP->parentSpriteLayerP = spriteLayerP;
	}

	SWSetStickyIfError( err );
	return err;
}


//---------------------------------------------------------------------------------------
//	SWGetNextSprite
//---------------------------------------------------------------------------------------

SpritePtr SWGetNextSprite(
	SpriteLayerPtr spriteLayerP,
	SpritePtr curSpriteP)
{
	SW_ASSERT(spriteLayerP != NULL);

	return (curSpriteP == NULL) ? spriteLayerP->headSpriteP : curSpriteP->nextSpriteP;
}


//---------------------------------------------------------------------------------------
//	SWGetPreviousSprite
//---------------------------------------------------------------------------------------

SpritePtr SWGetPreviousSprite(
	SpriteLayerPtr spriteLayerP,
	SpritePtr curSpriteP)
{
	SW_ASSERT(spriteLayerP != NULL);

	return (curSpriteP == NULL) ? spriteLayerP->tailSpriteP : curSpriteP->prevSpriteP;
}

#if 0
#pragma mark -
#endif

//---------------------------------------------------------------------------------------
//	SWLockSpriteLayer
//---------------------------------------------------------------------------------------

void SWLockSpriteLayer(
	SpriteLayerPtr spriteLayerP)
{
	SpritePtr curSpriteP;

	SW_ASSERT(spriteLayerP != NULL);

	curSpriteP = spriteLayerP->headSpriteP;

	while (curSpriteP != NULL)
	{
		SWLockSprite(curSpriteP);

		curSpriteP = curSpriteP->nextSpriteP;
	}
}


//---------------------------------------------------------------------------------------
//	SWUnlockSpriteLayer
//---------------------------------------------------------------------------------------

void SWUnlockSpriteLayer(
	SpriteLayerPtr spriteLayerP)
{
	SpritePtr curSpriteP;

	SW_ASSERT(spriteLayerP != NULL);

	curSpriteP = spriteLayerP->headSpriteP;

	while (curSpriteP != NULL)
	{
		SWUnlockSprite(curSpriteP);

		curSpriteP = curSpriteP->nextSpriteP;
	}
}


//---------------------------------------------------------------------------------------
//	SWCollideSpriteLayer
//---------------------------------------------------------------------------------------

void SWCollideSpriteLayer(
	SpriteWorldPtr	spriteWorldP,
	SpriteLayerPtr srcSpriteLayerP,
	SpriteLayerPtr dstSpriteLayerP)
{

//todo ben

	SpritePtr	srcSpriteP, nextSrcSpriteP;
	SpritePtr	dstSpriteP, nextDstSpriteP;
	SWRect		sectRect;
	int		collisionOccurred;

	SW_ASSERT(spriteWorldP != NULL);
	SW_ASSERT(srcSpriteLayerP != NULL && dstSpriteLayerP != NULL);

		// Don't check for collisions unless the frame has been processed!
	if (!spriteWorldP->frameHasOccurred)
		return;

	srcSpriteP = srcSpriteLayerP->headSpriteP;

	while (srcSpriteP != NULL)
	{
		dstSpriteP = dstSpriteLayerP->headSpriteP;
		nextSrcSpriteP = srcSpriteP->nextSpriteP;

		while (dstSpriteP != NULL)
		{
			nextDstSpriteP = dstSpriteP->nextSpriteP;

			if (srcSpriteP != dstSpriteP)
			{
					// are the spriteÕs rectangles overlapping?
				if ((srcSpriteP->destFrameRect.top < dstSpriteP->destFrameRect.bottom) &&
					(srcSpriteP->destFrameRect.bottom > dstSpriteP->destFrameRect.top) &&
					(srcSpriteP->destFrameRect.left < dstSpriteP->destFrameRect.right) &&
					(srcSpriteP->destFrameRect.right > dstSpriteP->destFrameRect.left))
				{
						// Once we've determined that a collision has occurred, do some
						// more precise checking if a collisionInset is used.
					if ((srcSpriteP->curFrameP->collisionInset.top ||
						srcSpriteP->curFrameP->collisionInset.bottom ||
						srcSpriteP->curFrameP->collisionInset.left ||
						srcSpriteP->curFrameP->collisionInset.right)   ||
						(dstSpriteP->curFrameP->collisionInset.top ||
						dstSpriteP->curFrameP->collisionInset.bottom ||
						dstSpriteP->curFrameP->collisionInset.left ||
						dstSpriteP->curFrameP->collisionInset.right))
					{
						collisionOccurred =
							((SW_COLLISION_RECT_TOP(srcSpriteP) < SW_COLLISION_RECT_BOTTOM(dstSpriteP)) &&
							(SW_COLLISION_RECT_BOTTOM(srcSpriteP) > SW_COLLISION_RECT_TOP(dstSpriteP)) &&
							(SW_COLLISION_RECT_LEFT(srcSpriteP) < SW_COLLISION_RECT_RIGHT(dstSpriteP)) &&
							(SW_COLLISION_RECT_RIGHT(srcSpriteP) > SW_COLLISION_RECT_LEFT(dstSpriteP)) );
					}
					else
					{
						collisionOccurred = true;
					}



						// call the source spriteÕs collision routine
					if (collisionOccurred && srcSpriteP->spriteCollideProc != NULL)
					{
							// Note: the following expands to something much larger
							// than you might think; try preprocessing and you'll see.
						sectRect.left =
							SW_MAX(SW_COLLISION_RECT_LEFT(srcSpriteP), SW_COLLISION_RECT_LEFT(dstSpriteP));
						sectRect.top =
							SW_MAX(SW_COLLISION_RECT_TOP(srcSpriteP), SW_COLLISION_RECT_TOP(dstSpriteP));
						sectRect.right =
							SW_MIN(SW_COLLISION_RECT_RIGHT(srcSpriteP), SW_COLLISION_RECT_RIGHT(dstSpriteP));
						sectRect.bottom =
							SW_MIN(SW_COLLISION_RECT_BOTTOM(srcSpriteP), SW_COLLISION_RECT_BOTTOM(dstSpriteP));

						(*srcSpriteP->spriteCollideProc)(srcSpriteP, dstSpriteP, &sectRect);

							// If the srcSpriteP was removed from the layer, collisions
							// between it and other Sprites shouldn't be detected anymore.
							// To avoid this, we begin processing the next srcSpriteP.
						if (srcSpriteP->spriteRemoval != kSWDontRemoveSprite ||
							srcSpriteP->parentSpriteLayerP == NULL)
						{
							break;
						}
					}
				}
			}

				// If the nextDstSpriteP has been removed, we must start over again
			if (nextDstSpriteP != NULL && nextDstSpriteP->spriteRemoval != kSWDontRemoveSprite)
				dstSpriteP = dstSpriteLayerP->headSpriteP;
			else
				dstSpriteP = nextDstSpriteP;
		}

			// If the nextSrcSpriteP has been removed, we must start over again
		if (nextSrcSpriteP != NULL && nextSrcSpriteP->spriteRemoval != kSWDontRemoveSprite)
			srcSpriteP = srcSpriteLayerP->headSpriteP;
		else
			srcSpriteP = nextSrcSpriteP;
	}

}


//---------------------------------------------------------------------------------------
//	SWSpriteCollidesWithLayer - does the sprite collide with any sprites in layer ?
//---------------------------------------------------------------------------------------

int SWSpriteCollidesWithLayer(SpritePtr srcSpriteP, SpriteLayerPtr spriteLayerP,
	SpriteCollisionProcPtr collideFunc)
{
	SpritePtr	dstSpriteP;

	SW_ASSERT(srcSpriteP != NULL);
	SW_ASSERT(spriteLayerP != NULL);

	for (dstSpriteP = spriteLayerP->headSpriteP;
		 dstSpriteP != NULL;
		 dstSpriteP = dstSpriteP->nextSpriteP )
	{
		if (srcSpriteP == dstSpriteP)
			continue;

			// are the spriteÕs rectangles overlapping?
		if ((srcSpriteP->destFrameRect.top < dstSpriteP->destFrameRect.bottom) &&
			(srcSpriteP->destFrameRect.bottom > dstSpriteP->destFrameRect.top) &&
			(srcSpriteP->destFrameRect.left < dstSpriteP->destFrameRect.right) &&
			(srcSpriteP->destFrameRect.right > dstSpriteP->destFrameRect.left))
		{
				// check for precise collision
			if ( (collideFunc == NULL) || (*collideFunc)( srcSpriteP, dstSpriteP ) )
			{
				return true;
			}
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------
//	_compareSpriteBottom - defines the sort criteria for SWSortSpriteLayer
//---------------------------------------------------------------------------------------
static short _compareSpriteBottom(
	SpritePtr srcSpriteP,
	SpritePtr dstSpriteP)
{
	return (srcSpriteP->destFrameRect.bottom - dstSpriteP->destFrameRect.bottom);
}


//---------------------------------------------------------------------------------------
//	SWSortSpriteLayer - Sort by vertical position
//		This means that higher Sprites will move first, and be drawn under lower sprites.
//---------------------------------------------------------------------------------------

void SWSortSpriteLayer(
	SpriteLayerPtr theLayer)
{
	SWCustomSortSpriteLayer(theLayer, &_compareSpriteBottom);
}


//---------------------------------------------------------------------------------------
//	SWSortSpriteLayer - Performs insertion sort on the sprite layer.
//  Code contributed by Arjun Raj <arjun@uclink4.berkeley.edu>
//---------------------------------------------------------------------------------------

void SWCustomSortSpriteLayer(SpriteLayerPtr theLayer, SpriteCompareProcPtr compareFunc)
{
	SpritePtr	curSpriteP, prevSprite, insertSprite;
	int		done;

	SW_ASSERT(theLayer != NULL);
	SW_ASSERT(compareFunc != NULL);

	prevSprite = theLayer->headSpriteP;
	curSpriteP = SWGetNextSprite(theLayer, prevSprite);

	while (curSpriteP != NULL)
	{
		if ( compareFunc(prevSprite,curSpriteP) > 0 )
		{
			insertSprite = curSpriteP->prevSpriteP;
			done = false;

			while (insertSprite != NULL && !done)
			{
				if ( compareFunc(insertSprite,curSpriteP) > 0 )
					insertSprite = insertSprite->prevSpriteP;
				else
					done = true;
			}

			if (insertSprite != NULL)
			{
				SWRemoveSprite(curSpriteP);

				SWInsertSpriteAfterSprite(curSpriteP, insertSprite);
				curSpriteP = SWGetNextSprite(theLayer, prevSprite);
			}
			else
			{
				SWRemoveSprite(curSpriteP);

				SWInsertSpriteBeforeSprite(curSpriteP, theLayer->headSpriteP);
				curSpriteP = SWGetNextSprite(theLayer, prevSprite);
			}
		}
		else
		{
			prevSprite = curSpriteP;
			curSpriteP = curSpriteP->nextSpriteP;
		}
	}
}


//---------------------------------------------------------------------------------------
//	SWPauseSpriteLayer
//---------------------------------------------------------------------------------------

void SWPauseSpriteLayer(
	SpriteLayerPtr	spriteLayerP)
{
	SW_ASSERT(spriteLayerP != NULL);

	spriteLayerP->isPaused = true;
}


//---------------------------------------------------------------------------------------
//	SWUnpauseSpriteLayer
//---------------------------------------------------------------------------------------

void SWUnpauseSpriteLayer(
	SpriteLayerPtr	spriteLayerP)
{
	SW_ASSERT(spriteLayerP != NULL);

	spriteLayerP->isPaused = false;
}


//---------------------------------------------------------------------------------------
//	SWSetLayerAsNonScrolling
//---------------------------------------------------------------------------------------

void SWSetLayerAsNonScrolling(
	SpriteLayerPtr	spriteLayerP,
	int layerIsNonScrolling)
{
	SW_ASSERT(spriteLayerP != NULL);

	spriteLayerP->layerIsNonScrolling = layerIsNonScrolling;
}


//---------------------------------------------------------------------------------------
//	SWFindSpriteByPoint
//---------------------------------------------------------------------------------------

SpritePtr SWFindSpriteByPoint(
	SpriteLayerPtr spriteLayerP,
	SpritePtr startSpriteP,
	int testPointH, int testPointV)
{
	return SWTopSpriteFromPoint(spriteLayerP, startSpriteP, testPointH, testPointV, NULL);
}


//---------------------------------------------------------------------------------------
//	SWTopSpriteFromPoint - returns the topmost sprite at a location, or NULL if none there
//---------------------------------------------------------------------------------------

SpritePtr SWTopSpriteFromPoint(
	SpriteLayerPtr spriteLayerP,
	SpritePtr startSpriteP,
	int testPointH, int testPointV,
	SpriteInMaskProcPtr testFunc)
{
	SpritePtr	topSpriteP;

	SW_ASSERT(spriteLayerP != NULL);

	topSpriteP = SWGetPreviousSprite(spriteLayerP, startSpriteP);

	while (topSpriteP != NULL)
	{
		if ( SWIsPointInSpriteMask(topSpriteP, testPointH, testPointV, testFunc) )
		{
			// pick this sprite
			break;
		}

		// note that we traverse the sprites in reverse order
		// since this will make the most sense if we are
		// looking for a sprite that was clicked
		topSpriteP = SWGetPreviousSprite(spriteLayerP, topSpriteP);
	}

	return topSpriteP;
}


//---------------------------------------------------------------------------------------
//	SWMoveSpriteLayerToTail
// Sprites the tail layer will be drawn last, and thus on top of the others
//---------------------------------------------------------------------------------------

void SWMoveSpriteLayerToTail (
	SpriteWorldPtr	spriteWorldP,
	SpriteLayerPtr moveSpriteLayerP)
{
	SpriteLayerPtr 	tailSpriteLayerP = spriteWorldP->tailSpriteLayerP;
	SW_ASSERT(spriteWorldP != NULL);

		// is it already at the tail?
	if (tailSpriteLayerP == moveSpriteLayerP)
		return;

		// is there a next layer?
	if (moveSpriteLayerP->nextSpriteLayerP != NULL)
	{
			// link the next layer to the prev layer
		moveSpriteLayerP->nextSpriteLayerP->prevSpriteLayerP = moveSpriteLayerP->prevSpriteLayerP;
	}
	else
	{
			// make the prev layer the tail
		spriteWorldP->tailSpriteLayerP = moveSpriteLayerP->prevSpriteLayerP;
	}

		// is there a prev layer?
	if (moveSpriteLayerP->prevSpriteLayerP != NULL)
	{
			// link the prev layer to the next layer
		moveSpriteLayerP->prevSpriteLayerP->nextSpriteLayerP = moveSpriteLayerP->nextSpriteLayerP;
	}
	else
	{
			// make the next layer the head
		spriteWorldP->headSpriteLayerP = moveSpriteLayerP->nextSpriteLayerP;
	}

		// add the layer back onto the tail

	tailSpriteLayerP->nextSpriteLayerP = moveSpriteLayerP;

	moveSpriteLayerP->prevSpriteLayerP = tailSpriteLayerP;
	moveSpriteLayerP->nextSpriteLayerP = NULL;

		// make the new layer the tail
	spriteWorldP->tailSpriteLayerP = moveSpriteLayerP;
}
