#ifndef _LIBSGL_FGLFRAMEBUFFEROBJECT_
#define _LIBSGL_FGLFRAMEBUFFEROBJECT_

#include "eglMem.h"
#include "fglobject.h"
#include "fglattach.h"

#include <GLES/gl.h>
#include <GLES/glext.h>

#define FGL_COLOR0_ATTACHABLE  (1<<0)
#define FGL_DEPTH_ATTACHABLE   (1<<1)
#define FGL_STENCIL_ATTACHABLE (1<<2)

struct FGLRenderbuffer : public FGLAttachable
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

void fglColorAttachDeleted(void *obj);
void fglColorAttachChanged(void *obj);

void fglColorDepthDeleted(void *obj);
void fglColorDepthChanged(void *obj);

void fglColorStencilDeleted(void *obj);
void fglColorStencilChanged(void *obj);

struct FGLFramebuffer
{
	unsigned colorName;
	unsigned depthName;
	unsigned stencilName;

	enum { NONE = 0, TEXTURE, RENDERBUFFER };
	unsigned colorType;
	unsigned depthType;
	unsigned stencilType;

	FGLAttach colorAttach;
	FGLAttach depthAttach;
	FGLAttach stencilAttach;

	FGLFramebuffer()
		: colorName(0), depthName(0), stencilName(0),
		  colorType(0), depthType(0), stencilType(0),
		  colorAttach  (this, fglColorAttachDeleted, fglColorAttachChanged),
		  depthAttach  (this, fglColorDepthDeleted,  fglColorDepthChanged),
		  stencilAttach(this, fglColorStencilDeleted,fglColorStencilChanged) {};

	~FGLFramebuffer() {}

	//Used for to GL_DEPTH_STENCIL_OES
	inline bool IsDepthStencilSameAttachment() {
		return depthAttach.get() == stencilAttach.get();
	}
};

typedef FGLObject<FGLRenderbuffer> FGLRenderBufferObject;
typedef FGLObjectBinding<FGLRenderbuffer> FGLRenderBufferObjectBinding;

typedef FGLObject<FGLFramebuffer> FGLFramebufferObject;
typedef FGLObjectBinding<FGLFramebuffer> FGLFramebufferObjectBinding;

#endif
