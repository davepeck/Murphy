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
#import "MurphyConstants.h"



@interface EAGLView (EAGLViewPrivate)

- (BOOL)createFramebuffer;
- (void)destroyFramebuffer;
- (void)setupView;
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)loadCurrentLevel;
- (void)switchLevels;

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
		
		levelNames = [[NSArray arrayWithObjects:@"Achtung!", @"And So It Begins", @"Bolder", @"Catacombs-Aquatron", @"Combinations", @"Crystalline", @"Dash Dash", @"Daylight", @"Dot Dot", @"Excavation", @"Falling", @"Going Up", @"Gold Rush", @"Golden Rule", @"Inflamatory", @"It's Alive!!", @"Labyrinth", @"Love Boat", @"No More Secrets", @"Out In Out", @"Robots & Plasmoids", @"Short Circuit", @"Stress", @"Thriller", @"Trapped Inside", @"Wizard", nil] retain];
		currentLevel = 0;
		animationInterval = 1.0 / 60.0;
		
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

-(void)loadCurrentLevel
{
	if (tileGrid != nil)
	{
		[tileGrid release];
		tileGrid = nil;		
	}
	
	// Load our level's textures
	MurphyLevel *levelFile = [MurphyLevel murphyLevelWithNamedResource:[levelNames objectAtIndex:currentLevel]];
	TileAtlas *worldAtlas = [TileAtlas tileAtlasWithResourcePNG:levelFile.graphicsSet tilePixelWidth:32 tilePixelHeight:32 tilesAcross:10 tilesDown:31];
	TileMap *worldMap = [TileMap tileMapWithAtlas:worldAtlas startTileId:0];	
	
	// find our boundaries
	uint16_t actual_width = 0;
	uint16_t actual_height = 0;
	
	for (uint16_t y = 0; y < levelFile.height && actual_width == 0; y++)
	{
		for (uint16_t x = 0; x < levelFile.width && actual_height == 0; x++)
		{
			uint8_t iconS;
			uint8_t iconT;
			[levelFile getIconForX:x y:y s:&iconS t:&iconT];
			uint16_t iconTileId = (iconT * 10) + iconS;
			
			if (iconTileId == kBottomRightBorderTile)
			{
				actual_width = x + 1;
				actual_height = y + 1;
			}
		}
	}
	
	tileGrid = [[TileGrid tileGridWithMap:worldMap width:actual_width height:actual_height] retain];	
	
	// Fill the tileGrid
	double murphy_x = 0;
	double murphy_y = 0;
	for (uint16_t y = 0; y < actual_height; y++)
	{
		for (uint16_t x = 0; x < actual_width; x++)
		{
			uint8_t iconS;
			uint8_t iconT;
			[levelFile getIconForX:x y:y s:&iconS t:&iconT];
			uint16_t iconTileId = (iconT * 10) + iconS;
			
			if (iconTileId == kInfotronTile)
			{
				iconTileId = kFirstInfotronTile;
			}
			else if (iconTileId == kQuarkTile)
			{
				iconTileId = kFirstQuarkTile;
			}
			else if (iconTileId == kTerminalTile)
			{
				iconTileId = kFirstTerminalTile;
			}
			else if (iconTileId == kOliverTile)
			{
				iconTileId = kBubFrame;
				murphy_x = x;
				murphy_y = y;
			}
			else if (iconTileId >= kUpScissorTile && iconTileId <= kLeftScissorTile)
			{
				iconTileId = kFirstUpScissorTile; /* kUpLeftScissorTile is last */
			}
			
			[tileGrid setTileIdAtX:x y:y tileId:iconTileId];
		}
	}
	
	[tileGrid setPixelWidth:320.0 height:480.0];
	[tileGrid setGridLeft:0.0f top:1.5f];

	flickDynamics.currentScrollLeft = 0.0f + ((1.0 / (320.0 / 32.0)) * (murphy_x + 0.5 - (320.0 / 64.0)));
	flickDynamics.currentScrollTop = 1.5f - ((1.0 / (320.0 / 32.0)) * (murphy_y + 0.5 - (480.0 / 64.0)));
	[flickDynamics setScrollBoundsLeft:tileGrid.gridLeft scrollBoundsTop:tileGrid.gridTop scrollBoundsRight:tileGrid.gridRight scrollBoundsBottom:tileGrid.gridBottom];
	[flickDynamics ensureValidScrollPosition];
	
	[tileGrid.map animateTileId:kFirstInfotronTile toTileId:kLastInfotronTile timeInterval:0.075f allInRange:NO];
	[tileGrid.map animateTileId:kFirstQuarkTile toTileId:kLastQuarkTile timeInterval:0.175f allInRange:NO];
	[tileGrid.map animateTileId:kFirstTerminalTile toTileId:kLastTerminalTile timeInterval:0.175f allInRange:NO];
	[tileGrid.map animateTileId:kFirstUpScissorTile toTileId:kUpLeftScissorTile timeInterval:0.175f allInRange:NO];
	[tileGrid.map animateTileId:kBubFrame toTileId:kDBubFrame2 timeInterval:0.175f allInRange:NO];
}

-(void)switchLevels
{
	currentLevel += 1;
	if (currentLevel == [levelNames count])
	{
		currentLevel = 0;
	}
	
	[flickDynamics stopMotion];
	flickDynamics.currentScrollTop = 1.5f;
	flickDynamics.currentScrollLeft = 0.0f;
	
	[self loadCurrentLevel];
}

- (void)setupView
{
	// Sets up matrices and transforms for OpenGL ES
	glViewport(0, 0, backingWidth, backingHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, 1.0f, 0.0f, 1.5f, -1.0f, 1.0f);	
	glMatrixMode(GL_MODELVIEW);
	
	flickDynamics = [[FlickDynamics flickDynamicsWithViewportWidth:1.0f viewportHeight:1.5f scrollBoundsLeft:0.0f scrollBoundsTop:1.5f scrollBoundsRight:1.0f scrollBoundsBottom:0.0f] retain];
	flickDynamics.currentScrollLeft = 0.0f;
	flickDynamics.currentScrollTop = 1.5f;
	
	currentLevel = 0;
	[self loadCurrentLevel];
	
	// Clears the view with black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	
}

// Updates the OpenGL view when the timer fires
- (void)drawView
{
	// Make sure that you are drawing to the current context
	[EAGLContext setCurrentContext:context];
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	
	[flickDynamics animate];
	
	GLfloat displayViewportLeft = flickDynamics.currentScrollLeft;
	GLfloat displayViewportTop = flickDynamics.currentScrollTop;
	
	[tileGrid alignViewportToPixel:&displayViewportLeft top:&displayViewportTop];
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(displayViewportLeft, displayViewportLeft + 1.0f, displayViewportTop - 1.5f, displayViewportTop, -1.0f, 1.0f);	
	
	glMatrixMode(GL_MODELVIEW);
	
	glClear(GL_COLOR_BUFFER_BIT);
	[tileGrid drawInViewportLeft:displayViewportLeft top:displayViewportTop right:displayViewportLeft + 1.0f bottom:displayViewportTop - 1.5f];
		
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

// Stop animating and release resources when they are no longer needed.
- (void)dealloc
{
	if (flickDynamics != nil)
	{
		[flickDynamics release];
		flickDynamics = nil;
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
	UITouch *touch = [touches anyObject];
	CGPoint first = [touch locationInView:self];	
	GLfloat glFirstX = LINEAR_MAP(first.x, 0.0f, 320.0f, 0.0f, 1.0f);
	GLfloat glFirstY = LINEAR_MAP(first.y, 0.0f, 480.0f, 1.5f, 0.0f);
	[flickDynamics startTouchAtX:glFirstX y:glFirstY];
	
	// THE LEVEL SWITCHER!
	if ([touch tapCount] == 3)
	{
		[self switchLevels];
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event 
{
	UITouch *touch = [touches anyObject];
	CGPoint new = [touch locationInView:self];
	GLfloat glNewX = LINEAR_MAP(new.x, 0.0f, 320.0f, 0.0f, 1.0f);
	GLfloat glNewY = LINEAR_MAP(new.y, 0.0f, 480.0f, 1.5f, 0.0f);	
	[flickDynamics moveTouchAtX:glNewX y:glNewY];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	UITouch *touch = [touches anyObject];
	CGPoint last = [touch locationInView:self];
	GLfloat glLastX = LINEAR_MAP(last.x, 0.0f, 320.0f, 0.0f, 1.0f);
	GLfloat glLastY = LINEAR_MAP(last.y, 0.0f, 480.0f, 1.5f, 0.0f);	
	[flickDynamics endTouchAtX:glLastX y:glLastY];	
}
@end
