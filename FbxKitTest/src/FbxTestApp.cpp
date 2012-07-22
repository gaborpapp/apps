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

#include "fieldkit/fbx/FbxKit.h"
#include "fieldkit/fbx/CinderRenderer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace fieldkit;

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

		fbx::Scene *mFbxSceneRef;
		fbx::SceneController mFbxSceneController;
		fbx::Renderer mFbxRenderer;
		fbx::CinderRenderer mFbxCinderRenderer;
};

void FbxTestApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void FbxTestApp::setup()
{
	gl::disableVerticalSync();

	mFbxSceneRef = fbx::SceneImporter::load( getAssetPath( "humanoid.fbx" ).string() );
	mFbxSceneController = fbx::SceneController( mFbxSceneRef );

	mFbxRenderer.setMarkerDelegate( &mFbxCinderRenderer );
	mFbxRenderer.setSkeletonDelegate( &mFbxCinderRenderer );
	mFbxRenderer.setMeshDelegate( &mFbxCinderRenderer );
	mFbxRenderer.setLightDelegate( &mFbxCinderRenderer );
	mFbxRenderer.setNullDelegate( &mFbxCinderRenderer );
	mFbxRenderer.setCameraDelegate( &mFbxCinderRenderer );
	mFbxRenderer.setPerspectiveDelegate( &mFbxCinderRenderer );

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );


}
void FbxTestApp::update()
{
	mFbxSceneController.update();
}

void FbxTestApp::draw()
{
	gl::clear( Color::black() );

	mFbxRenderer.drawPerspective( mFbxSceneRef, 1 );
	mFbxRenderer.drawScene( mFbxSceneRef );

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

