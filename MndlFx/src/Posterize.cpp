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
#include <vector>

#include "cinder/app/AppBasic.h"
#include "cinder/Surface.h"

#include "Posterize.h"

using namespace ci;
using namespace std;

namespace mndl { namespace fx {

const char *Posterize::sVertexShader =
	STRINGIFY(
		void main(void)
		{
			gl_Position = ftransform();
			gl_TexCoord[0] = gl_MultiTexCoord0;
		}
	);

const char *Posterize::sFragmentShader =
	STRINGIFY(
		uniform sampler2D tex;

		uniform vec4 levels;
		uniform bool dither;
		uniform bool alpha;

		void main(void)
		{
			vec4 c = texture2D( tex, gl_TexCoord[0].st );
			float a = c.a;
			vec4 cp = c * levels;
			c = floor( cp +
					   float(dither) * dot( floor( mod( gl_FragCoord.xy, 2. ) ),
											vec2( .75, .25 ) ) *
							fract( cp ) ) / levels;
			if ( !alpha )
				c.a = a;
			gl_FragColor = c;
		}
	);

Posterize::Posterize( int w, int h ) :
	mObj( new Obj( w, h ) ),
	Effect( "Posterize" )
{
	mObj->mLevels = ParamColorA( "Levels", ColorA::hexA( 0x04040404 ) );
	mObj->mDither = Paramb( "Dither", false );
	mObj->mAlpha = Paramb( "Process alpha", false );
	addParam( &mObj->mLevels );
	addParam( &mObj->mDither );
	addParam( &mObj->mAlpha );
}


Posterize::Obj::Obj( int w, int h ) :
	mWidth( w ),
	mHeight( h )
{
	mFbo = gl::Fbo( mWidth, mHeight );
	try
	{
		mShader = gl::GlslProg( sVertexShader, sFragmentShader );
	}
	catch( gl::GlslProgCompileExc &exc )
	{
		app::console() << "Shader compile error: " << endl;
		app::console() << exc.what();
	}

	mShader.bind();
	mShader.uniform( "tex", 0 );
	mShader.unbind();
}

gl::Texture &Posterize::Obj::process( const gl::Texture &source )
{
	Area viewport = gl::getViewport();
	gl::pushMatrices();

	// set orthographic projection with lower-left origin
	gl::setMatricesWindow( mFbo.getSize(), false );
	gl::setViewport( mFbo.getBounds() );

	// draw
	mFbo.bindFramebuffer();
	gl::clear( ColorA::black() );
	gl::color( Color::white() );

	mShader.bind();

	Vec4f levels( ColorA( mLevels ).ptr() );
	mShader.uniform( "levels", levels * 255 );
	mShader.uniform( "dither", mDither );
	mShader.uniform( "alpha", mAlpha );
	gl::draw( source, mFbo.getBounds() );
	mShader.unbind();

	mFbo.unbindFramebuffer();

	gl::popMatrices();
	gl::setViewport( viewport );
	return mFbo.getTexture();
}

} }; // namespace mndl::fx

