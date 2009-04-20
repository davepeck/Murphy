//
//  Texture.m
//  iMurphy
//
//  Created by Dave Peck on 4/16/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "Texture.h"

// statics
static Texture *currently_engaged = NULL;

@interface Texture (Private)

-(id)initWithFile:(NSString*)path;
-(void)dealloc;

@end


@implementation Texture (private)

-(id)initWithFile:(NSString*)path
{
	self = [super init];
	
	if (self != nil)
	{		
		// Grab the CoreGraphics image
		CGImageRef textureImage = [UIImage imageWithContentsOfFile:path].CGImage;
		NSAssert(textureImage != NULL, "TODO handle this error");
		
		// How big is this thing?
		size_t actualWidth = CGImageGetWidth(textureImage);
		size_t actualHeight = CGImageGetHeight(textureImage);	
		NSAssert(actualWidth <= 1024 && actualHeight <= 1024, "TODO handle this error");
		originalWidth = (uint16_t) actualWidth;
		originalHeight = (uint16_t) actualHeight;
		
		// ensure that our final width/height are powers of two.
		width = originalWidth;
		height = originalHeight;
		
		if ((width != 1) && (width & (width - 1)))
		{
			uint16_t i = 1;
			while (i < width) { i *= 2; }
			width = i;
		}
		
		if ((height != 1) && (height & (height - 1)))
		{
			uint16_t i = 1;
			while (i < height) { i *= 2; }
			height = i;
		}
		
		// Disingage the previous texture, if any
		if (currently_engaged != NULL)
		{
			[currently_engaged disengage];
		}
		
		// Create a bitmap from the image...	
		GLubyte *textureData = (GLubyte *) malloc(width * height * 4);	
		CGContextRef textureContext = CGBitmapContextCreate(textureData, width, height, 8, width * 4, CGImageGetColorSpace(textureImage), kCGImageAlphaPremultipliedLast);
		CGContextDrawImage(textureContext, CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height), textureImage);
		CGContextRelease(textureContext);
		
		// Create a GL texture from the bitmap, and engage
		glGenTextures(1, &name);
		glBindTexture(GL_TEXTURE_2D, name);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
		free(textureData);
		
		currently_engaged = self;
		engaged = YES;		
	}
	
	return self;
}

-(void)dealloc
{
	glDeleteTextures(1, &name);
	[super dealloc];
}

@end

@implementation Texture

+(id)textureWithFile:(NSString*)path
{
	return [[[Texture alloc] initWithFile:path] autorelease];
}

+(id)textureWithResourcePNG:(NSString*)resourceName
{	
	return [Texture textureWithFile:[[NSBundle mainBundle] pathForResource:name ofType:@"png"]];
}

@synthesize engaged;

-(void)engage
{
	if (currently_engaged != NULL)
	{
		[currently_engaged disengage];
	}
	
	currently_engaged = self;
	engaged = YES;
	glBindTexture(GL_TEXTURE_2D, name);	
}

-(void)disengage
{
	if (engaged)
	{
		NSAssert(currently_engaged == self, @"Texture engagement got out of wack.");
		currently_engaged = NULL;
		engaged = NO;
	}
}

@end
