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

#include "Devil.h"

using namespace ci;

namespace mndl {

void Devil::setup()
{
	try
	{
		mShader = gl::GlslProg( app::loadResource( RES_BLEND_VERT ),
								app::loadResource( RES_BLEND_FRAG ) );
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

	mMaterial = gl::Material( Color::gray( .0 ), Color::gray( .5 ), Color::white(), 50.f );

	mAiHorns = assimp::AssimpLoader( app::getAssetPath( "models/devil.dae" ) );
	mAiHorns.disableSkinning();
	mAiHorns.disableMaterials();
	mAiHorns.enableAnimation();
	mAiHorns.setAnimation( 0 );
	mAiHorns.disableTextures();
}

void Devil::setupParams()
{
	GlobalData& data = GlobalData::get();
	data.mParams.addSeparator();
	data.mParams.addPersistentParam( "Devilness", &mDevilness, 0.f,
			"min=0 max=1 step=.05 group=Devil" );
}

void Devil::update()
{
	if ( mAiHorns.getNumAnimations() > 0 )
	{
		mAiHorns.setTime( mDevilness * mAiHorns.getAnimationDuration( 0 ) );
		mAiHorns.update();
	}
}

void Devil::draw()
{
	gl::enableDepthRead();
	gl::enableDepthWrite();

	mShader.bind();
	GLint location = mShader.getAttribLocation( "index" );
	GlobalData::get().mVboMesh.setCustomStaticLocation( 0, location );

	const std::vector< float >& weights = GlobalData::get().mFaceShift.getBlendshapeWeights();
	mShader.uniform( "blendshapeWeights", &( weights[ 0 ] ), weights.size() );

	mMaterial.apply();

	// draw head
	gl::enable( GL_TEXTURE_RECTANGLE_ARB );
	gl::enable( GL_TEXTURE_2D );
	GlobalData::get().mBlendshapeTexture.bind( 0 );
	GlobalData::get().mHeadTexture.bind( 1 );
	gl::draw( GlobalData::get().mVboMesh );
	GlobalData::get().mBlendshapeTexture.unbind();
	GlobalData::get().mHeadTexture.unbind();
	gl::disable( GL_TEXTURE_RECTANGLE_ARB );
	gl::disable( GL_TEXTURE_2D );

	// draw horns
	mAiHorns.draw();

	mShader.unbind();
}

} // namespace mndl
