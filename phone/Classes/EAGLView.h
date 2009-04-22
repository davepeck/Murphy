//
//  EAGLView.h
//  iMurphy
//
//  Created by Dave Peck on 4/3/09.
//  Copyright Code Orange 2009. All rights reserved.
//


#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import "TileGrid.h"

/*
This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
The view content is basically an EAGL surface you render your OpenGL scene into.
Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
*/
@interface EAGLView : UIView
{
@private
	
	/* The pixel dimensions of the backbuffer */
	GLint backingWidth;
	GLint backingHeight;
	
	EAGLContext *context;
	
	/* OpenGL names for the renderbuffer and framebuffers used to render to this view */
	GLuint viewRenderbuffer, viewFramebuffer;
	
	/* OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist) */
	GLuint depthRenderbuffer;
	
	/* OpenGL name for the sprite texture */
	GLuint spriteTexture;
	
	NSTimer *animationTimer;
	NSTimeInterval animationInterval;	
	
	TileGrid *tileGrid;
	
	GLfloat currentViewportLeft;
	GLfloat currentViewportTop;
	
	GLfloat xMotion;
	GLfloat yMotion;
	
	NSDate *dateInMotion;
}

- (void)startAnimation;
- (void)stopAnimation;
- (void)drawView;

@property NSTimeInterval animationInterval;

@end
