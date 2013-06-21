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

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"

#include "AssimpLoader.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace mndl;

class MultiAssimpTest : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void resize();
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );

		void update();
		void draw();

		void keyDown( KeyEvent event );

	private:
		struct ModelInfo
		{
			assimp::AssimpLoaderRef mModel;
			bool mEnabled;
			bool mEnableSkinning;
			bool mEnableAnimation;
		};
		typedef std::shared_ptr< ModelInfo > ModelInfoRef;

		void loadModels( const fs::path &relativeDir );
		vector< ModelInfoRef > mModels;

		int mCameraIndex;
		vector< CameraPersp > mCameras;

		MayaCamUI mMayaCam;

		params::InterfaceGl mParams;
		float mCurrentCameraFov;
		float mCurrentCameraAspectRatio;
		float mCurrentCameraNearClip;
		float mCurrentCameraFarClip;
		Vec3f mCurrentCameraLocation;

		float mFps;
		bool mVerticalSyncEnabled = false;
};

void MultiAssimpTest::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1280, 800 );
}

void MultiAssimpTest::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 350, 400 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addParam( "Vertical sync", &mVerticalSyncEnabled );
	mParams.addSeparator();

	loadModels( "models" );

	mParams.addSeparator();
}

void MultiAssimpTest::loadModels( const fs::path &relativeDir )
{
	fs::path dataPath = app::getAssetPath( relativeDir );

	if ( dataPath.empty() )
	{
		app::console() << "Could not find model directory assets/" << relativeDir.string() << std::endl;
		return;
	}

	vector< string > cameraNames;

	// default camera
	CameraPersp cam;
	cam.setPerspective( 60, getWindowAspectRatio(), 0.1f, 1000.0f );
	cam.setEyePoint( Vec3f( 0, 7, 20 ) );
	cam.setCenterOfInterestPoint( Vec3f( 0, 7, 0 ) );
	cameraNames.push_back( "default" );
	mCameras.push_back( cam );

	// load models
	for ( fs::directory_iterator it( dataPath ); it != fs::directory_iterator(); ++it )
	{
		if ( fs::is_regular_file( *it ) &&
			 ( ( it->path().extension().string() == ".dae" ) ||
			   ( it->path().extension().string() == ".obj" ) ) )
		{
			assimp::AssimpLoaderRef model;

			try
			{
				model = assimp::AssimpLoader::create( getAssetPath( relativeDir / it->path().filename() ) );
			}
			catch ( const assimp::AssimpLoaderExc &exc  )
			{
				app::console() << "Unable to load model " << it->path() << ": " << exc.what() << std::endl;
			}

			if ( model )
			{
				model->setAnimation( 0 );
				model->enableTextures( true );

				ModelInfoRef mi = ModelInfoRef( new ModelInfo() );
				mi->mModel = model;
				mi->mEnabled = true;
				mi->mEnableSkinning = false;
				mi->mEnableAnimation = false;

				string filename = it->path().filename().stem().string();
				mParams.addParam( filename + " visible", &mi->mEnabled, "group=" + filename );
				mParams.addParam( filename + " skinning enabled", &mi->mEnableSkinning, "group=" + filename );
				mParams.addParam( filename + " animation enabled", &mi->mEnableAnimation, "group=" + filename );

				mModels.push_back( mi );

				// store model cameras
				for ( size_t i = 0; i < model->getNumCameras(); i++ )
				{
					string name = model->getCameraName( i );
					cameraNames.push_back( name );
					mCameras.push_back( model->getCamera( i ) );
				}
			}
		}
	}

	mParams.addSeparator();
	mCameraIndex = 0;
	mParams.addParam( "Cameras", cameraNames, &mCameraIndex );
	mParams.addParam( "Fov", &mCurrentCameraFov );
	mParams.addParam( "Aspect ratio", &mCurrentCameraAspectRatio, "", true );
	mParams.addParam( "Near clip", &mCurrentCameraNearClip, "", true );
	mParams.addParam( "Far clip", &mCurrentCameraFarClip, "", true );
	mParams.addParam( "Location", &mCurrentCameraLocation, "", true );
}

void MultiAssimpTest::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

	for ( auto mi : mModels )
	{
		if ( !mi->mEnabled )
			continue;

		assimp::AssimpLoaderRef model = mi->mModel;

		model->enableSkinning( mi->mEnableSkinning );
		model->enableAnimation( mi->mEnableAnimation );

		if ( model->getNumAnimations() )
		{
			double time = fmod( getElapsedSeconds(), model->getAnimationDuration( 0 ) );
			model->setTime( time );
		}
		model->update();
	}

	static int lastCamera = -1;
	static float lastFov = -1;
	if ( lastCamera != mCameraIndex )
	{
		mMayaCam.setCurrentCam( mCameras[ mCameraIndex ] );
		mCurrentCameraNearClip = mCameras[ mCameraIndex ].getNearClip();
		mCurrentCameraFarClip = mCameras[ mCameraIndex ].getFarClip();
		mCurrentCameraFov = mCameras[ mCameraIndex ].getFov();
		lastCamera = mCameraIndex;
		lastFov = -1;
	}
	mCurrentCameraAspectRatio = mMayaCam.getCamera().getAspectRatio();
	mCurrentCameraLocation = mMayaCam.getCamera().getEyePoint();
	if ( lastFov != mCurrentCameraFov )
	{
		CameraPersp cam = mMayaCam.getCamera();
		cam.setFov( mCurrentCameraFov );
		mMayaCam.setCurrentCam( cam );
		lastFov = mCurrentCameraFov;
	}
}

void MultiAssimpTest::draw()
{
	gl::clear();

	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mMayaCam.getCamera() );

	gl::enableDepthWrite();
	gl::enableDepthRead();

	for ( auto mi : mModels )
	{
		if ( !mi->mEnabled )
			continue;

		assimp::AssimpLoaderRef model = mi->mModel;

		gl::color( Color::white() );
		gl::pushModelView();
		model->draw();
		gl::popModelView();
	}

	mParams.draw();
}

void MultiAssimpTest::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void MultiAssimpTest::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void MultiAssimpTest::resize()
{
	CameraPersp cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}

void MultiAssimpTest::keyDown( KeyEvent event )
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

CINDER_APP_BASIC( MultiAssimpTest, RendererGl )

