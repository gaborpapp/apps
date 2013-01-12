/*
 Copyright (C) 2013 Gabor Papp

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

#include "cinder/gl/gl.h"

#include "cinder/Camera.h"
#include "cinder/Cinder.h"

#include "PParams.h"
#include "TrackerManager.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class KecskeAr : public AppBasic
{
	public:
		void setup();

		void resize();
		void keyDown( KeyEvent event );

		void update();
		void draw();

		void shutdown();

	private:
		// params
		params::PInterfaceGl mParams;

		// tracker
		TrackerManager mTrackerManager;

		float mFps;
		int mObjectId = 0;

		CameraPersp mCamera;
};

void KecskeAr::setup()
{
	gl::disableVerticalSync();

	// params
	params::PInterfaceGl::load( "params.xml" );

	mParams = params::PInterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addParam( "Fps", &mFps, "", true );

	vector< string > objectNames = { "frame", "cube" };
	mParams.addParam( "Object", objectNames, &mObjectId );

	// set up the camera
	mCamera.setPerspective( 60.f, getWindowAspectRatio(), 0.1f, 1000.0f );
	mCamera.lookAt( Vec3f( 0.f, 0.f, -10.f ), Vec3f( 0.f, 0.f, 0.f ) );

	mTrackerManager.setup();
}

void KecskeAr::update()
{
	mFps = getAverageFps();

	mTrackerManager.update();
}

void KecskeAr::draw()
{
	gl::clear( Color::black() );

	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mCamera );

	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::pushModelView();
	gl::rotate( mTrackerManager.getRotation() );
	gl::scale( mTrackerManager.getScale() );
	if ( mObjectId == 0 )
		gl::drawCoordinateFrame();
	else
		gl::drawColorCube( Vec3f::zero(), Vec3f::one() );
	gl::popModelView();


	mTrackerManager.draw();

	params::InterfaceGl::draw();
}

void KecskeAr::shutdown()
{
	params::PInterfaceGl::save();
}

void KecskeAr::resize()
{
	mCamera.setPerspective( 60.f, getWindowAspectRatio(), 0.1f, 1000.0f );
}

void KecskeAr::keyDown( KeyEvent event )
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
			params::PInterfaceGl::showAllParams( !mParams.isVisible(), false );
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


CINDER_APP_BASIC( KecskeAr, RendererGl )

