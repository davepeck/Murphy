//
//  TMTileGrid.m
//  iMurphy
//
//  Created by Dave Peck on 4/16/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "TMTileGrid.h"

@interface TMTileGrid (Private)

-(id)initWithMap:(TMTileMap *)map width:(uint16_t)tilesWide height:(uint16_t)tilesHigh;
-(void)dealloc;

@end


@implementation TMTileGrid (Private)

-(id)initWithMap:(TMTileMap *)myMap width:(uint16_t)tilesWide height:(uint16_t)tilesHigh
{
	self = [super init];
	
	if (self != nil)
	{
		map = [myMap retain];
		width = tilesWide;
		height = tilesHigh;
		grid = malloc(sizeof(uint16_t) * width * height);
		bzero(grid, sizeof(uint16_t) * width * height);
		
		// TODO move this stuff
		gridLeft = 0.0;
		gridTop = 0.0;
		pixelWidth = 0.0;
		pixelHeight = 0.0;
		tileCoordinates = nil;
		tileTextureCoordinates = nil;
		tileIndexes = nil;
		numCoordinates = 0;
		numIndexes = 0;
	}
	
	return self;
}

-(void)dealloc
{
	if (tileCoordinates != nil)
	{
		free(tileCoordinates);
	}
	
	if (tileTextureCoordinates != nil)
	{
		free(tileTextureCoordinates);
	}
	
	if (tileIndexes != nil)
	{
		free(tileIndexes);
	}
	
	numCoordinates = 0;
	numIndexes = 0;
	
	free(grid);
	grid = nil;
	
	if (map != nil)
	{
		[map stopAllAnimations];
		[map release];
		map = nil;
	}
	
	[super dealloc];
}

@end


@implementation TMTileGrid

+(id)tileGridWithMap:(TMTileMap *)myMap width:(uint16_t)tilesWide height:(uint16_t)tilesHigh
{
	return [[[TMTileGrid alloc] initWithMap:myMap width:tilesWide height:tilesHigh] autorelease];
}

@synthesize map;
@synthesize width;
@synthesize height;
@synthesize gridLeft;
@synthesize gridTop;
@synthesize gridRight;
@synthesize gridBottom;

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
-(void)setGridLeft:(GLfloat)myGridLeft top:(GLfloat)myGridTop
{
	gridLeft = myGridLeft;
	gridTop = myGridTop;
	
	// compute the left, top, right, bottom
	// TODO this sucks
	GLfloat tilePixelWidth = (GLfloat) map.atlas.tilePixelWidth;
	GLfloat tilePixelHeight = (GLfloat) map.atlas.tilePixelHeight;
	GLfloat glTileWidth = ((1.0f) * tilePixelWidth) / pixelWidth;
	GLfloat glTileHeight = ((1.5f) * tilePixelHeight) / pixelHeight;		
	gridRight = gridLeft + (width * glTileWidth);
	gridBottom = gridTop - (height * glTileHeight);	
}

-(void)setPixelWidth:(GLfloat)myPixelWidth height:(GLfloat)myPixelHeight
{
	pixelWidth = myPixelWidth;
	pixelHeight = myPixelHeight;
}

-(void)alignViewportToPixel:(GLfloat *)left top:(GLfloat *)top
{
	// General suckiness.
	GLfloat offsetLeft = *left - gridLeft;
	GLfloat offsetTop = gridTop - *top;
	
	GLfloat glPixelWidth = 1.0f / pixelWidth;
	GLfloat glPixelHeight = 1.5f / pixelHeight;
	
	offsetLeft -= fmodf(offsetLeft, glPixelWidth);
	offsetTop -= fmodf(offsetTop, glPixelHeight);
	
	if (offsetLeft < 0.0f)
	{
		offsetLeft = 0.0f;
	}
	
	if (offsetTop < 0.0f)
	{
		offsetTop = 0.0f;
	}
	
	*left = gridLeft + offsetLeft;
	*top = gridTop - offsetTop;
}

-(void)drawInViewportLeft:(GLfloat)left top:(GLfloat)top right:(GLfloat)right bottom:(GLfloat)bottom
{
	// Sanity checks
	// need epislon here NSAssert((pixelWidth / pixelHeight) == (right - left) / (top - bottom), @"This drawing will not preserve the tile aspect ratio.");
	
	// What tiles does this viewport intersect with?
		
	// Well, first, what is the natural width and height of a tile in GL-space?
	GLfloat tilePixelWidth = (GLfloat) map.atlas.tilePixelWidth;
	GLfloat tilePixelHeight = (GLfloat) map.atlas.tilePixelHeight;
	// PERFORMANCE: compute at least some of this earlier, perhaps when pixelWidth/pixelHeight are set. 
	GLfloat glTileWidth = ((right - left) * tilePixelWidth) / pixelWidth;
	GLfloat glTileHeight = ((top - bottom) * tilePixelHeight) / pixelHeight;
	
	uint16_t tilesAcrossScreen = (uint16_t) ceilf(pixelWidth / tilePixelWidth);
	uint16_t tilesDownScreen = (uint16_t) ceilf(pixelHeight / tilePixelHeight);
	
	// Okay, now we know the gl-space size of a tile. But what portion of our grid is intersected?	
	int16_t visibleLeft = floorf((left - gridLeft) / glTileWidth);
	int16_t visibleTop = floorf((gridTop - top) / glTileHeight);
	int16_t visibleRight = visibleLeft + tilesAcrossScreen;
	int16_t visibleBottom = visibleTop + tilesDownScreen;
	
	// In order to account for partial intersection, extend our "visible" grid in all directions
	visibleLeft -= 1;
	visibleTop -= 1;
	visibleRight += 1;
	visibleBottom += 1;
	
	// make sure we're displaying in-bounds
	if (visibleLeft < 0) 
	{
		visibleLeft = 0;
	}
	
	if (visibleRight > width)
	{
		visibleRight = width;
	}
	
	if (visibleTop < 0)
	{
		visibleTop = 0;
	}
	
	if (visibleBottom > height)
	{
		visibleBottom = height;
	}
	
	// Compute values for the viewPort's left and top that are snapped to the appropriate tile boundary
	GLfloat snappedLeft = gridLeft + (visibleLeft * glTileWidth);
	GLfloat snappedTop = gridTop - (visibleTop * glTileHeight);	
	
	// how big must our arrays be?
	// Vertex & Texture: We need 4 points per tile, 2 entries per point = tilesAcrossScreen * tilesDownScreen * 4 * 2
	// How many indicies? Well, we want two triangles per tile, or 6 indicies = tilesAcrossScreen * tilesDownScreen * 6
	uint16_t currentTilesAcrossScreen = visibleRight - visibleLeft;
	uint16_t currentTilesDownScreen = visibleBottom - visibleTop;	
	
	GLushort nextNumCoordinates = currentTilesDownScreen * currentTilesAcrossScreen * 8;
	GLushort nextNumIndexes = currentTilesDownScreen * currentTilesAcrossScreen * 6;
	
	// If the number of tiles changed since last time, free up our memory and allocate some new memory
	// PERFORMANCE: should the number really change?
	if (tileCoordinates == nil || nextNumCoordinates != numCoordinates)
	{
		if (tileCoordinates != nil)
		{
			free(tileCoordinates); 
		}
		
		if (tileTextureCoordinates != nil)
		{
			free(tileTextureCoordinates);
		}
		
		if (tileIndexes != nil)
		{
			free(tileIndexes);
		}
		
		numCoordinates = nextNumCoordinates;
		numIndexes = nextNumIndexes;

		tileCoordinates = malloc(sizeof(GLfloat) * numCoordinates);
		tileTextureCoordinates = malloc(sizeof(GLfloat) * numCoordinates);
		tileIndexes = malloc(sizeof(GLushort) * numIndexes);		
	}

	// Now go ahead and compute all the coordinates for the damned thing
	GLushort coord_index = 0;
	GLushort index_index = 0;
	GLushort half_coord_index = 0;
	GLfloat vertexLeft = 0.0;
	GLfloat vertexTop = 0.0;
	GLfloat vertexRight = 0.0;
	GLfloat vertexBottom = 0.0;
		
	// PERFORMANCE: okay, look, we're doing a ton of unnecessary computations here. Fix that.	
	for (uint16_t y = visibleTop; y < visibleBottom; y++)
	{
		vertexTop = snappedTop - ((y - visibleTop) * glTileHeight);
		vertexBottom = vertexTop - glTileHeight; // snappedTop - (((y + 1) - visibleTop) * glTileHeight);
		
		for (uint16_t x = visibleLeft; x < visibleRight; x++)
		{
			vertexLeft = snappedLeft + ((x - visibleLeft) * glTileWidth);
			vertexRight = vertexLeft + glTileWidth; // snappedLeft + (((x + 1) - visibleLeft) * glTileWidth);
			
			tileCoordinates[coord_index]   = vertexLeft;
			tileCoordinates[coord_index+1] = vertexBottom;
			tileCoordinates[coord_index+2] = vertexRight;
			tileCoordinates[coord_index+3] = vertexBottom;
			tileCoordinates[coord_index+4] = vertexLeft;
			tileCoordinates[coord_index+5] = vertexTop;
			tileCoordinates[coord_index+6] = vertexRight;
			tileCoordinates[coord_index+7] = vertexTop;
			
			uint16_t tileId = [self getTileIdAtX:x y:y];
			[map textureCoordinatesForTileId:tileId coordinates:&tileTextureCoordinates[coord_index]];
						
			tileIndexes[index_index++] = half_coord_index;
			tileIndexes[index_index++] = half_coord_index + 1;
			tileIndexes[index_index++] = half_coord_index + 2;			
			tileIndexes[index_index++] = half_coord_index + 1;
			tileIndexes[index_index++] = half_coord_index + 2;
			tileIndexes[index_index++] = half_coord_index + 3;
			
			coord_index += 8;
			half_coord_index += 4;
			
			NSAssert((coord_index <= numCoordinates) && (index_index <= numIndexes), @"We stepped over our coordinate boundaries.");
		}
	}
	
	NSAssert((coord_index == numCoordinates) && (index_index == numIndexes), @"We didn't reach our coordinate boundaries.");	
	[map.atlas.texture engage];
		
	// Draw the damned thing	
	glVertexPointer(2, GL_FLOAT, 0, tileCoordinates);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tileTextureCoordinates);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);	
	glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_SHORT, (const GLvoid *) tileIndexes);
}
@end
