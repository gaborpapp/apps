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

#include "cinder/Cinder.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"

#include "Effect.h"

namespace mndl { namespace fx {

class LumaOffset : public Effect
{
	public:
		LumaOffset() {}
		LumaOffset( int w, int h );

		ci::gl::Texture &process( const ci::gl::Texture &source )
		{
			return mObj->process( source );
		}

	protected:
		struct Obj
		{
			Obj( int w, int h );

			ci::gl::Texture &process( const ci::gl::Texture &source );

			void createMesh();

			const int mWidth;
			const int mHeight;

			ci::gl::Fbo mFbo;
			ci::gl::VboMesh mVboMesh;
			ci::gl::GlslProg mShader;

			Paramf mOffsetScale;
			Parami mLineGap;
			Paramf mPointSize;
			Paramb mFilled;
			Paramb mSmooth;
		};
		std::shared_ptr< Obj > mObj;

		static const char *sVertexShader;
		static const char *sFragmentShader;
};

} } // namespace mndl::fx

