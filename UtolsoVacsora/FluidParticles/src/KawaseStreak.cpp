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

#include "KawaseStreak.h"

using namespace ci;
using namespace ci::app;

namespace mndl { namespace gl { namespace fx {

#define STRINGIFY(x) #x

const char *KawaseStreak::sStreakVertexShader = "#version 120\n"
	STRINGIFY(
		void main()
		{
			gl_FrontColor = gl_Color;
			gl_TexCoord[ 0 ] = gl_MultiTexCoord0;
			gl_Position = ftransform();
		}
);

const char *KawaseStreak::sStreakFragmentShader = "#version 120\n"
	"#define NUM_SAMPLES 4\n"
	STRINGIFY(
		uniform sampler2D tex;
		uniform vec2 pixelSize;
		uniform vec2 direction;
		uniform float attenuation;
		uniform int iteration;
		void main()
		{
			vec2 texCoord = gl_TexCoord[ 0 ].st;
			vec3 cout = vec3( 0, 0, 0 );
			vec2 sampleCoord = vec2( 0, 0 );

			// sample weight = a^(b*s)
			// a = attenuation
			// b = 4^(pass-1)
			// s = sample number
			float b = pow( NUM_SAMPLES, iteration );
			for ( int s = 0; s < NUM_SAMPLES; s++ )
			{
				float weight = pow( attenuation, b * s );
				// direction = per-pixel, 2D orientation vector
				sampleCoord = texCoord + ( direction * b * vec2( s, s ) * pixelSize );
				cout += clamp( weight, 0., 1.) * texture2D( tex, sampleCoord ).rgb;
			}
			gl_FragColor = vec4( clamp( cout, 0., 1. ), texture2D( tex, texCoord ).a );
		}
);


const char *KawaseStreak::sMixerVertexShader = "#version 120\n"
	STRINGIFY(
		void main()
		{
			gl_FrontColor = gl_Color;
			gl_Position = ftransform();
			gl_TexCoord[ 0 ] = gl_MultiTexCoord0;
		}
);

const char *KawaseStreak::sMixerFragmentShader = "#version 120\n"
	STRINGIFY(
		uniform sampler2D tex[ 5 ];
		uniform float strength;

		void main()
		{
			vec2 uv = gl_TexCoord[ 0 ].st;
			vec4 c0 = texture2D( tex[ 0 ], uv );

			vec4 c1 = texture2D( tex[ 1 ], uv ) *.25;
			c1 += texture2D( tex[ 2 ], uv ) * .25;
			c1 += texture2D( tex[ 3 ], uv ) * .25;
			c1 += texture2D( tex[ 4 ], uv ) * .25;

			gl_FragColor = c0 + strength * c1;
		}
);

KawaseStreak::KawaseStreak( int w, int h ) :
	mObj(new Obj(w, h))
{
}

KawaseStreak::Obj::Obj( int w, int h )
{
	ci::gl::Fbo::Format format;
	format.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
	format.enableDepthBuffer( false );
	mOutputFbo = ci::gl::Fbo( w, h, format );

	format.enableColorBuffer( true, 8 );

	int width = w / 4;
	int height = h / 4;
	width = math< int >::max( width, 1 );
	height = math< int >::max( height, 1 );

	mStreakFbo = ci::gl::Fbo( width, height, format );

	try
	{
		mStreakShader = ci::gl::GlslProg( sStreakVertexShader, sStreakFragmentShader );
		mStreakShader.bind();
		mStreakShader.uniform( "tex", 0 );
		mStreakShader.unbind();
	}
	catch ( ci::gl::GlslProgCompileExc &exc )
	{
		ci::app::console() << "Streak shader " << exc.what() << std::endl;
		ci::app::console() << sStreakFragmentShader << std::endl;
		ci::app::App::get()->quit();
	}

	try
	{
		mMixerShader = ci::gl::GlslProg( sMixerVertexShader, sMixerFragmentShader );
		mMixerShader.bind();
		int texUnits[] = { 0, 1, 2, 3, 4 };
		mMixerShader.uniform( "tex", texUnits, 5 );
		mMixerShader.unbind();
	}
	catch ( ci::gl::GlslProgCompileExc &exc )
	{
		ci::app::console() << exc.what() << std::endl;
		ci::app::App::get()->quit();
	}
}

ci::gl::Texture &KawaseStreak::process( const ci::gl::Texture &source, float attenuation, int iterations, float strength )
{
	Area viewport = ci::gl::getViewport();
	ci::gl::pushMatrices();

	ci::gl::disableDepthRead();
	ci::gl::disableDepthWrite();

	ci::gl::color( Color::white() );

	mObj->mStreakFbo.bindFramebuffer();
	// set orthographic projection with lower-left origin
	ci::gl::setMatricesWindow( mObj->mStreakFbo.getSize(), false );
	ci::gl::setViewport( mObj->mStreakFbo.getBounds() );

	mObj->mStreakShader.bind();
	mObj->mStreakShader.uniform( "pixelSize",
			Vec2f( .1f / mObj->mStreakFbo.getWidth(), .1f / mObj->mStreakFbo.getHeight() ) );
	mObj->mStreakShader.uniform( "attenuation", attenuation );

	for ( int d = 0; d < 4; d++ ) // for all 4 directions
	{
		Vec2f dir( 1.f, 1.f );
		dir.rotate( d * M_PI / 2.f );
		mObj->mStreakShader.uniform( "direction", dir.normalized() );

		source.bind();
		for ( int i = 0; i < iterations; i++ )
		{
			glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + 2 * d + ( i & 1 ) );

			ci::gl::clear();

			mObj->mStreakShader.uniform( "iteration", i );

			ci::gl::drawSolidRect( mObj->mStreakFbo.getBounds() );
			mObj->mStreakFbo.bindTexture( 0, 2 * d + ( i & 1 ) );
		}
	}
	mObj->mStreakShader.unbind();

	source.unbind();

	// output
	mObj->mOutputFbo.bindFramebuffer();
	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );

	ci::gl::setMatricesWindow( mObj->mOutputFbo.getSize(), false );
	ci::gl::setViewport( mObj->mOutputFbo.getBounds() );

	ci::gl::enableAlphaBlending();

	mObj->mMixerShader.bind();
	mObj->mMixerShader.uniform( "strength", strength );

	source.bind();
	for ( int d = 0; d < 4; d++ ) // for all 4 directions
		mObj->mStreakFbo.bindTexture( 1 + d, 2 * d + ( ( iterations - 1 ) & 1 ) );

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

