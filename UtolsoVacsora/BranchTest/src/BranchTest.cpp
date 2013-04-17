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

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#include "mndlkit/params/PParams.h"

#include "Branch.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class BranchTestApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );

		void update();
		void draw();

	private:
		mndl::params::PInterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled;

		float mBearingDelta;
		float mLengthMin, mLengthMax;

		vector< BranchRef > mBranches;
		int mPointNum = 0;
		Vec2i mPoints[ 2 ];
};

void BranchTestApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1280, 800 );
}

void BranchTestApp::setup()
{
	mndl::params::PInterfaceGl::load( "params.xml" );
	mParams = mndl::params::PInterfaceGl( "Parameters", Vec2i( 200, 300 ), Vec2i( 16, 16 ) );
	mParams.addPersistentSizeAndPosition();
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addPersistentParam( "Vertical sync", &mVerticalSyncEnabled, false );
	mParams.addSeparator();

	mParams.addPersistentParam( "Bearing delta", &mBearingDelta, .12, "min=0 max=1 step=.01" );
	mParams.addPersistentParam( "Length min", &mLengthMin, 10, "min=1 max=512 step=1" );
	mParams.addPersistentParam( "Length max", &mLengthMax, 64, "min=1 max=512 step=1" );
}

void BranchTestApp::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );
}

void BranchTestApp::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear();
	gl::color( Color::white() );
	for ( auto &b : mBranches )
	{
		b->draw();
	}

	mParams.draw();
}


void BranchTestApp::mouseDown( MouseEvent event )
{
	mPoints[ mPointNum ] = event.getPos();
	mPointNum++;
	if ( mPointNum == 2 )
	{
		mBranches.push_back( Branch::create( mPoints[ 0 ], mPoints[ 1 ] ) );
		mBranches.back()->setStemBearingDelta( mBearingDelta * 2 * M_PI );
		mBranches.back()->setStemLength( mLengthMin, mLengthMax );
		mBranches.back()->update();
		mPointNum = 0;
	}
}

void BranchTestApp::keyDown( KeyEvent event )
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

		case KeyEvent::KEY_SPACE:
			mBranches.clear();
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

void BranchTestApp::shutdown()
{
	mndl::params::PInterfaceGl::save();
}

CINDER_APP_BASIC( BranchTestApp, RendererGl )

