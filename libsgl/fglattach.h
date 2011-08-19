#ifndef _LIBSGL_FGLATTACH_
#define _LIBSGL_FGLATTACH_

#include "eglMem.h"
#include "fglobject.h"

#define FGL_COLOR0_ATTACHABLE  (1<<0)
#define FGL_DEPTH_ATTACHABLE   (1<<1)
#define FGL_STENCIL_ATTACHABLE (1<<2)

class FGLAttach;

/**
  FGLAttachable are objects that can be attached by a Framebuffer Object
 */
class FGLAttachable
{
	FGLAttach *list;
public:
	/* Memory surface */
	FGLSurface  *surface;

	/* GL state */
	GLint        width;
	GLint        height;
	unsigned     attachmentMask;

	/* HW state */
	uint32_t     fglFbFormat;
	uint32_t     bpp;
	bool         swap;
	bool         swapAlpha;

	FGLAttachable();
	virtual ~FGLAttachable();

	void deleted(void);
	void changed(void);

	//FBO notifies to attachable that it can do internal
	//updates like GenMipmaps because of modifications.
	virtual void updateAttachable() {};

	inline void unattachAll(void);
	inline void unattach(FGLAttach *a);
	inline void attach(FGLAttach *a);
	inline bool isAttached(FGLAttach *a);
	friend class FGLAttach;
};

/**
  FGLAttach
 */

class FGLFramebuffer;
class FGLAttach
{
	FGLAttachable *attachable;
	FGLAttach *next;
	FGLAttach *prev;

	typedef void (*AttachSignal)(FGLFramebuffer *);

	FGLFramebuffer *fbo;
	AttachSignal deleted;
	AttachSignal changed;

public:
	FGLAttach(FGLFramebuffer *fbo, AttachSignal deleted, AttachSignal changed);

	inline bool isAttached(void);
	inline void unattach(void);
	inline void attach(FGLAttachable *o);
	inline FGLAttachable *get() {return attachable;}

	void updateAttachable() {
		if(attachable) attachable->updateAttachable();
	}

	friend class FGLAttachable;
};



#endif // FGLATTACH_H
