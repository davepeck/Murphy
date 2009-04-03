/*
	BlitKernel - GL functions

	Copyright (C) 2005 Anders F Bjoerklund <afb@users.sourceforge.net>
	
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any
	damages arising from the use of this software.
	
	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:
	
	1. The origin of this software must not be misrepresented; you must
	   not claim that you wrote the original software. If you use this
	   software in a product, an acknowledgment in the product
	   documentation would be appreciated but is not required.
	
	2. Altered source versions must be plainly marked as such, and must
	   not be misrepresented as being the original software.
	
	3. This notice may not be removed or altered from any source
	   distribution.
*/


#include <string.h>
#include <stdlib.h>

#include "BlitKernel.h"

#ifndef BK_ASSERT
#include <assert.h>
#define BK_ASSERT(c)	assert(c)
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define BK_WEAKLINK_OPENGL		0	// Mac OS X always has OpenGL
#elif !defined(BK_WEAKLINK_OPENGL)
#define BK_WEAKLINK_OPENGL		1	// link dynamically to OpenGL ?
#endif

#ifdef HAVE_OPENGL

static SDL_bool					GL_functions_loaded = SDL_FALSE;
static struct BK_GL_func		GL_functions;

static SDL_bool					GL_capabilities_loaded = SDL_FALSE;
static struct BK_GL_caps		GL_capabilities;


#define BK_DEFAULT_TEXTURE_SIZE	256 // conservative

#define BK_MAX_TEXTURE_SIZE		512

struct BK_GL_Surface
{
	SDL_Surface		*textureSurface;
	SDL_Rect		textureRect;
	SDL_bool		sharesSurface;
	BK_GL_Surface	*sharedParent;
	int				sharedChildren;
	
	SDL_bool		needsBlend;
	SDL_bool		useArrays;
	
	// texture size in pixels (per texture)
	int 			textureWidth;	
	int 			textureHeight;

	// texture format
	GLenum			texExtension;
	GLenum			texInternal;
	GLenum			texFormat;
	GLenum			texPixel;
	
	// number of textures (for the entire surface)
	int 			textureRows;	
	int 			textureCols;

	int				numTextures;		// rows x cols

	GLuint			*textureIDs;		// texture id
	GLshort			*pixelCoords;		// vertex coordinates (4x)
	GLfloat			*textureCoords;		// texture coordinates (4x)
};

/// rounds up to a power of two
static unsigned int power2(unsigned int x)
{
	int i = 1;
	while ( i < x )
		i *= 2;
	return i;
}

//---------------------------------------------------------------------------------------
//	BKCreateNewGLSurface
//---------------------------------------------------------------------------------------

/// Creates a new GL surface, from an existing SDL surface
BK_GL_Surface *BKCreateNewGLSurface(SDL_Surface *surface, SDL_Rect *rect)
{
	BK_GL_Surface *glsurface = NULL;
	SDL_Surface *textureSurface;
	SDL_Rect textureRect;
	struct BK_GL_func *gl;
	struct BK_GL_caps *caps;
	SDL_Rect 	surfaceRect;
	SDL_Color 	colorkey;
	Uint32		key;
	SDL_bool	hasAlpha,hasMask;
	SDL_bool	usePackedPixels;
	SDL_bool	useClientStorage;
	SDL_bool	useArrays;
	GLenum		texInternal;
	GLenum		texFormat;
	GLenum		texPixel;
	int			width,height,depth;
	int			rows,cols;
	int			i, x,y;
	int			l, t, r, b;
	int			numTex;
	GLuint		*texList;
	GLshort		*vertexList;
	GLfloat		*coordList;
	GLenum		glerr;
	float		u,v;
	
	BK_ASSERT(surface != NULL);
	
	surfaceRect.x = 0;
	surfaceRect.y = 0;
	surfaceRect.w = surface->w;
	surfaceRect.h = surface->h;

	gl = BKLoadGLfunctions();
	if (gl == NULL)
	{
		SDL_SetError("Failed to load GL functions");
		return NULL;
	}

	caps = BKLoadGLcapabilities();
	if (caps == NULL)
	{
		SDL_SetError("Failed to load GL capabilities");
		return NULL;
	}

		// bail early, if no memory
	glsurface = calloc(1, sizeof(BK_GL_Surface));
	if (glsurface == NULL)
	{
		SDL_SetError("Out of memory");
		return NULL;
	}
		
		// use whole surface, if no partial rect given
	if (rect == NULL)
		rect = &surfaceRect;
	
		// calculate texture width (pixels)
	width = rect->w;
	if ( caps->power2 )
		width = power2(width);
	if (width > caps->maxtexture)
		width = caps->maxtexture;

		// calculate texture height (pixels)
	height = rect->h;
	if ( caps->power2 )
		height = power2(height);
	if (height > caps->maxtexture)
		height = caps->maxtexture;
	
	depth = surface->format->BitsPerPixel;
	
		// calculate number of textures needed
	cols = (rect->w + width - 1) / width;
	rows = (rect->h + height - 1) / height;
	numTex = rows * cols;

	texList = (GLuint *) calloc(numTex, sizeof(GLuint));
	if (texList == NULL)
	{
		SDL_SetError("Out of memory");
		return NULL;
	}

	vertexList = (GLshort *) calloc(4*numTex, sizeof(GLshort));
	if (vertexList == NULL)
	{
		SDL_SetError("Out of memory");
		return NULL;
	}

	coordList = (GLfloat *) calloc(4*numTex, sizeof(GLfloat));
	if (coordList == NULL)
	{
		SDL_SetError("Out of memory");
		return NULL;
	}
	
	hasAlpha = (surface->flags & SDL_SRCALPHA) ? SDL_TRUE : SDL_FALSE;
	hasMask = (surface->flags & SDL_SRCCOLORKEY) ? SDL_TRUE : SDL_FALSE;

//	printf("%dx%d%s%s\n", width, height, hasAlpha ? " alpha" : "", hasMask ? " mask" : "");

	#if 0
		// packed pixels
	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		if (caps->hasPackedPixels && gl->hasReverseFormats && (depth == 16 || depth == 32))
		{
			usePackedPixels = SDL_TRUE;

			texFormat = (hasAlpha || hasMask) ? GL_BGRA : GL_BGR;

			if (depth == 16)
				texPixel = gotAlpha ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_SHORT_1_5_5_5_REV;
			else if (depth == 32)
				texPixel = GL_UNSIGNED_INT_8_8_8_8_REV;

			texInternal = (hasAlpha || hasMask) ? GL_RGBA : GL_RGB;
		}
	#else // SDL_BYTEORDER == SDL_LIL_ENDIAN
		if (caps->hasPackedPixels && (depth == 16 || depth == 32))
		{
			usePackedPixels = SDL_TRUE;

			texFormat = (hasAlpha || hasMask) ? GL_RGBA : GL_RGB;

			if (depth == 16)
				texPixel = gotAlpha ? GL_UNSIGNED_SHORT_4_4_4_4 : GL_UNSIGNED_SHORT_1_5_5_5;
			else if (depth == 32)
				texPixel = GL_UNSIGNED_INT_8_8_8_8;

			texInternal = (hasAlpha || hasMask) ? GL_RGBA : GL_RGB;
		}
	#endif // SDL_BYTEORDER
		else
	#endif
		// no packed pixels / reverse formats, use textbook
		{
			usePackedPixels = SDL_FALSE;
	
			texFormat = (hasAlpha || hasMask) ? GL_RGBA : GL_RGB;

			depth = (hasAlpha || hasMask) ? 32 : 24;
			
			texPixel = GL_UNSIGNED_BYTE;

			if (depth == 16)
				texInternal = hasAlpha ? GL_RGBA4 : (hasMask ? GL_RGB5_A1 : GL_RGB5);
			else if (depth == 32)
		        texInternal = hasAlpha ? GL_RGBA8 : (hasMask ? GL_RGBA8   : GL_RGB8);
			else
				texInternal = hasAlpha ? GL_RGBA  : (hasMask ? GL_RGBA    : GL_RGB);
		}

		// create one big SDL surface to cover all textures
	textureSurface = BKCreateGLSurface(
			cols * width, rows * height, depth,
			SDL_FALSE, hasMask, hasAlpha);
	if (textureSurface == NULL)
	{
		free(glsurface);
		free(texList);
		free(vertexList);
		free(coordList);
		return NULL;
	}

/*
	printf("r %08X %d\ng %08X %d\nb %08X %d\na %08X %d\n",
		textureSurface->format->Rmask, textureSurface->format->Rshift,
		textureSurface->format->Gmask, textureSurface->format->Gshift,
		textureSurface->format->Bmask, textureSurface->format->Bshift,
		textureSurface->format->Amask, textureSurface->format->Ashift);
*/
	
	textureRect.x = 0;
	textureRect.y = 0;
	textureRect.w = cols * width;
	textureRect.h = rows * height;
	
	if (hasMask) // clear the surface to colorkey
	{
		/*
		 DAVEPECK_CHANGED
		 
		SDL_GetRGBA( surface->format->colorkey, surface->format,
				&colorkey.r, &colorkey.g, &colorkey.b, &colorkey.unused );
		*/
		colorkey.r = 0;
		colorkey.g = 0;
		colorkey.b = 0;
		
		// END DAVEPECK_CHANGED
	
		key = SDL_MapRGBA(textureSurface->format,
				colorkey.r, colorkey.g, colorkey.b, SDL_ALPHA_OPAQUE );
	
		SDL_FillRect( textureSurface, &textureRect, key );
				
		SDL_SetColorKey( textureSurface,
				textureSurface->flags & SDL_SRCCOLORKEY, key);
	}
	else if (hasAlpha) // clear the alpha channel
	{
		SDL_FillRect( textureSurface, &textureRect, SDL_MapRGBA(
			textureSurface->format, 0, 0, 0, 0) );

		SDL_SetAlpha(textureSurface,
				textureSurface->flags & SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
	}
	else // fill with black
	{
		SDL_FillRect( textureSurface, &textureRect, SDL_MapRGB(
			textureSurface->format, 0, 0, 0) );
	}

	textureRect.x = 0;
	textureRect.y = 0;
	textureRect.w = rect->w;
	textureRect.h = rect->h;

		// no use drawing outside the used part of the textures
	SDL_SetClipRect(textureSurface, &textureRect);
	
		// transfer pixels (*with* the alpha/colorkey too!)
	if (!hasAlpha)
	{
		SDL_BlitSurface( surface, rect, textureSurface, &textureRect );
	}
	else
	{
		for (y = 0; y < rect->h; y++)
			for (x = 0; x < rect->w; x++)
				{
					SDL_Color color;
	
					SDL_GetRGBA( BKGetPixel( surface, rect->x + x, rect->y + y ), surface->format,
						&color.r, &color.g, &color.b, &color.unused );

					BKPutPixel(textureSurface, x, y, SDL_MapRGBA(textureSurface->format,
						color.r, color.g, color.b, color.unused) );
				}
	}

	if (hasMask) // convert any colorkey to alpha channel
	{
		BKConvertColorKeyToAlphaChannel( textureSurface, &colorkey );
	}
	else if (hasAlpha) // invert the alpha channel for GL
	{
	//	BKInvertAlphaChannel(textureSurface);	// doesn't seem to be needed anymore ?
	}

	useClientStorage = SDL_FALSE;
	useArrays = SDL_FALSE;

	gl->glGenTextures(numTex,texList);
	glerr = gl->glGetError();
	if (glerr)
	{
		fprintf(stderr, "glGenTextures (%d) error: 0x%04X\n", numTex, (int) glerr);
	}

	if (SDL_MUSTLOCK(textureSurface))
		SDL_LockSurface(textureSurface);


			gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			gl->glPixelStorei(GL_UNPACK_ROW_LENGTH,
				textureSurface->pitch / textureSurface->format->BytesPerPixel);

			if (useClientStorage)
				gl->glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 1);
			else if (caps->hasClientStorage)
				gl->glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 0);

	t = 0;
	b = height;

	i = 0;
	for ( y = 0; y < rows; y++ )
	{
		l = 0;
		r = width;
	
			// clip to bottom (partial texture)
		if (b > textureRect.h)
			b = textureRect.h;

		for ( x = 0; x < cols; x++ )
		{
			Uint8 *pixels = textureSurface->pixels;
			pixels += (y * height) * textureSurface->pitch;
			pixels += (x * width) * textureSurface->format->BytesPerPixel;
		
				// upload pixels to texture
			gl->glBindTexture(caps->extension, texList[i]);

			gl->glTexImage2D(caps->extension, 0, texInternal,
				width, height, 0, texFormat, texPixel, pixels);

			glerr = gl->glGetError();
			if (glerr)
			{
				fprintf(stderr, "glTexImage2D %dx%dx%d (0x%04X) error: 0x%04X\n",
									width, height, depth, (int) texInternal, (int) glerr);
			}
		
				// clip to right (partial texture)
			if (r > textureRect.w)
				r = textureRect.w;
		
			 // last row/column, we might need a partial texture
			 if ((x == cols-1) && (rect->w % width != 0))
				u = (float) (rect->w % width) / (float) width;		
			else
				u = 1.0f;
			if ((y == rows-1) && (rect->h % height != 0))
				v = (float) (rect->h % height) / (float) height;		
			else
				v = 1.0f;
			
			if (caps->extension == GL_TEXTURE_RECTANGLE_EXT)
			{
				// suddenly, texels are not normalized anymore
				u *= (float) width;
				v *= (float) height;			
			}
			
				// store vertex coordinates in the list
			vertexList[4*i+0] = l;
			vertexList[4*i+1] = t;
			vertexList[4*i+2] = r;
			vertexList[4*i+3] = b;
			
				// store texture coordinates in the list
			coordList[4*i+0] = 0.0f;
			coordList[4*i+1] = 0.0f;
			coordList[4*i+2] = u;
			coordList[4*i+3] = v;

//			printf("(%d,%d)-(%d,%d)\n",  l,t,r,b);
//			printf("%ld\t(%f,%f)-(%f,%f)\n", texList[i] , 0.0f, 0.0f, u, v);

			l += width;
			r += width;

			i++;
		}

		t += height;
		b += height;
	}
	
	if (SDL_MUSTLOCK(textureSurface))
		SDL_UnlockSurface(textureSurface);
	
	glsurface->textureSurface = textureSurface;
	glsurface->textureRect = textureRect;
	glsurface->needsBlend = hasAlpha || hasMask;
	glsurface->useArrays = useArrays;
	glsurface->sharesSurface = SDL_FALSE;
	glsurface->sharedParent = NULL;
	glsurface->sharedChildren = 0;

	glsurface->textureWidth = width;
	glsurface->textureHeight = height;
	glsurface->texExtension = caps->extension;
	glsurface->texInternal = texInternal;
	glsurface->texFormat = texFormat;
	glsurface->texPixel = texPixel;
	glsurface->textureRows = rows;	
	glsurface->textureCols = cols;

	glsurface->numTextures = numTex;
	glsurface->textureIDs = texList;
	glsurface->pixelCoords = vertexList;
	glsurface->textureCoords = coordList;

	return glsurface;
}

//---------------------------------------------------------------------------------------
//	BKReloadGLSurface -- updates the textures, with the changed surface contents
//---------------------------------------------------------------------------------------

int BKReloadGLSurface(BK_GL_Surface *glsurface, SDL_Rect *rect)
{
	SDL_Surface *textureSurface;
	struct BK_GL_func *gl;
	int			i, x,y;
	GLenum		glerr;
	int			tl, tt, tr, tb;
	int			l, t, r, b;

	gl = BKLoadGLfunctions();
	if (gl == NULL)
	{
		SDL_SetError("Failed to load GL functions");
		return -1;
	}

	textureSurface = glsurface->textureSurface;
	BK_ASSERT(textureSurface = NULL);

	if (SDL_MUSTLOCK(textureSurface))
		SDL_LockSurface(textureSurface);

	if (rect != NULL)
	{
			// convert to texture cols/rows
		tl = (rect->x) / glsurface->textureWidth;
		tt = (rect->y) / glsurface->textureHeight;
		tr = (rect->x + rect->w) / glsurface->textureWidth;
		tb = (rect->y + rect->h) / glsurface->textureHeight;

			// clip rect to texture surface
		if ( tl < 0 )
			tl = 0;
		if ( tt < 0 )
			tt = 0;
		if ( tr > glsurface->textureCols )
			tr = glsurface->textureCols;
		if ( tb > glsurface->textureRows )
			tb = glsurface->textureRows;
	}
	else
	{
		rect = &glsurface->textureRect;
	
		tl = 0;
		tt = 0;
		tr = glsurface->textureCols;
		tb = glsurface->textureRows;
	}

	t = rect->y;
	b = rect->y + glsurface->textureHeight;

		// reload all affected textures
	i = 0;
	for ( y = tt; y <= tb; y++ )
	{
		l = rect->x;
		r = rect->x + glsurface->textureWidth;

		for ( x = tl; x <= tr; x++ )
		{
			Uint8 *pixels = textureSurface->pixels;
			pixels += (rect->y + y * glsurface->textureHeight) * textureSurface->pitch;
			pixels += (rect->x + x * glsurface->textureWidth) * textureSurface->format->BytesPerPixel;
		
				// upload pixels to texture
			gl->glBindTexture(glsurface->texExtension, glsurface->textureIDs[i]);

			gl->glTexSubImage2D(glsurface->texExtension, 0, 0, 0,
				glsurface->textureWidth, glsurface->textureHeight,
				glsurface->texFormat, glsurface->texPixel, pixels);

			glerr = gl->glGetError();
			if (glerr)
			{
				fprintf(stderr, "glTexSubImage2D %dx%d error: %04X\n",
					glsurface->textureWidth, glsurface->textureHeight, (int) glerr);
			}

			l += glsurface->textureWidth;
			r += glsurface->textureWidth;

			i++;
		}

		t += glsurface->textureHeight;
		b += glsurface->textureHeight;

	}

	if (SDL_MUSTLOCK(textureSurface))
		SDL_UnlockSurface(textureSurface);

	return 0;
}

//---------------------------------------------------------------------------------------
//	BKCreatePartialGLSurface -- creates a clone of a part of a bigger original
//---------------------------------------------------------------------------------------

BK_GL_Surface *BKCreatePartialGLSurface(BK_GL_Surface *glsurface, SDL_Rect *rect)
{
	BK_GL_Surface *partialsurface = NULL;
	struct BK_GL_func *gl;
	int			i, x,y;
	int			tl, tt, tr, tb;
	int			rows, cols;
	int			l, t, r, b;
	int			numTex;
	GLuint		*texList;
	GLshort		*vertexList;
	GLfloat		*coordList;

	BK_ASSERT(glsurface != NULL);

	gl = BKLoadGLfunctions();
	if (gl == NULL)
	{
		SDL_SetError("Failed to load GL functions");
		return NULL;
	}

		// calculate texture coordinates
	if (rect != NULL)
	{
		tl = (rect->x) / glsurface->textureWidth;
		tt = (rect->y) / glsurface->textureHeight;
		tr = (rect->x + rect->w) / glsurface->textureWidth;
		tb = (rect->y + rect->h) / glsurface->textureHeight;

			// clip rect to texture surface
		if ( tl < 0 )
			tl = 0;
		if ( tt < 0 )
			tt = 0;
		if ( tr > glsurface->textureCols )
			tr = glsurface->textureCols;
		if ( tb > glsurface->textureRows )
			tb = glsurface->textureRows;
	}
	else
	{
		rect = &glsurface->textureRect;
	
		tl = 0;
		tt = 0;
		tr = glsurface->textureCols;
		tb = glsurface->textureRows;
	}

		// bail early, if no memory
	partialsurface = calloc(1, sizeof(BK_GL_Surface));
	if (partialsurface == NULL)
	{
		SDL_SetError("Out of memory");
		return NULL;
	}

		// calculate number of textures needed
	cols = (tr - tl + 1);
	rows = (tb - tt + 1);
	numTex = rows * cols;

	texList = (GLuint *) calloc(numTex, sizeof(GLuint));
	if (texList == NULL)
	{
		SDL_SetError("Out of memory");
		return NULL;
	}

	vertexList = (GLshort *) calloc(4*numTex, sizeof(GLshort));
	if (vertexList == NULL)
	{
		SDL_SetError("Out of memory");
		return NULL;
	}

	coordList = (GLfloat *) calloc(4*numTex, sizeof(GLfloat));
	if (coordList == NULL)
	{
		SDL_SetError("Out of memory");
		return NULL;
	}

	// TODO: don't upload the whole textures, check with rect

	t = rect->y;
	b = rect->y + glsurface->textureHeight;

	i = 0;
	for ( y = tt; y <= tb; y++ )
	{

			// clip to top/bottom (partial texture)
/*
		if (t < rect->y)
			b = textureRect.h;
		if (b > textureRect.h)
			b = textureRect.h;
*/

		l = rect->x;
		r = rect->x + glsurface->textureWidth;

		for ( x = tl; x <= tr; x++ )
		{
	
				// clip to top/bottom (partial texture)
		/*
			if (t < rect->y)
				b = textureRect.h;
			if (b > textureRect.h)
				b = textureRect.h;
		*/
			
			texList[i] = 0;

			texList[i] = 0;
			texList[i] = 0;
			texList[i] = 0;

			l += glsurface->textureWidth;
			r += glsurface->textureWidth;

			texList += 1;
			vertexList += 4;
			coordList += 4;

			i++;
		}
		
		t += glsurface->textureHeight;
		b += glsurface->textureHeight;

	}
	
	partialsurface->textureSurface = glsurface->textureSurface;
	partialsurface->textureRect = glsurface->textureRect;
	partialsurface->needsBlend = glsurface->needsBlend;
	partialsurface->useArrays = glsurface->useArrays;
	partialsurface->sharesSurface = SDL_TRUE; 
	partialsurface->sharedParent = glsurface; 

	partialsurface->textureWidth = glsurface->textureWidth;
	partialsurface->textureHeight = glsurface->textureHeight;
	partialsurface->texExtension = glsurface->texExtension;
	partialsurface->texInternal = glsurface->texInternal;
	partialsurface->texFormat = glsurface->texFormat;
	partialsurface->texPixel = glsurface->texPixel;
	partialsurface->textureRows = rows;	
	partialsurface->textureCols = cols;

	partialsurface->numTextures = numTex;
	partialsurface->textureIDs = texList;
	partialsurface->pixelCoords = vertexList;
	partialsurface->textureCoords = coordList;

	glsurface->sharedChildren++; // hello, son

	return partialsurface;
}

//---------------------------------------------------------------------------------------
//	BKFreeGLSurface
//---------------------------------------------------------------------------------------

/// Frees the memory used by a GL surface
void BKFreeGLSurface(BK_GL_Surface *glsurface)
{
	struct BK_GL_func *gl;
	
	gl = BKLoadGLfunctions();
	if (gl == NULL)
	{
		SDL_SetError("Failed to load GL functions");
		return;
	}

	if (glsurface != NULL)
	{
		if (glsurface->pixelCoords)
		{
			free(glsurface->pixelCoords);
			glsurface->pixelCoords = NULL;
		}

		if (glsurface->textureCoords)
		{
			free(glsurface->textureCoords);
			glsurface->textureCoords = NULL;
		}

		if (glsurface->textureIDs)
		{
			if (!glsurface->sharesSurface) // is it mine ?
				gl->glDeleteTextures(glsurface->numTextures, glsurface->textureIDs);
			free(glsurface->textureIDs);
		}

		if (glsurface->textureSurface)
		{
			if (!glsurface->sharesSurface) // is it mine ?
				SDL_FreeSurface(glsurface->textureSurface);
			glsurface->textureSurface = NULL;
		}
	
		if (glsurface->sharedParent != NULL)
			glsurface->sharedParent->sharedChildren++; // bye, dad
	
		free(glsurface);
	}
}

//---------------------------------------------------------------------------------------
//	BKGetSDLSurface -- return the texture surface, for messing with (don't forget reload)
//---------------------------------------------------------------------------------------

SDL_Surface *BKGetSDLSurface(BK_GL_Surface *glsurface)
{
	return glsurface->textureSurface;
}

//---------------------------------------------------------------------------------------
//	BKBlitGLSurface -- draws the entire surface, at the current position/rotation/scale
//---------------------------------------------------------------------------------------

int BKBlitGLSurface(BK_GL_Surface *glsurface)
{
	struct BK_GL_func *gl;
	int			i;
	GLuint		*id;
	GLshort		*vert;
	GLfloat		*tex;
	int			count;
	
	BK_ASSERT(glsurface != NULL);
	BK_ASSERT(glsurface->textureSurface != NULL);
	BK_ASSERT(glsurface->textureIDs != NULL);
	BK_ASSERT(glsurface->pixelCoords != NULL);
	BK_ASSERT(glsurface->textureCoords != NULL);
	
	gl = BKLoadGLfunctions();
	if (gl == NULL)
	{
		SDL_SetError("Failed to load GL functions");
		return -1;
	}

	if (glsurface->needsBlend)
	{
		gl->glEnable(GL_BLEND);
		gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// TODO: remove this
	gl->glEnable(glsurface->texExtension);
			
	id = glsurface->textureIDs;
	vert = glsurface->pixelCoords;
	tex = glsurface->textureCoords;

/*	// TODO: use arrays, if possible
	count = 0;
	for ( i = 0; i <= glsurface->numTextures; i++ )
	{
		if (i != glsurface->numTextures)
		{
			count++;

			if (glsurface->useArrays && i > 1 && id[0] == id[-1])
			{
				 // same as last time
				continue;
			}
		}
		else
		{
			if (!glsurface->useArrays)
				break;
		}
		
			// draw with arrays, if possible and necessary
		if (glsurface->useArrays && count > 1)
		{
			gl->glTexCoordPointer(2, GL_FLOAT, 0, tex);
			gl->glVertexPointer(2, GL_SHORT, 0, vert);
			gl->glDrawArrays(GL_QUADS, 0, 4 * count);

			id += count;
			vert += 4 * count;
			tex += 4 * count;
			count = 0;
		}
		else
		{
			// select this texture
			gl->glBindTexture(glsurface->texExtension, id[0]);
	
			if (!(glsurface->useArrays && count > 1))
			{
					// draw this single quad as a fan
				gl->glBegin(GL_TRIANGLE_FAN);
					gl->glTexCoord2f(tex[0], tex[1]);
					gl->glVertex2i(vert[0], vert[1]);
					gl->glTexCoord2f(tex[2], tex[1]);
					gl->glVertex2i(vert[0], vert[1]);
					gl->glTexCoord2f(tex[2], tex[3]);
					gl->glVertex2i(vert[0], vert[1]);
					gl->glTexCoord2f(tex[0], tex[4]);
					gl->glVertex2i(vert[0], vert[1]);
				gl->glEnd();
	
				id += 1;
				vert += 4;
				tex += 4;
				count = 0;
			}
		}
	}
*/
		for (i = 0; i < glsurface->numTextures; i++)
		{

				// select this texture
			gl->glBindTexture(glsurface->texExtension, id[0]);
	
				// draw this single quad as a fan
			gl->glBegin(GL_TRIANGLE_FAN);
				gl->glTexCoord2f(tex[0], tex[1]);
				gl->glVertex2i(vert[0], vert[1]);
				gl->glTexCoord2f(tex[2], tex[1]);
				gl->glVertex2i(vert[2], vert[1]);
				gl->glTexCoord2f(tex[2], tex[3]);
				gl->glVertex2i(vert[2], vert[3]);
				gl->glTexCoord2f(tex[0], tex[3]);
				gl->glVertex2i(vert[0], vert[3]);
			gl->glEnd();
		
			id += 1;
			vert += 4;
			tex += 4;
			count = 0;
		}

	// TODO: remove this
	gl->glDisable(glsurface->texExtension);

	if (glsurface->needsBlend)
	{
		gl->glDisable(GL_BLEND);
	}

		if (0)	// draw yellow lines at texture divisions
		{
			id = glsurface->textureIDs;
			vert = glsurface->pixelCoords;
			tex = glsurface->textureCoords;

			for (i = 0; i < glsurface->numTextures; i++)
			{
				gl->glColor4ub(255,255,0, SDL_ALPHA_OPAQUE);

				gl->glDisable(glsurface->texExtension);
				gl->glBegin(GL_LINE_LOOP);
					gl->glVertex2i(vert[0], vert[1]);
					gl->glVertex2i(vert[2], vert[1]);
					gl->glVertex2i(vert[2], vert[3]);
					gl->glVertex2i(vert[0], vert[3]);
				gl->glEnd();
				gl->glEnable(glsurface->texExtension);
		
				gl->glColor4ub(255,255,255, SDL_ALPHA_OPAQUE);
	
				id += 1;
				vert += 4;
				tex += 4;
			}
		}
	
	return 0;
}

#if 0
#pragma mark -
#endif

//---------------------------------------------------------------------------------------
//	OpenGL functions
//---------------------------------------------------------------------------------------

#define GL_LIB	NULL /* default filename */

// DAVEPECK_CHANGE
/*
 #ifndef HAVE_OPENGL
#define OPENGL_AVAILABLE(l)		SDL_FALSE
#define OPENGL_FUNCTION(f)		NULL
#elif !BK_WEAKLINK_OPENGL
#define OPENGL_AVAILABLE(l)		SDL_TRUE
#define OPENGL_FUNCTION(f)		&f
#elif BK_WEAKLINK_OPENGL
#define OPENGL_AVAILABLE(l)		(SDL_GL_LoadLibrary(l) == 0)
#define OPENGL_FUNCTION(f)		SDL_GL_GetProcAddress(#f); BK_ASSERT(GL_functions.f != NULL)
#endif
*/

#define OPENGL_AVAILABLE(l)		SDL_TRUE
#define OPENGL_FUNCTION(f)		&f

// DAVEPECK_CHANGE


struct BK_GL_func *BKLoadGLfunctions(void)
{
	if (!GL_functions_loaded)
	{
		memset(&GL_functions, 0, sizeof(GL_functions));

		if (OPENGL_AVAILABLE(GL_LIB))
		{
			/* 
			 DAVEPECK_CHANGED
			GL_functions.glBegin = OPENGL_FUNCTION(glBegin);
			GL_functions.glEnd = OPENGL_FUNCTION(glEnd);
			 */
			GL_functions.glFinish = OPENGL_FUNCTION(glFinish);
			GL_functions.glGetError = OPENGL_FUNCTION(glGetError);
			GL_functions.glGetString = OPENGL_FUNCTION(glGetString);
			GL_functions.glEnable = OPENGL_FUNCTION(glEnable);
			GL_functions.glDisable = OPENGL_FUNCTION(glDisable);
			GL_functions.glGenTextures = OPENGL_FUNCTION(glGenTextures);
			GL_functions.glGetIntegerv = OPENGL_FUNCTION(glGetIntegerv);
			GL_functions.glHint = OPENGL_FUNCTION(glHint);
			GL_functions.glDeleteTextures = OPENGL_FUNCTION(glDeleteTextures);
			GL_functions.glBindTexture = OPENGL_FUNCTION(glBindTexture);
			GL_functions.glTexParameteri = OPENGL_FUNCTION(glTexParameteri);
			GL_functions.glTexImage2D = (void *) OPENGL_FUNCTION(glTexImage2D);
			GL_functions.glTexSubImage2D = (void *) OPENGL_FUNCTION(glTexSubImage2D);
			GL_functions.glReadBuffer = OPENGL_FUNCTION(glReadBuffer);
			GL_functions.glReadPixels = OPENGL_FUNCTION(glReadPixels);
			GL_functions.glPixelStorei = OPENGL_FUNCTION(glPixelStorei);
			GL_functions.glClearColor = OPENGL_FUNCTION(glClearColor);
			GL_functions.glClear = OPENGL_FUNCTION(glClear);
			GL_functions.glBlendFunc = OPENGL_FUNCTION(glBlendFunc);
			GL_functions.glPointSize = OPENGL_FUNCTION(glPointSize);
			GL_functions.glFogf = OPENGL_FUNCTION(glFogf);
			GL_functions.glVertex2f = OPENGL_FUNCTION(glVertex2f);
			GL_functions.glVertex2i = OPENGL_FUNCTION(glVertex2i);
			GL_functions.glVertex3f = OPENGL_FUNCTION(glVertex3f);
			GL_functions.glTexCoord2f = OPENGL_FUNCTION(glTexCoord2f);
			GL_functions.glColor4f = OPENGL_FUNCTION(glColor4f);
			GL_functions.glColor4ub = OPENGL_FUNCTION(glColor4ub);
			GL_functions.glMatrixMode = OPENGL_FUNCTION(glMatrixMode);
			GL_functions.glLoadIdentity = OPENGL_FUNCTION(glLoadIdentity);
			GL_functions.glPushMatrix = OPENGL_FUNCTION(glPushMatrix);
			GL_functions.glPopMatrix = OPENGL_FUNCTION(glPopMatrix);
			GL_functions.glRotatef = OPENGL_FUNCTION(glRotatef);
			GL_functions.glScalef = OPENGL_FUNCTION(glScalef);
			GL_functions.glTranslatef = OPENGL_FUNCTION(glTranslatef);
			GL_functions.glShadeModel = OPENGL_FUNCTION(glShadeModel);
			GL_functions.glOrtho = OPENGL_FUNCTION(glOrtho);
			GL_functions.glViewport = OPENGL_FUNCTION(glViewport);
			GL_functions.glLightfv = OPENGL_FUNCTION(glLightfv);

			GL_functions.glTexCoordPointer = OPENGL_FUNCTION(glTexCoordPointer);
			GL_functions.glVertexPointer = OPENGL_FUNCTION(glVertexPointer);
			GL_functions.glDrawArrays = OPENGL_FUNCTION(glDrawArrays);
		}
		else
		{
			// OpenGL is not available, or could not be loaded
		#if defined(HAVE_OPENGL) && BK_WEAKLINK_OPENGL
			printf("SDL_GL_LoadLibrary: %s\n", SDL_GetError());
		#endif
			return NULL;
		}
		
		GL_functions_loaded = SDL_TRUE;
	}

	return &GL_functions;
}

//---------------------------------------------------------------------------------------
//	OpenGL capabilities
//---------------------------------------------------------------------------------------

struct BK_GL_caps *BKLoadGLcapabilities(void)
{
	if (!GL_capabilities_loaded)
	{
		memset(&GL_capabilities, 0, sizeof(GL_capabilities));

		if (BKLoadGLfunctions() != NULL)
		{
			const char *strVersion = NULL;
			const char *strExtensions = NULL;

			// on Mac, glGet... crashes without a current AGL context!

			GL_functions.glGetIntegerv(GL_MAX_TEXTURE_SIZE,&GL_capabilities.maxtexture);

			strVersion = (const char *) GL_functions.glGetString(GL_VERSION);
			strExtensions = (const char *) GL_functions.glGetString(GL_EXTENSIONS);
			
			if (strVersion && strExtensions)
			{
			
			// fprintf(stderr, "%s\n", strExtensions);
			
			if (strstr(strVersion, "1.2") || strstr(strVersion, "1.3") ||
				strstr(strExtensions, "GL_EXT_texture_edge_clamp"))
				GL_capabilities.hasClampToEdge = SDL_TRUE;
			else
				GL_capabilities.hasClampToEdge = SDL_FALSE;

			if (strstr(strExtensions, "GL_EXT_texture_rectangle") ||
				strstr(strExtensions, "GL_ARB_texture_rectangle"))
				GL_capabilities.hasTextureRectangle = SDL_TRUE;
			else
				GL_capabilities.hasTextureRectangle = SDL_FALSE;

			if (strstr(strVersion, "1.2") || strstr(strVersion, "1.3") ||
				strstr(strExtensions, "GL_EXT_bgra"))
				GL_capabilities.hasReverseFormats = SDL_TRUE;
			else
				GL_capabilities.hasReverseFormats = SDL_FALSE;

	        if (strstr(strVersion, "1.2") || strstr(strVersion, "1.3") ||
				strstr(strExtensions, "GL_APPLE_packed_pixel") ||
				strstr(strExtensions, "GL_APPLE_packed_pixels"))
				GL_capabilities.hasPackedPixels = SDL_TRUE;
			else
				GL_capabilities.hasPackedPixels = SDL_FALSE;

			if (strstr(strExtensions, "GL_APPLE_client_storage"))
				GL_capabilities.hasClientStorage = SDL_TRUE;
			else
				GL_capabilities.hasClientStorage = SDL_FALSE;

			if (strstr(strExtensions, "GL_APPLE_texture_range"))
				GL_capabilities.hasTextureRange = SDL_TRUE;
			else
				GL_capabilities.hasTextureRange = SDL_FALSE;

			if (strstr(strVersion, "2.0") ||
				strstr(strExtensions, "GL_ARB_texture_non_power_of_two"))
				GL_capabilities.hasNonPowerOfTwo = SDL_TRUE;
			else
				GL_capabilities.hasNonPowerOfTwo = SDL_FALSE;
			}
					
			// ------------------------------------------------------

			if (GL_capabilities.hasTextureRectangle && !GL_capabilities.hasNonPowerOfTwo)
				GL_capabilities.extension = GL_TEXTURE_RECTANGLE_EXT;
			else
				GL_capabilities.extension = GL_TEXTURE_2D;

			if (GL_capabilities.hasTextureRectangle || GL_capabilities.hasNonPowerOfTwo)
				GL_capabilities.power2 = SDL_FALSE;
			else
				GL_capabilities.power2 = SDL_TRUE;
				
			if (GL_capabilities.maxtexture == 0)
				GL_capabilities.maxtexture = BK_DEFAULT_TEXTURE_SIZE;

			if (GL_capabilities.maxtexture > BK_MAX_TEXTURE_SIZE)
				GL_capabilities.maxtexture = BK_MAX_TEXTURE_SIZE;
				
		}
		else
		{
			// OpenGL is not available, or could not be loaded
			return NULL;
		}
		
		GL_capabilities_loaded = SDL_TRUE;
	}

	return &GL_capabilities;
}

#endif //HAVE_OPENGL
