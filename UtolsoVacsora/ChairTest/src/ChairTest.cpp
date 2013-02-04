/*
 Copyright (C) 2013 Gabor Papp

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

#include "cinder/Cinder.h"
#include "cinder/MayaCamUI.h"
#include "cinder/ObjLoader.h"
#include "cinder/TriMesh.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/gl.h"

#include "mndlkit/params/PParams.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl;

class ChairTest : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );
		void resize();

		void update();
		void draw();

		void shutdown();

	private:
		kit::params::PInterfaceGl mParams;

		void loadModel();
		TriMesh mTriMesh;
		MayaCamUI mMayaCam;

		float mFps;
		Vec3f mEffectCenter;
		float mEffectRadius;
		float mEffectMaxRadius;
		bool mDrawCenter;

		void loadShader();
		gl::GlslProg mShader;
};

void ChairTest::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void ChairTest::setup()
{
	gl::disableVerticalSync();

	kit::params::PInterfaceGl::load( "params.xml" );
	mParams = kit::params::PInterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addPersistentParam( "Radius", &mEffectRadius, .1f, "min=.01 max=5 step=.01" );
	mParams.addPersistentParam( "Max Radius", &mEffectMaxRadius, .1f, "min=.01 max=5 step=.01" );
	mParams.addPersistentParam( "Draw center", &mDrawCenter, true );

	loadModel();
	loadShader();

	CameraPersp cam;
	cam.setPerspective( 60, getWindowAspectRatio(), 1, 100 );
	cam.lookAt( Vec3f( 2.12f, 3.1f, -6.3f ), Vec3f( -15.6f, -4.7f, 27.7f ), Vec3f( 0, 1, 0 ) );
	mMayaCam.setCurrentCam( cam );
}

void ChairTest::loadModel()
{
	const fs::path objName = "3chairs.obj";
	fs::path meshName( objName );
	meshName.replace_extension( "msh" );

	try
	{
		// try to load the mesh from its binary file
		fs::path objFile = getAssetPath( fs::path( "model" ) / meshName );
		if ( !objFile.empty() )
			mTriMesh.read( loadFile( objFile ) );
		else
			throw ci::Exception();
	}
	catch ( ... )
	{
		// load model into mesh
		ObjLoader loader( loadAsset( fs::path( "model" ) / objName ) );
		loader.load( &mTriMesh );

		// one-time only: write mesh to binary file
		mTriMesh.write( writeFile( getAssetPath( "model" ) / meshName ) );
	}
}

void ChairTest::loadShader()
{
	try
	{
		mShader = gl::GlslProg( loadAsset( "shaders/Chair.vert" ),
								loadAsset( "shaders/Chair.frag" ) );
	}
	catch( const std::exception &e )
	{
		console() << "Could not compile shader:" << e.what() << std::endl;
	}
}

void ChairTest::update()
{
	mFps = getAverageFps();

	float a = getElapsedSeconds();
	mEffectCenter = Vec3f( math< float >::cos( a ), 0.f, math< float >::sin( a ) );
	mEffectCenter *= 1.8f;
	mEffectCenter += Vec3f( -.5f, 0.f, -.5f );
}

void ChairTest::draw()
{
	gl::clear( Color::black() );
	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mMayaCam.getCamera() );

	gl::clear( Color::white() );
	gl::color( Color::gray( .5f ) );

	if ( mShader )
	{
		mShader.bind();
		mShader.uniform( "effectCenter", mEffectCenter );
		mShader.uniform( "effectRadius", mEffectRadius );
		mShader.uniform( "effectMaxRadius", mEffectMaxRadius );
	}

	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::enable( GL_LIGHTING );
	gl::Light light(gl::Light::POINT, 0);

	light.lookAt( Vec3f( 2.12f, 3.1f, -6.3f ), Vec3f( -15.6f, -4.7f, 27.7f ) );
	light.setAmbient( Color( 0.0f, 0.0f, 0.0f ) );
	light.setDiffuse( Color( 1.0f, 1.0f, 1.0f ) );
	light.setSpecular( Color( 1.0f, 1.0f, 1.0f ) );
	light.enable();

	gl::draw( mTriMesh );

	light.disable();
	gl::disable( GL_LIGHTING );

	if ( mShader )
		mShader.unbind();

	if ( mDrawCenter )
	{
		gl::color( Color( 1, 0, 0 ) );
		gl::drawSphere( mEffectCenter, .1f );
	}

	kit::params::PInterfaceGl::draw();
}

void ChairTest::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams.isVisible() )
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
			kit::params::PInterfaceGl::showAllParams( !mParams.isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams.isVisible() )
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

void ChairTest::shutdown()
{
	kit::params::PInterfaceGl::save();
	CameraPersp cam = mMayaCam.getCamera();
	console() << cam.getEyePoint() << " " << cam.getCenterOfInterestPoint() << endl;
}

void ChairTest::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void ChairTest::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void ChairTest::resize()
{
	CameraPersp cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}

CINDER_APP_BASIC( ChairTest, RendererGl )

