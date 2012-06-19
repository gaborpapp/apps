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
#include <string>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CaptureChangeApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		vector< Capture > mCaptures;

		static const int CAPTURE_WIDTH = 320;
		static const int CAPTURE_HEIGHT = 240;

		int mCurrentCapture;

		gl::Texture mTexture;
		params::InterfaceGl mParams;
};

void CaptureChangeApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void CaptureChangeApp::setup()
{
	// list out the capture devices
	vector< Capture::DeviceRef > devices( Capture::getDevices() );
	vector< string > deviceNames;

    for ( vector< Capture::DeviceRef >::const_iterator deviceIt = devices.begin();
			deviceIt != devices.end(); ++deviceIt )
	{
        Capture::DeviceRef device = *deviceIt;
		string deviceName = device->getName() + " " + device->getUniqueId();

        try
		{
            if ( device->checkAvailable() )
			{
                mCaptures.push_back( Capture( CAPTURE_WIDTH, CAPTURE_HEIGHT,
							device ) );
				deviceNames.push_back( deviceName );
            }
            else
			{
                mCaptures.push_back( Capture() );
				deviceNames.push_back( deviceName + " not available" );
			}
        }
        catch ( CaptureExc & )
		{
            console() << "Unable to initialize device: " << device->getName() <<
 endl;
        }
	}

	// params
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );

	if ( deviceNames.empty() )
	{
		deviceNames.push_back( "Camera not available" );
		mCaptures.push_back( Capture() );
	}
	mCurrentCapture = 0;
	mParams.addParam( "Capture", deviceNames, &mCurrentCapture );

	gl::disableVerticalSync();
}

void CaptureChangeApp::update()
{
	static int lastCapture = -1;

	if ( lastCapture != mCurrentCapture )
	{
		if ( ( lastCapture >= 0 ) && ( mCaptures[ lastCapture ] ) )
			mCaptures[ lastCapture ].stop();

		if ( mCaptures[ mCurrentCapture ] )
			mCaptures[ mCurrentCapture ].start();
		lastCapture = mCurrentCapture;
	}

	if ( mCaptures[ mCurrentCapture ] &&
		 mCaptures[ mCurrentCapture ].checkNewFrame() )
	{
		mTexture = gl::Texture( mCaptures[ mCurrentCapture ].getSurface() );
	}

}

void CaptureChangeApp::draw()
{
	gl::clear( Color::black() );
	gl::setMatricesWindow( getWindowSize() );

	if ( mTexture )
	{
		Area bounds = Area::proportionalFit( mTexture.getBounds(),
				getWindowBounds(), true, true );
		gl::draw( mTexture, bounds );
	}

	params::InterfaceGl::draw();
}

void CaptureChangeApp::keyDown( KeyEvent event )
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

CINDER_APP_BASIC( CaptureChangeApp, RendererGl( RendererGl::AA_NONE ) )

