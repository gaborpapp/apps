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

#include "algorithm"

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Xml.h"

#include "S9FbxLoader.h"
#include "S9FbxDrawer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FbxTestApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void resize( ResizeEvent event );
		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;
		void setupParams();

		// Fbx
		S9::S9FbxLoader mFBXLoader;
		S9::S9FbxDrawer mFBXDrawer;

		MayaCamUI mMayaCam;

		void resetRotations();

		shared_ptr< S9::FbxDrawable > mDrawable;
		int mBoneIndex;
		bool mNoBones;

		vector< string > mBoneNames;
		struct Bone
		{
			Quatf rot;
			int id;
		};
		vector< Bone > mBones;

		float mFps;
};

void FbxTestApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void FbxTestApp::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );

	// load model
	XmlTree doc( loadFile( getAssetPath( "config.xml" ) ) );
	string fbxPath = doc.getChild( "model" ).getValue();
	mDrawable = mFBXLoader.load( getAssetPath( fbxPath ).string() );

	// list all bone names
	for ( map< string, int >::iterator it = mDrawable->meshes[0]->boneNameToIndex.begin();
			it != mDrawable->meshes[0]->boneNameToIndex.end(); ++it )
	{
		mBoneNames.push_back( it->first );
		console() << it->first << " " << it->second << endl;
	}
	sort( mBoneNames.begin(), mBoneNames.end() );

	if ( mBoneNames.empty() )
	{
		mBoneNames.push_back( "NO BONES!" );
		mNoBones = true;
	}
	else
	{
		for ( vector< string >::const_iterator it = mBoneNames.begin(); it != mBoneNames.end(); ++it )
		{
			Bone b;
			b.id = mDrawable->meshes[0]->boneNameToIndex[ *it ];
			mBones.push_back( b );
		}
		mNoBones = false;
	}

	mBoneIndex = 0;

	gl::enableDepthRead();
	gl::enableDepthWrite();

	CameraPersp cam;
	cam.setEyePoint( Vec3f( 0, 15, -5 ) );
	cam.setCenterOfInterestPoint( Vec3f( 1, 1, -5 ) );
	cam.setPerspective( 60.0, getWindowAspectRatio(), 1.0, 1000.0 );
	cam.setWorldUp( Vec3f( 1, 1, -1 ) );
	mMayaCam.setCurrentCam( cam );
}

void FbxTestApp::setupParams()
{
	mParams.clear();

	mParams.addParam( "Bones", mBoneNames, &mBoneIndex, "", mNoBones );

	mParams.addSeparator();

	if ( mNoBones )
		return;

	mParams.addParam( "Rotation", &mBones[ mBoneIndex ].rot, "opened=true" );
	mParams.addParam( "Id", &mBones[ mBoneIndex ].id, "", true );
	mParams.addButton( "Reset", bind( &FbxTestApp::resetRotations, this ) );

	mParams.addSeparator();
	mParams.addParam( "Fps", &mFps, "", true );
}

void FbxTestApp::resetRotations()
{
	//mFBXDrawer.resetRotations( mDrawable->meshes[0] );
	for ( vector< Bone >::iterator it = mBones.begin(); it != mBones.end(); ++it )
	{
		it->rot.set( 1, 0, 0, 0 );
		Matrix44d m = it->rot.toMatrix44();
		mFBXDrawer.rotateBone( mDrawable->meshes[0], it->id, m );
	}
}

void FbxTestApp::update()
{
	static int lastBoneIndex = -1;

	if ( mBoneIndex != lastBoneIndex )
	{
		setupParams();
		lastBoneIndex = mBoneIndex;
	}

	if ( !mNoBones )
	{
		Matrix44d m = mBones[ mBoneIndex ].rot.toMatrix44();
		mFBXDrawer.rotateBone( mDrawable->meshes[0], mBones[ mBoneIndex ].id, m );
	}

	mFps = getAverageFps();
}

void FbxTestApp::draw()
{
	gl::pushMatrices();

	/*
	CameraPersp cam;
	cam.lookAt( Vec3f( 0, 15, -5 ), Vec3f( 0, 0, -5 ) );
	cam.setPerspective( 60, getWindowAspectRatio(), 0.01, 500 );
	gl::setMatrices( cam );
	*/
	gl::setMatrices( mMayaCam.getCamera() );

	gl::clear( Color::black() );

	gl::enable( GL_TEXTURE_2D );
	mFBXDrawer.draw( mDrawable );
	gl::disable( GL_TEXTURE_2D );

	gl::popMatrices();

	params::InterfaceGl::draw();
}

void FbxTestApp::keyDown( KeyEvent event )
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

void FbxTestApp::resize( ResizeEvent event )
{
	CameraPersp cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}

void FbxTestApp::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void FbxTestApp::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

CINDER_APP_BASIC( FbxTestApp, RendererGl( RendererGl::AA_NONE ) )

