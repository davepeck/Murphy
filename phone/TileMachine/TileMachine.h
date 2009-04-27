//
//  TileMachine.h
//  iMurphy
//
//  Created by Dave Peck on 4/26/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/EAGLDrawable.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

/*
 
A TileMachine is an abstraction that knows how to draw sprites and update them at regular intervals.
 
Put simply, it is the "engine" that powers all graphics in a game.

TileMachines have a notion of their pixel width and height. All (outward) interaction with
tile machines is in terms of pixels. Mapping into OpenGL coordinate space is handled internally.
In the TileMachine coordinate system, X increases rightward and Y increases downward. The 
top-left of the system is always (0,0).

It is possible to manually rotate the system into landscape mode. In this case,
the viewport width and height change but the coordinate system _does not change_. 
TileMachine internally manages drawing into the rotated context, and the user needs do nothing 
special if it is not desired.

TileMachine currently "parents" three separate types:

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
 
TileMachines report events to a delegate. There are several events that can be reported:
 
	beforeAnimation:(TileMachine*)tileMachine;
	afterAnimation:(TileMachine*)tileMachine;
	collisionDetected:(TileMachine*)tileMachine firstSprite:(Sprite*)firstSprite secondSprite:(Sprite*)secondSprite;
 
Sprites, Tile Grids, and overlays are created and then added/removed explicitly from the SpriteWorld.

PERFORMANCE At the moment, drawing takes place on a per-object basis. In the future we're going
to want to accmulate all verticies, texture coordinates, and indexes into one huge array. (?)
 
*/

typedef enum 
{
	TileMachineOrientationPortrait,
	TileMachineOrientationPortraitUpsideDown,
	TileMachineOrientationLandscapeLeft,
	TileMachineOrientationLandscapeRight
} TileMachineOrientation;

typedef struct
{
	GLfloat width;
	GLfloat height;
} TileMachineSize;

typedef struct
{
	GLfloat left;
	GLfloat top;
	GLfloat right;
	GLfloat bottom;
} TileMachineRect;

typedef struct
{
	TileMachineSize pixelViewportSize;
	TileMachineRect glScrollBounds;
	TileMachineOrientation orientation;
} TileMachineRenderMetrics;

@class TileMachine;

@protocol TileMachineChild

-(void)attachTileMachine:(TileMachine *)tileMachine;
-(void)detatchTileMachine;

@end


@protocol SpriteDrawable

-(void)drawElementsForViewport:(TileMachineRect)glViewport;

@end




@interface TileMachine : NSObject {	
}

+(id)tileMachineWithEAGLLayer:(CAEAGLLayer*)layer;

-(void)setAnimationRate:(NSTimeInterval) animationRate;
-(void)startAnimation;
-(void)stopAnimation;

-(TileMachineSize)pixelViewportSize;
-(TileMachineRect)glScrollBounds;

-(void)setOrientation:(TileMachineOrientation) orientation;
-(TileMachineOrientation)orientation;

@end
