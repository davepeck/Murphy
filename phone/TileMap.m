//
//  TileMap.m
//  iMurphy
//
//  Created by Dave Peck on 4/16/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "TileMap.h"

@interface TileMap (Private)

-(id)initWithAtlas:(TileAtlas *)atlas startTileId:(uint16_t)startId;
-(void)dealloc;

@end

@implementation TileMap (Private)

-(id)initWithAtlas:(TileAtlas *)myAtlas startTileId:(uint16_t)startId
{
	self = [super init];
	
	if (self != nil)
	{
		atlas = [myAtlas retain];
		
		// Set up our IDs
		startTileId = startId;		
		uint16_t tile_count = atlas.tilesAcross * atlas.tilesDown;
		stopTileId = startTileId + tile_count;	
		
		// Allocate the map from ID to tile coordinates in the atlas
		tileIdToS = malloc(tile_count);
		tileIdToT = malloc(tile_count);		
		NSAssert(tileIdToS != nil, @"TODO memory");
		NSAssert(tileIdToT != nil, @"TODO memory");
		
		// Now perform a default fill
		uint16_t index = 0;
		for (uint8_t t = 0; t < atlas.tilesDown; t++)
		{
			for (uint8_t s = 0; s < atlas.tilesAcross; s++)
			{
				tileIdToS[index] = s;
				tileIdToT[index] = t;
				index += 1;
			}
		}
		
	}
	
	return self;
}

-(void)dealloc
{
	[atlas release];
	atlas = nil;
	
	free(tileIdToS);
	tileIdToS = nil;
	free(tileIdToT);
	tileIdToT = nil;
	
	[super dealloc];
}

@end

@implementation TileMap

+(id)tileMapWithAtlas:(TileAtlas *)atlas startTileId:(uint16_t)startId
{
	return [[TileMap alloc] initWithAtlas:atlas startTileId:startId];
}

@synthesize atlas;
@synthesize startTileId;
@synthesize stopTileId;

-(void)textureCoordinatesForTileId:(uint16_t)tileId coordinates:(GLfloat*)coordinates
{
	NSAssert(tileId >= startTileId && tileId < stopTileId, @"Tile ID is out of bounds.");	
	uint16_t index = (tileId - startTileId);
	uint8_t s = tileIdToS[index];
	uint8_t t = tileIdToT[index];
	[atlas textureCoordinatesForS:s t:t coordinates:coordinates];
}

-(void)getMapForTileId:(uint16_t)tileId s:(uint8_t *)s t:(uint8_t *)t
{
	NSAssert(tileId >= startTileId && tileId < stopTileId, @"Tile ID is out of bounds.");	
	uint16_t index = (tileId - startTileId);
	*s = tileIdToS[index];
	*t = tileIdToT[index];
}

-(void)setMapForTileId:(uint16_t)tileId s:(uint8_t)s t:(uint8_t)t
{
	NSAssert(tileId >= startTileId && tileId < stopTileId, @"Tile ID is out of bounds.");	
	uint16_t index = (tileId - startTileId);
	tileIdToS[index] = s;
	tileIdToT[index] = t;
}

@end
