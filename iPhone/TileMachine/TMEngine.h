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

#import "TMMath.h"
#import "TMTileGrid.h"
#import "TMSprite.h"

@protocol TMEngineDelegate;

@interface TMEngine : NSObject 
{	
	// drawing context 
	GLint backingWidth;
	GLint backingHeight;		
	CAEAGLLayer *drawLayer;
	EAGLContext *context;	
	
	// GL rendering contexts
	GLuint viewRenderbufferName;
	GLuint viewFramebufferName;	
	GLuint depthRenderbufferName;	
	
	// animation management
	NSTimer *animationTimer;
	NSTimeInterval animationInterval;	
	
	// drawable children (TODO will ultimately be a scene graph like thing?)
	TMTileGrid *tileGrid;

	// viewport management
	GLfloat displayViewportTop;
	GLfloat displayViewportLeft;
	
	// delegate
	id<TMEngineDelegate> delegate;
}

@property (nonatomic, assign) id<TMEngineDelegate> delegate;
@property (nonatomic, retain) TMTileGrid *tileGrid;
@property GLfloat displayViewportTop;
@property GLfloat displayViewportLeft;

+ (id)engineWithGlLayer:(CAEAGLLayer *)layer;

- (void)setAnimationRate:(NSTimeInterval)animationRate;
- (void)startAnimation;
- (void)stopAnimation;

@end


@protocol TMEngineDelegate<NSObject>

- (void)beforeFrame;
- (void)afterFrame;

@end

