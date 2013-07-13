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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class KaleidoscopeApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled = false;

		gl::Texture mTexture;
		gl::GlslProg mShader;
		int mNumReflectionLines;
		float mRotation;
		Vec2f mCenter;
};

void KaleidoscopeApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 800 );
}

void KaleidoscopeApp::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addParam( "Vertical sync", &mVerticalSyncEnabled );
	mParams.addSeparator();
	mNumReflectionLines = 3;
	mParams.addParam( "Reflection lines", &mNumReflectionLines, "min=0, max=32" );
	mRotation = 0.f;
	mParams.addParam( "Rotation", &mRotation, "step=.01" );
	mCenter = Vec2f( .5f, .5f );
	mParams.addParam( "Center X", &mCenter.x, "min=0 max=1 step=.005 group='Center'" );
	mParams.addParam( "Center Y", &mCenter.y, "min=0 max=1 step=.005 group='Center'" );

	mTexture = loadImage( loadAsset( "tx.jpg" ) );
	mTexture.setWrap( true, true );

	try
	{
		mShader = gl::GlslProg( loadAsset( "Kaleidoscope.vert" ),
								loadAsset( "Kaleidoscope.frag" ) );
	}
	catch ( gl::GlslProgCompileExc &exc )
	{
		console() << exc.what() << endl;
	}
}

void KaleidoscopeApp::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );
}

void KaleidoscopeApp::draw()
{
	gl::clear();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	if ( mShader )
	{
		mShader.bind();
		mShader.uniform( "center", mCenter );
		mShader.uniform( "numReflectionLines", mNumReflectionLines );
		mShader.uniform( "rotation", mRotation );
		mShader.uniform( "txt", 0 );
	}

	if ( mTexture )
		gl::draw( mTexture, getWindowBounds() );

	if ( mShader )
		mShader.unbind();

	mParams.draw();
}

void KaleidoscopeApp::keyDown( KeyEvent event )
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

CINDER_APP_BASIC( KaleidoscopeApp, RendererGl )

