#ifndef _LIBSGL_FGLATTACH_
#define _LIBSGL_FGLATTACH_

class FGLAttach;
class FGLAttachable
{
	FGLAttach *list;
public:

	FGLAttachable();
	~FGLAttachable();
	void deleted(void);
	void changed(void);
	inline void unattachAll(void);
	inline void unattach(FGLAttach *a);
	inline void attach(FGLAttach *a);
	inline bool isAttached(FGLAttach *a);
	friend class FGLAttach;
};

class FGLAttach
{
	FGLAttachable *attachable;
	FGLAttach *next;
	FGLAttach *prev;

	void *obj;
	void (*deleted)(void*);
	void (*changed)(void*);

public:
	FGLAttach(void *obj, void (*deleted)(void*), void (*changed)(void*));

	inline bool isAttached(void);
	inline void unattach(void);
	inline void attach(FGLAttachable *o);
	inline FGLAttachable *get() {return attachable;}

	friend class FGLAttachable;
};



#endif // FGLATTACH_H
