//
//  Texture.m
//  iMurphy
//
//  Created by Dave Peck on 4/16/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import "TMTexture.h"

// statics
static TMTexture *currently_engaged = NULL;

@interface TMTexture (Private)

-(id)initWithFile:(NSString*)path;
-(void)dealloc;

@end


@implementation TMTexture (private)

-(id)initWithFile:(NSString*)path
{
	self = [super init];
	
	if (self != nil)
	{		
		// Grab the CoreGraphics image
		CGImageRef textureImage = [UIImage imageWithContentsOfFile:path].CGImage;
		NSAssert(textureImage != NULL, @"TODO handle this error");
		
		// How big is this thing?
		size_t actualWidth = CGImageGetWidth(textureImage);
		size_t actualHeight = CGImageGetHeight(textureImage);	
		NSAssert(actualWidth <= 1024 && actualHeight <= 1024, @"TODO handle this error");
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
		
		// Disengage the previous texture, if any
		if (currently_engaged != NULL)
		{
			[currently_engaged disengage];
		}
		
		// Create a bitmap from the image...	
		GLubyte *textureData = (GLubyte *) malloc(width * height * 4);	
		CGContextRef textureContext = CGBitmapContextCreate(textureData, width, height, 8, width * 4, CGImageGetColorSpace(textureImage), kCGImageAlphaPremultipliedLast);
		CGContextDrawImage(textureContext, CGRectMake(0.0, 0.0, (CGFloat)originalWidth, (CGFloat)originalHeight), textureImage);
		CGContextRelease(textureContext);
		
		// Create a GL texture from the bitmap, and engage
		glGenTextures(1, &name);
		glBindTexture(GL_TEXTURE_2D, name);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
		free(textureData);
		
		// Choose our default texture parameters: pixelate to high hell if we need to
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				
		glEnable(GL_TEXTURE_2D);
		
		// a lot of times you want to inset your texture coordinates by this amount
		// in order to avoid edge artifacts when doing subpixel rendering.
		halfTexelWide = 1.0 / (2.0f * ((GLfloat) width));
		halfTexelHigh = 1.0 / (2.0f * ((GLfloat) height));
		
		// Finish engaging this texture		
		currently_engaged = [self retain];
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

@implementation TMTexture

+(id)textureWithFile:(NSString*)path
{
	return [[[TMTexture alloc] initWithFile:path] autorelease];
}

+(id)textureWithResourcePNG:(NSString*)resourceName
{	
	return [TMTexture textureWithFile:[[NSBundle mainBundle] pathForResource:resourceName ofType:@"png"]];
}

@synthesize engaged;
@synthesize width;
@synthesize height;
@synthesize halfTexelWide;
@synthesize halfTexelHigh;

-(void)engage
{
	if (engaged) { return; }
	
	if (currently_engaged != NULL)
	{
		[currently_engaged disengage];
	}
	
	currently_engaged = [self retain];
	engaged = YES;
	glBindTexture(GL_TEXTURE_2D, name);	
	glEnable(GL_TEXTURE_2D);
}

-(void)disengage
{
	if (engaged)
	{
		NSAssert(currently_engaged == self, @"Texture engagement got out of wack.");
		[currently_engaged release];
		currently_engaged = NULL;
		engaged = NO;
	}
}

@end
