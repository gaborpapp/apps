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

#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Channel.h"
#include "cinder/Cinder.h"

namespace mndl { namespace gl { namespace fx {

class KawaseHBloom
{
	public:
		KawaseHBloom( int w, int h );
		KawaseHBloom() {}

		ci::gl::Texture & process( const ci::gl::Texture &source,
				int iterations, const ci::Channel32f &rowStrength );

	protected:
		struct Obj
		{
			Obj() {}
			Obj( int w, int h );

			ci::gl::Fbo mBloomFbo;
			ci::gl::Fbo mOutputFbo;

			ci::gl::GlslProg mBloomShader;
			ci::gl::GlslProg mMixerShader;
		};
		std::shared_ptr< Obj > mObj;

		static const char *sBloomVertexShader;
		static const char *sBloomFragmentShader;
		static const char *sMixerVertexShader;
		static const char *sMixerFragmentShader;
};

} } } // namespace cinder::gl::ip

