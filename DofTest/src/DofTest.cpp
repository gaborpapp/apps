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

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"
#include "cinder/params/Params.h"
#include "cinder/Cinder.h"
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/ObjLoader.h"
#include "cinder/TriMesh.h"

#include "CubeMap.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DofTest : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );

		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );

		void resize( ResizeEvent event );

		void update();
		void draw();

	private:
		mndl::gl::CubeMap mCubeMap;

		gl::VboMesh mVboMesh;
		TriMesh mTriMesh;

		MayaCamUI mMayaCam;

		params::InterfaceGl mParams;
};

void DofTest::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void DofTest::setup()
{
	gl::disableVerticalSync();

	ObjLoader loader( loadAsset( "sphere.obj" ) );
	loader.load( &mTriMesh );
	mVboMesh = gl::VboMesh( mTriMesh );

	mCubeMap = mndl::gl::CubeMap( loadImage( loadAsset( "cubemap/px.jpg" ) ),
								loadImage( loadAsset( "cubemap/py.jpg" ) ),
								loadImage( loadAsset( "cubemap/pz.jpg" ) ),
								loadImage( loadAsset( "cubemap/nx.jpg" ) ),
								loadImage( loadAsset( "cubemap/ny.jpg" ) ),
								loadImage( loadAsset( "cubemap/nz.jpg" ) ) );

	CameraPersp cam;
	cam.setPerspective( 60.f, getWindowAspectRatio(), 0.1f, 1000.0f );
	mMayaCam.setCurrentCam( cam );

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
}

void DofTest::update()
{
}

void DofTest::draw()
{
	gl::clear( Color::black() );

	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mMayaCam.getCamera() );

	gl::enable( GL_NORMALIZE );
	mndl::gl::enableReflectionMapping();
	mCubeMap.bind();
	gl::pushMatrices();
	gl::rotate( Vec3f( getElapsedSeconds() * 27 , getElapsedSeconds() * 15, 90 ) );
	gl::scale( Vec3f( 20, 20, 20 ) );
	gl::draw( mVboMesh );
	gl::popMatrices();
	mCubeMap.unbind();
	mndl::gl::disableReflectionMapping();

	params::InterfaceGl::draw();
}

void DofTest::keyDown( KeyEvent event )
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
			mParams.show( !mParams.isVisible() );
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

void DofTest::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void DofTest::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void DofTest::resize( ResizeEvent event )
{
	CameraPersp cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}


CINDER_APP_BASIC( DofTest, RendererGl() )

