/*
 Copyright (C) 2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>

#include "cinder/app/App.h"

#include "GlobalData.h"
#include "Resources.h"

#include "Twister.h"

using namespace ci;

namespace mndl {

void Twister::setup()
{
	try
	{
		mShader = gl::GlslProg( app::loadResource( RES_TWISTER_VERT ),
								app::loadResource( RES_TWISTER_FRAG ) );
		mEyeShader = gl::GlslProg( app::loadResource( RES_TWISTER_EYE_VERT ),
								app::loadResource( RES_TWISTER_EYE_FRAG ) );
	}
	catch ( const std::exception &exc )
	{
		app::console() << exc.what() << std::endl;
		throw exc;
	}

	size_t numBlendShapes = GlobalData::get().mFaceShift.getNumBlendshapes();
	mShader.bind();
	mShader.uniform( "blendshapes", 0 );
	mShader.uniform( "tex", 1 );
	mShader.uniform( "numBlendshapes", static_cast< int >( numBlendShapes ) );
	mShader.unbind();

	mEyeShader.bind();
	mEyeShader.uniform( "tex", 0 );
	mEyeShader.unbind();

	mMaterial = gl::Material( Color::gray( .0 ), Color::gray( .5 ), Color::white(), 50.f );
}

void Twister::setupParams()
{
	GlobalData& data = GlobalData::get();
	data.mParams.addSeparator();
	data.mParams.addPersistentParam( "Twist", &mTwistValue, 0.f,
			"min=-20 max=20 step=.05 group=Twister" );
}

void Twister::draw()
{
	GlobalData& data = GlobalData::get();

	gl::enableDepthRead();
	gl::enableDepthWrite();

	mShader.bind();
	GLint location = mShader.getAttribLocation( "index" );
	data.mVboMesh.setCustomStaticLocation( 0, location );

	const std::vector< float >& weights = data.mFaceShift.getBlendshapeWeights();
	mShader.uniform( "blendshapeWeights", &( weights[ 0 ] ), weights.size() );
	mShader.uniform( "twist", mTwistValue );

	mMaterial.apply();

	// draw head
	gl::enable( GL_TEXTURE_RECTANGLE_ARB );
	gl::enable( GL_TEXTURE_2D );
	data.mBlendshapeTexture.bind( 0 );
	data.mHeadTexture.bind( 1 );
	gl::draw( data.mVboMesh );
	data.mBlendshapeTexture.unbind();
	data.mHeadTexture.unbind();
	gl::disable( GL_TEXTURE_RECTANGLE_ARB );
	gl::disable( GL_TEXTURE_2D );

	mShader.unbind();

	/*
	mEyeShader.bind();
	mEyeShader.uniform( "twist", mTwistValue );
	gl::enable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( 10, 10 );
	data.mAiEyes.draw();
	gl::disable( GL_POLYGON_OFFSET_FILL );
	mEyeShader.unbind();
	*/
}

} // namespace mndl
