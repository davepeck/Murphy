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

#define PIXEL_SCREEN_WIDTH 320.0f
#define PIXEL_SCREEN_HEIGHT 480.0f

#define SCREEN_LEFT 0.0f
#define SCREEN_RIGHT 320.0f
#define SCREEN_TOP 0.0f
#define SCREEN_BOTTOM 320.0f

#define VIEWPORT_LEFT -1.0f
#define VIEWPORT_RIGHT 1.0f
#define VIEWPORT_TOP 1.5f
#define VIEWPORT_BOTTOM -1.5f

#define LEVEL_START_X 80
#define LEVEL_START_Y 40

#define TILES_ACROSS 14
#define TILES_DOWN 20


@interface EAGLView (EAGLViewPrivate)

- (BOOL)createFramebuffer;
- (void)destroyFramebuffer;

@end

@interface EAGLView (EAGLViewSprite)

- (void)setupView;
- (GLfloat)linearMap:(GLfloat)value valueMin:(GLfloat)valueMin valueMax:(GLfloat)valueMax targetMin:(GLfloat)targetMin targetMax:(GLfloat)targetMax;
- (void)computeTextureCoordinatesForTileAtlasS:(int)s t:(int)t coordinates:(GLfloat*)coords;
- (void)computeTextureCoordinatesForLevelX:(int)x y:(int)y coordinates:(GLfloat*) coords;
- (void)computeSpriteCoordinatesForScreenGridX:(int)x y:(int)y coordinates:(GLfloat*)coords;

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
				
		level = [[MurphyLevel murphyLevelWithNamedResource:@"World"] retain];		
		
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

GLfloat textureCoordinates[8];
GLfloat spriteCoordinates[8];


-(void)computeTextureCoordinatesForTileAtlasS:(int)s t:(int)t coordinates:(GLfloat*)coords
{
	GLfloat sf = (float) s;
	GLfloat tf = (float) t;
	
	GLfloat s_left = sf * TILE_SIZE;
	GLfloat s_right = s_left + TILE_SIZE;
	GLfloat t_top = tf * TILE_SIZE;
	GLfloat t_bottom = t_top + TILE_SIZE;
	
	coords[0] = s_left;
	coords[1] = t_bottom;	
	coords[2] = s_right;
	coords[3] = t_bottom;	
	coords[4] = s_left;
	coords[5] = t_top;	
	coords[6] = s_right;
	coords[7] = t_top;
}

-(void)computeTextureCoordinatesForLevelX:(int)x y:(int)y coordinates:(GLfloat*)coords
{
	uint8_t s = [level getIconSForX:x Y:y];
	uint8_t t = [level getIconTForX:x Y:y];
	[self computeTextureCoordinatesForTileAtlasS:s t:t coordinates:coords];
}

-(GLfloat)linearMap:(GLfloat)value valueMin:(GLfloat)valueMin valueMax:(GLfloat)valueMax targetMin:(GLfloat)targetMin targetMax:(GLfloat)targetMax
{
	// if value == valueMin, return targetMin
	// if value == valueMax, return targetMax
	// if value == (valueMin + valueMax) / 2, return (targetMin + targetMax) / 2
	// if value == (valueMin + valueMax) * (1/3), return (targetMin + targetMax) * (1/3)
	// so, if value == (valueMin + valueMax) * X, return (targetMin + targetMax) * X
	// or: X = value / (valueMin + valueMax)
	// so: return value * ((targetMin + targetMax) / (valueMin + valueMax))
	
	return value * ((targetMin + targetMax) / (valueMin + valueMax));
}

-(void)computeSpriteCoordinatesForScreenGridX:(int)x y:(int)y coordinates:(GLfloat*)coords;
{
	// TODO handle offsets.
	
	// X=0, Y=0 is top-left (for now)
	// x max is (PIXEL_SCREEN_WIDTH / PIXEL_TILE_SIZE)
	// y max is (PIXEL_SCREEN_HEIGHT / PIXEL_TILE_SIZE)

	// first, figure out the pixel positions that you want on the screen
	GLfloat screen_left = x * PIXEL_TILE_SIZE;
	GLfloat screen_right = screen_left + PIXEL_TILE_SIZE;
	GLfloat screen_top = y * PIXEL_TILE_SIZE;
	GLfloat screen_bottom = screen_top + PIXEL_TILE_SIZE;
	
	// okay, now map that pixel position to OpenGL coordinates
	// how? 
	// Well, you have a X viewport from -1 to 1. And you have a screen viewport of 0 to 320.
	// So, map that shit -- linear relationship.
	GLfloat viewport_left = [self linearMap:screen_left valueMin:SCREEN_LEFT valueMax:SCREEN_RIGHT targetMin:VIEWPORT_LEFT targetMax:VIEWPORT_RIGHT];
	GLfloat viewport_right = [self linearMap:screen_right valueMin:SCREEN_LEFT valueMax:SCREEN_RIGHT targetMin:VIEWPORT_LEFT targetMax:VIEWPORT_RIGHT];
	GLfloat viewport_top = [self linearMap:screen_top valueMin:SCREEN_TOP valueMax:SCREEN_BOTTOM targetMin:VIEWPORT_TOP targetMax:VIEWPORT_BOTTOM];
	GLfloat viewport_bottom = [self linearMap:screen_bottom valueMin:SCREEN_TOP valueMax:SCREEN_BOTTOM targetMin:VIEWPORT_TOP targetMax:VIEWPORT_BOTTOM];
	
	coords[0] = viewport_left;
	coords[1] = viewport_bottom;	
	coords[2] = viewport_right;
	coords[3] = viewport_bottom;	
	coords[4] = viewport_left;
	coords[5] = viewport_top;	
	coords[6] = viewport_right;
	coords[7] = viewport_top;
}

- (void)setupView
{
	CGImageRef spriteImage;
	CGContextRef spriteContext;
	GLubyte *spriteData;
	size_t	width, height;
		
	// Compute our tile and texture coordinates, along with indicies into them.	
	tileCoordinates = malloc(sizeof(GLfloat) * 2240);			// there will be 14 * 20 tiles on screen; each has 4 coordinates, each of which is 2 entries (x, then y) in this array
	tileTextureCoordinates = malloc(sizeof(GLfloat) * 2240);	// again for the textures
	coordinateIndexes = malloc(sizeof(GLushort) * 1680);					// because we have to have disconnected geometry, we need 6 indicies for each of our coordinates
	
	// XXX TODO DAVEPECK
	// Are there ways to simplify the arrays above?
	//
	// For the tileCoordinates array, we can substantially reduce the number of coordinates by congealing coordinates shared by neighbors.
	// For the tileTextureCoordinates array, perhaps we can tie the coordinates to the number of textures rather than the number of tiles on-screen.
	//
	// If we're lucky, both arrays will then be 256 or fewer elements, which means our index array can be of bytes rather than shorts!
	
	GLushort coord_index = 0;
	GLushort index_index = 0;
	for (int y = 0; y < TILES_DOWN; y++)
	{
		for (int x = 0; x < TILES_ACROSS; x++)
		{
			[self computeSpriteCoordinatesForScreenGridX:x y:y coordinates:&tileCoordinates[coord_index]];
			[self computeTextureCoordinatesForLevelX:(x + LEVEL_START_X) y:(y + LEVEL_START_Y) coordinates:&tileTextureCoordinates[coord_index]];
			
			coordinateIndexes[index_index++] = coord_index;
			coordinateIndexes[index_index++] = coord_index + 2;
			coordinateIndexes[index_index++] = coord_index + 4;			
			coordinateIndexes[index_index++] = coord_index + 2;
			coordinateIndexes[index_index++] = coord_index + 4;
			coordinateIndexes[index_index++] = coord_index + 6;
			
			coord_index += 8;
			
			NSAssert(coord_index <= 2240 && index_index <= 1680, @"We stepped over our coordinate boundaries.");
		}
	}
	
		
	// Sets up matrices and transforms for OpenGL ES
	glViewport(0, 0, backingWidth, backingHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(-1.0f, 1.0f, -1.5f, 1.5f, -1.0f, 1.0f);
	//glOrthof(0.0f, 1.0f, 0.0f, 1.5f, -1.0f, 1.0f);
	
	glMatrixMode(GL_MODELVIEW);
		
	// DAVEPECK where does this belong in, really?
	//glTranslatef(0.0f, 0.25f, 0.0f);
		
	// Clears the view with black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	// Sets up pointers and enables states needed for using vertex arrays and textures
	glVertexPointer(2, GL_FLOAT, 0, tileCoordinates);
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, tileTextureCoordinates);
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
		
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 1680, GL_UNSIGNED_SHORT, (const GLvoid *) coordinateIndexes);
	
	// glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
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
