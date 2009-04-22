//
//  Texture.h
//  iMurphy
//
//  Represents a single OpenGL texture loaded from a graphics (probably PNG) file.
//  Provides hooks to engage the texture (place it into an OpenGL texture unit)
//  Automatically handles the fact that OpenGL textures must be power-of-two sized.
//
//  Created by Dave Peck on 4/16/09.
//  Copyright 2009 Code Orange. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>


@interface Texture : NSObject 
{
	GLuint name;
	
	uint16_t originalWidth;
	uint16_t originalHeight;
	
	uint16_t width;			// will always be a power of 2
	uint16_t height;		// will always be a power of 2
	
	GLfloat halfTexelWide;
	GLfloat halfTexelHigh;
	
	BOOL engaged;
}

+(id)textureWithFile:(NSString*)path;
+(id)textureWithResourcePNG:(NSString*)resourceName;

@property (readonly) BOOL engaged;
@property (readonly) uint16_t width;
@property (readonly) uint16_t height;
@property (readonly) GLfloat halfTexelWide;
@property (readonly) GLfloat halfTexelHigh;

-(void)engage;
-(void)disengage;

@end
