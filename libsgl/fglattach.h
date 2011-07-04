#ifndef _LIBSGL_FGLATTACH_
#define _LIBSGL_FGLATTACH_

class FGLAttach;

class FGLAttach {
	FGLAttachable *attachable;
	FGLAttach *next;
	FGLAttach *prev;

	void *obj;
	void (*deleted)(void*);
	void (*changed)(void*);

public:
	FGLAttach(void *obj, void (*deleted)(void*), void (*changed)(void*)) :
		attachable(0), obj(obj), deleted(deleted), changed(changed)  {};

	inline bool isAttached(void)
	{
		return attachable != NULL;
	}

	inline void unattach(void)
	{
		if (!attachable)
			return;

		attachable->unattach(this);
	}

	inline void attach(FGLAttachable *o)
	{
		o->attach(this);
	}

	friend class FGLAttachable;
};

class FGLAttachable {
	FGLAttach *list;

public:

	FGLAttachable(unsigned int id) :
		list(NULL) {};

	~FGLAttachable()
	{
		deleted();
		unattachAll();
	}

	inline void deleted(void)
	{
		FGLAttach *b = list;

		while(b) {
			if (b->deleted) b->deleted(b->obj);
			b = b->next;
		}

		list = NULL;
	}

	inline void changed(void)
	{
		FGLAttach *b = list;

		while(b) {
			if (b->changed) b->changed(b->obj);
			b = b->next;
		}

		list = NULL;
	}

	inline void unattachAll(void)
	{
		FGLAttach *b = list;

		while(b) {
			b->attachable = NULL;
			b = b->next;
		}

		list = NULL;
	}

	inline void unattach(FGLAttach *b)
	{
		if (!isAttached(b))
			return;

		if (b->next)
			b->next->prev = b->prev;

		if (b->prev)
			b->prev->next = b->next;
		else
			list = NULL;

		b->attachable = NULL;
	}

	inline void attach(FGLAttach *b)
	{
		if(b->isBound())
			b->unbind();

		b->next = list;
		b->prev = NULL;

		if(list)
			list->prev = b;

		list = b;
		b->attachable = this;
	}

	inline bool isAttached(FGLAttach *b)
	{
		return b->attachable == this;
	}

	friend class FGLAttach;
};

#endif // FGLATTACH_H
