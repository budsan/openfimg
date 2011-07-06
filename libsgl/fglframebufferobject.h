#ifndef _LIBSGL_FGLFRAMEBUFFEROBJECT_
#define _LIBSGL_FGLFRAMEBUFFEROBJECT_

#include "eglMem.h"
#include "fglobject.h"
#include "fglattach.h"

#include <GLES/gl.h>
#include <GLES/glext.h>

struct FGLRenderbuffer : public FGLAttachable
{
	GLenum format;
	FGLRenderbuffer() : format(GL_RGBA) {}
};

void fglColorAttachDeleted(FGLFramebuffer *fbo);
void fglColorAttachChanged(FGLFramebuffer *fbo);

void fglColorDepthDeleted(FGLFramebuffer *fbo);
void fglColorDepthChanged(FGLFramebuffer *fbo);

void fglColorStencilDeleted(FGLFramebuffer *fbo);
void fglColorStencilChanged(FGLFramebuffer *fbo);

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
