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
#include "cinder/params/Params.h"
#include "cinder/Camera.h"

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

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		// Fbx
		S9::S9FbxLoader mFBXLoader;
		S9::S9FbxDrawer mFBXDrawer;

		shared_ptr< S9::FbxDrawable > mDrawable;
		int mBoneIndex;

		CameraPersp mCam;
};

void FbxTestApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void FbxTestApp::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );

	mDrawable = mFBXLoader.load( getAssetPath("seymour.fbx").string() );

	// list all names
	vector< string > boneNames;
	for ( map< string, int >::iterator it = mDrawable->meshes[0]->boneNameToIndex.begin();
			it != mDrawable->meshes[0]->boneNameToIndex.end(); ++it )
	{
		boneNames.push_back( it->first );
		console() << it->first << " " << it->second << endl;
	}
	if ( boneNames.empty() )
		boneNames.push_back( "NO BONES!" );

	mBoneIndex = 0;
	mParams.addParam( "bones", boneNames, &mBoneIndex );

	gl::enableDepthRead();
	gl::enableDepthWrite();
}


void FbxTestApp::update()
{
}

void FbxTestApp::draw()
{
	gl::pushModelView();

	mCam.lookAt( Vec3f( 0, 15, -5 ), Vec3f( 0, 0, -5 ) );
	mCam.setPerspective( 60, getWindowAspectRatio(), 0.01, 500 );
	gl::setMatrices( mCam );

	gl::clear( Color::black() );

	gl::enable( GL_TEXTURE_2D );
	mFBXDrawer.draw( mDrawable );
	gl::disable( GL_TEXTURE_2D );

	gl::popModelView();

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

CINDER_APP_BASIC( FbxTestApp, RendererGl( RendererGl::AA_NONE ) )

