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
#include "cinder/gl/Texture.h"

#include "cinder/params/Params.h"

#include "cinder/Area.h"
#include "cinder/Camera.h"
#include "cinder/Capture.h"
#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/Matrix.h"
#include "cinder/Quaternion.h"
#include "cinder/Timeline.h"

#include "ArTracker.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class KecskeAr : public AppBasic
{
	public:
		void setup();
		void setupCapture();

		void update();
		void draw();

		void shutdown();

	private:
		// capture
		ci::Capture mCapture;
		gl::Texture mCaptTexture;

		vector< ci::Capture > mCaptures;

		static const int CAPTURE_WIDTH = 640;
		static const int CAPTURE_HEIGHT = 480;

		int32_t mCurrentCapture;

		// ar
		mndl::artkp::ArTracker mArTracker;

		// params
		params::InterfaceGl mParams;

		float mFps;
		bool mFlipX = false;
		bool mFlipY = false;
		int mObjectId = 0;
		float mRotationSmoothness = .1f;
		bool mDebugTracking;

		CameraPersp mCamera;

		// rotation logic
		Quatf mRot = Quatf( Vec3f( 1, 0, 0 ), 0 ); //< rotation
		/*! if true - the marker cube is lost for a longer period, and slerp is started
		 * back to the original orientation, if false - the cube has just been lost this frame,
		 * indeterminate - the cube is beeing seen */
		boost::tribool mBackToInit;;
};

void KecskeAr::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 300, 300 ) );

	mParams.addParam( "Fps", &mFps, "", true );

	vector< string > objectNames = { "frame", "cube" };
	mParams.addParam( "Object", objectNames, &mObjectId );
	mParams.addParam( "Rotation smoothness", &mRotationSmoothness, "min=0.01 max=1.00 step=.01" );
	mDebugTracking = false;
	mParams.addParam( "Debug capture", &mDebugTracking );

	setupCapture();

	// ar tracker
	mndl::artkp::ArTracker::Options options;
	options.setCameraFile( getAssetPath( "camera_para.dat" ) );
	options.setMultiMarker( getAssetPath( "marker_cube.cfg" ) );
	mArTracker = mndl::artkp::ArTracker( CAPTURE_WIDTH, CAPTURE_HEIGHT, options );

	// set up the camera
	mCamera.setPerspective( 60.f, getWindowAspectRatio(), 0.1f, 1000.0f );
	mCamera.lookAt( Vec3f( 0.f, 0.f, -10.f ), Vec3f( 0.f, 0.f, 0.f ) );

	// rotation
	mBackToInit = true;
}

void KecskeAr::setupCapture()
{
	// list out the capture devices
	vector< Capture::DeviceRef > devices( Capture::getDevices() );
	vector< string > deviceNames;

	for ( vector< Capture::DeviceRef >::const_iterator deviceIt = devices.begin();
			deviceIt != devices.end(); ++deviceIt )
	{
		Capture::DeviceRef device = *deviceIt;
		string deviceName = device->getName();

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

	if ( deviceNames.empty() )
	{
		deviceNames.push_back( "Camera not available" );
		mCaptures.push_back( Capture() );
	}

	mCurrentCapture = 0;
	mParams.addParam( "Capture", deviceNames, &mCurrentCapture );
}

void KecskeAr::update()
{
	static int lastCapture = -1;

	mFps = getAverageFps();

	// switch between capture devices
	if ( lastCapture != mCurrentCapture )
	{
		if ( ( lastCapture >= 0 ) && ( mCaptures[ lastCapture ] ) )
			mCaptures[ lastCapture ].stop();

		if ( mCaptures[ mCurrentCapture ] )
			mCaptures[ mCurrentCapture ].start();

		mCapture = mCaptures[ mCurrentCapture ];
		lastCapture = mCurrentCapture;
	}

	// detect the markers
	if ( mCapture && mCapture.checkNewFrame() )
	{
		Surface8u captSurf( mCapture.getSurface() );
		mCaptTexture = gl::Texture( captSurf );
		mArTracker.update( captSurf );
	}
}

void KecskeAr::draw()
{
	gl::clear( Color::black() );

	gl::pushMatrices();

	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mCamera );

	gl::enableDepthRead();
	gl::enableDepthWrite();

	//static float scale;
	bool markerFound = false;
	for ( int i = 0; i < mArTracker.getNumMarkers(); i++ )
	{
		// id -1 means unknown marker, false positive
		if ( mArTracker.getMarkerId( i ) == -1 )
			continue;

		// gets the modelview matrix of the i'th marker
		Matrix44f m = mArTracker.getModelView( i );

		// rotation
		Quatf q( m );
		q *= Quatf( Vec3f( 1, 0, 0 ), M_PI );

		mRot = mRot.slerp( mRotationSmoothness, q );

		markerFound = true;
		mBackToInit = boost::indeterminate;

		break;
	}

	if ( !markerFound )
	{
		if ( boost::indeterminate( mBackToInit ) )
		{
			console() << "cube lost - stating countdown " << getElapsedFrames() << endl;
			// allow loosing the cube for a small amount of time
			mBackToInit = false;
			timeline().add( [ & ]{ mBackToInit = true; }, timeline().getCurrentTime() + .5f );
		}
		else
		if ( mBackToInit == true )
		{
			console() << "cube lost totaly - interpolation back to original" << getElapsedFrames() << endl;
			// lost the cube for a longer period, start moving back to original position
			mRot = mRot.slerp( mRotationSmoothness, Quatf( Vec3f( 1, 0, 0 ), .0 ) );
		}
	}

	mRot.normalize();

	gl::pushModelView();
	gl::rotate( mRot );
	gl::scale( Vec3f::one() * 3.f );
	if ( mObjectId == 0 )
		gl::drawCoordinateFrame();
	else
		gl::drawColorCube( Vec3f::zero(), Vec3f::one() );
	gl::popModelView();

	gl::popMatrices();

	if ( mDebugTracking )
	{
		gl::disableDepthRead();
		gl::disableDepthWrite();

		gl::disableDepthRead();
		gl::disableDepthWrite();

		// draws camera image
		Area mDebugArea = Area( Rectf( getWindowBounds() ) * .2f + Vec2f( getWindowSize() ) * Vec2f( .8f, .0f ) );
		gl::color( Color::white() );
		Area outputArea = Area::proportionalFit( Area( 0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT ), mDebugArea, false );
		gl::setViewport( getWindowBounds() );
		gl::setMatricesWindow( getWindowSize() );
		if ( mCaptTexture )
		{
			gl::draw( mCaptTexture, outputArea );
		}

		gl::enableDepthRead();
		gl::enableDepthWrite();

		gl::setViewport( Area( Rectf( outputArea ) + Vec2f( getWindowSize() ) * Vec2f( .0, .8f ) ) );
		// sets the projection matrix
		mArTracker.setProjection();

		// scales the pattern according the the pattern width
		Vec3d patternScale = Vec3d::one() * mArTracker.getOptions().getPatternWidth();
		for ( int i = 0; i < mArTracker.getNumMarkers(); i++ )
		{
			// id -1 means unknown marker, false positive
			if ( mArTracker.getMarkerId( i ) == -1 )
				continue;

			// sets the modelview matrix of the i'th marker
			mArTracker.setModelView( i );
			gl::scale( patternScale );
			gl::drawColorCube( Vec3f::zero(), Vec3f::one() );
			break;
		}
	}

	params::InterfaceGl::draw();
}

void KecskeAr::shutdown()
{
	if ( mCapture )
	{
		mCapture.stop();
	}
}


CINDER_APP_BASIC( KecskeAr, RendererGl )

