//
//  TMEngine.m
//  iMurphy
//
//  Created by Dave Peck on 4/26/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "TMEngine.h"

@interface TMEngine (TMEnginePrivate)

-(id)initWithGlLayer:(CAEAGLLayer*)layer;
-(BOOL)createFramebuffer;
-(void)destroyFramebuffer;
-(void)setupView;

@end


@implementation TMEngine (TMEnginePrivate)

-(id)initWithGlLayer:(CAEAGLLayer*)layer
{
	drawLayer = layer;
	
	drawLayer.opaque = YES;		
	drawLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
	
	context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
	
	if (!context || ![EAGLContext setCurrentContext:context] || ![self createFramebuffer]) 
	{
		[self release];
		return nil;
	}	
	
	return self;
}

-(BOOL)createFramebuffer
{
	glGenFramebuffersOES(1, &viewFramebufferName);
	glGenRenderbuffersOES(1, &viewRenderbufferName);
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebufferName);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbufferName);
	[context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(id<EAGLDrawable>)drawLayer];
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbufferName);
	
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
	
	if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) 
	{
		NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		return NO;
	}
	
	return YES;
}

-(void)destroyFramebuffer
{
	glDeleteFramebuffersOES(1, &viewFramebufferName);
	viewFramebufferName = 0;
	glDeleteRenderbuffersOES(1, &viewRenderbufferName);
	viewRenderbufferName = 0;	
}


-(void)setupView
{
	// Sets up matrices and transforms for OpenGL ES
	glViewport(0, 0, backingWidth, backingHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, 1.0f, 0.0f, 1.5f, -1.0f, 1.0f);	
	glMatrixMode(GL_MODELVIEW);
	
	// Clears the view with black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	
}

@end



@implementation TMEngine

+(id)engineWithGlLayer:(CAEAGLLayer*)layer
{
	// TODO
	return nil;
}

-(void)setAnimationRate:(NSTimeInterval) animationRate
{
	animationInterval = animationRate;
	
	if (animationTimer != nil) 
	{
		[self stopAnimation];
		[self startAnimation];
	}
}

- (void)startAnimation
{
	animationTimer = [NSTimer scheduledTimerWithTimeInterval:animationInterval target:self selector:@selector(drawView) userInfo:nil repeats:YES];
}

- (void)stopAnimation
{
	[animationTimer invalidate];
	animationTimer = nil;
}

-(TMSize)pixelViewportSize
{
	// TODO
	TMSize todo;
	return todo;
}

-(TMRect)glScrollBounds
{
	// TODO
	TMRect todo;
	return todo;
}

-(void)setOrientation:(TMOrientation) orientation
{
	// TODO
}

-(TMOrientation)orientation
{
	// TODO
	return TMOrientationPortrait;
}

@end
