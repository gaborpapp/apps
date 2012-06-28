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

#include "PassThrough.h"

using namespace ci;
using namespace std;

namespace mndl { namespace fx {

const char *PassThrough::sVertexShader =
	STRINGIFY(
		void main(void)
		{
			gl_Position = ftransform();
			gl_TexCoord[0] = gl_MultiTexCoord0;
		}
	);

const char *PassThrough::sFragmentShader =
	STRINGIFY(
		uniform sampler2D tex;
		uniform float fade;

		void main(void)
		{
			vec4 c = texture2D( tex, gl_TexCoord[0].st );
			gl_FragColor = c * fade;
		}
	);

PassThrough::PassThrough( int w, int h ) :
	mObj( new Obj( w, h ) ),
	Effect( "PassThrough" )
{
	mObj->mFadeValue = Paramf( "Fade", 1.f, 0.f, 1.f );
	addParam( &mObj->mFadeValue );
}


PassThrough::Obj::Obj( int w, int h ) :
	mWidth( w ),
	mHeight( h )
{
	mFbo = gl::Fbo( mWidth, mHeight );
	try
	{
		/*
		mShader = gl::GlslProg( app::loadResource( RES_VERT ),
				app::loadResource( RES_FRAG ) );
				*/
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

gl::Texture &PassThrough::Obj::process( const gl::Texture &source )
{
	Area viewport = gl::getViewport();
	gl::pushMatrices();

	// set orthographic projection with lower-left origin
	gl::setMatricesWindow( mFbo.getSize(), false );
	gl::setViewport( mFbo.getBounds() );

	// draw
	mFbo.bindFramebuffer();
	gl::clear( Color::black() );
	gl::color( Color::white() );

	mShader.bind();
	mShader.uniform( "fade", mFadeValue );
	gl::draw( source, mFbo.getBounds() );
	mShader.unbind();

	mFbo.unbindFramebuffer();

	gl::popMatrices();
	gl::setViewport( viewport );
	return mFbo.getTexture();
}

} }; // namespace mndl::fx

