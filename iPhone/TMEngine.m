//
//  TMEngine.m
//  iMurphy
//
//  Created by Dave Peck on 4/26/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "TMEngine.h"

@interface TMEngine (TMEnginePrivate)

- (id)initWithGlLayer:(CAEAGLLayer *)layer;
- (BOOL)createFramebuffer;
- (void)destroyFramebuffer;
- (void)setupView;

@end

@implementation TMEngine (TMEnginePrivate)

- (id)initWithGlLayer:(CAEAGLLayer *)layer
{
	self = [super init];
	
	if (self != nil)
	{
		drawLayer = [layer retain];
		drawLayer.opaque = YES;		
		drawLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];		
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
		
		if (!context || ![EAGLContext setCurrentContext:context] || ![self createFramebuffer]) 
		{
			[self release];
			return nil;
		}	
		
		[self setupView];
	}	
	
	return self;
}

- (BOOL)createFramebuffer
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

- (void)destroyFramebuffer
{
	glDeleteFramebuffersOES(1, &viewFramebufferName);
	viewFramebufferName = 0;
	glDeleteRenderbuffersOES(1, &viewRenderbufferName);
	viewRenderbufferName = 0;	
}

- (void)setupView
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

@synthesize delegate;
@synthesize tileGrid;
@synthesize displayViewportTop;
@synthesize displayViewportLeft;

- (void)animationTick:(NSTimer *)animationTimer
{
	if (delegate != nil)
	{
		[delegate beforeFrame];
	}
	
	// Make sure that you are drawing to the current context
	[EAGLContext setCurrentContext:context];	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebufferName);
	
	[tileGrid alignViewportToPixel:&displayViewportLeft top:&displayViewportTop];
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(displayViewportLeft, displayViewportLeft + 1.0f, displayViewportTop - 1.5f, displayViewportTop, -1.0f, 1.0f);	
	
	glMatrixMode(GL_MODELVIEW);
	
	glClear(GL_COLOR_BUFFER_BIT);
	[tileGrid drawInViewportLeft:displayViewportLeft top:displayViewportTop right:displayViewportLeft + 1.0f bottom:displayViewportTop - 1.5f];
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbufferName);
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];	
	
	if (delegate != nil)
	{
		[delegate afterFrame];		
	}
}

+ (id)engineWithGlLayer:(CAEAGLLayer *)layer
{
	return [[[TMEngine alloc] initWithGlLayer:layer] autorelease];
}

- (void)setAnimationRate:(NSTimeInterval)animationRate
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
	animationTimer = [NSTimer scheduledTimerWithTimeInterval:animationInterval target:self selector:@selector(animationTick:) userInfo:nil repeats:YES];
}

- (void)stopAnimation
{
	[animationTimer invalidate];
	animationTimer = nil;
}

-(void)dealloc
{
	self.delegate = nil;
	self.tileGrid = nil;
	
	[self stopAnimation];	
	[super dealloc];
}

@end
