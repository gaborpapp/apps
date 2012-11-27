/*
 Copyright (C) 2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"

namespace mndl { namespace gl {

class CubeMap
{
	public:
		CubeMap() {}

		CubeMap( const ci::ImageSourceRef &pos_x, const ci::ImageSourceRef &pos_y,
				const ci::ImageSourceRef &pos_z, const ci::ImageSourceRef &neg_x,
				const ci::ImageSourceRef &neg_y, const ci::ImageSourceRef &neg_z,
				const ci::gl::Texture::Format &format = ci::gl::Texture::Format() );

		GLint getWidth() const;
		GLint getHeight() const;

		void enableAndBind() const;
		void bind( GLuint textureUnit = 0 ) const;
		void unbind( GLuint textureUnit = 0 ) const;
		void disable() const;

	private:
		struct Obj
		{
			Obj() : mWidth( -1 ), mHeight( -1 ), mInternalFormat( -1 ), mTextureID( 0 ) {}
			~Obj();

			mutable GLint mWidth, mHeight;
			mutable GLint mInternalFormat;
			float mMaxU, mMaxV;
			GLenum mTarget;
			GLuint mTextureID;
		};
		std::shared_ptr<Obj> mObj;

	public:
		//@{
		//! Emulates shared_ptr-like behavior
		typedef std::shared_ptr<Obj> CubeMap::*unspecified_bool_type;
		operator unspecified_bool_type() const { return ( mObj.get() == 0 ) ? 0 : &CubeMap::mObj; }
		void reset() { mObj.reset(); }
		//@}
};

void enableReflectionMapping();
void disableReflectionMapping();

} } // namespace mndl::gl

