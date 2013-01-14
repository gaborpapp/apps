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
#include "cinder/MayaCamUI.h"

#include "AssimpLoader.h"
#include "PParams.h"
#include "TrackerManager.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class KecskeAr : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void resize();
		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );

		void update();
		void draw();

		void shutdown();

	private:
		// params
		params::PInterfaceGl mParams;

		// tracker
		TrackerManager mTrackerManager;

		float mFps;

		void loadModel();
		mndl::assimp::AssimpLoader mAssimpLoader;

		// cameras
		typedef enum
		{
			TYPE_INDOOR = 0,
			TYPE_OUTDOOR = 1
		} CameraType;

		struct CameraExt
		{
			string mName;
			CameraType mType;
			CameraPersp mCam;
		};

		int mCameraIndex;
		vector< CameraExt > mCameras;
		CameraExt mCurrentCamera;
		MayaCamUI mMayaCam;
		float mFov;

		enum
		{
			MODE_MOUSE = 0,
			MODE_AR = 1
		};
		int mInteractionMode;
};

void KecskeAr::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 768 );
}

void KecskeAr::setup()
{
	gl::disableVerticalSync();

	getWindow()->setTitle( "KecskeAr" );

	// params
	params::PInterfaceGl::load( "params.xml" );

	mParams = params::PInterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addSeparator();

	vector< string > interactionNames = { "mouse", "ar" };
	mParams.addPersistentParam( "Interaction mode", interactionNames, &mInteractionMode, 0 );
	mFov = 60.f;
	mParams.addParam( "Fov", &mFov, "min=10 max=90" );

	loadModel();

	mTrackerManager.setup();
}

void KecskeAr::loadModel()
{
	XmlTree doc( loadFile( getAssetPath( "config.xml" ) ) );
	XmlTree xmlLicense = doc.getChild( "model" );
	string filename = xmlLicense.getAttributeValue< string >( "name", "" );

	try
	{
		mAssimpLoader = mndl::assimp::AssimpLoader( getAssetPath( filename ) );
	}
	catch ( mndl::assimp::AssimpLoaderExc &exc )
	{
		console() << exc.what() << endl;
		quit();
	};

	// set up cameras
	vector< string > cameraNames;
	mCameras.resize( mAssimpLoader.getNumCameras() );
	for ( size_t i = 0; i < mAssimpLoader.getNumCameras(); i++ )
	{
		string name = mAssimpLoader.getCameraName( i );
		cameraNames.push_back( name );
		mCameras[ i ].mName = name;
		mCameras[ i ].mType = TYPE_INDOOR;
		mCameras[ i ].mCam = mAssimpLoader.getCamera( i );

		// load camera parameter from config file
		for ( XmlTree::Iter cIt = doc.begin( "camera" ); cIt != doc.end(); ++cIt )
		{
			string cName = cIt->getAttributeValue< string >( "name", "" );
			if ( cName != name )
				continue;
			string cTypeStr = cIt->getAttributeValue< string >( "type", "indoor" );
			if ( cTypeStr == "indoor" )
				mCameras[ i ].mType = TYPE_INDOOR;
			else
			if ( cTypeStr == "outdoor" )
				mCameras[ i ].mType = TYPE_OUTDOOR;
		}
	}
	mCameraIndex = 0;
	mParams.addParam( "Cameras", cameraNames, &mCameraIndex );
}

void KecskeAr::update()
{
	static int lastCameraIndex = -1;
	static int lastInteractionMode = -1;
	if ( ( lastCameraIndex != mCameraIndex ) ||
		 ( lastInteractionMode != mInteractionMode ) )
	{
		mCurrentCamera = mCameras[ mCameraIndex ];
		lastCameraIndex = mCameraIndex;
		lastInteractionMode = mInteractionMode;
	}

	mFps = getAverageFps();

	mTrackerManager.update();
}

void KecskeAr::draw()
{
	gl::clear( Color::black() );

	if ( mInteractionMode == MODE_AR )
	{
		mFov = lmap( mTrackerManager.getZoom(), .0f, 1.f, 10.f, 90.f );

		// original orientation * ar orientation
		Quatf oriInit = mCameras[ mCameraIndex ].mCam.getOrientation();
		// mirror rotation
		Quatf q = mTrackerManager.getRotation();
		q.v = -q.v;
		mCurrentCamera.mCam.setOrientation( oriInit * q );
	}

	gl::setViewport( getWindowBounds() );
	mCurrentCamera.mCam.setFov( mFov );
	if ( mCurrentCamera.mType == TYPE_INDOOR )
	{
		gl::setMatrices( mCurrentCamera.mCam );
	}
	else // outdoor camera
	{
		// set the originial camera without rotation and rotate the object instead
		CameraPersp cam = mCameras[ mCameraIndex ].mCam;
		cam.setFov( mFov );
		gl::setMatrices( cam );
	}

	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::pushModelView();

	if ( mCurrentCamera.mType == TYPE_OUTDOOR )
	{
		Quatf q = mCurrentCamera.mCam.getOrientation();
		q.v = -q.v;
		gl::rotate( q );
	}

	gl::color( Color::white() );
	mAssimpLoader.draw();
	gl::popModelView();

	mTrackerManager.draw();

	params::InterfaceGl::draw();
}

void KecskeAr::shutdown()
{
	params::PInterfaceGl::save();
}

void KecskeAr::mouseDown( MouseEvent event )
{
	if ( mInteractionMode != MODE_MOUSE )
		return;

	mMayaCam.setCurrentCam( mCurrentCamera.mCam );
	mMayaCam.mouseDown( event.getPos() );
	mCurrentCamera.mCam = mMayaCam.getCamera();
}

void KecskeAr::mouseDrag( MouseEvent event )
{
	if ( mInteractionMode != MODE_MOUSE )
		return;

	mMayaCam.setCurrentCam( mCurrentCamera.mCam );
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCurrentCamera.mCam = mMayaCam.getCamera();
}

void KecskeAr::resize()
{
	for ( CameraExt &ce : mCameras )
		ce.mCam.setAspectRatio( getWindowAspectRatio() );
	mCurrentCamera.mCam.setAspectRatio( getWindowAspectRatio() );
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

