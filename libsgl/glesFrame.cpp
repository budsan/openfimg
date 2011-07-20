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

inline void fglSetDefaultFramebuffer(FGLContext *ctx)
{
	//FUNCTION_TRACER;
	glFinish();

	memcpy(&ctx->surface, &ctx->framebuffer.defBuffer, sizeof(FGLSurfaceState));

	ctx->framebuffer.externalBufferInUse = false;
	ctx->framebuffer.status = GL_FRAMEBUFFER_COMPLETE_OES;
	fglSetCurrentBuffers(ctx);
}

bool fglIsFramebufferAttachmentComplete(FGLAttach *attach, unsigned mask, unsigned &w, unsigned &h)
{
	if (!attach->isAttached()) {
		h = w = 0;
		return true;
	}

	FGLAttachable *att = attach->get();
	if (att->width == 0 || att->height == 0) {
		return false;
	}

	if (!(att->attachmentMask & mask)) {
		return false;
	}

	w = att->width;
	h = att->height;

	return true;
}

void fglUpdateFramebufferStatus(FGLContext *ctx, FGLFramebuffer* fbo)
{
	//FUNCTION_TRACER;
	struct {
		FGLAttach *attach;
		unsigned   mask;
	} atts[3] = {
		{&fbo->colorAttach,   FGL_COLOR0_ATTACHABLE },
		{&fbo->depthAttach,   FGL_DEPTH_ATTACHABLE  },
		{&fbo->stencilAttach, FGL_STENCIL_ATTACHABLE}
	};

	unsigned fw = 0, fh = 0;
	for (unsigned int i = 0 ; i < 3; i++) {
		unsigned w = 0, h = 0;
		if (!fglIsFramebufferAttachmentComplete(atts[i].attach, atts[i].mask, w, h)) {
			ctx->framebuffer.status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			return;
		}

		if (  w == 0 ) continue;
		if ( fw == 0 ) {
			fw = w;
			fh = h;
		}
		else {
			if (fw != w || fh != h) {
				// an attachment have different sizes
				ctx->framebuffer.status = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES;
				return;
			}
		}
	}

	// if fw still being 0 it means there is no attachment
	if ( fw == 0 ) {
		ctx->framebuffer.status = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES;
		return;
	}

	glFinish();

	FGLSurfaceState &curr = ctx->surface;
	if(fbo->depthAttach.isAttached() && fbo->stencilAttach.isAttached()) {
		if (fbo->IsDepthStencilSameAttachment()) { //GL_DEPTH_STENCIL_OES
			curr.depth       = fbo->depthAttach.get()->surface;
			curr.depthFormat = fbo->depthAttach.get()->fglFbFormat;
		}
		else  {
			ctx->framebuffer.status = GL_FRAMEBUFFER_UNSUPPORTED_OES;
			return;
		}
	}
	else if( fbo->depthAttach.isAttached() && !fbo->stencilAttach.isAttached()) {
		curr.depth       = fbo->depthAttach.get()->surface;
		curr.depthFormat = fbo->depthAttach.get()->fglFbFormat;
	}
	else if(!fbo->depthAttach.isAttached() &&  fbo->stencilAttach.isAttached()) {
		curr.depth       = fbo->stencilAttach.get()->surface;
		curr.depthFormat = fbo->stencilAttach.get()->fglFbFormat;
	}
	else {
		curr.depth = 0;
		curr.depthFormat = 0;
	}

	if (fbo->colorAttach.isAttached()) {
		curr.draw  = fbo->colorAttach.get()->surface;
		curr.format = fbo->colorAttach.get()->fglFbFormat;
	}
	else {
		curr.draw  = 0;
		curr.format = 0;
	}

	curr.stride = fw;
	curr.width  = fw;
	curr.height = fh;

	ctx->framebuffer.externalBufferInUse = true;
	ctx->framebuffer.status = GL_FRAMEBUFFER_COMPLETE_OES; //hurray!
	fglSetCurrentBuffers(ctx);
}

static int fglGetRenderbufferFormatInfo(GLenum format, unsigned *bpp, unsigned *attachment, bool *swap)
{
	*attachment = 0;
	*swap = 0;

	switch (format) {
	case GL_RGBA4_OES: //REQUIRED
		*bpp = 2;
		*attachment = FGL_COLOR0_ATTACHABLE;
		return FGPF_COLOR_MODE_4444;
	case GL_RGB5_A1_OES: //REQUIRED
		*bpp = 2;
		*attachment = FGL_COLOR0_ATTACHABLE;
		return FGPF_COLOR_MODE_1555;
	case GL_RGB565_OES: //REQUIRED
		*bpp = 2;
		*attachment = FGL_COLOR0_ATTACHABLE;
		return FGPF_COLOR_MODE_565;
	case GL_RGBA:
	case GL_RGBA8_OES:// OPTIONAL
		*bpp = 4;
		*swap = 1;
		*attachment = FGL_COLOR0_ATTACHABLE;
		return FGPF_COLOR_MODE_8888;
	case GL_RGB:
	case GL_RGB8_OES: // OPTIONAL
		*bpp = 4;
		*swap = 1;
		*attachment = FGL_COLOR0_ATTACHABLE;
		return FGPF_COLOR_MODE_0888;
	case GL_DEPTH_COMPONENT16_OES: //REQUIRED
	case GL_DEPTH_COMPONENT24_OES: //OPTIONAL
		*bpp = 4;
		*attachment = FGL_DEPTH_ATTACHABLE;
		return 24;
	case GL_DEPTH_COMPONENT32_OES: //OPTIONAL
		return -1;
	case GL_STENCIL_INDEX1_OES: //OPTIONAL
	case GL_STENCIL_INDEX4_OES: //OPTIONAL
	case GL_STENCIL_INDEX8_OES: //OPTIONAL
		*bpp = 4;
		*attachment = FGL_STENCIL_ATTACHABLE;
		return (8 << 8);
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
	//FUNCTION_TRACER;
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
	//FUNCTION_TRACER;
	if (target != GL_RENDERBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if (width > FGL_MAX_TEXTURE_SIZE || height > FGL_MAX_TEXTURE_SIZE) {
		setError(GL_INVALID_VALUE);
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
	int fglFbFormat = fglGetRenderbufferFormatInfo(internalformat, &bpp, &attachment, &swap);
	if (fglFbFormat < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if (width != obj->width || height != obj->height || bpp != obj->bpp) {
		delete obj->surface;
		obj->surface = 0;
	}

	if (!obj->surface) {
		obj->width = width;
		obj->height = height;
		obj->format = internalformat;
		obj->fglFbFormat = fglFbFormat;
		obj->bpp = bpp;
		obj->attachmentMask = attachment;
		obj->swap = swap;
		unsigned size = width * height * bpp;

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
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLContext *ctx = getContext();

	if(!ctx->renderbuffer.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLRenderbuffer *obj = ctx->renderbuffer.get();

	switch (pname)
	{
	case GL_RENDERBUFFER_WIDTH_OES:
		*params = obj->width;
		return;
	case GL_RENDERBUFFER_HEIGHT_OES:
		*params = obj->height;
		return;
	case GL_RENDERBUFFER_INTERNAL_FORMAT_OES:
		*params = obj->format;
		return;
	}

	if (obj->attachmentMask&FGL_COLOR0_ATTACHABLE)
	{
		switch (pname)
		{
		case GL_RENDERBUFFER_RED_SIZE_OES:
			*params = fglColorConfigs[obj->fglFbFormat].red;
			return;
		case GL_RENDERBUFFER_GREEN_SIZE_OES:
			*params = fglColorConfigs[obj->fglFbFormat].green;
			return;
		case GL_RENDERBUFFER_BLUE_SIZE_OES:
			*params = fglColorConfigs[obj->fglFbFormat].blue;
			return;
		case GL_RENDERBUFFER_ALPHA_SIZE_OES:
			*params = fglColorConfigs[obj->fglFbFormat].alpha;
			return;
		case GL_RENDERBUFFER_DEPTH_SIZE_OES:
		case GL_RENDERBUFFER_STENCIL_SIZE_OES:
			*params = 0;
			return;
		}
	}
	else if (obj->attachmentMask&FGL_DEPTH_ATTACHABLE)
	{
		switch (pname)
		{
		case GL_RENDERBUFFER_RED_SIZE_OES:
		case GL_RENDERBUFFER_GREEN_SIZE_OES:
		case GL_RENDERBUFFER_BLUE_SIZE_OES:
		case GL_RENDERBUFFER_ALPHA_SIZE_OES:
		case GL_RENDERBUFFER_STENCIL_SIZE_OES:
			*params = 0;
			return;
		case GL_RENDERBUFFER_DEPTH_SIZE_OES:
			*params = obj->fglFbFormat&0xff;
			return;
		}
	}
	else if (obj->attachmentMask&FGL_STENCIL_ATTACHABLE)
	{
		switch (pname)
		{
		case GL_RENDERBUFFER_RED_SIZE_OES:
		case GL_RENDERBUFFER_GREEN_SIZE_OES:
		case GL_RENDERBUFFER_BLUE_SIZE_OES:
		case GL_RENDERBUFFER_ALPHA_SIZE_OES:
		case GL_RENDERBUFFER_DEPTH_SIZE_OES:
			*params = 0;
			return;
		case GL_RENDERBUFFER_STENCIL_SIZE_OES:
			*params = obj->fglFbFormat >> 8;
			return;
		}
	}
	else //This means Renderbuffer didn't be init
	{
		//Not sure what this should return in this case
		switch (pname)
		{
		case GL_RENDERBUFFER_RED_SIZE_OES:
		case GL_RENDERBUFFER_GREEN_SIZE_OES:
		case GL_RENDERBUFFER_BLUE_SIZE_OES:
		case GL_RENDERBUFFER_ALPHA_SIZE_OES:
		case GL_RENDERBUFFER_DEPTH_SIZE_OES:
		case GL_RENDERBUFFER_STENCIL_SIZE_OES:
			*params = 0;
			return;
		}
	}

	setError(GL_INVALID_ENUM);
	return;
}

GL_API GLboolean GL_APIENTRY glIsFramebufferOES (GLuint framebuffer)
{
	if (framebuffer == 0 || !fglFramebufferObjects.isValid(framebuffer))
		return GL_FALSE;

	return GL_TRUE;
}

GL_API void GL_APIENTRY glBindFramebufferOES (GLenum target, GLuint framebuffer)
{
	//FUNCTION_TRACER;
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if(framebuffer == 0) {
		FGLContext *ctx = getContext();
		if (ctx->framebuffer.binding.isBound()) {
			ctx->framebuffer.binding.get()->updateAttaches();
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

	//This could be easier using some callback unbind
	if (ctx->framebuffer.binding.isBound())
		ctx->framebuffer.binding.get()->updateAttaches();

	obj->bind(&ctx->framebuffer.binding);
	fglUpdateFramebufferStatus(ctx,& obj->object);
}

GL_API void GL_APIENTRY glDeleteFramebuffersOES (GLsizei n, const GLuint* framebuffers)
{
	//FUNCTION_TRACER;
	unsigned name;

	if(n <= 0)
		return;

	FGLContext *ctx = getContext();
	FGLFramebuffer *curr = ctx->framebuffer.binding.get();

	do {
		name = *framebuffers;
		framebuffers++;

		if(!fglFramebufferObjects.isValid(name)) {
			LOGD("Tried to free invalid framebuffer %d", name);
			continue;
		}

		//This could be CLEANER using callbacks, this is crappy
		FGLFramebufferObject *obj = fglFramebufferObjects[name];
		if (curr == &obj->object) curr->updateAttaches();

		delete (obj);
		fglFramebufferObjects.put(name);
	} while (--n);

	//Here we're checking if this some deleted framebuffer was bound
	//If it was, we must set default framebuffer.

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

GL_API void GL_APIENTRY glFramebufferRenderbufferOES(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	//FUNCTION_TRACER;
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLRenderbuffer *rb = NULL;
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
		if (rb) {
			*name = renderbuffer;
			*type = FGLFramebuffer::RENDERBUFFER;
			rb->attach(attach);
		}
		else {
			*name = 0;
			*type = FGLFramebuffer::NONE;
			attach->unattach();
		}

		fglUpdateFramebufferStatus(ctx, fb);
	}
}

GL_API void GL_APIENTRY glFramebufferTexture2DOES (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	//FUNCTION_TRACER;
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLTexture *tx = NULL;
	if (texture != 0) {

		if (textarget != GL_TEXTURE_2D) {
			setError(GL_INVALID_OPERATION);
			return;
		}

		if (level != 0) {
			setError(GL_INVALID_VALUE);
			return;
		}

		if(!fglTextureObjects.isValid(texture)) {
			setError(GL_INVALID_OPERATION);
			return;
		}

		FGLTextureObject *obj = fglTextureObjects[texture];
		if(obj == NULL) {
			return;
		}
		tx = &obj->object;
	}

	FGLContext *ctx = getContext();

	if (!ctx->framebuffer.binding.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

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
		if (tx) {
			*name = texture;
			*type = FGLFramebuffer::TEXTURE;
			tx->attach(attach);
		}
		else {
			*name = 0;
			*type = FGLFramebuffer::NONE;
			attach->unattach();
		}

		fglUpdateFramebufferStatus(ctx, fb);
	}
}

GL_API void GL_APIENTRY glGetFramebufferAttachmentParameterivOES (GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLContext *ctx = getContext();

	if (!ctx->framebuffer.binding.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLFramebuffer *fb = ctx->framebuffer.binding.get();
	FGLAttach *attach;
	unsigned name;
	unsigned type;
	switch (attachment)
	{
	case GL_COLOR_ATTACHMENT0_OES:
		name = fb->colorName;
		type = fb->colorType;
		attach = &fb->colorAttach;
		break;
	case GL_DEPTH_ATTACHMENT_OES:
		name = fb->depthName;
		type = fb->depthType;
		attach = &fb->depthAttach;
		break;
	case GL_STENCIL_ATTACHMENT_OES:
		name = fb->stencilName;
		type = fb->stencilType;
		attach = &fb->stencilAttach;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	switch(pname)
	{
	case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES: {
		static GLenum typeConv[] = {
			GL_NONE_OES,
			GL_TEXTURE_2D,
			GL_RENDERBUFFER_OES
		};
		*params = typeConv[type];
		return; }
	case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES:
		*params = name;
		return;
	case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES:
	case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES:
		if (type == FGLFramebuffer::RENDERBUFFER) {
			setError(GL_INVALID_ENUM);
			return;
		}
		*params = 0;
		return;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}
}

GL_API void GL_APIENTRY glGenerateMipmapOES (GLenum target)
{
	if (target != GL_TEXTURE_2D) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	FGLTexture *obj =
		ctx->texture[ctx->activeTexture].getTexture();

	fglGenerateMipmaps(obj);
}

/*
	Attach callbacks
*/
void fglColorAttachDeleted(FGLFramebuffer *fbo)
{
	fbo->colorName = 0;
	fbo->colorType = FGLFramebuffer::NONE;

	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		fglUpdateFramebufferStatus(ctx, fbo);
	}
}

void fglColorAttachChanged(FGLFramebuffer *fbo)
{
	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		fglUpdateFramebufferStatus(ctx, fbo);
	}
}

void fglColorDepthDeleted(FGLFramebuffer *fbo)
{
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

void fglColorDepthChanged(FGLFramebuffer *fbo)
{
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

void fglColorStencilDeleted(FGLFramebuffer *fbo)
{
	fbo->stencilName = 0;
	fbo->stencilType = FGLFramebuffer::NONE;

	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		fglUpdateFramebufferStatus(ctx, fbo);
	}
}

void fglColorStencilChanged(FGLFramebuffer *fbo)
{
	FGLContext *ctx = getContext();
	if (ctx->framebuffer.binding.get() == fbo) {
		fglUpdateFramebufferStatus(ctx, fbo);
	}
}

/*
	FGLAttach & FGLAttachable code
*/
FGLAttach::FGLAttach(FGLFramebuffer *fbo, AttachSignal deleted, AttachSignal changed)
	: attachable(0), fbo(fbo), deleted(deleted), changed(changed)
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

	attachable->updateAttachable();
	attachable->unattach(this);
}

inline void FGLAttach::attach(FGLAttachable *o)
{
	o->attach(this);
}

FGLAttachable::FGLAttachable()
	: list(0), surface(0), width(0), height(0), attachmentMask(0), swap(false)
{

}

FGLAttachable::~FGLAttachable()
{
	deleted();
	unattachAll();

	delete surface;
}

void FGLAttachable::deleted(void)
{
	FGLAttach *a = list;

	while(a) {
		if (a->deleted) a->deleted(a->fbo);
		a = a->next;
	}

	list = NULL;
}

void FGLAttachable::changed(void)
{
	FGLAttach *a = list;

	while(a) {
		if (a->changed) a->changed(a->fbo);
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
