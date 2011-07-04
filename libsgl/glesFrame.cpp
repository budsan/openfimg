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

FGLObjectManager<FGLFramebuffer, FGL_MAX_FRAMEBUFFER_OBJECTS> fglFramebufferObjects;
FGLObjectManager<FGLRenderbuffer, FGL_MAX_RENDERBUFFER_OBJECTS> fglRenderbufferObjects;
extern FGLObjectManager<FGLTexture, FGL_MAX_TEXTURE_OBJECTS> fglTextureObjects;

inline void fglSetDefaultFramebuffer(FGLContext *gl)
{
	memcpy(&gl->framebuffer.curBuffer, &gl->framebuffer.defBuffer, sizeof(FGLSurfaceData));
	gl->framebuffer.externalBufferInUse = false;
	gl->framebuffer.status = GL_FRAMEBUFFER_COMPLETE_OES;
	fglSetCurrentBuffers(gl);
}

bool fglIsFramebufferAttachmentComplete(unsigned name, unsigned type, unsigned attachmask)
{
	switch(type)
	{
	case FGLFramebuffer::RENDERBUFFER:
	{
		//Object must exists due to when its deleted callback notice that
		FGLRenderBufferObject *obj = fglRenderbufferObjects[name];

		if (obj->object.width || obj->object.height == 0) {
			return false;
		}

		if (!(obj->object.attachment & attachmask)) {
			return false;
		}

		break;
	}
	case FGLFramebuffer::TEXTURE:
	{
		//Object must exists due to when its deleted callback notice that
		FGLTextureObject *obj = fglTextureObjects[name];

		if (obj->object.width || obj->object.height == 0) {
			return false;
		}

		//TODO:
		//if (!(obj->attachment & attachmask)) {
		//	return false;
		//}
		break;
	}
	case FGLFramebuffer::NONE:
	default:
		break;
	}

	return true;
}

void fglGetFramebufferAttachmentDimensions(unsigned name, unsigned type,
					   unsigned &width, unsigned &height) {
	switch(type)
	{
	case FGLFramebuffer::RENDERBUFFER:
	{
		FGLRenderBufferObject *obj = fglRenderbufferObjects[name];
		width  = obj->object.width;
		height = obj->object.height;
		break;
	}
	case FGLFramebuffer::TEXTURE:
	{
		FGLTextureObject *obj = fglTextureObjects[name];
		width  = obj->object.width;
		height = obj->object.height;
		break;
	}
	case FGLFramebuffer::NONE:
	default:
		width  = 0;
		height = 0;
		break;
	}
}

void fglUpdateFramebufferStatus(FGLContext *ctx, FGLFramebuffer* fbo)
{
	struct {
		unsigned name;
		unsigned type;
		unsigned mask;
	} atts[3] = {
		{fbo->colorName,   fbo->colorType,   FGL_COLOR0_ATTACHABLE},
		{fbo->depthName,   fbo->depthType,   FGL_DEPTH_ATTACHABLE},
		{fbo->stencilName, fbo->stencilType, FGL_STENCIL_ATTACHABLE}
	};

	for (unsigned int i = 0 ; i < 3; i++) {
		if (!fglIsFramebufferAttachmentComplete(
			atts[i].name, atts[i].type, atts[i].mask)) {
			ctx->framebuffer.status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			return;
		}
	}

	unsigned fw = 0, fh = 0;
	for (unsigned int i = 0 ; i < 3; i++)
	{

		unsigned w = 0, h = 0;
		fglGetFramebufferAttachmentDimensions( atts[i].name, atts[i].type, w, h);

		if ( w  == 0 ) continue;
		if ( fw == 0 ) {
			fw = w;
			fh = h;
		}
		else {
			if (fw != w || fh != h) {
				ctx->framebuffer.status = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES;
				return;
			}
		}
	}

	if ( fw == 0 ) {
		ctx->framebuffer.status = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES;
		return;
	}

	//TODO: CHECK FOR GL_FRAMEBUFFER_UNSUPPORTED_OES
}

static int fglGetRenderbufferFormatInfo(GLenum format, unsigned *bpp, GLenum *attachment, bool *swap)
{
	*attachment = 0;
	*swap = 0;

	switch (format) {
	case GL_RGBA4_OES: //REQUIRED
		//*bpp = 2;
		//*attachment = FGL_COLOR0_ATTACHABLE;
		//return -1;
		*bpp = 4;
		*swap = 1;
		*attachment = FGL_COLOR0_ATTACHABLE;
	case GL_RGB5_A1_OES: //REQUIRED
		*bpp = 2;
		*attachment = FGL_COLOR0_ATTACHABLE;
		return FGPF_COLOR_MODE_565;
	case GL_RGB565_OES: //REQUIRED
		*bpp = 2;
		*attachment = FGL_COLOR0_ATTACHABLE;
		return FGPF_COLOR_MODE_565;
	case GL_RGBA8_OES:// OPTIONAL - Needs swapping in pixel shader
		*bpp = 4;
		*swap = 1;
		*attachment = FGL_COLOR0_ATTACHABLE;
		return FGPF_COLOR_MODE_8888;
	case GL_RGB8_OES: // OPTIONAL - Needs swapping in pixel shader
		*bpp = 4;
		*swap = 1;
		*attachment = FGL_COLOR0_ATTACHABLE;
		return FGPF_COLOR_MODE_0888;
	case GL_DEPTH_COMPONENT16_OES: //REQUIRED
	case GL_DEPTH_COMPONENT24_OES: //OPTIONAL
		*attachment = FGL_DEPTH_ATTACHABLE;

	case GL_DEPTH_COMPONENT32_OES: //OPTIONAL
		return -1;
	case GL_STENCIL_INDEX1_OES: //OPTIONAL
	case GL_STENCIL_INDEX4_OES: //OPTIONAL
	case GL_STENCIL_INDEX8_OES: //OPTIONAL
		*attachment = FGL_STENCIL_ATTACHABLE;
		return -1;
	case GL_DEPTH_STENCIL_OES: //OES_packed_depth_stencil
		*bpp = 4;
		*attachment = FGL_STENCIL_ATTACHABLE | FGL_DEPTH_ATTACHABLE;
		return (8 << 8) | 24;
	default:
		return -1;
	}
}

GL_API GLboolean GL_APIENTRY glIsRenderbufferOES (GLuint renderbuffer)
{
	if (renderbuffer == 0 || !fglRenderbufferObjects.isValid(renderbuffer))
		return GL_FALSE;

	return GL_TRUE;
}

GL_API void GL_APIENTRY glBindRenderbufferOES (GLenum target, GLuint renderbuffer)
{
	if(target != GL_RENDERBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if(renderbuffer == 0) {
		FGLContext *ctx = getContext();
		ctx->renderbuffer.unbind();
		return;
	}

	if(!fglRenderbufferObjects.isValid(renderbuffer)) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();

	FGLRenderBufferObject *obj = fglRenderbufferObjects[renderbuffer];
	if(obj == NULL) {
		obj = new FGLRenderBufferObject(renderbuffer);
		if (obj == NULL) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglRenderbufferObjects[renderbuffer] = obj;
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

		if(!fglRenderbufferObjects.isValid(name)) {
			LOGD("Tried to free invalid renderbuffer %d", name);
			continue;
		}

		delete (fglRenderbufferObjects[name]);
		fglRenderbufferObjects.put(name);
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
		name = fglRenderbufferObjects.get(ctx);
		if(name < 0) {
			glDeleteRenderbuffersOES (n - i, renderbuffers);
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglRenderbufferObjects[name] = NULL;
		*cur = name;
		cur++;
	} while (--i);
}

GL_API void GL_APIENTRY glRenderbufferStorageOES (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	if (target != GL_RENDERBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if (width > FGL_MAX_TEXTURE_SIZE || height > FGL_MAX_TEXTURE_SIZE) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLContext *ctx = getContext();

	if(!ctx->renderbuffer.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLRenderbuffer *obj = ctx->renderbuffer.get();

	unsigned bpp;
	GLenum attachment;
	bool swap;
	int fglFormat = fglGetRenderbufferFormatInfo(internalformat, &bpp, &attachment, &swap);
	if (fglFormat < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if (!obj->surface) {
		obj->width = width;
		obj->height = height;
		obj->format = internalformat;
		obj->fglFormat = fglFormat;
		obj->bpp = bpp;
		obj->attachment = attachment;
		obj->swap = swap;
		unsigned size = width * height * bpp;

		delete obj->surface;
		obj->surface = 0;

		// Setup surface
		if (size)
		{
			obj->surface = new FGLLocalSurface(size);
			if(!obj->surface || !obj->surface->isValid()) {
				delete obj->surface;
				obj->surface = 0;
				setError(GL_OUT_OF_MEMORY);
				return;
			}
		}

		obj->changed();
	}
}

GL_API void GL_APIENTRY glGetRenderbufferParameterivOES (GLenum target, GLenum pname, GLint* params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API GLboolean GL_APIENTRY glIsFramebufferOES (GLuint framebuffer)
{
	if (framebuffer == 0 || !fglFramebufferObjects.isValid(framebuffer))
		return GL_FALSE;

	return GL_TRUE;
}

GL_API void GL_APIENTRY glBindFramebufferOES (GLenum target, GLuint framebuffer)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if(framebuffer == 0) {
		FGLContext *ctx = getContext();
		if (ctx->framebuffer.binding.isBound()) {
			ctx->framebuffer.binding.unbind();
			fglSetDefaultFramebuffer(ctx);
		}
		return;
	}

	if(!fglFramebufferObjects.isValid(framebuffer)) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();

	FGLFramebufferObject *obj = fglFramebufferObjects[framebuffer];
	if(obj == NULL) {
		obj = new FGLFramebufferObject(framebuffer);
		if (obj == NULL) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglFramebufferObjects[framebuffer] = obj;
	}

	obj->bind(&ctx->framebuffer.binding);
	fglUpdateFramebufferStatus(ctx,& obj->object);
}

GL_API void GL_APIENTRY glDeleteFramebuffersOES (GLsizei n, const GLuint* framebuffers)
{
	unsigned name;

	if(n <= 0)
		return;

	do {
		name = *framebuffers;
		framebuffers++;

		if(!fglFramebufferObjects.isValid(name)) {
			LOGD("Tried to free invalid framebuffer %d", name);
			continue;
		}

		delete (fglFramebufferObjects[name]);
		fglFramebufferObjects.put(name);
	} while (--n);

	//Here we're checking if this some deleted framebuffer was bound
	//If it was, we must set default framebuffer.
	FGLContext *ctx = getContext();
	if ( ctx->framebuffer.externalBufferInUse &&
	    !ctx->framebuffer.binding.isBound()) {
		fglSetDefaultFramebuffer(ctx);
	}
}

GL_API void GL_APIENTRY glGenFramebuffersOES (GLsizei n, GLuint* framebuffers)
{
	if(n <= 0)
		return;

	int name;
	GLsizei i = n;
	GLuint *cur = framebuffers;
	FGLContext *ctx = getContext();

	do {
		name = fglFramebufferObjects.get(ctx);
		if(name < 0) {
			glDeleteFramebuffersOES (n - i, framebuffers);
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglFramebufferObjects[name] = NULL;
		*cur = name;
		cur++;
	} while (--i);
}

GL_API GLenum GL_APIENTRY glCheckFramebufferStatusOES (GLenum target)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return 0;
	}

	FGLContext *ctx = getContext();
	return ctx->framebuffer.status;
}

GL_API void GL_APIENTRY glFramebufferRenderbufferOES (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLRenderbuffer *rb;
	if (renderbuffer != 0) {

		if (renderbuffertarget != GL_RENDERBUFFER_OES) {
			setError(GL_INVALID_OPERATION);
			return;
		}

		if(!fglRenderbufferObjects.isValid(renderbuffer)) {
			setError(GL_INVALID_OPERATION);
			return;
		}

		FGLRenderBufferObject *obj = fglRenderbufferObjects[renderbuffer];
		if(obj == NULL) {
			return;
		}
		rb = &obj->object;
	}

	FGLContext *ctx = getContext();

	if (!ctx->framebuffer.binding.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	//WE KNOW FOR SURE FB IS NOT NULL
	FGLFramebuffer *fb = ctx->framebuffer.binding.get();
	FGLAttach *attach = NULL;
	unsigned *name;
	unsigned *type;


	switch (attachment)
	{
	case GL_COLOR_ATTACHMENT0_OES:
		name = &fb->colorName;
		type = &fb->colorType;
		attach = &fb->colorAttach;
		break;
	case GL_DEPTH_ATTACHMENT_OES:
		name = &fb->depthName;
		type = &fb->depthType;
		attach = &fb->depthAttach;
		break;
	case GL_STENCIL_ATTACHMENT_OES:
		name = &fb->stencilName;
		type = &fb->stencilType;
		attach = &fb->stencilAttach;
		break;
	}

	if (attach)
	{
		if (renderbuffer == 0) {
			*name = 0;
			*type = FGLFramebuffer::NONE;
			attach->unattach();
		}
		else {
			*name = renderbuffer;
			*type = FGLFramebuffer::RENDERBUFFER;
			rb->attach(attach);
		}

		fglUpdateFramebufferStatus(ctx, fb);
	}
}

GL_API void GL_APIENTRY glFramebufferTexture2DOES (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetFramebufferAttachmentParameterivOES (GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	FUNC_UNIMPLEMENTED;
}

/**
	Attach callbacks
*/

void fglColorAttachDeleted(void *obj)
{
	FGLFramebuffer *fbo = static_cast<FGLFramebuffer*>(obj);
	fbo->colorName = 0;
	fbo->colorType = FGLFramebuffer::NONE;

	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		fglUpdateFramebufferStatus(ctx, fbo);
	}
}

void fglColorAttachChanged(void *obj)
{
	FGLFramebuffer *fbo = static_cast<FGLFramebuffer*>(obj);

	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		fglUpdateFramebufferStatus(ctx, fbo);
	}
}

void fglColorDepthDeleted(void *obj)
{
	FGLFramebuffer *fbo = static_cast<FGLFramebuffer*>(obj);
	fbo->depthName = 0;
	fbo->depthType = FGLFramebuffer::NONE;

	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		//If depth and stencil share same attachment
		//means attachment is GL_DEPTH_STENCIL_OES and
		//for don't do same work twice Framebuffer
		//status will be updated in stencil callback.
		if (!fbo->IsDepthStencilSameAttachment()) {
			fglUpdateFramebufferStatus(ctx, fbo);
		}
	}
}

void fglColorDepthChanged(void *obj)
{
	FGLFramebuffer *fbo = static_cast<FGLFramebuffer*>(obj);
	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		//If depth and stencil share same attachment
		//means attachment is GL_DEPTH_STENCIL_OES and
		//for don't do same work twice Framebuffer
		//status will be updated in stencil callback.
		if (!fbo->IsDepthStencilSameAttachment()) {
			fglUpdateFramebufferStatus(ctx, fbo);
		}
	}
}

void fglColorStencilDeleted(void *obj)
{
	FGLFramebuffer *fbo = static_cast<FGLFramebuffer*>(obj);
	fbo->stencilName = 0;
	fbo->stencilType = FGLFramebuffer::NONE;

	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		fglUpdateFramebufferStatus(ctx, fbo);
	}
}

void fglColorStencilChanged(void *obj)
{
	FGLFramebuffer *fbo = static_cast<FGLFramebuffer*>(obj);
	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		fglUpdateFramebufferStatus(ctx, fbo);
	}
}

/**
	FGLAttach & FGLAttachable
*/

FGLAttach::FGLAttach(void *obj, void (*deleted)(void*), void (*changed)(void*)) :
	attachable(0), obj(obj), deleted(deleted), changed(changed)
{

}

inline bool FGLAttach::isAttached(void)
{
	return attachable != NULL;
}

inline void FGLAttach::unattach(void)
{
	if (!attachable)
		return;

	attachable->unattach(this);
}

inline void FGLAttach::attach(FGLAttachable *o)
{
	o->attach(this);
}

inline bool FGLAttach::sameAttachment(FGLAttach *a)
{
	return a->attachable == this->attachable;
}

FGLAttachable::FGLAttachable() : list(NULL) {}
FGLAttachable::~FGLAttachable()
{
	deleted();
	unattachAll();
}

void FGLAttachable::deleted(void)
{
	FGLAttach *a = list;

	while(a) {
		if (a->deleted) a->deleted(a->obj);
		a = a->next;
	}

	list = NULL;
}

void FGLAttachable::changed(void)
{
	FGLAttach *a = list;

	while(a) {
		if (a->changed) a->changed(a->obj);
		a = a->next;
	}

	list = NULL;
}

void FGLAttachable::unattachAll(void)
{
	FGLAttach *a = list;

	while(a) {
		a->attachable = NULL;
		a = a->next;
	}

	list = NULL;
}

void FGLAttachable::unattach(FGLAttach *a)
{
	if (!isAttached(a))
		return;

	if (a->next)
		a->next->prev = a->prev;

	if (a->prev)
		a->prev->next = a->next;
	else
		list = NULL;

	a->attachable = NULL;
}

void FGLAttachable::attach(FGLAttach *a)
{
	if(a->isAttached())
		a->unattach();

	a->next = list;
	a->prev = NULL;

	if(list)
		list->prev = a;

	list = a;
	a->attachable = this;
}

bool FGLAttachable::isAttached(FGLAttach *a)
{
	return a->attachable == this;
}
