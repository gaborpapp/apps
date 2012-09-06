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

#include "cinder/app/AppBasic.h"
#include "cinder/Arcball.h"
#include "cinder/Camera.h"
#include "cinder/Cinder.h"
#include "cinder/ImageIo.h"
#include "cinder/ip/Flip.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/TriMesh.h"

#include "PParams.h"

#include "ciFaceshift.h"
#include "GlobalData.h"
#include "Resources.h"

#include "Android.h"
#include "Devil.h"
#include "Twister.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl;

namespace mndl {

class fsExperiments : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void resize( ResizeEvent event );
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );
		void mouseWheel ( MouseEvent event );
		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		Arcball mArcball;
		float mEyeDistance;

		void setupParams();
		void setupVbo();
		void setupEyes();
		void setupEffects();

		float mFps;
		Color mBackground;

		std::vector< fsExpRef > mEffects;
		int mCurrentEffect;
};

void fsExperiments::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void fsExperiments::setup()
{
	gl::disableVerticalSync();

	GlobalData::get().mFaceShift.import( "export" );

	mEyeDistance = -300;

	mEffects.push_back( fsExpRef( new Android() ) );
	mEffects.push_back( fsExpRef( new Devil() ) );
	mEffects.push_back( fsExpRef( new Twister() ) );

	setupParams();
	setupVbo();
	setupEyes();
	setupEffects();

	Surface headImg = loadImage( loadAsset( "loki2.png" ) );
	ip::flipVertical( &headImg );
	GlobalData::get().mHeadTexture = gl::Texture( headImg );

	//GlobalData::get().mFaceShift.connect();

	gl::enable( GL_CULL_FACE );
}

void fsExperiments::setupEffects()
{
	for ( vector< fsExpRef >::iterator it = mEffects.begin(); it != mEffects.end(); ++it )
	{
		(*it)->setup();
	}
}

void fsExperiments::setupParams()
{
	fs::path paramsXml( getAssetPath( "params.xml" ));
	if ( paramsXml.empty() )
	{
#if defined( CINDER_MAC )
		fs::path assetPath( getResourcePath() / "assets" );
#else
		fs::path assetPath( getAppPath() / "assets" );
#endif
		createDirectories( assetPath );
		paramsXml = assetPath / "params.xml" ;
	}
	params::PInterfaceGl::load( paramsXml );

	GlobalData& data = GlobalData::get();
	data.mParams = params::PInterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	data.mParams.addPersistentSizeAndPosition();

	data.mParams.addParam( "Fps", &mFps, "", false );
	data.mParams.addPersistentParam( "Background", &mBackground, Color::gray( .1 ) );

	// effect params setup
	mCurrentEffect = 0;
	vector< string > effectNames;
	for ( vector< fsExpRef >::iterator it = mEffects.begin(); it != mEffects.end(); ++it )
	{
		effectNames.push_back( (*it)->getName() );
	}
	data.mParams.addParam( "Effect", effectNames, &mCurrentEffect );

	for ( vector< fsExpRef >::iterator it = mEffects.begin(); it != mEffects.end(); ++it )
	{
		(*it)->setupParams();
	}
}

void fsExperiments::setupVbo()
{
	GlobalData& data = GlobalData::get();
	TriMesh neutralMesh = data.mFaceShift.getNeutralMesh();
	size_t numVertices = neutralMesh.getNumVertices();
	size_t numBlendShapes = data.mFaceShift.getNumBlendshapes();

	gl::VboMesh::Layout layout;

	layout.setStaticPositions();
	layout.setStaticIndices();
	layout.setStaticNormals();
	layout.setStaticTexCoords2d();

	// TODO: int attribute
	layout.mCustomStatic.push_back( std::make_pair( gl::VboMesh::Layout::CUSTOM_ATTR_FLOAT, 0 ) );

	data.mVboMesh = gl::VboMesh( neutralMesh.getNumVertices(),
							neutralMesh.getNumIndices(), layout, GL_TRIANGLES );

	data.mVboMesh.bufferPositions( neutralMesh.getVertices() );
	data.mVboMesh.bufferIndices( neutralMesh.getIndices() );
	if ( !neutralMesh.hasNormals() )
		neutralMesh.recalculateNormals();
	data.mVboMesh.bufferNormals( neutralMesh.getNormals() );
	data.mVboMesh.bufferTexCoords2d( 0, neutralMesh.getTexCoords() );
	data.mVboMesh.unbindBuffers();

	vector< float > vertexIndices( numVertices, 0.f );
	for ( uint32_t i = 0; i < numVertices; ++i )
		vertexIndices[ i ] = static_cast< float >( i );

	data.mVboMesh.getStaticVbo().bind();
	size_t offset = sizeof( GLfloat ) * ( 3 + 3 + 2 ) * neutralMesh.getNumVertices();
	data.mVboMesh.getStaticVbo().bufferSubData( offset,
			numVertices * sizeof( float ),
			&vertexIndices[ 0 ] );
	data.mVboMesh.getStaticVbo().unbind();

	// blendshapes texture
	gl::Texture::Format format;
	format.setTargetRect();
	format.setWrap( GL_CLAMP, GL_CLAMP );
	format.setMinFilter( GL_NEAREST );
	format.setMagFilter( GL_NEAREST );
	format.setInternalFormat( GL_RGB32F_ARB );

	int verticesPerRow = 32;
	int txtWidth = verticesPerRow * numBlendShapes;
	int txtHeight = math< float >::ceil( numVertices / (float)verticesPerRow );

	Surface32f blendshapeSurface = Surface32f( txtWidth, txtHeight, false );
	float *ptr = blendshapeSurface.getData();

	size_t idx = 0;
	for ( int j = 0; j < txtHeight; j++ )
	{
		for ( int s = 0; s < verticesPerRow; s++ )
		{
			for ( size_t i = 0; i < numBlendShapes; i++ )
			{
				*( reinterpret_cast< Vec3f * >( ptr ) ) = data.mFaceShift.getBlendshapeMesh( i ).getVertices()[ idx ] - neutralMesh.getVertices()[ idx ];
				ptr += 3;
			}
			idx++;
		}
	}

	data.mBlendshapeTexture = gl::Texture( blendshapeSurface, format );
}

void fsExperiments::setupEyes()
{
	GlobalData& data = GlobalData::get();

	data.mAiEyes = assimp::AssimpLoader( getAssetPath( "models/eyes.dae" ) );
	data.mAiEyes.disableSkinning();
	//data.mAiEyes.disableMaterials();
	data.mAiEyes.enableMaterials();

	// FIXME: load positions from export/eye
	data.mLeftEyeRef = data.mAiEyes.getAssimpNode( "left" );
	assert( data.mLeftEyeRef );
	data.mLeftEyeRef->setPosition( Vec3f( 31.3636, -24.4065, -24.7222 ) );

	data.mRightEyeRef = data.mAiEyes.getAssimpNode( "right" );
	assert( data.mRightEyeRef );
	data.mRightEyeRef->setPosition( Vec3f( -31.1631, -26.0238, -24.7322 ) );
}

void fsExperiments::shutdown()
{
	params::PInterfaceGl::save();
}

void fsExperiments::update()
{
	GlobalData& data = GlobalData::get();

	data.mHeadRotation = data.mFaceShift.getRotation();
	data.mLeftEyeRotation = data.mFaceShift.getLeftEyeRotation();
	data.mRightEyeRotation = data.mFaceShift.getRightEyeRotation();

	data.mLeftEyeRef->setOrientation( data.mLeftEyeRotation );
	data.mRightEyeRef->setOrientation( data.mRightEyeRotation );

	mEffects[ mCurrentEffect ]->update();
	mFps = getAverageFps();
}

void fsExperiments::draw()
{
	gl::clear( mBackground );

	CameraPersp cam( getWindowWidth(), getWindowHeight(), 60.0f );
	cam.setPerspective( 60, getWindowAspectRatio(), 1, 5000 );
	cam.lookAt( Vec3f( 0, 0, mEyeDistance ), Vec3f::zero(), Vec3f( 0, -1, 0 ) );
	gl::setMatrices( cam );

	gl::setViewport( getWindowBounds() );

	gl::enableDepthRead();
	gl::enableDepthWrite();

	GlobalData& data = GlobalData::get();

	gl::pushModelView();
	gl::rotate( mArcball.getQuat() );
	gl::rotate( data.mHeadRotation );

	mEffects[ mCurrentEffect ]->draw();

	gl::popModelView();

	params::PInterfaceGl::draw();
}

void fsExperiments::mouseDown( MouseEvent event )
{
	mArcball.mouseDown( event.getPos() );
}

void fsExperiments::mouseDrag( MouseEvent event )
{
	mArcball.mouseDrag( event.getPos() );
}

void fsExperiments::mouseWheel ( MouseEvent event )
{
	float w = event.getWheelIncrement();

	if ( w < 0 )
		mEyeDistance *= ( 1.1 + .5 * w );
	else
		mEyeDistance /= ( 1.1 - .5 * w );
}

void fsExperiments::resize( ResizeEvent event )
{
	mArcball.setWindowSize( getWindowSize() );
	mArcball.setCenter( getWindowCenter() );
	mArcball.setRadius( 150 );
}

void fsExperiments::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				if ( GlobalData::get().mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			GlobalData::get().mParams.show( !GlobalData::get().mParams.isVisible() );
			if ( isFullScreen() )
			{
				if ( GlobalData::get().mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

} // namespace mndl

CINDER_APP_BASIC( mndl::fsExperiments, RendererGl( RendererGl::AA_NONE ) )

