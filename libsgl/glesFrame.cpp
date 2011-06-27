#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "glesCommon.h"
#include "fglobjectmanager.h"
#include "libfimg/fimg.h"
#include "s3c_g2d.h"

FGLObjectManager<FGLFramebuffer, FGL_MAX_FRAMEBUFFER_OBJECTS> fglFrameBufferObjects;
FGLObjectManager<FGLRenderbuffer, FGL_MAX_RENDERBUFFER_OBJECTS> fglRenderBufferObjects;


GL_API GLboolean GL_APIENTRY glIsRenderbufferOES (GLuint renderbuffer)
{
	if (renderbuffer == 0 || !fglRenderBufferObjects.isValid(renderbuffer))
		return GL_FALSE;

	return GL_TRUE;
}

GL_API void GL_APIENTRY glBindRenderbufferOES (GLenum target, GLuint renderbuffer)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if(renderbuffer == 0) {
		FGLContext *ctx = getContext();
		ctx->renderbuffer.unbind();
		return;
	}

	if(!fglRenderBufferObjects.isValid(renderbuffer)) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();

	FGLRenderBufferObject *obj = fglRenderBufferObjects[renderbuffer];
	if(obj == NULL) {
		obj = new FGLRenderBufferObject(renderbuffer);
		if (obj == NULL) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglRenderBufferObjects[renderbuffer] = obj;
	}

	obj->bind(&ctx->renderbuffer);
}

GL_API void GL_APIENTRY glDeleteRenderbuffersOES (GLsizei n, const GLuint* renderbuffers)
{
	unsigned name;

	if(n <= 0)
		return;

	do {
		name = *renderbuffers;
		renderbuffers++;

		if(!fglRenderBufferObjects.isValid(name)) {
			LOGD("Tried to free invalid renderbuffer %d", name);
			continue;
		}

		delete (fglRenderBufferObjects[name]);
		fglRenderBufferObjects.put(name);
	} while (--n);
}

GL_API void GL_APIENTRY glGenRenderbuffersOES (GLsizei n, GLuint* renderbuffers)
{
	if(n <= 0)
		return;

	int name;
	GLsizei i = n;
	GLuint *cur = renderbuffers;
	FGLContext *ctx = getContext();

	do {
		name = fglRenderBufferObjects.get(ctx);
		if(name < 0) {
			glDeleteRenderbuffersOES (n - i, renderbuffers);
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglRenderBufferObjects[name] = NULL;
		*cur = name;
		cur++;
	} while (--i);
}

static int fglGetRenderbufferFormatInfo(GLenum format, unsigned *bpp, bool *conv, bool *swap)
{
	//TODO FORMATS:
	//DEPTH_COMPONENT16_OES

	//OPTIONAL FORMATS:
	//STENCIL_INDEX1_OES
	//STENCIL_INDEX4_OES
	//STENCIL_INDEX8_OES
	//DEPTH_COMPONENT24_OES
	//DEPTH_COMPONENT32_OES
	//STENCIL_INDEX1_OES
	//STENCIL_INDEX4_OES
	//STENCIL_INDEX8_OES

	*conv = 0;
	*swap = 0;
	switch (format) {
		case GL_RGBA4_OES:
			*bpp = 2;
			return FGTU_TSTA_TEXTURE_FORMAT_4444;
		case GL_RGB5_A1_OES:
			*bpp = 2;
			return FGTU_TSTA_TEXTURE_FORMAT_1555;
		case GL_RGB565_OES:
			*bpp = 2;
			return FGTU_TSTA_TEXTURE_FORMAT_565;
		case GL_RGBA8_OES:// Needs swapping in pixel shader
			*swap = 1;
			*bpp = 4;
			return FGTU_TSTA_TEXTURE_FORMAT_8888;
		case GL_RGB8_OES: // Needs conversion
			*conv = 1;
			*bpp = 4;
			return FGTU_TSTA_TEXTURE_FORMAT_8888;
		default:
			return -1;

	}
}

GL_API void GL_APIENTRY glRenderbufferStorageOES (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	if (target != GL_RENDERBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return;
	}

	// TODO:
	// CHECK width & height greater than MAX_RENDERBUFFER_SIZE_EXT -> error INVALID_VALUE

	FGLContext *ctx = getContext();

	if(!ctx->renderbuffer.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLRenderbuffer *obj = ctx->renderbuffer.get();

	unsigned bpp;
	bool convert;
	bool swap;
	int fglFormat = fglGetRenderbufferFormatInfo(internalformat, &bpp, &convert, &swap);
	if (fglFormat < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	if (!obj->surface) {
		obj->width = width;
		obj->height = height;
		obj->format = internalformat;
		obj->fglFormat = fglFormat;
		obj->bpp = bpp;
		obj->convert = convert;
		obj->swap = swap;

		unsigned size = width * height * bpp;

		// Setup surface
		obj->surface = new FGLLocalSurface(size);
		if(!obj->surface || !obj->surface->isValid()) {
			delete obj->surface;
			obj->surface = 0;
			setError(GL_OUT_OF_MEMORY);
			return;
		}
	}
}

GL_API void GL_APIENTRY glGetRenderbufferParameterivOES (GLenum target, GLenum pname, GLint* params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API GLboolean GL_APIENTRY glIsFramebufferOES (GLuint framebuffer)
{
	FUNC_UNIMPLEMENTED;
	return GL_FALSE;
}

GL_API void GL_APIENTRY glBindFramebufferOES (GLenum target, GLuint framebuffer)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glDeleteFramebuffersOES (GLsizei n, const GLuint* framebuffers)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGenFramebuffersOES (GLsizei n, GLuint* framebuffers)
{
	FUNC_UNIMPLEMENTED;
}

GL_API GLenum GL_APIENTRY glCheckFramebufferStatusOES (GLenum target)
{
	FUNC_UNIMPLEMENTED;
	return 0;
}

GL_API void GL_APIENTRY glFramebufferRenderbufferOES (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glFramebufferTexture2DOES (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetFramebufferAttachmentParameterivOES (GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	FUNC_UNIMPLEMENTED;
}

