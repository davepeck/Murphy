//
//  EAGLView.m
//  iMurphy
//
//  Created by Dave Peck on 4/3/09.
//  Copyright Code Orange 2009. All rights reserved.
//



#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "EAGLView.h"

#define PIXEL_TILE_SIZE 24.0f
#define PIXEL_ATLAS_SIZE 512.0f
/* a tile's size in texture space; PIXEL_TILE_SIZE / PIXEL_ATLAS_SIZE */
#define TILE_SIZE (0.046875f)

@interface EAGLView (EAGLViewPrivate)

- (BOOL)createFramebuffer;
- (void)destroyFramebuffer;

@end

@interface EAGLView (EAGLViewSprite)

- (void)setupView;
- (void)computeTextureCoordinatesForX:(int)x y:(int)y coordinates:(GLfloat*)coords;


@end

@implementation EAGLView

@synthesize animationInterval;

// You must implement this
+ (Class) layerClass
{
	return [CAEAGLLayer class];
}


//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder
{
	if((self = [super initWithCoder:coder])) {
		// Get the layer
		CAEAGLLayer *eaglLayer = (CAEAGLLayer*) self.layer;
		
		eaglLayer.opaque = YES;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
										[NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
		
		if(!context || ![EAGLContext setCurrentContext:context] || ![self createFramebuffer]) {
			[self release];
			return nil;
		}
		
		animationInterval = 1.0 / 60.0;
		
		[self setupView];
		[self drawView];
	}
	
	return self;
}


- (void)layoutSubviews
{
	[EAGLContext setCurrentContext:context];
	[self destroyFramebuffer];
	[self createFramebuffer];
	[self drawView];
}


- (BOOL)createFramebuffer
{
	glGenFramebuffersOES(1, &viewFramebuffer);
	glGenRenderbuffersOES(1, &viewRenderbuffer);
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	[context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(id<EAGLDrawable>)self.layer];
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, viewRenderbuffer);
	
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
	
	if(glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
		NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
		return NO;
	}
	
	return YES;
}


- (void)destroyFramebuffer
{
	glDeleteFramebuffersOES(1, &viewFramebuffer);
	viewFramebuffer = 0;
	glDeleteRenderbuffersOES(1, &viewRenderbuffer);
	viewRenderbuffer = 0;
	
	if(depthRenderbuffer) {
		glDeleteRenderbuffersOES(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
	}
}


- (void)startAnimation
{
	animationTimer = [NSTimer scheduledTimerWithTimeInterval:animationInterval target:self selector:@selector(drawView) userInfo:nil repeats:YES];
}


- (void)stopAnimation
{
	[animationTimer invalidate];
	animationTimer = nil;
}


- (void)setAnimationInterval:(NSTimeInterval)interval
{
	animationInterval = interval;
	
	if(animationTimer) {
		[self stopAnimation];
		[self startAnimation];
	}
}


// Sets up an array of values to use as the sprite vertices.
const GLfloat spriteVertices[] = {
0.0f, 0.0f,
1.0f, 0.0f,
0.0f, 1.0f,
1.0f, 1.0f,

/*
-1.0f, -1.0f,
1.0f, -1.0f,
-1.0f, 1.0f,
1.0f, 1.0f,
*/

/*
-0.5f, -0.5f,
0.5f, -0.5f,
-0.5f,  0.5f,
0.5f,  0.5f,
 */
};

/*
// Sets up an array of values for the texture coordinates.
const GLfloat spriteTexcoords[] = {
0,TILE_SIZE,
TILE_SIZE,TILE_SIZE,
0,0,
TILE_SIZE,0,

/* 0, 1,
 1, 1,
 0, 0,
 1, 0 */

/*
0, 0,
1, 0,
0, 1, 
1, 1,
};
*/

GLfloat textureCoordinates[8];


-(void)computeTextureCoordinatesForX:(int)x y:(int)y coordinates:(GLfloat*)coords
{
	GLfloat xf = (float) x;
	GLfloat yf = (float) y;
	
	GLfloat x_left = xf * TILE_SIZE;
	GLfloat x_right = x_left + TILE_SIZE;
	GLfloat y_top = yf * TILE_SIZE;
	GLfloat y_bottom = y_top + TILE_SIZE;
	
	coords[0] = x_left;
	coords[1] = y_bottom;	
	coords[2] = x_right;
	coords[3] = y_bottom;	
	coords[4] = x_left;
	coords[5] = y_top;	
	coords[6] = x_right;
	coords[7] = y_top;
}

- (void)setupView
{
	CGImageRef spriteImage;
	CGContextRef spriteContext;
	GLubyte *spriteData;
	size_t	width, height;
	
		
	// Sets up matrices and transforms for OpenGL ES
	glViewport(0, 0, backingWidth, backingHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrthof(-1.0f, 1.0f, -1.5f, 1.5f, -1.0f, 1.0f);
	glOrthof(0.0f, 1.0f, 0.0f, 1.5f, -1.0f, 1.0f);
	
	glMatrixMode(GL_MODELVIEW);
	
	
	// DAVEPECK where does this belong in, really?
	glTranslatef(0.0f, 0.25f, 0.0f);
	
	
	// Clears the view with black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	// Sets up pointers and enables states needed for using vertex arrays and textures
	glVertexPointer(2, GL_FLOAT, 0, spriteVertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	[self computeTextureCoordinatesForX:1 y:1 coordinates:&textureCoordinates[0]];	
	glTexCoordPointer(2, GL_FLOAT, 0, textureCoordinates);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	// Creates a Core Graphics image from an image file
	spriteImage = [UIImage imageNamed:@"example-world-tiles-24-512.png"].CGImage;
	// Get the width and height of the image
	width = CGImageGetWidth(spriteImage);
	height = CGImageGetHeight(spriteImage);
	// Texture dimensions must be a power of 2. If you write an application that allows users to supply an image,
	// you'll want to add code that checks the dimensions and takes appropriate action if they are not a power of 2.
	
	if(spriteImage) {
		// Allocated memory needed for the bitmap context
		spriteData = (GLubyte *) malloc(width * height * 4);
		// Uses the bitmatp creation function provided by the Core Graphics framework. 
		spriteContext = CGBitmapContextCreate(spriteData, width, height, 8, width * 4, CGImageGetColorSpace(spriteImage), kCGImageAlphaPremultipliedLast);
		// After you create the context, you can draw the sprite image to the context.
		CGContextDrawImage(spriteContext, CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height), spriteImage);
		// You don't need the context at this point, so you need to release it to avoid memory leaks.
		CGContextRelease(spriteContext);
		
		// Use OpenGL ES to generate a name for the texture.
		glGenTextures(1, &spriteTexture);
		// Bind the texture name. 
		glBindTexture(GL_TEXTURE_2D, spriteTexture);
		// Speidfy a 2D texture image, provideing the a pointer to the image data in memory
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, spriteData);
		// Release the image data
		free(spriteData);
		
		// Use texture parameters that make sense for THE BAD ASS SHIT I AM DOING
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		
		// Enable use of the texture
		glEnable(GL_TEXTURE_2D);
		
		/*
		// Set a blending function to use
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		// Enable blending
		glEnable(GL_BLEND);
		*/
	}
}

// Updates the OpenGL view when the timer fires
- (void)drawView
{
	// Make sure that you are drawing to the current context
	[EAGLContext setCurrentContext:context];
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, viewFramebuffer);
		
	[self computeTextureCoordinatesForX:(rand() % 12) + 1 y:(rand() % 16) coordinates:&textureCoordinates[0]];	
	glTexCoordPointer(2, GL_FLOAT, 0, textureCoordinates);
	
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

// Stop animating and release resources when they are no longer needed.
- (void)dealloc
{
	[self stopAnimation];
	
	if([EAGLContext currentContext] == context) {
		[EAGLContext setCurrentContext:nil];
	}
	
	[context release];
	context = nil;
	
	[super dealloc];
}

@end
