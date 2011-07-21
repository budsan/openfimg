/**
 * libsgl/fgltextureobject.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:	2010 by Tomasz Figa < tomasz.figa at gmail.com >
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LIBSGL_FGLTEXTUREOBJECT_
#define _LIBSGL_FGLTEXTUREOBJECT_

#include "eglMem.h"
#include "fglobject.h"
#include "fglattach.h"

struct FGLTexture : public FGLAttachable {
	/* GL state */
	GLboolean	compressed;
	GLint		levels;
	GLint		maxLevel;
	GLenum		format;
	GLenum		type;
	GLenum		minFilter;
	GLenum		magFilter;
	GLenum		sWrap;
	GLenum		tWrap;
	GLboolean	genMipmap;
	GLboolean	useMipmap;
	GLboolean	redoMipmap;
	GLint		cropRect[4];
	void*		eglImage;
	/* HW state */
	fimgTexture	*fimg;
	uint32_t	fglFormat;
	bool		convert;
	bool		valid;
	bool		dirty;

	FGLTexture() :
		compressed(0), levels(0), maxLevel(0), format(GL_RGB),
		type(GL_UNSIGNED_BYTE), minFilter(GL_NEAREST_MIPMAP_LINEAR),
		magFilter(GL_LINEAR), sWrap(GL_REPEAT), tWrap(GL_REPEAT),
		genMipmap(0), useMipmap(GL_TRUE), redoMipmap(0), eglImage(0),
		fimg(NULL), valid(false), dirty(false)
	{
		fimg = fimgCreateTexture();
		if(fimg == NULL)
			return;

		valid = true;
	}

	~FGLTexture()
	{
		if(!isValid())
			return;

		fimgDestroyTexture(fimg);
	}

	inline bool isValid(void)
	{
		return valid;
	}

	inline bool isComplete(void)
	{
		if (!useMipmap)
			return levels & 1;

		return levels == ((1 << (maxLevel + 1)) - 1);
	}

	inline void texelsChanged()
	{
		redoMipmap = genMipmap || redoMipmap;
	}

	void updateAttachable();
};

void fglGenerateMipmaps(FGLTexture *obj);

typedef FGLObject<FGLTexture> FGLTextureObject;
typedef FGLObjectBinding<FGLTexture> FGLTextureObjectBinding;

#endif
