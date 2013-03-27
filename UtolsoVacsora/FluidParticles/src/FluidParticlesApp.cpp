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
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "mndlkit/params/PParams.h"

#include "CaptureSource.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FludParticlesApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		mndl::kit::params::PInterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled;

		mndl::CaptureSource mCaptureSource;
		gl::Texture mCaptureTexture;
};

void FludParticlesApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void FludParticlesApp::setup()
{
	mndl::kit::params::PInterfaceGl::load( "params.xml" );
	mParams = mndl::kit::params::PInterfaceGl( "Parameters", Vec2i( 310, 300 ), Vec2i( 16, 16 ) );
	mParams.addPersistentSizeAndPosition();
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addPersistentParam( "Vertical sync", &mVerticalSyncEnabled, false );

	mCaptureSource.setup();
}

void FludParticlesApp::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

	mCaptureSource.update();

	if ( mCaptureSource.isCapturing() && mCaptureSource.checkNewFrame() )
	{
		mCaptureTexture = mCaptureSource.getSurface();
	}
}

void FludParticlesApp::draw()
{
	gl::clear();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	if ( mCaptureTexture )
	{
		gl::color( Color::white() );
		gl::draw( mCaptureTexture, getWindowBounds() );
	}

	mndl::kit::params::PInterfaceGl::draw();
}

void FludParticlesApp::shutdown()
{
	mCaptureSource.shutdown();
	mndl::kit::params::PInterfaceGl::save();
}

void FludParticlesApp::keyDown( KeyEvent event )
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

		case KeyEvent::KEY_v:
			 mVerticalSyncEnabled = !mVerticalSyncEnabled;
			 break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( FludParticlesApp, RendererGl )

