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
#include <iostream>

#include "cinder/app/AppBasic.h"
#include "cinder/Surface.h"
#include "cinder/CinderMath.h"

#include "KawaseBloom.h"

using namespace ci;
using namespace ci::app;

namespace mndl { namespace gl { namespace fx {

#define STRINGIFY(x) #x

const char *KawaseBloom::sBloomVertexShader = "#version 120\n"
	STRINGIFY(
		uniform vec2 pixelSize;
		uniform int iteration;

		varying vec2 topLeft;
		varying vec2 topRight;
		varying vec2 bottomLeft;
		varying vec2 bottomRight;

		void main()
		{
			gl_FrontColor = gl_Color;
			vec2 st = gl_MultiTexCoord0.st;

			vec2 halfPixelSize = pixelSize / 2.;
			vec2 dst = pixelSize * iteration + halfPixelSize;

			topLeft = vec2( st.s - dst.s, st.t + dst.t );
			topRight = vec2( st.s + dst.s, st.t + dst.t );
			bottomLeft = vec2( st.s - dst.s, st.t - dst.t );
			bottomRight = vec2( st.s + dst.s, st.t - dst.t );

			gl_Position = ftransform();
		}
);

const char *KawaseBloom::sBloomFragmentShader = "#version 120\n"
	STRINGIFY(
		uniform sampler2D tex;

		varying vec2 topLeft;
		varying vec2 topRight;
		varying vec2 bottomLeft;
		varying vec2 bottomRight;

		void main()
		{
			vec4 accum;

			accum = texture2D( tex, topLeft );
			accum += texture2D( tex, topRight );
			accum += texture2D( tex, bottomLeft );
			accum += texture2D( tex, bottomRight );

			accum *= 0.25;

			gl_FragColor = accum;
		}
);


const char *KawaseBloom::sMixerVertexShader = "#version 120\n"
	STRINGIFY(
		void main()
		{
			gl_FrontColor = gl_Color;
			gl_Position = ftransform();
			gl_TexCoord[0] = gl_MultiTexCoord0;
		}
);

const char *KawaseBloom::sMixerFragmentShader = "#version 120\n"
	STRINGIFY(
		uniform sampler2D tex[9];
		uniform float bloomStrength;

		void main()
		{
			vec2 uv = gl_TexCoord[0].st;
			vec4 c0 = texture2D( tex[0], uv );

			vec4 c1 = texture2D( tex[1], uv );
			c1 += texture2D( tex[2], uv );
			c1 += texture2D( tex[3], uv );
			c1 += texture2D( tex[4], uv );
			c1 += texture2D( tex[5], uv );
			c1 += texture2D( tex[6], uv );
			c1 += texture2D( tex[7], uv );
			c1 += texture2D( tex[8], uv );

			gl_FragColor = c0 + bloomStrength * c1;
		}
);

KawaseBloom::KawaseBloom( int w, int h ) :
	mObj(new Obj(w, h))
{
}

KawaseBloom::Obj::Obj( int w, int h )
{
	ci::gl::Fbo::Format format;
	format.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
	format.enableDepthBuffer( false );
	mOutputFbo = ci::gl::Fbo( w, h, format );

	format.enableColorBuffer( true, 8 );

	mBloomFbo = ci::gl::Fbo( w / 4, h / 4, format );

	mBloomShader = ci::gl::GlslProg( sBloomVertexShader, sBloomFragmentShader );
	mBloomShader.bind();
	mBloomShader.uniform( "tex", 0 );
	mBloomShader.uniform( "pixelSize", Vec2f( 1. / mBloomFbo.getWidth(), 1. / mBloomFbo.getHeight() ) );
	mBloomShader.unbind();

	mMixerShader = ci::gl::GlslProg( sMixerVertexShader, sMixerFragmentShader );
	mMixerShader.bind();
	int texUnits[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	mMixerShader.uniform( "tex", texUnits, 9 );
	mMixerShader.unbind();
}

ci::gl::Texture &KawaseBloom::process( const ci::gl::Texture &source, int iterations, float strength )
{
	Area viewport = ci::gl::getViewport();
	ci::gl::pushMatrices();

	// bloom
	mObj->mBloomFbo.bindFramebuffer();

	// set orthographic projection with lower-left origin
	ci::gl::setMatricesWindow( mObj->mBloomFbo.getSize(), false );
	ci::gl::setViewport( mObj->mBloomFbo.getBounds() );

	ci::gl::color( Color::white() );
	source.enableAndBind();

	mObj->mBloomShader.bind();
	iterations = math< int >::min( iterations, 8 );
	for ( int i = 0; i < iterations; i++ )
	{
		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + i );
		mObj->mBloomShader.uniform( "iteration", i );
		ci::gl::drawSolidRect( mObj->mBloomFbo.getBounds() );
		mObj->mBloomFbo.bindTexture( 0, i );
	}
	for ( int i = iterations; i < 8; i++ )
	{
		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + i );
		ci::gl::clear( Color::black() );
	}

	mObj->mBloomShader.unbind();

	source.disable();

	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
	mObj->mBloomFbo.unbindFramebuffer();

	// output
	mObj->mOutputFbo.bindFramebuffer();
	ci::gl::setMatricesWindow( mObj->mOutputFbo.getSize(), false );
	ci::gl::setViewport( mObj->mOutputFbo.getBounds() );

	ci::gl::enableAlphaBlending();

	mObj->mMixerShader.bind();
	mObj->mMixerShader.uniform( "bloomStrength", strength );

	source.enableAndBind();
	for ( int i = 0; i < iterations; i++ )
	{
		mObj->mBloomFbo.getTexture( i ).bind( i + 1 );
	}

	ci::gl::drawSolidRect( mObj->mOutputFbo.getBounds() );
	mObj->mMixerShader.unbind();
	source.disable();

	mObj->mOutputFbo.unbindFramebuffer();

	ci::gl::disableAlphaBlending();

	ci::gl::popMatrices();
	ci::gl::setViewport(viewport);

	return mObj->mOutputFbo.getTexture();
}

} } } // namespace mndl::gl::fx

