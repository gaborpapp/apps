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

#include "LumaOffset.h"

using namespace ci;
using namespace std;

namespace mndl { namespace fx {

const char *LumaOffset::sVertexShader =
	STRINGIFY(
		uniform sampler2D tex;

		uniform float offsetScale;

		void main(void)
		{
			vec2 uv = gl_MultiTexCoord0.st;
			vec3 c = texture2D( tex, uv ).rgb;
			float grey = dot( c, vec3( .3, .59, .11 ) );

			vec4 vertex = gl_Vertex;
			vertex.y += grey * offsetScale;

			gl_Position = gl_ModelViewProjectionMatrix * vertex;
			gl_TexCoord[0] = gl_MultiTexCoord0;
		}
	);

const char *LumaOffset::sFragmentShader =
	STRINGIFY(
		uniform sampler2D tex;

		void main(void)
		{
			vec2 uv = gl_TexCoord[0].st;
			vec4 c = texture2D( tex, uv );
			gl_FragColor = c;
		}
	);

LumaOffset::LumaOffset( int w, int h ) :
	mObj( new Obj( w, h, Effect::mParams ) ),
	Effect( "LumaOffset" )
{
}


LumaOffset::Obj::Obj( int w, int h, shared_ptr< Effect::Params > params ) :
	mParams( params ),
	mWidth( w ),
	mHeight( h )
{
	mOffsetScale = ParameterFloat( "Offset scale", -64.f, -w, w );
	mLineGap = ParameterInt( "Line gap", 2, 1, h );
	mPointSize = ParameterFloat( "Poiunt size", 1, .5, 100 );
	mFilled = ParameterBool( "Filled", false );
	mSmooth = ParameterBool( "Smooth", false );

	mParams->addParameter( &mOffsetScale );
	mParams->addParameter( &mLineGap );
	mParams->addParameter( &mPointSize );
	mParams->addParameter( &mFilled );
	mParams->addParameter( &mSmooth );

	mFbo = gl::Fbo( mWidth, mHeight );
	try
	{
		/*
		mShader = gl::GlslProg( app::loadResource( RES_LUMAOFFSET_VERT ),
				app::loadResource( RES_LUMAOFFSET_FRAG ) );
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

void LumaOffset::Obj::createMesh()
{
	vector< Vec2f > texCoords;
	vector< Vec3f > vertices;
	vector< uint32_t > indices;

	gl::VboMesh::Layout layout;
	layout.setStaticIndices();
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticNormals();

	int i = 0;
	float uStep = 1. / static_cast< float >( mWidth );
	float vStep = mLineGap / static_cast< float >( mHeight );

	if ( !mSmooth && mFilled )
	{
		float u = 0;
		for ( int x = 0; x < mWidth; x++ )
		{
			float v = 0;
			for ( int y = 0; y < mHeight + mLineGap; y += mLineGap, i += 2 )
			{
				vertices.push_back( Vec3f( x, y, 0 ) );
				vertices.push_back( Vec3f( x, y + mLineGap, 0 ) );
				texCoords.push_back( Vec2f( u, v ) );
				texCoords.push_back( Vec2f( u, v ) );
				indices.push_back( i );
				indices.push_back( i + 1);

				v += vStep;
			}
			u += uStep;
		}
	}
	else
	{
		float u = 0;
		for ( int x = 0; x < mWidth; x++ )
		{
			float v = 0;
			for ( int y = 0; y < mHeight + mLineGap; y += mLineGap, i++ )
			{
				vertices.push_back( Vec3f( x, y, 0 ) );
				texCoords.push_back( Vec2f( u, v ) );
				indices.push_back( i );

				v += vStep;
			}
			u += uStep;
		}
	}

	if ( mFilled )
		mVboMesh = gl::VboMesh( vertices.size(), indices.size(), layout, GL_LINE_STRIP );
	else
		mVboMesh = gl::VboMesh( vertices.size(), indices.size(), layout, GL_POINTS );

	mVboMesh.bufferPositions( vertices );
	mVboMesh.bufferTexCoords2d( 0, texCoords );
	mVboMesh.bufferIndices( indices );
	mVboMesh.unbindBuffers();
}

gl::Texture &LumaOffset::Obj::process( const gl::Texture &source )
{
	Area viewport = gl::getViewport();
	gl::pushMatrices();

	// set orthographic projection with lower-left origin
	gl::setMatricesWindow( mFbo.getSize(), false );
	gl::setViewport( mFbo.getBounds() );

	createMesh();

	// draw
	glPointSize( mPointSize );
	mFbo.bindFramebuffer();
	gl::clear( Color::black() );
	gl::color( Color::white() );
	mShader.bind();
	mShader.uniform( "offsetScale", mOffsetScale );

	gl::draw( mVboMesh );
	mShader.unbind();
	mFbo.unbindFramebuffer();
	glPointSize( 1.0 );

	gl::popMatrices();
	gl::setViewport( viewport );
	return mFbo.getTexture();
}

} }; // namespace mndl::fx

