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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Capture.h"

#include "cinder/ip/Resize.h"

#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class OptFlowApp : public AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void keyDown(KeyEvent event);

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		ci::Capture mCapture;
		gl::Texture mCaptTexture;

		float mFps;

		bool mFlip;
		bool mDrawFlow;

		cv::Mat mPrevFrame;
		cv::Mat mFlow;

		float mPyrScale;
		int mOptFlowLevels;
		int mOptFlowWinSize;
		int mOptFlowIterations;
		float mFlowMultiplier;
		int mOptFlowPolyN;
		float mOptFlowPolySigma;

		static const int OPTFLOW_WIDTH = 80;
		static const int OPTFLOW_HEIGHT = 60;
};

void OptFlowApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void OptFlowApp::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );

	mParams.addParam( "Fps", &mFps, "", false );
	mFlip = true;
	mParams.addParam( "Flip", &mFlip );
	mDrawFlow = true;
	mParams.addParam( "Draw flow", &mDrawFlow );
	mFlowMultiplier = .3;
	mParams.addParam( "Flow multiplier", &mFlowMultiplier, "min=.05 max=2 step=.05" );

	mParams.addSeparator();
	mPyrScale = .5;
	mParams.addParam( "Pyramid scale", &mPyrScale, "min=.01 max=.99 step=.01" );
	mOptFlowLevels = 5;
	mParams.addParam( "Levels", &mOptFlowLevels, "min=1 max=20" );
	mOptFlowWinSize = 13;
	mParams.addParam( "Window size", &mOptFlowWinSize, "min=1 max=20" );
	mOptFlowIterations = 5;
	mParams.addParam( "Iterations", &mOptFlowIterations, "min=1 max=20" );
	mOptFlowPolyN = 5;
	mParams.addParam( "PolyN", &mOptFlowPolyN, "min=1 max=20" );
	mOptFlowPolySigma = 1.1;
	mParams.addParam( "Poly sigma", &mOptFlowPolySigma, "min=.1 max=5 step=.1" );
	mParams.addSeparator();

	// capture
	try
	{
		mCapture = Capture( 640, 480 );
		//mCapture = Capture( 80, 60 );
		mCapture.start();
	}
	catch (...)
	{
		console() << "Failed to initialize capture" << endl;
	}

	gl::disableVerticalSync();
}

void OptFlowApp::shutdown()
{
	if ( mCapture )
	{
		mCapture.stop();
	}
}

void OptFlowApp::update()
{
	mFps = getAverageFps();

	if ( mCapture && mCapture.checkNewFrame() )
	{
		Surface8u captSurf( Channel8u( mCapture.getSurface() ) );

		Surface8u smallSurface( OPTFLOW_WIDTH, OPTFLOW_HEIGHT, false );
		if ( ( captSurf.getWidth() != OPTFLOW_WIDTH ) ||
			 ( captSurf.getHeight() != OPTFLOW_HEIGHT ) )
		{
			ip::resize( captSurf, &smallSurface );
		}
		else
		{
			smallSurface = captSurf;
		}

		mCaptTexture = gl::Texture( captSurf );

		cv::Mat currentFrame( toOcv( Channel( smallSurface ) ) );
		if ( mFlip )
			cv::flip( currentFrame, currentFrame, 1 );
        if ( mPrevFrame.data )
		{
			cv::calcOpticalFlowFarneback(
					mPrevFrame, currentFrame,
					mFlow,
					mPyrScale,
					mOptFlowLevels,
					mOptFlowWinSize,
					mOptFlowIterations,
					mOptFlowPolyN,
					mOptFlowPolySigma,
					0 );
		}
        mPrevFrame = currentFrame;
	}
}

void OptFlowApp::draw()
{
	gl::clear( Color::black() );

	if ( mCaptTexture )
	{
		// camera
		gl::pushModelView();
		if ( mFlip )
		{
			gl::translate( getWindowWidth(), 0 );
			gl::scale( -1, 1 );
		}

		Area mappedArea = Area::proportionalFit( mCaptTexture.getBounds(),
					getWindowBounds(), true, true );
		gl::draw( mCaptTexture, mappedArea );
		gl::popModelView();

		// flow vectors
		if ( mDrawFlow && mFlow.data )
		{
			RectMapping ofToWin( Area( 0, 0, mFlow.cols, mFlow.rows ),
				mappedArea );
			float ofScale = mFlowMultiplier * mappedArea.getWidth() / (float)OPTFLOW_WIDTH;

			gl::color( Color::white() );
			for ( int y = 0; y < mFlow.rows; y++ )
			{
				for ( int x = 0; x < mFlow.cols; x++ )
				{
					Vec2f v = fromOcv( mFlow.at< cv::Point2f >( y, x ) );
					Vec2f p( x + .5, y + .5 );
					gl::drawLine( ofToWin.map( p ),
								  ofToWin.map( p + ofScale * v ) );
				}
			}
		}
	}

	params::InterfaceGl::draw();
}

void OptFlowApp::keyDown( KeyEvent event )
{
	if ( event.getChar() == 'f' )
		setFullScreen( !isFullScreen() );
	if ( event.getCode() == KeyEvent::KEY_ESCAPE )
		quit();
}

CINDER_APP_BASIC( OptFlowApp, RendererGl( RendererGl::AA_NONE ) )

