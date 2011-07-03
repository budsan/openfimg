#ifndef _LIBSGL_FGLFRAMEBUFFEROBJECT_
#define _LIBSGL_FGLFRAMEBUFFEROBJECT_

#include "eglMem.h"
#include "fglobject.h"

#include <GLES/gl.h>
#include <GLES/glext.h>

#define FGL_COLOR0_ATTACHABLE  (1<<0)
#define FGL_DEPTH_ATTACHABLE   (1<<1)
#define FGL_STENCIL_ATTACHABLE (1<<2)

struct FGLRenderbuffer
{
	FGLSurface	*surface;
	GLint		width;
	GLint		height;
	GLenum		format;

	unsigned	attachment;
	uint32_t	fglFormat;
	uint32_t	bpp;
	bool		swap;

	FGLRenderbuffer()
		: surface(0), width(0), height(0), format(GL_RGB),
		  attachment(0)
	{

	}

	~FGLRenderbuffer()
	{
		delete surface;
	}
};

struct FGLFramebuffer
{
	unsigned colorAttach;
	unsigned depthAttach;
	unsigned stencilAttach;

	enum { NONE = 0, TEXTURE, RENDERBUFFER };
	unsigned colorType;
	unsigned depthType;
	unsigned stencilType;

	//------------------//

	FGLSurface *color;
	FGLSurface *depth;

	unsigned int width;
	unsigned int height;
	unsigned int stride;
	unsigned int format;
	unsigned int depthFormat;

	FGLFramebuffer()
		: status(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES),
		  colorAttach(0), depthAttach(0), stencilAttach(0),
		  colorType(0), depthType(0), stencilType(0),
		  color(0), depth(0), width(0), stride(0), format(0)
	{

	}

	~FGLFramebuffer()
	{

	}
};

typedef FGLObject<FGLRenderbuffer> FGLRenderBufferObject;
typedef FGLObjectBinding<FGLRenderbuffer> FGLRenderBufferObjectBinding;

typedef FGLObject<FGLFramebuffer> FGLFramebufferObject;
typedef FGLObjectBinding<FGLFramebuffer> FGLFramebufferObjectBinding;

#endif
