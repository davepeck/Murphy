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
-(void)getDefaultMapForTileId:(uint16_t)tileId s:(uint8_t *)s t:(uint8_t *)t;
-(void)animationTick:(NSTimer*)animationTimer;
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
		
		animations = [[NSMutableDictionary dictionaryWithCapacity:4] retain];
	}
	
	return self;
}

-(void)dealloc
{	
	for (id key in animations) 
	{
		NSTimer *animationTimer = [animations objectForKey:key];
		[animationTimer invalidate];
		[animations removeObjectForKey:key];
	}	
	
	[animations release];
	animations = nil;
	
	[atlas release];
	atlas = nil;
	
	free(tileIdToS);
	tileIdToS = nil;
	free(tileIdToT);
	tileIdToT = nil;
	
	[super dealloc];
}

-(void)getDefaultMapForTileId:(uint16_t)tileId s:(uint8_t *)s t:(uint8_t *)t
{
	*s = (tileId - startTileId) % (atlas.tilesAcross);
	*t = (tileId - startTileId) / (atlas.tilesAcross);
}

-(void)animationTick:(NSTimer*)animationTimer
{
	if (animationTimer == nil || !animationTimer.isValid)
	{
		return;
	}
	
	NSMutableDictionary *userInfo = [animationTimer userInfo];
	NSNumber *tileIdFromNumber = [userInfo objectForKey:@"tileIdFrom"];
	NSNumber *tileIdToNumber = [userInfo objectForKey:@"tileIdTo"];
	NSNumber *tileIdCurrentNumber = [userInfo objectForKey:@"tileIdCurrent"];
	NSNumber *allInRangeNumber = [userInfo objectForKey:@"allInRange"];
	
	BOOL allInRange = [allInRangeNumber boolValue];
	
	uint16_t tileIdFrom = [tileIdFromNumber unsignedShortValue];
	uint16_t tileIdTo = [tileIdToNumber unsignedShortValue];
	uint16_t tileIdCurrent = [tileIdCurrentNumber unsignedShortValue];

	tileIdCurrent += 1;
	if (tileIdCurrent > tileIdTo)
	{
		tileIdCurrent = tileIdFrom;
	}
	
	[userInfo setObject:[NSNumber numberWithUnsignedShort:tileIdCurrent] forKey:@"tileIdCurrent"];
	
	if (allInRange)
	{
		uint16_t currentOffset = tileIdCurrent - tileIdFrom;
		
		for (uint16_t i = tileIdFrom; i <= tileIdTo; i++)
		{			
			uint16_t moveTo = i + currentOffset;
			while (moveTo > tileIdTo)
			{
				moveTo = moveTo - ((tileIdTo - tileIdFrom) + 1);
			}
			[self setMapForTileId:i withTileId:moveTo];
		}
	}
	else
	{	
		[self setMapForTileId:tileIdFrom withTileId:tileIdCurrent];
	}
}


@end

@implementation TileMap

+(id)tileMapWithAtlas:(TileAtlas *)atlas startTileId:(uint16_t)startId
{
	return [[[TileMap alloc] initWithAtlas:atlas startTileId:startId] autorelease];
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

-(uint16_t)tileIdForAtlasS:(uint8_t)s t:(uint8_t)t /* helper function */
{
	return ((uint16_t) t * (uint16_t) atlas.tilesAcross) + s + startTileId;
}

-(void)setMapForTileId:(uint16_t)tileId withTileId:(uint16_t)newTileId
{
	uint8_t s;
	uint8_t t;
	
	// TODO this method is swill
	[self getDefaultMapForTileId:newTileId s:&s t:&t];
	[self setMapForTileId:tileId s:s t:t];
}

-(void)animateTileId:(uint16_t)tileIdFrom toTileId:(uint16_t)tileIdTo timeInterval:(NSTimeInterval)animationInterval allInRange:(BOOL)allInRange
{
	NSMutableDictionary *userInfo = [NSMutableDictionary dictionaryWithCapacity:4];	
	[userInfo setObject:[NSNumber numberWithUnsignedShort:tileIdFrom] forKey:@"tileIdFrom"];
	[userInfo setObject:[NSNumber numberWithUnsignedShort:tileIdTo] forKey:@"tileIdTo"];
	[userInfo setObject:[NSNumber numberWithUnsignedShort:tileIdFrom] forKey:@"tileIdCurrent"];
	[userInfo setObject:[NSNumber numberWithBool:allInRange] forKey:@"allInRange"];
	
	NSTimer *animationTimer = [NSTimer scheduledTimerWithTimeInterval:animationInterval target:self selector:@selector(animationTick:) userInfo:userInfo repeats:YES];
	[animations setObject:animationTimer forKey:[NSNumber numberWithUnsignedShort:tileIdFrom]];
}

-(void)stopAnimatingTileId:(uint16_t)tileIdFrom
{
	NSNumber *key = [NSNumber numberWithUnsignedShort:tileIdFrom];
	NSTimer *animationTimer = [animations objectForKey:key];
	[animationTimer invalidate];
	[animations removeObjectForKey:key];
}

@end
