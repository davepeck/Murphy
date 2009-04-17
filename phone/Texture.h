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
	
	BOOL engaged;
	GLenum textureUnit;
}

+(id)textureWithFile:(NSString*)path;
+(id)textureWithResourcePNG:(NSString*)resourceName;

@property (readonly) BOOL engaged;
@property (readonly) GLenum textureUnit;

-(void)engage;
-(void)engageOnUnit:(GLenum)textureUnit;		/* GL_TEXTURE0 or GL_TEXTURE1 on the iPhone */
-(void)disengage;

@end
