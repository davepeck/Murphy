//
//  TMEngine.h
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

typedef enum 
{
	TMOrientationPortrait,
	TMOrientationPortraitUpsideDown,
	TMOrientationLandscapeLeft,
	TMOrientationLandscapeRight
} TMOrientation;

typedef struct
{
	GLfloat width;
	GLfloat height;
} TMSize;

typedef struct
{
	GLfloat left;
	GLfloat top;
	GLfloat right;
	GLfloat bottom;
} TMRect;

typedef struct
{
	TMSize pixelViewportSize;
	TMRect glScrollBounds;
	TMOrientation orientation;
} TMRenderMetrics;

@class TMEngine;

@protocol TMLayer

-(void)attachEngine:(TMEngine *)engine;
-(void)detatchEngine;

@end


@protocol TMDrawable

-(void)drawElementsForViewport:(TMRect)glViewport;

@end




@interface TMEngine : NSObject {	
}

+(id)engineWithGlLayer:(CAEAGLLayer*)layer;

-(void)setAnimationRate:(NSTimeInterval) animationRate;
-(void)startAnimation;
-(void)stopAnimation;

-(TMSize)pixelViewportSize;
-(TMRect)glScrollBounds;

-(void)setOrientation:(TMOrientation) orientation;
-(TMOrientation)orientation;

@end
