//
//  MurphyLevel.m
//  iMurphy
//
//  Created by Dave Peck on 4/15/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "MurphyLevel.h"
#import "OrangeDataScanner.h"

@interface MurphyLevel (Private)

-(id)initWithData:(NSData*)data;
-(uint32_t)iconIndexForX:(uint16_t)x Y:(uint16_t)y;
-(void)dealloc;

@end


@implementation MurphyLevel (Private)

-(id)initWithData:(NSData*)data
{
	self = [super init];
	if (self != nil)
	{
		OrangeDataScanner *scanner = [OrangeDataScanner orangeDataScannerWithData:data littleEndian:NO defaultEncoding:NSASCIIStringEncoding];
		
		name = [[scanner readNullTerminatedString] retain];
		graphicsSet = [[scanner readNullTerminatedString] retain];
		infotrons = [scanner readWord];
		width = [scanner readWord];
		height = [scanner readWord];
		
		uint32_t array_size = width * height;
		iconS = malloc(sizeof(uint8_t) * array_size);
		iconT = malloc(sizeof(uint8_t) * array_size);

		uint8_t *current_s = iconS;
		uint8_t *current_t = iconT;
		
		for (uint16_t y = 0; y < height; y++)
		{
			for (uint16_t x = 0; x < width; x++)
			{
				*current_s = [scanner readByte]; current_s++;
				*current_t = [scanner readByte]; current_t++;
			}
		}
	}
	return self;
}

-(uint32_t)iconIndexForX:(uint16_t)x Y:(uint16_t)y
{
	return ((y*width) + x);
}

-(void)dealloc
{
	[name release];
	name = nil;
	
	free(iconS);
	iconS = nil;
	
	free(iconT);
	iconT = nil;
	
	[super dealloc];
}

@end


@implementation MurphyLevel

@synthesize name;
@synthesize graphicsSet;
@synthesize infotrons;
@synthesize width;
@synthesize height;

+(id)murphyLevelWithData:(NSData*)data
{
	return [[[MurphyLevel alloc] initWithData:data] autorelease];
}

+(id)murphyLevelWithFile:(NSString*)path
{
	return [MurphyLevel murphyLevelWithData:[NSData dataWithContentsOfFile:path]];
}

+(id)murphyLevelWithNamedResource:(NSString*)name
{
	return [MurphyLevel murphyLevelWithFile:[[NSBundle mainBundle] pathForResource:name ofType:@"mbl"]];
}

-(void) getIconForX:(uint16_t)x y:(uint16_t)y s:(uint8_t *)s t:(uint8_t *)t
{
	uint32_t index = [self iconIndexForX:x Y:y];
	*s = iconS[index];
	*t = iconT[index];
}

@end
