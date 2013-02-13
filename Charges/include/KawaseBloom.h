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

#ifndef KAWASE_BLOOM_H
#define KAWASE_BLOOM_H

#include "cinder/Cinder.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"

namespace mndl { namespace gl { namespace fx {

class KawaseBloom
{
	public:
		KawaseBloom( int w, int h );
		KawaseBloom() {}

		ci::gl::Texture & process( const ci::gl::Texture &source,
				int iterations, float strength );

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

} } } // namespace mndl::gl::fx

#endif

