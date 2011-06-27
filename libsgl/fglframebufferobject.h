#ifndef _LIBSGL_FGLFRAMEBUFFEROBJECT_
#define _LIBSGL_FGLFRAMEBUFFEROBJECT_

#include "eglMem.h"
#include "fglobject.h"

#include <GLES/gl.h>
#include <GLES/glext.h>

struct FGLRenderbuffer
{
	/* Memory surface */
	FGLSurface	*surface;
	/* GL state */
	GLint		width;
	GLint		height;
	GLenum		format;
	/* HW state */
	uint32_t	fglFormat;
	uint32_t	bpp;
	bool		convert;
	bool		valid;
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
		enum AttachType {NONE, RENDERBUFFER, TEXTURE};
		unsigned int name;
		unsigned int type;

		Attach() : name(0), type(NONE) {}
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
