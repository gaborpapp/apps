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

#include <sstream>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/Capture.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"

#include "LumaOffset.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FXApp : public ci::app::AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void update();
		void draw();

		void keyDown( KeyEvent event );

	private:
		Capture mCapture;

		mndl::fx::LumaOffset mLumaOffsetFx;

		params::InterfaceGl	mParams;

		float mFps;
};

void FXApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void FXApp::setup()
{
	gl::disableVerticalSync();

	mLumaOffsetFx = mndl::fx::LumaOffset( 640, 480 );

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );

	mParams.addText( "LumaOffset" );
	mLumaOffsetFx.addToParams( mParams );

	mParams.addSeparator();
	mParams.addParam( "Fps", &mFps, "", true );

	// capture
	try
	{
		mCapture = Capture( 640, 480 );
		mCapture.start();
	}
	catch (...)
	{
		console() << "Failed to initialize capture" << std::endl;
	}
}

void FXApp::shutdown()
{
	if ( mCapture )
	{
		mCapture.stop();
	}
}

void FXApp::update()
{
	mFps = getAverageFps();
}

void FXApp::draw()
{
	static gl::Texture source;

	gl::clear( Color::black() );

	bool isNewFrame = mCapture && mCapture.checkNewFrame();

	if ( isNewFrame )
	{
		source = gl::Texture( mCapture.getSurface() );
	}

	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );

	if ( isNewFrame )
		source = mLumaOffsetFx.process( source );

	if ( source )
	{
		gl::draw( source,
				Area::proportionalFit( source.getBounds(), getWindowBounds(),
									   true, true ) );
	}

	params::InterfaceGl::draw();
}

void FXApp::keyDown( KeyEvent event )
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

CINDER_APP_BASIC( FXApp, RendererGl( RendererGl::AA_NONE ) )

