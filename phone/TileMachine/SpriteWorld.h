//
//  SpriteWorld.h
//  iMurphy
//
//  Created by Dave Peck on 4/26/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import <Foundation/Foundation.h>

/*
 
A SpriteWorld is an abstraction that knows how to draw sprites and update them at regular intervals.

I chose the name because my abstraction seemed similar to the SpriteWorld library I used 
over a decade ago -- but this is new code so if you're familiar with the old sprite world library,
you should forget what you know about it.

SpriteWorlds have a notion of their pixel width and height. All (outward) interaction with
sprite worlds is in terms of pixels. Mapping into OpenGL coordinate space is handled internally.
In the SpriteWorld coordinate system, X increases rightward and Y increases downward. The 
top-left of the sprite world is always (0,0).

It is possible to manually rotate a spriteworld into landscape mode. In this case,
the viewport width and height change but the coordinate system _does not change_. 
The spriteworld internally manages drawing into the rotated context, and the user needs do nothing 
special if it is not desired. Only the Overlay objects are 

SpriteWorlds currently contain three separate things:

A single "TileGrid" object implements the background image and animation from a set of tiles.
 
	"TileGrid" implements SpriteDrawable, which knows how to draw itself into the GL viewport.
	"TileGrid" implements SpriteBounds, which indicate the current boundaries of the sprite or grid. 
    In the case of a tile grid, these are fixed boundaries.
 
A list of "Sprite" objects. Each can have independent motion and animation.
 
	"Sprite" implements SpriteDrawable and SpriteBounds.
	"Sprite" also implements SpriteCollision, which is used to indicate whether the sprite wants to
	participate in collision detection.
 
A list of "Overlay" objects. These are used to draw portions of the UI that remain fixed on the 
screen regardless of the motion of the viewport. There is no one single "Overlay" object; it's
up to the developer to build them.
 
	"Overlay" objects must implement SpriteDrawable and SpriteWorldView
 
The drawing order is:
 
	1. TileGrid
	2. All sprites in the order we have in their array
	3. All overlay objects
 
SpriteWorlds report events to a delegate. There are several events that can be reported:
 
	beforeAnimation:(SpriteWorld*)spriteWorld;
	afterAnimation:(SpriteWorld*)spriteWorld;
	collisionDetected:(SpriteWorld*)spriteWorld firstSprite:(Sprite*)firstSprite secondSprite:(Sprite*)secondSprite;
 
Sprites, Tile Grids, and overlays are created and then added/removed explicitly from the SpriteWorld.

PERFORMANCE At the moment, drawing takes place on a per-object basis. In the future we're going
to want to accmulate all verticies, texture coordinates, and indexes into one huge array. (?)
 
*/

typedef enum 
{
	SpriteWorldOrientationPortrait,
	SpriteWorldOrientationPortraitUpsideDown,
	SpriteWorldOrientationLandscapeLeft,
	SpriteWorldOrientationLandscapeRight
} SpriteWorldOrientation;

typedef struct
{
	GLfloat width;
	GLfloat height;
} SpriteSize;

typedef struct
{
	GLfloat left;
	GLfloat top;
	GLfloat right;
	GLfloat bottom;
} SpriteRect;

typedef struct
{
	SpriteSize pixelViewportSize;
	SpriteRect glScrollBounds;
	SpriteWorldOrientation orientation;
} SpriteRenderMetrics;


@protocol SpriteWorldChild

-(void)attachSpriteWorld:(SpriteWorld *)spriteWorld;
-(void)detatchSpriteWorld;

@end


@protocol SpriteDrawable

-(void)drawElementsForViewport:(SpriteRect)glViewport;

@end



@protocol SpriteBounds

@end


@interface SpriteWorld : NSObject {	
}

+(id)spriteWorldWithEAGLLayer:(CAEAGLLayer*)layer;

-(void)setAnimationRate:(NSTimeInterval) animationRate;
-(void)startAnimation;
-(void)stopAnimation;

-(SpriteSize)pixelViewportSize;
-(SpriteRect)glScrollBounds;

-(void)setOrientation:(SpriteWorldOrientation) orientation;
-(SpriteWorldOrientation)orientation;

@end
