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
	/* Memory surface */
	FGLSurface	*surface;
	/* GL state */
	GLint		width;
	GLint		height;
	GLenum		format;
	unsigned	attachment;
	/* HW state */
	uint32_t	fglFormat;
	uint32_t	bpp;
	bool		swap;

	FGLRenderbuffer()
		: surface(0), width(0), height(0), format(GL_RGB)
	{

	}

	~FGLRenderbuffer()
	{
		delete surface;
	}
};

struct FGLFramebuffer
{
	GLenum status;

	struct Attach {
		unsigned int name;
		GLenum type;

		Attach() : name(0), type(0) {}
	};

	Attach colorAttach;
	Attach depthAttach;
	Attach stencilAttach;

	FGLFramebuffer()
		: status(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES),
		  colorAttach(), depthAttach(), stencilAttach()
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
