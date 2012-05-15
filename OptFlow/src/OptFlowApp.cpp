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
#include "opencv2/gpu/gpu.hpp"

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
		bool mGpu;

		cv::Mat mPrevFrame;
		cv::Mat mFlow;

		static const int OPTFLOW_WIDTH = 80;
		static const int OPTFLOW_HEIGHT = 30;
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
	mParams.addParam( "Flip", &mFlip);
	mDrawFlow = true;
	mParams.addParam( "Draw flow", &mDrawFlow);
	mGpu = false;
	mParams.addParam( "Use gpu", &mGpu);

	// capture
	try
	{
		mCapture = Capture( 640, 480 );
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

		Surface smallSurface( OPTFLOW_WIDTH, OPTFLOW_HEIGHT, false );
		ip::resize( captSurf, &smallSurface );

		mCaptTexture = gl::Texture( captSurf );

		cv::Mat currentFrame( toOcv( Channel( smallSurface ) ) );
		if ( mFlip )
			cv::flip( currentFrame, currentFrame, 1 );
        if ( mPrevFrame.data )
		{
			cv::calcOpticalFlowFarneback( mPrevFrame, currentFrame,
					mFlow, .5, 3, 13, 3, 5, 1.1, 0 );

			/*
			pyrScale = 0.5;
			numLevels = 5;
			winSize = 13;
			fastPyramids = false;
			numIters = 10;
			polyN = 5;
			polySigma = 1.1;
			flags = 0;
			*/

			/*
			   calcOpticalFlowFarneback(prevgray, gray, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
			   calcOpticalFlowFarneback(const Mat& prevImg, const Mat& nextImg, Mat& flow, double pyrScale, int levels, int winsize, int iterations, int polyN, double polySigma, int flags)
			*/

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
			float ofScale = .2 * mappedArea.getWidth() / (float)OPTFLOW_WIDTH;

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

