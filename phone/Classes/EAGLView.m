//
//  EAGLView.m
//  iMurphy
//
//  Created by Dave Peck on 4/3/09.
//  Copyright Code Orange 2009. All rights reserved.
//



#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "EAGLView.h"
#import "MurphyLevel.h"
#import "Texture.h"
#import "TileAtlas.h"
#import "TileMap.h"
#import "TileGrid.h"
#import "SpriteMath.h"

#define MOVE_INCREMENT 0.025f

#define MOTION_DAMP 0.95f
#define TOO_SMALL_MOTION 0.0001f
#define FLICK_TIME_BACK 0.075f
#define MIN_PIXELS_FOR_FLICK 5
#define MOTION_MULTIPLIER 0.25f
#define MOTION_MAX 0.065


const uint16_t TILE_WATER_FIRST = 238;
const uint16_t TILE_WATER_LAST = 241; 
const uint16_t TILE_WHIRLPOOL_FIRST = 256;
const uint16_t TILE_WHIRLPOOL_LAST = 259;
const uint16_t TILE_RADAR_FIRST = 270;
const uint16_t TILE_RADAR_LAST = 273;
const uint16_t TILE_LIFE_FIRST = 288;
const uint16_t TILE_LIFE_LAST = 293;
const uint16_t TILE_LEVEL1_FIRST = 1;
const uint16_t TILE_LEVEL1_LAST = 8;
const uint16_t TILE_LEVEL2_FIRST = 19;
const uint16_t TILE_LEVEL2_LAST = 26;
const uint16_t TILE_LEVEL3_FIRST = 37;
const uint16_t TILE_LEVEL3_LAST = 44;


@interface EAGLView (EAGLViewPrivate)

- (BOOL)createFramebuffer;
- (void)destroyFramebuffer;
- (void)setupView;
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)ensureViewportBoundaries;

@end

@implementation EAGLView

@synthesize animationInterval;

+ (Class) layerClass
{
	return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder
{
	self = [super initWithCoder:coder];
	
	if (self != nil) 
	{
		// Get the layer
		CAEAGLLayer *eaglLayer = (CAEAGLLayer*) self.layer;
		
		eaglLayer.opaque = YES;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
		
		if (!context || ![EAGLContext setCurrentContext:context] || ![self createFramebuffer]) 
		{
			[self release];
			return nil;
		}
		
		animationInterval = 1.0 / 60.0;
		
		xMotion = 0.0f;
		yMotion = 0.0f;
		touchBuffer = nil;
				
		[self setupView];
		[self drawView];
	}
	
	return self;
}

- (void)layoutSubviews
{
	[EAGLContext setCurrentContext:context];
	[self destroyFramebuffer];
	[self createFramebuffer];
	[self drawView];
}

- (BOOL)createFramebuffer
{
	glGenFramebuffersOES(1, &viewFramebuffer);
	glGenRenderbuffersOES(1, &viewRenderbuffer);
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	[context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(id<EAGLDrawable>)self.layer];
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
	
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
	glDeleteFramebuffersOES(1, &viewFramebuffer);
	viewFramebuffer = 0;
	glDeleteRenderbuffersOES(1, &viewRenderbuffer);
	viewRenderbuffer = 0;
	
	if (depthRenderbuffer) 
	{
		glDeleteRenderbuffersOES(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
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

- (void)setAnimationInterval:(NSTimeInterval)interval
{
	animationInterval = interval;
	
	if (animationTimer) 
	{
		[self stopAnimation];
		[self startAnimation];
	}
}

- (void)setupView
{
	// Sets up matrices and transforms for OpenGL ES
	glViewport(0, 0, backingWidth, backingHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, 1.0f, 0.0f, 1.5f, -1.0f, 1.0f);	
	glMatrixMode(GL_MODELVIEW);
	
	currentViewportLeft = 3.875006f;
	currentViewportTop = -1.824998f;
	
	// Load our world textures
	MurphyLevel *levelFile = [MurphyLevel murphyLevelWithNamedResource:@"World"];		
	TileAtlas *worldAtlas = [TileAtlas tileAtlasWithResourcePNG:@"example-world-tiles-24-512" tilePixelWidth:24 tilePixelHeight:24 tilesAcross:18 tilesDown:20];
	TileMap *worldMap = [TileMap tileMapWithAtlas:worldAtlas startTileId:0];
	tileGrid = [[TileGrid tileGridWithMap:worldMap width:levelFile.width height:levelFile.height] retain];

	// Fill the tileGrid
	for (uint16_t y = 0; y < levelFile.height; y++)
	{
		for (uint16_t x = 0; x < levelFile.width; x++)
		{
			uint8_t iconS;
			uint8_t iconT;
			[levelFile getIconForX:x y:y s:&iconS t:&iconT];
			uint16_t iconTileId = (iconT * 18) + iconS; // 18 = tilesAcross. Not pretty, I know.
			[tileGrid setTileIdAtX:x y:y tileId:iconTileId];
		}
	}
	
	[tileGrid setPixelWidth:320.0 height:480.0];
	[tileGrid setGridLeft:0.0f top:1.5f];
	
	[tileGrid.map animateTileId:TILE_WATER_FIRST toTileId:TILE_WATER_LAST timeInterval:0.5f allInRange:YES];
	[tileGrid.map animateTileId:TILE_WHIRLPOOL_FIRST toTileId:TILE_WHIRLPOOL_LAST timeInterval:0.4f allInRange:NO];
	[tileGrid.map animateTileId:TILE_RADAR_FIRST toTileId:TILE_RADAR_LAST timeInterval:0.6f allInRange:NO];
	[tileGrid.map animateTileId:TILE_LIFE_FIRST toTileId:TILE_LIFE_LAST timeInterval:0.73f allInRange:YES];
	[tileGrid.map animateTileId:TILE_LEVEL1_FIRST toTileId:TILE_LEVEL1_LAST timeInterval:0.2f allInRange:NO];
	[tileGrid.map animateTileId:TILE_LEVEL2_FIRST toTileId:TILE_LEVEL2_LAST timeInterval:0.3f allInRange:NO];
	[tileGrid.map animateTileId:TILE_LEVEL3_FIRST toTileId:TILE_LEVEL3_LAST timeInterval:0.7f allInRange:NO];
	
	// Clears the view with black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	
}

- (void)ensureViewportBoundaries
{
	if (currentViewportTop > tileGrid.gridTop)
	{
		currentViewportTop = tileGrid.gridTop;
	}

	if (currentViewportTop - 1.5f < tileGrid.gridBottom)
	{
		currentViewportTop = tileGrid.gridBottom + 1.5f;
	}
	
	if (currentViewportLeft < tileGrid.gridLeft)
	{
		currentViewportLeft = tileGrid.gridLeft;
	}

	if (currentViewportLeft + 1.0f > tileGrid.gridRight)
	{
		currentViewportLeft = tileGrid.gridRight - 1.0f;
	}			
}

// Updates the OpenGL view when the timer fires
- (void)drawView
{
	// Make sure that you are drawing to the current context
	[EAGLContext setCurrentContext:context];
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
		
	if (xMotion != 0.0f || yMotion != 0.0f)
	{
		currentViewportLeft += xMotion;
		currentViewportTop += yMotion;
		
		xMotion *= MOTION_DAMP;
		yMotion *= MOTION_DAMP;
		
		if (fabs(xMotion) < TOO_SMALL_MOTION)
		{
			xMotion = 0.0f;
		}
		
		if (fabs(yMotion) < TOO_SMALL_MOTION)
		{
			yMotion = 0.0f;
		}
		
		[self ensureViewportBoundaries];
	}
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(currentViewportLeft, currentViewportLeft + 1.0f, currentViewportTop - 1.5f, currentViewportTop, -1.0f, 1.0f);	
	glMatrixMode(GL_MODELVIEW);
	
	glClear(GL_COLOR_BUFFER_BIT);
	[tileGrid drawInViewportLeft:currentViewportLeft top:currentViewportTop right:currentViewportLeft + 1.0f bottom:currentViewportTop - 1.5f];
		
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

// Stop animating and release resources when they are no longer needed.
- (void)dealloc
{
	if (touchBuffer != nil)
	{
		[touchBuffer release];
		touchBuffer = nil;
	}	
	
	[self stopAnimation];
	
	if ([EAGLContext currentContext] == context) 
	{
		[EAGLContext setCurrentContext:nil];
	}
	
	[context release];
	context = nil;
	
	[super dealloc];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event 
{
	xMotion = 0.0;
	yMotion = 0.0;
	
	if (touchBuffer != nil)
	{
		[touchBuffer release];
		touchBuffer = nil;
	}
	
	touchBuffer = [[NSBuffer bufferWithCapacity:25] retain];
	
	UITouch *touch = [touches anyObject];
	CGPoint new = [touch locationInView:self];
	
	[touchBuffer addObject:[NSDictionary dictionaryWithObjectsAndKeys:
							[NSNumber numberWithDouble:new.x], @"locationX",
							[NSNumber numberWithDouble:new.y], @"locationY",
							[NSDate date], @"timeStamp", nil]];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event 
{
	UITouch *touch = [touches anyObject];
	CGPoint new = [touch locationInView:self];
	CGPoint old = [touch previousLocationInView:self];
	
	[touchBuffer addObject:[NSDictionary dictionaryWithObjectsAndKeys:
							[NSNumber numberWithDouble:new.x], @"locationX",
							[NSNumber numberWithDouble:new.y], @"locationY",
							[NSDate date], @"timeStamp", nil]];	
	
	GLfloat glOldX = LINEAR_MAP(old.x, 0.0f, 320.0f, 0.0f, 1.0f);
	GLfloat glOldY = LINEAR_MAP(old.y, 0.0f, 480.0f, 1.5f, 0.0f);
	GLfloat glNewX = LINEAR_MAP(new.x, 0.0f, 320.0f, 0.0f, 1.0f);
	GLfloat glNewY = LINEAR_MAP(new.y, 0.0f, 480.0f, 1.5f, 0.0f);
	
	currentViewportLeft += (glOldX - glNewX);
	currentViewportTop += (glOldY - glNewY);
	[self ensureViewportBoundaries];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	NSDate *endTime = [NSDate date];
	
	UITouch *touch = [touches anyObject];
	CGPoint new = [touch locationInView:self];
	CGPoint old = [touch previousLocationInView:self];

	[touchBuffer addObject:[NSDictionary dictionaryWithObjectsAndKeys:
							[NSNumber numberWithDouble:new.x], @"locationX",
							[NSNumber numberWithDouble:new.y], @"locationY",
							endTime, @"timeStamp", nil]];	
	
	// NSLog(@"end touches...");
	
	// do standard motion first
	GLfloat glOldX = LINEAR_MAP(old.x, 0.0f, 320.0f, 0.0f, 1.0f);
	GLfloat glOldY = LINEAR_MAP(old.y, 0.0f, 480.0f, 1.5f, 0.0f);
	GLfloat glNewX = LINEAR_MAP(new.x, 0.0f, 320.0f, 0.0f, 1.0f);
	GLfloat glNewY = LINEAR_MAP(new.y, 0.0f, 480.0f, 1.5f, 0.0f);
	
	currentViewportLeft += (glOldX - glNewX);
	currentViewportTop += (glOldY - glNewY);
	[self ensureViewportBoundaries];
	
	// find the first buffered location that is less than FLICK_TIME_BACK seconds behind our endTime
	NSUInteger lessThanIndex = 999;
	
	for (NSUInteger testIndex = 0; testIndex < touchBuffer.length; testIndex++)
	{
		NSDictionary *dict = [touchBuffer objectAtIndex:testIndex];
		NSTimeInterval delta = [endTime timeIntervalSinceDate:[dict objectForKey:@"timeStamp"]];
		if (delta < FLICK_TIME_BACK)
		{
			lessThanIndex = testIndex;
			break;
		}
	}
	
	if (lessThanIndex == 999)
	{
		// NSLog(@"XXXXXXX Couldn't find a reasonable lessThanIndex. Oops.");
		[touchBuffer release];
		touchBuffer = nil;
		return;
	}
	
	if (lessThanIndex == 0)
	{
		// NSLog(@"XXXXXXX Defaulting the lessThanIndex (must be a short gesture.)");
		lessThanIndex = 1;
	}
	
	// use linear interpolation to "decide" where the touch was exactly FLICK_TIME_BACK seconds ago	
	// NSLog(@"   >>> lessThanIndex is %i", lessThanIndex); 
	
	NSDictionary *lessThanDict = [touchBuffer objectAtIndex:lessThanIndex];
	NSDictionary *greaterThanDict = [touchBuffer objectAtIndex:(lessThanIndex - 1)];
	
	double lessThanX = [[lessThanDict objectForKey:@"locationX"] doubleValue];
	double lessThanY = [[lessThanDict objectForKey:@"locationY"] doubleValue];
	NSDate *lessThanTime = [lessThanDict objectForKey:@"timeStamp"];
	double greaterThanX = [[greaterThanDict objectForKey:@"locationX"] doubleValue];
	double greaterThanY = [[greaterThanDict objectForKey:@"locationY"] doubleValue];
	NSDate *greaterThanTime = [greaterThanDict objectForKey:@"timeStamp"];

	NSTimeInterval lessThanBack = [endTime timeIntervalSinceDate:lessThanTime];
	NSTimeInterval greaterThanBack = [endTime timeIntervalSinceDate:greaterThanTime];
		
	double between = LINEAR_MAP(FLICK_TIME_BACK,lessThanBack,greaterThanBack,0.0f,1.0f);
	//NSAssert((between >= 0.0f) && (between <= 1.0f), @"Should never happen!");
	
	// NSLog(@"   >>> between is %f", between); 
	
	double flickX = (greaterThanX * between) + (lessThanX * (1.0f-between));
	double flickY = (greaterThanY * between) + (lessThanY * (1.0f-between));
	
	// NSLog(@"   >>> flick: (%f, %f) and new: (%f, %f)", flickX, flickY, new.x, new.y); 
	
	// now: fully dampen the motion if it is within the "they aren't flicking" variance
	if (abs(new.x - flickX) < MIN_PIXELS_FOR_FLICK) 
	{
		flickX = new.x;
	}
	
	if (abs(new.y - flickY) < MIN_PIXELS_FOR_FLICK)
	{
		flickY = new.y;
	}	
	
	if (new.x == flickX && new.y == flickY)
	{
		[touchBuffer release];
		touchBuffer = nil;
		return;
	}

	// Convert to GL coordinates and compute the final motion
	GLfloat glFlickX = LINEAR_MAP(flickX, 0.0f, 320.0f, 0.0f, 1.0f);
	GLfloat glFlickY = LINEAR_MAP(flickY, 0.0f, 480.0f, 1.5f, 0.0f);

	xMotion = (glFlickX - glNewX) * MOTION_MULTIPLIER;
	yMotion = (glFlickY - glNewY) * MOTION_MULTIPLIER;

	// Clamp the motion to prevent insanity for very short gestures.
	// To keep the direction "correct", make sure this clamping preserves
	// the aspect ratio. In other words, find the "largest" offender and
	// clamp that down to MOTION_MAX
	GLfloat absX = fabs(xMotion);
	GLfloat absY = fabs(yMotion);
	if (absX >= MOTION_MAX && absX >= absY)
	{
	//	NSLog(@"   >>> Scaling along X");
		GLfloat scaleFactor = MOTION_MAX / absX;
		xMotion *= scaleFactor;
		yMotion *= scaleFactor;
	}
	else if (absY >= MOTION_MAX)
	{
	//	NSLog(@"   >>> Clamping along Y");
		GLfloat scaleFactor = MOTION_MAX / absY;
		xMotion *= scaleFactor;
		yMotion *= scaleFactor;
	}
		
	//NSLog(@"   >>> motion: (%f, %f)", xMotion, yMotion);
	
	[touchBuffer release];
	touchBuffer = nil;
}
@end
