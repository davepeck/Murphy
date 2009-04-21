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

#define MOVE_INCREMENT 0.025f

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
		
		movingLeft = NO;
		movingRight = NO;
		movingUp = NO;
		movingDown = NO;
				
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
	
	[tileGrid.map animateTileId:TILE_WATER_FIRST toTileId:TILE_WATER_LAST timeInterval:0.25f];
	[tileGrid.map animateTileId:TILE_WHIRLPOOL_FIRST toTileId:TILE_WHIRLPOOL_LAST timeInterval:0.5f];
	[tileGrid.map animateTileId:TILE_RADAR_FIRST toTileId:TILE_RADAR_LAST timeInterval:0.6f];
	[tileGrid.map animateTileId:TILE_LIFE_FIRST toTileId:TILE_LIFE_LAST timeInterval:0.73f];
	[tileGrid.map animateTileId:TILE_LEVEL1_FIRST toTileId:TILE_LEVEL1_LAST timeInterval:0.4f];
	[tileGrid.map animateTileId:TILE_LEVEL2_FIRST toTileId:TILE_LEVEL2_LAST timeInterval:0.3f];
	[tileGrid.map animateTileId:TILE_LEVEL3_FIRST toTileId:TILE_LEVEL3_LAST timeInterval:0.7f];
	
	// Clears the view with black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	
}

// Updates the OpenGL view when the timer fires
- (void)drawView
{
	// Make sure that you are drawing to the current context
	[EAGLContext setCurrentContext:context];
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
		
	if (movingUp)
	{
		currentViewportTop += MOVE_INCREMENT;
		if (currentViewportTop > tileGrid.gridTop)
		{
			currentViewportTop = tileGrid.gridTop;
		}
	}
	
	if (movingDown)
	{
		currentViewportTop -= MOVE_INCREMENT;
		if (currentViewportTop - 1.5f < tileGrid.gridBottom)
		{
			currentViewportTop = tileGrid.gridBottom + 1.5f;
		}
	}
	
	if (movingLeft)
	{
		currentViewportLeft -= MOVE_INCREMENT;
		if (currentViewportLeft < tileGrid.gridLeft)
		{
			currentViewportLeft = tileGrid.gridLeft;
		}
	}
	
	if (movingRight)
	{
		currentViewportLeft += MOVE_INCREMENT;
		if (currentViewportLeft + 1.0f > tileGrid.gridRight)
		{
			currentViewportLeft = tileGrid.gridRight - 1.0f;
		}		
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
	UITouch *touch = [touches anyObject];
	CGPoint point = [touch locationInView:self];
//	NSLog(@"Touch Began at: (%f, %f)", point.x, point.y);
	
	movingUp = NO;
	movingDown = NO;
	movingLeft = NO;
	movingRight = NO;
	
	if (point.x <= 160.0)
	{
		// Left half of screen.
		
		if (point.x >= point.y)
		{
			// Top-right of the left half of the screen
			movingUp = YES;
		}
		else if (point.x >= (480.0 - point.y))
		{
			// Bottom-right of the left half of the screen
			movingDown = YES;
		}
		else
		{
			movingLeft = YES;
		}
	}
	else
	{
		// Right half of the screen
		
		if ((320.0 - point.x) >= point.y)
		{
			// Top-left of the right half of the screen
			movingUp = YES;
		}
		else if ((320.0 - point.x) >= (480.0 - point.y))
		{
			// Bottom-left of the right half of the screen
			movingDown = YES;
		}
		else
		{
			movingRight = YES;
		}
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event 
{
	UITouch *touch = [touches anyObject];
	CGPoint point = [touch locationInView:self];
//	NSLog(@"Touch Moved at: (%f, %f)", point.x, point.y);
	
	movingUp = NO;
	movingDown = NO;
	movingLeft = NO;
	movingRight = NO;
	
	if (point.x <= 160.0)
	{
		// Left half of screen.
		
		if (point.x >= point.y)
		{
			// Top-right of the left half of the screen
			movingUp = YES;
		}
		else if (point.x >= (480.0 - point.y))
		{
			// Bottom-right of the left half of the screen
			movingDown = YES;
		}
		else
		{
			movingLeft = YES;
		}
	}
	else
	{
		// Right half of the screen
		
		if ((320.0 - point.x) >= point.y)
		{
			// Top-left of the right half of the screen
			movingUp = YES;
		}
		else if ((320.0 - point.x) >= (480.0 - point.y))
		{
			// Bottom-left of the right half of the screen
			movingDown = YES;
		}
		else
		{
			movingRight = YES;
		}
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	movingUp = NO;
	movingDown = NO;
	movingLeft = NO;
	movingRight = NO;	
}
@end
