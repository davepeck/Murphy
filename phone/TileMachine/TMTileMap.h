//
//  TileMap.h
//  iMurphy
//
//  Provides a mapping from a TileID (a single integer) to the (S,T) tile coordinates in the underlying tile atlas.
//  This allows you to refer to a given image with a single number -- and it allows you to efficiently
//  switch out what specific image a given Tile ID refers to.
//
//  Basically, this class is a simple indirection -- it was a useful idea in the SpriteWorld era so I kept it.
//
//  Created by Dave Peck on 4/16/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TMTileAtlas.h"

@interface TMTileMap : NSObject {
	TMTileAtlas *atlas;	
	uint16_t startTileId;
	uint16_t stopTileId;
	uint8_t *tileIdToS;
	uint8_t *tileIdToT;	
	NSMutableDictionary *animations;
}

+(id)tileMapWithAtlas:(TMTileAtlas *)atlas startTileId:(uint16_t)startTileId;

@property (readonly) TMTileAtlas *atlas;
@property (readonly) uint16_t startTileId;
@property (readonly) uint16_t stopTileId;

-(uint16_t)tileIdForAtlasS:(uint8_t)s t:(uint8_t)t; /* helper function */

-(void)textureCoordinatesForTileId:(uint16_t)tileId coordinates:(GLfloat*)coordinates;
-(void)getMapForTileId:(uint16_t)tileId s:(uint8_t *)s t:(uint8_t *)t;
-(void)setMapForTileId:(uint16_t)tileId s:(uint8_t)s t:(uint8_t)t;

// TODO these are swill as designed
-(void)setMapForTileId:(uint16_t)tileId withTileId:(uint16_t)newTileId;
-(void)animateTileId:(uint16_t)tileIdFrom toTileId:(uint16_t)tileIdTo timeInterval:(NSTimeInterval)animationInterval allInRange:(BOOL)allInRange;
-(void)stopAnimatingTileId:(uint16_t)tileIdFrom;
-(void)stopAllAnimations;

@end
