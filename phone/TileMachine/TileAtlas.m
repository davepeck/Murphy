//
//  TileAtlas.m
//  iMurphy
//
//  Created by Dave Peck on 4/16/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "TileAtlas.h"
#import "Texture.h"

@interface TileAtlas (Private)

-(id)initWithFile:(NSString*)path tilePixelWidth:(uint16_t)width tilePixelHeight:(uint16_t)height tilesAcross:(uint16_t)across tilesDown:(uint16_t)down;
-(void)dealloc;

@end

@implementation TileAtlas (Private)

-(id)initWithFile:(NSString*)path tilePixelWidth:(uint16_t)width tilePixelHeight:(uint16_t)height tilesAcross:(uint16_t)across tilesDown:(uint16_t)down;
{
	self = [super init];
	
	if (self != nil)
	{
		texture = [[Texture textureWithFile:path] retain];
		tilePixelWidth = width;
		tilePixelHeight = height;
		tilesAcross = across;
		tilesDown = down;
		
		tileWidthInTextureSpace = ((GLfloat)tilePixelWidth) / ((GLfloat)texture.width);
		tileHeightInTextureSpace = ((GLfloat)tilePixelHeight) / ((GLfloat)texture.height);
	}
	
	return self;
}

-(void)dealloc
{
	[texture release];
	texture = nil;	
	[super dealloc];
}

@end


@implementation TileAtlas

+(id)tileAtlasWithFile:(NSString*)path tilePixelWidth:(uint16_t)width tilePixelHeight:(uint16_t)height tilesAcross:(uint16_t)across tilesDown:(uint16_t)down
{
	return [[[TileAtlas alloc] initWithFile:path tilePixelWidth:width tilePixelHeight:height tilesAcross:across tilesDown:down] autorelease];
}

+(id)tileAtlasWithResourcePNG:(NSString*)resourceName tilePixelWidth:(uint16_t)width tilePixelHeight:(uint16_t)height tilesAcross:(uint16_t)across tilesDown:(uint16_t)down
{	
	return [TileAtlas tileAtlasWithFile:[[NSBundle mainBundle] pathForResource:resourceName ofType:@"png"]  tilePixelWidth:width tilePixelHeight:height tilesAcross:across tilesDown:down];
}

@synthesize texture;
@synthesize tilesAcross;
@synthesize tilesDown;
@synthesize tilePixelWidth;
@synthesize tilePixelHeight;

-(void)textureCoordinatesForS:(uint8_t)s t:(uint8_t)t coordinates:(GLfloat*)coordinates
{
	// fill "coordinates" with eight floats indicating the (B,L) > (B,R) > (T,L) > (T,R) traversal of this texture
	// PERFORMANCE: could actually precompute these and just make it a lookup.
	GLfloat sf = (GLfloat) s;
	GLfloat tf = (GLfloat) t;
	
	GLfloat s_left = sf * tileWidthInTextureSpace;
	GLfloat s_right = (sf + 1.0) * tileWidthInTextureSpace;
	GLfloat t_top = tf * tileHeightInTextureSpace;
	GLfloat t_bottom = (tf + 1.0) * tileHeightInTextureSpace; /* tile textures are inverted when we load them */
	
	// inset by half a texel in order to ensure smoothness even when doing subpixel/subtexel rendering
	// (for example, when rotating/shearing/scaling the 2D world.)
	/*
	s_left += texture.halfTexelWide;
	s_right -= texture.halfTexelWide;
	t_top += texture.halfTexelHigh;
	t_bottom -= texture.halfTexelHigh;
	*/
	
	coordinates[0] = s_left;
	coordinates[1] = t_bottom;	
	coordinates[2] = s_right;
	coordinates[3] = t_bottom;	
	coordinates[4] = s_left;
	coordinates[5] = t_top;	
	coordinates[6] = s_right;
	coordinates[7] = t_top;
}

@end
