/*
	BlitKernel, helper for extending and integrating SDL and OpenGL

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

#ifndef __BLITKERNEL__
#define __BLITKERNEL__

#ifdef HAVE_CONFIG_H
#include "config.h"
#else

#ifdef HAVE_OPENGL
#undef HAVE_OPENGL
#endif


/// Define if you have the ZZIP library
/* #undef HAVE_ZZIP */

/// Define if you have the SDL_image library
/* #undef HAVE_SDL_IMAGE */

#endif /* HAVE_CONFIG_H */

/**
	@file	BlitKernel.h
	@brief	helper for extending and integrating SDL and OpenGL

	"It was not really a light; it made this light by flashing about
	so quickly, but when it came to rest for a second you saw it was a
	fairy, no longer than your hand, but still growing." -- J.M. Barrie
*/

#if macintosh
#include "SDL.h"
#else
#include <SDL/SDL.h>
#endif

#ifdef HAVE_OPENGL

#include <SDL/SDL_opengl.h>

#endif /* HAVE_OPENGL */

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------

/// Loads a surface from a image file (possibly in archive)
SDL_Surface *BKLoadSurface(const char *filename);

/// Loads a surface from a 8/24/32-bit Targa (.tga) file
SDL_Surface *BKLoadTGA_RW(SDL_RWops *src, int freesrc);

/// Creates a blank surface in an OpenGL-suitable format
SDL_Surface *BKCreateGLSurface(int w, int h, int bpp,
         SDL_bool power2, SDL_bool mask, SDL_bool alpha);

// -----------------------------------------------------------------------------

/// Mirror image horizontal
/// @return error or 0
int BKFlipHorizontal(SDL_Surface *surface);

/// Mirror image vertical
/// @return error or 0
int BKFlipVertical(SDL_Surface *surface);

/// Invert the alpha channel (mostly for use with OpenGL)
/// @return error or 0
int BKInvertAlphaChannel(SDL_Surface *surface);

/// Converts the alpha channel to black-white
/// @return error or 0
int BKDitherAlphaChannel(SDL_Surface *surface);

/// Convert the color key to a black-white alpha channel
/// @return error or 0
int BKConvertColorKeyToAlphaChannel(SDL_Surface *surface, SDL_Color *colorkey);

/// Convert the alpha channel to a (threshold) color key
/// @return error or 0
int BKConvertAlphaChannelToColorKey(SDL_Surface *surface, SDL_Color *colorkey);

/// Sets the color key according to a black-white mask
/// @return error or 0
int BKAddColorKeyFromMask(SDL_Surface *surface, SDL_Surface *mask);

/// Sets the alpha channel according to a grayscale mask
/// @return error or 0
int BKAddAlphaChannelFromMask(SDL_Surface *surface, SDL_Surface *mask);

// -----------------------------------------------------------------------------

/// Checks whether pixel is transparent (colorkey or alpha)
/// @warning The surface must be locked before calling this!
SDL_bool BKTransparentPixel(SDL_Surface *surface, int x , int y);

/// Checks whether two bounding boxes are overlapping
SDL_bool BKCollideBoundingBox(SDL_Surface *sa , int ax , int ay,
                               SDL_Surface *sb , int bx , int by);

/// Checks if two masks collides (note: checks bounds first)
SDL_bool BKCollidePixels(SDL_Surface *sa , int ax , int ay,
                          SDL_Surface *sb , int bx , int by);

// -----------------------------------------------------------------------------

/// Set the pixel at position (x, y) to the given value
/// @warning The surface must be locked before calling this!
void BKPutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

/// Returns the value of the pixel at position (x, y)
/// @warning The surface must be locked before calling this!
Uint32 BKGetPixel(SDL_Surface *surface, int x, int y);

/// Draws a line from (x1,y1) to (x2,y2) using the given value
/// @warning The surface must be locked before calling this!
void BKDrawLine(SDL_Surface *surface, int x1, int y1, int x2, int y2, Uint32 pixel);

/// Draws an oval within the rect, optionally filled solid
/// @warning The surface must be locked before calling this!
void BKDrawOval(SDL_Surface *surface, SDL_Rect *rect, Uint32 pixel, SDL_bool filled);

#ifdef HAVE_OPENGL

// ----------------------------------------------------------------------------

/// Load OpenGL functions, or return NULL if not available
	struct BK_GL_func *BKLoadGLfunctions(void) { return NULL; }

#ifndef APIENTRY
#define APIENTRY
#endif

/// OpenGL function pointers
struct BK_GL_func
{
    // (Note: don't forget to also *load* any new additions to this list)
    void (APIENTRY *glBegin) (GLenum mode);
    void (APIENTRY *glEnd) (void);
    void (APIENTRY *glFinish) (void);
    GLenum (APIENTRY *glGetError) (void);
    const GLubyte *(APIENTRY *glGetString) (GLenum name);
    void (APIENTRY *glEnable) (GLenum cap);
    void (APIENTRY *glDisable) (GLenum cap);
    void (APIENTRY *glGenTextures) (GLsizei n, GLuint *textures);
    void (APIENTRY *glGetIntegerv) (GLenum pname, GLint *params);
    void (APIENTRY *glHint) (GLenum target, GLenum mode);
    void (APIENTRY *glDeleteTextures) (GLsizei n, const GLuint *textures);
    void (APIENTRY *glBindTexture) (GLenum target, GLuint texture);
    void (APIENTRY *glTexParameteri) (GLenum target, GLenum pname, GLint param);
    void (APIENTRY *glTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (APIENTRY *glTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    void (APIENTRY *glReadBuffer) (GLenum mode);
    void (APIENTRY *glReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
    void (APIENTRY *glPixelStorei) (GLenum pname, GLint param);
    void (APIENTRY *glClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (APIENTRY *glClear) (GLbitfield mask);
    void (APIENTRY *glBlendFunc) (GLenum sfactor, GLenum dfactor);
    void (APIENTRY *glFogf) (GLenum pname, GLfloat param);
    void (APIENTRY *glPointSize) (GLfloat size);
    void (APIENTRY *glVertex2f) (GLfloat x, GLfloat y);
    void (APIENTRY *glVertex2i) (GLint x, GLint y);
    void (APIENTRY *glVertex3f) (GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY *glTexCoord2f) (GLfloat s, GLfloat t);
    void (APIENTRY *glColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void (APIENTRY *glColor4ub) (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
    void (APIENTRY *glMatrixMode) (GLenum mode);
    void (APIENTRY *glLoadIdentity) (void);
    void (APIENTRY *glPushMatrix) (void);
    void (APIENTRY *glPopMatrix) (void);
    void (APIENTRY *glRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY *glScalef) (GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY *glTranslatef) (GLfloat x, GLfloat y, GLfloat z);
    void (APIENTRY *glShadeModel) (GLenum mode);
    void (APIENTRY *glOrtho) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
    void (APIENTRY *glViewport) (GLint x, GLint y, GLsizei width, GLsizei height);
    void (APIENTRY *glLightfv) (GLenum light, GLenum pname, const GLfloat *params);

    void (APIENTRY *glTexCoordPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void (APIENTRY *glVertexPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void (APIENTRY *glDrawArrays) (GLenum mode, GLint first, GLsizei count);

};

// ----------------------------------------------------------------------------

/// Load OpenGL capabilities, or return NULL if not available
struct BK_GL_caps *BKLoadGLcapabilities(void);

/// OpenGL capabilities
struct BK_GL_caps
{
    GLenum extension; ///< GL_TEXTURE_2D or GL_TEXTURE_RECTANGLE_EXT
 
    GLint maxtexture; ///< maximum size of a texture side (in pixels)

    SDL_bool power2;  ///< do texture sizes need to be a power of two ?
 
    SDL_bool    hasClampToEdge;         ///< GL_EXT_texture_edge_clamp
    SDL_bool    hasTextureRectangle;    ///< GL_EXT_texture_rectangle
    SDL_bool    hasReverseFormats;      ///< GL_EXT_bgra

    SDL_bool    hasPackedPixels;        ///< GL_APPLE_packed_pixels
    SDL_bool    hasClientStorage;       ///< GL_APPLE_client_storage
    SDL_bool    hasTextureRange;        ///< GL_APPLE_texture_range

    SDL_bool    hasNonPowerOfTwo;       ///< GL_ARB_texture_non_power_of_two
};

#ifndef GL_BGR                          /// introduced in OpenGL 1.2
#define GL_BGR                          0x80E0
#endif
#ifndef GL_BGRA                         /// introduced in OpenGL 1.2
#define GL_BGRA                         0x80E1
#endif

#ifndef GL_CLAMP_TO_EDGE                /// introduced in OpenGL 1.2
#define GL_CLAMP_TO_EDGE                0x812F
#endif

#ifndef GL_UNPACK_CLIENT_STORAGE_APPLE  /// introduced in OpenGL 1.2.1
#define GL_UNPACK_CLIENT_STORAGE_APPLE  0x85B2
#endif

#ifndef GL_TEXTURE_RECTANGLE_EXT        /// introduced in OpenGL 1.3
#define GL_TEXTURE_RECTANGLE_EXT        0x84F5
#endif 

#ifndef GL_TEXTURE_STORAGE_HINT_APPLE   /// introduced in Mac OS X 10.2
#define GL_TEXTURE_STORAGE_HINT_APPLE   0x85BC
#endif 

#ifndef GL_STORAGE_PRIVATE_APPLE        /// introduced in Mac OS X 10.2
#define GL_STORAGE_PRIVATE_APPLE        0x85BD
#endif
#ifndef GL_STORAGE_CACHED_APPLE         /// introduced in Mac OS X 10.2
#define GL_STORAGE_CACHED_APPLE         0x85BD
#endif
#ifndef GL_STORAGE_SHARED_APPLE         /// introduced in Mac OS X 10.2
#define GL_STORAGE_SHARED_APPLE         0x85BD
#endif

// -----------------------------------------------------------------------------

/// opaque GL surface type
typedef struct BK_GL_Surface	BK_GL_Surface;

/// Creates a new GL surface, from an existing SDL surface
BK_GL_Surface *BKCreateNewGLSurface(SDL_Surface *surface, SDL_Rect *rect);

/// Creates a new GL surface, from an existing GL surface
BK_GL_Surface *BKCreatePartialGLSurface(BK_GL_Surface *glsurface, SDL_Rect *rect);

/// Uploads changed pixels to the GL textures
int BKReloadGLSurface(BK_GL_Surface *glsurface, SDL_Rect *rect);

/// Frees the memory used by a GL surface (partial or not)
void BKFreeGLSurface(BK_GL_Surface *glsurface);

/// Returns the internal / texture surface (possibly shared)
SDL_Surface *BKGetSDLSurface(BK_GL_Surface *glsurface);

/// Draws the entire surface, at the current position/rotation/scale
int BKBlitGLSurface(BK_GL_Surface *surface);

// -----------------------------------------------------------------------------

#endif /* HAVE_OPENGL */

#ifdef __cplusplus
}
#endif

#endif /* __BLITKERNEL__ */
