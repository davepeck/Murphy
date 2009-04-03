//---------------------------------------------------------------------------------------
///	@file SpriteCreation.h
///	constants, structures, and function prototypes for creation of sprites
//
//	Portions are copyright: (C) 1991-94 Tony Myles, All rights reserved worldwide.
//---------------------------------------------------------------------------------------

#ifndef __SPRITECREATION__
#define __SPRITECREATION__

#ifndef __SPRITE__
#include <Sprite.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------------------
//	sprite creation function prototypes
//---------------------------------------------------------------------------------------
SWError SWCreateSpriteFromSingleFileXY(SpritePtr* newSpriteP,
	void* spriteStorageP,const char* fileName,int frameWidth,int frameHeight,int borderWidth,
	int borderHeight,int hasOutsideBorder,long maxFrames );

SWError SWCreateSpriteFromFileSequence(SpritePtr* newSpriteP,void* spriteStorageP, 
    const char *filename, int start,long maxFrames);

SWError SWCreateSpriteFromFile(SpritePtr* newSpriteP,void* spriteStorageP,
	const char * file,long maxFrames);

SWError SWCreateSprite(SpritePtr* newSpriteP,void* spriteStorage,long maxFrames);
	
SWError SWCloneSprite(
	SpritePtr	cloneSpriteP,
	SpritePtr*	newSpriteP,
	void*		spriteStorageP);
	
#ifdef __cplusplus
}
#endif

#endif	/* __SPRITECREATION__ */

