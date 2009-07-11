//
//  MurphyView.m
//  iMurphy
//
//  Created by Dave Peck on 4/3/09.
//  Copyright Code Orange 2009. All rights reserved.
//



#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "MurphyView.h"
#import "MurphyLevel.h"
#import "TMTexture.h"
#import "TMTileAtlas.h"
#import "TMTileMap.h"
#import "TMTileGrid.h"
#import "TMMath.h"
#import "MurphyConstants.h"

@interface MurphyView (MurphyViewPrivate)

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)loadCurrentLevel;
- (void)switchLevels;

@end

@implementation MurphyView

+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder
{
	self = [super initWithCoder:coder];
	
	if (self != nil) 
	{
		CAEAGLLayer *eaglLayer = (CAEAGLLayer*) self.layer;		
		tileEngine = [[TMEngine engineWithGlLayer:eaglLayer] retain];
		tileEngine.delegate = self;
		[tileEngine setAnimationRate:1.0 / 60.0];		
		
		levelNames = [[NSArray arrayWithObjects:@"Achtung!", @"And So It Begins", @"Bolder", @"Catacombs-Aquatron", @"Combinations", @"Crystalline", @"Dash Dash", @"Daylight", @"Dot Dot", @"Excavation", @"Falling", @"Going Up", @"Gold Rush", @"Golden Rule", @"Inflamatory", @"It's Alive!!", @"Labyrinth", @"Love Boat", @"No More Secrets", @"Out In Out", @"Robots & Plasmoids", @"Short Circuit", @"Stress", @"Thriller", @"Trapped Inside", @"Wizard", nil] retain];
		currentLevel = 0;
	}
	
	return self;
}

- (void)layoutSubviews
{
	/*
	[EAGLContext setCurrentContext:context];
	[self destroyFramebuffer];
	[self createFramebuffer];
	[self drawView];
	 */
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
	TMTileAtlas *worldAtlas = [TMTileAtlas tileAtlasWithResourcePNG:levelFile.graphicsSet tilePixelWidth:32 tilePixelHeight:32 tilesAcross:10 tilesDown:31];
	TMTileMap *worldMap = [TMTileMap tileMapWithAtlas:worldAtlas startTileId:0];	
	
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
	
	tileGrid = [[TMTileGrid tileGridWithMap:worldMap width:actual_width height:actual_height] retain];	
	
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

// Stop animating and release resources when they are no longer needed.
- (void)dealloc
{
	if (flickDynamics != nil)
	{
		[flickDynamics release];
		flickDynamics = nil;
	}
	
	[super dealloc];
}

- (void)beforeFrame
{
	[flickDynamics animate];
	tileEngine.displayViewportTop = flickDynamics.currentScrollTop;
	tileEngine.displayViewportLeft = flickDynamics.currentScrollLeft;
}

- (void)afterFrame
{
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

- (void)setAnimationRate:(NSTimeInterval)theRate
{
	[tileEngine setAnimationRate:theRate];
}

- (void)startAnimation
{
	[tileEngine startAnimation];
}

@end
