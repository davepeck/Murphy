//---------------------------------------------------------------------------------------
//	SpriteCreation.c
//
//	Portions are copyright: © 1991-94 Tony Myles, All rights reserved worldwide.
//  X86 Port to SDL created by: Michael Cabral 2003
//
//	Description:	sprite creation routines
//---------------------------------------------------------------------------------------

#ifndef __SWCOMMON__
#include <SWCommonHeaders.h>
#endif

#ifndef __SPRITEWORLD__
#include <SpriteWorld.h>
#endif

#ifndef __SPRITE__
#include <Sprite.h>
#endif

#ifndef __SPRITECREATION__
#include <SpriteCreation.h>
#endif

#ifndef __BLITKERNEL__
#include <BlitKernel.h>
#endif

//---------------------------------------------------------------------------------------
//	SWCreateSpriteFromSingleFileXY - Create a sprite from an Image file
//---------------------------------------------------------------------------------------

SWError SWCreateSpriteFromSingleFileXY( 
	SpritePtr* newSpriteP,
	void* spriteStorageP,
	const char* fileName,
	int frameWidth,
	int frameHeight,
	int borderWidth,
	int borderHeight,
	int hasOutsideBorder,
	long maxFrames )
{
	SWError			err = kNoError;
	SDL_Surface	*	surface;
	SpritePtr		tempSpriteP;
	int 			widthStart, heightStart;
	SWRect			frameRect;
	int				frame;
	FramePtr 		newFrameP;
		
	surface = BKLoadSurface( fileName );
	if( ! surface ) 
		err = kSDLCreateSurfaceFromFile;
	
	if( err == kNoError )
	{
		if( maxFrames == 0 )
			maxFrames = (surface->w/frameWidth) * (surface->h/frameHeight);
		
		if ( hasOutsideBorder )
		{
			widthStart = borderWidth / 2;
			heightStart = borderHeight / 2;
		}
		else
		{
			widthStart = 0;
			heightStart = 0;
		}

		err = SWCreateSprite( newSpriteP, spriteStorageP, maxFrames);
	}
					
	if( err == kNoError )
	{
		tempSpriteP = *newSpriteP;
		tempSpriteP->sharedSurface = surface;

		frameRect.left = widthStart;
		frameRect.top = heightStart;
		frameRect.right = widthStart + frameWidth;
		frameRect.bottom = heightStart + frameHeight;
		
		for (frame = 0; frame < maxFrames; frame++)
		{
			err = SWCreateFrameFromSurfaceAndRect( &newFrameP, surface, &frameRect );
			
			if (err == kNoError)
			{
				err = SWAddFrame(tempSpriteP, newFrameP);
				if ( frame < maxFrames-1 )
				{
					frameRect.left += frameWidth + borderWidth;
					frameRect.right = frameRect.left + frameWidth;
					if ( frameRect.right > surface->w )
					{
						frameRect.left = widthStart;
						frameRect.right = widthStart + frameWidth;

						frameRect.top += frameHeight + borderHeight;
						frameRect.bottom = frameRect.top + frameHeight;
						if ( frameRect.bottom > surface->h )
						{
							err = kOutOfRangeErr;
						}
					}
				}
			}
			
			if ( err != kNoError )
			{
				SWDisposeFrame(&newFrameP);
				SWDisposeSprite(&tempSpriteP);
				break;
			}
		}
	}
		
	if (err == kNoError )
	{
		SWSetSpriteFrameRange(tempSpriteP, 0, maxFrames - 1);
		err = SWSetCurrentFrameIndex(tempSpriteP, 0);
	}
	
	SWSetStickyIfError( err );
	return err;
}

//---------------------------------------------------------------------------------------
//	SWCreateSpriteFromSequence - Create a sprite from a sequence of Image file
//  filename is a printf format string, use only one %d
//  ie pass "sprite%03d.png", 1, 200 will get "sprite001.png" through "sprite200.png"
//  pass "sprite-%d.png, 1, 200 will get "sprite-1.png" through "sprite-200.png"
//---------------------------------------------------------------------------------------

SWError SWCreateSpriteFromFileSequence(
	SpritePtr* newSpriteP,
	void* spriteStorageP, 
    const char *filename, 
	int start, 
	long maxFrames)
{
	SWError err = kNoError;
    char tmp_file[255];
    int x;
    
    FramePtr tempFrameP;
    SpritePtr tempSpriteP;
    
	SW_UNUSED(spriteStorageP);

    tempSpriteP = *newSpriteP;
    
    /*initialize the sprite*/
	err =  SWCreateSprite(&tempSpriteP, 0, maxFrames);
	
	if (err == kNoError)
	{
		for(x = 0; x < maxFrames; x++)
		{
			sprintf(tmp_file, filename, x+start);
			
			err = SWCreateFrameFromFile(&tempFrameP, tmp_file);
			
			if (err == kNoError)
			{
				err = SWAddFrame(tempSpriteP, tempFrameP);
			}
			
			if (err != kNoError) 
			{
				SWDisposeFrame(&tempFrameP);
				SWDisposeSprite(&tempSpriteP);
				break;
			}
		}
	}
    
	if (err == kNoError)
	{
		SWSetSpriteFrameRange(tempSpriteP, 0, maxFrames - 1);
		err = SWSetCurrentFrameIndex(tempSpriteP, 0);
		*newSpriteP = tempSpriteP;
	}
	
	return err;
}

//---------------------------------------------------------------------------------------
//	SWCreateSpriteFromImage - Create a sprite from an Image file
//---------------------------------------------------------------------------------------

/*Create a sprite from an Image file*/
SWError SWCreateSpriteFromFile(
	SpritePtr* newSpriteP,
	void* spriteStorageP,
	const char * file,
	long maxFrames)
{
	SWError err = kNoError;
    FramePtr  tempFrameP = 0;
    SpritePtr tempSpriteP = 0;
    
    /*initialize the sprite*/
    err =  SWCreateSprite(&tempSpriteP, spriteStorageP, maxFrames);
	if (err == kNoError)
	{
		/*Add a single frame to the Sprite (for now) */
		err = SWCreateFrameFromFile(&tempFrameP, file);
	}
	
	if (err == kNoError)
	{
		err = SWAddFrame(tempSpriteP, tempFrameP);
	}

	if(err == kNoError )
	{
		err = SWSetCurrentFrameIndex(tempSpriteP, 0);
	}
    
	if (err == kNoError)
	{
		*newSpriteP = tempSpriteP;
	}
	
	return err;
}

//---------------------------------------------------------------------------------------
//	SWCreateSprite
//---------------------------------------------------------------------------------------

SWError SWCreateSprite(
	SpritePtr* newSpriteP,
	void* spriteStorage,
	long maxFrames)
{
	SWError err = kNoError;
    long frameArraySize;
    SpritePtr tempSpriteP = 0;
    *newSpriteP = 0;
    
    tempSpriteP = (spriteStorage == 0) ? (SpritePtr)calloc(1,sizeof(SpriteRec)) : (SpritePtr)spriteStorage;
    
    if(tempSpriteP)
    {
		frameArraySize = maxFrames * sizeof(FramePtr);
		tempSpriteP->frameArray = (FramePtr*)calloc(1,frameArraySize);
		
		if (tempSpriteP->frameArray)
		{
                tempSpriteP->parentSpriteLayerP = 0;
                tempSpriteP->nextSpriteP = 0;
		        tempSpriteP->prevSpriteP = 0;
		        
					// initialize drawing fields
		        tempSpriteP->isVisible = true;
              	tempSpriteP->needsToBeDrawn = 1;
                tempSpriteP->needsToBeErased = 0;
 				SW_SET_RECT(tempSpriteP->destFrameRect,0,0,0,0);
				SW_SET_RECT(tempSpriteP->oldFrameRect,0,0,0,0);
				SW_SET_RECT(tempSpriteP->deltaFrameRect,0,0,0,0);
		        tempSpriteP->frameDrawProc = SWStdSpriteDrawProc;
				tempSpriteP->tileDepth = 1;
 		            		
					// initialize frame fields
				tempSpriteP->curFrameP = NULL;
				tempSpriteP->numFrames = 0;
				tempSpriteP->maxFrames = maxFrames;
				tempSpriteP->frameTimeInterval = -1;
				tempSpriteP->timeOfLastFrameChange = 0;
				tempSpriteP->frameAdvance = 1;
				tempSpriteP->curFrameIndex = 0;
				tempSpriteP->firstFrameIndex = 0;
				tempSpriteP->lastFrameIndex = 0;
				tempSpriteP->frameChangeProc = NULL;

					// initialize movement fields
				tempSpriteP->moveTimeInterval = 0;
				tempSpriteP->timeOfLastMove = 0;
				tempSpriteP->pseudoMoveTimeInterval = -1;
				tempSpriteP->moveTimeAccum = 0;
				tempSpriteP->moveProcHasFired = false;
				SW_SET_RECT(tempSpriteP->moveBoundsRect,0,0,0,0);
				tempSpriteP->spriteMoveProc = SWTempSpriteMoveProc;
				tempSpriteP->realSpriteMoveProc = NULL;

				tempSpriteP->spriteCollideProc = NULL;
				
				tempSpriteP->sharedSurface = NULL;
				tempSpriteP->spriteRemoval = kSWDontRemoveSprite;
				tempSpriteP->frameAdvanceMode = kSWWrapAroundMode;
				
 				tempSpriteP->drawData = SWCreateDrawData();
 				
				tempSpriteP->doFastMovingCheck = false;

				tempSpriteP->drawData->parentSpriteP = tempSpriteP;
				tempSpriteP->drawData->rotationOffsetHoriz = 0.0f;
				tempSpriteP->drawData->rotationOffsetVert = 0.0f;
       			
                tempSpriteP->userData = 0;
                
		        *newSpriteP = tempSpriteP;
		}
		else
			err = kMemoryAllocationError;
    } 
	else 
	{
		err = kMemoryAllocationError;
	} 
	
	return err;
}

SWError SWCloneSprite(
	SpritePtr	cloneSpriteP,
	SpritePtr*	newSpriteP,
	void*		spriteStorageP)
{
	SpritePtr 		tempSpriteP;
	short 			index;
	int				frameArraySize;
	SWError 		err = kNoError;

	SW_ASSERT(cloneSpriteP != NULL);

		// Allocate memory for the new Sprite
	tempSpriteP = (spriteStorageP == NULL) ? (SpritePtr)malloc(sizeof(SpriteRec)) : (SpritePtr)spriteStorageP;
	if (tempSpriteP == NULL)
		err = kMemoryAllocationError;

	if (err == kNoError)
	{
			// Make sure the Ptr passed to this function is large enough.
		//SW_ASSERT( GetPtrSize((Ptr)tempSpriteP) >= sizeof(SpriteRec) );
		
			// copy the clone sprite into the temp sprite
		*tempSpriteP = *cloneSpriteP;

			// clear the layer fields, in case the parent sprite is in a layer
		tempSpriteP->parentSpriteLayerP = NULL;
		tempSpriteP->nextSpriteP = NULL;
		tempSpriteP->prevSpriteP = NULL;

			// Initialize other fields
		tempSpriteP->spriteMoveProc = SWTempSpriteMoveProc;
		tempSpriteP->realSpriteMoveProc = cloneSpriteP->realSpriteMoveProc;
		tempSpriteP->drawData = (DrawDataPtr)calloc(1,sizeof(DrawDataRec));

		*(tempSpriteP->drawData) = *(cloneSpriteP->drawData);
		tempSpriteP->drawData->parentSpriteP = tempSpriteP;
	}

	if (err == kNoError)
	{
			// Create a new frameArray for this sprite
		frameArraySize = cloneSpriteP->maxFrames * sizeof(FramePtr);
		tempSpriteP->frameArray = (FramePtr *)calloc(1,frameArraySize);
		
		if (tempSpriteP->frameArray != NULL)
		{
				// copy the frame array, and increment useCount for each frame
			for (index = 0; index < cloneSpriteP->maxFrames; index++)
			{
				tempSpriteP->frameArray[index] = cloneSpriteP->frameArray[index];
				SW_ASSERT(tempSpriteP->frameArray[index] != NULL );
				tempSpriteP->frameArray[index]->useCount++;
			}

		}
		else
		{
			err = kMemoryAllocationError;
			free((void*)tempSpriteP);
		}
	}

	if (err == kNoError)
		*newSpriteP = tempSpriteP;

	SWSetStickyIfError( err );
	return err;
}


