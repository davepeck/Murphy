//
//  TileAtlas.h
//  iMurphy
//
//  Represents a collection of tile images that are arranged in a grid
//  inside a single larger image. Allows you to get opengl texture coordinates
//  for the individual icons.
//
//  Created by Dave Peck on 4/16/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "Texture.h"

@interface TileAtlas : NSObject 
{	
	Texture *texture;	
	uint16_t tilePixelWidth;
	uint16_t tilePixelHeight;	
	uint8_t tilesAcross;
	uint8_t tilesDown;		
}

+(id)tileAtlasWithFile:(NSString*)path tilePixelWidth:(uint16_t)tilePixelWidth tilePixelHeight:(uint16_t)tilePixelHeight tilesAcross:(uint16_t)tilesAcross tilesDown:(uint16_t)tilesDown;
+(id)tileAtlasWithResourcePNG:(NSString*)resourceName;

@property (readonly) Texture *texture;
@property (readonly) uint8_t tilesAcross;
@property (readonly) uint8_t tilesDown;
@property (readonly) uint16_t tilePixelWidth;
@property (readonly) uint16_t tilePixelHeight;

-(void)textureCoordinatesForS:(uint8_t)s t:(uint8_t)t coordinates:(GLfloat*)coordinates;

@end
