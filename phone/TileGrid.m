//
//  TileGrid.m
//  iMurphy
//
//  Created by Dave Peck on 4/16/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "TileGrid.h"

@interface TileGrid (Private)

-(id)initWithMap:(TileMap *)map width:(uint16_t)tilesWide height:(uint16_t)tilesHigh;
-(void)dealloc;

@end


@implementation TileGrid (Private)

-(id)initWithMap:(TileMap *)myMap width:(uint16_t)tilesWide height:(uint16_t)tilesHigh
{
	self = [super init];
	
	if (self != nil)
	{
		map = [myMap retain];
		width = tilesWide;
		height = tilesHigh;
		grid = malloc(sizeof(uint16_t) * width * height);
		
		// TODO move this stuff
		originX = 0.0;
		originY = 0.0;
		pixelWidth = 0.0;
		pixelHeight = 0.0;
	}
	
	return self;
}

-(void)dealloc
{
	free(grid);
	grid = nil;
	
	[map release];
	map = nil;
	
	[super dealloc];
}

@end


@implementation TileGrid

+(id)tileGridWithMap:(TileMap *)myMap width:(uint16_t)tilesWide height:(uint16_t)tilesHigh
{
	return [[[TileGrid alloc] initWithMap:myMap width:tilesWide height:tilesHigh] autorelease];
}

@synthesize map;
@synthesize width;
@synthesize height;

-(uint16_t)getTileIdAtX:(uint16_t)x y:(uint16_t)y
{
	NSAssert((x < width) && (y < height), @"Invalid x/y tile requested.");
	return grid[(y*width) + x];
}

-(void)setTileIdAtX:(uint16_t)x y:(uint16_t)y tileId:(uint16_t)tileId
{
	NSAssert((x < width) && (y < height), @"Invalid x/y tile requested.");
	grid[(y*width) + x] = tileId;
}

// Do these belong here?
-(void)setViewportOriginX:(GLfloat)myOriginX y:(GLfloat)myOriginY
{
	originX = myOriginX;
	originY = myOriginY;
}

-(void)setPixelWidth:(GLfloat)myPixelWidth height:(GLfloat)myPixelHeight
{
	pixelWidth = myPixelWidth;
	pixelHeight = myPixelHeight;
}

-(void)drawInViewportLeft:(GLfloat)left top:(GLfloat)top right:(GLfloat)right bottom:(GLfloat)bottom
{
	// What tiles does this viewport intersect with?
	
	// Well, first, what is the natural width and height of a tile in GL-space?
	
}

@end
