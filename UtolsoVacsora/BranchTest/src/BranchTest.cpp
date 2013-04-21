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

		ColorA  mColor;
		float mBearingDelta;
		float mLengthMin, mLengthMax;
		float mSpawnInterval;
		float mBranchAngle;
		float mBranchItemScale;
		double mGrowSpeed;
		float mThickness;

		BranchRef mTextureBranch; //< branch for holding the textures
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

	mParams.addPersistentParam( "Color", &mColor, ColorA::white() );
	mParams.addPersistentParam( "Bearing delta", &mBearingDelta, .12, "min=0 max=1 step=.01" );
	mParams.addPersistentParam( "Length min", &mLengthMin, 10, "min=1 max=512" );
	mParams.addPersistentParam( "Length max", &mLengthMax, 64, "min=1 max=512" );
	mParams.addPersistentParam( "Spawn interval", &mSpawnInterval, 32.f, "min=1 max=512" );
	mParams.addPersistentParam( "Branch angle", &mBranchAngle, .25f, "min=0 max=1. step=.01" );
	mParams.addPersistentParam( "Item scale", &mBranchItemScale, .5f, "min=0 max=16 step=.05" );
	mParams.addPersistentParam( "Grow speed", &mGrowSpeed, 3., "min=0 step=.05" );
	mParams.addPersistentParam( "Thickness", &mThickness, 16.f, "min=1 max=256" );

	mTextureBranch = Branch::create();
	mTextureBranch->loadTextures( "moppi_flower" );
	//mTextureBranch->loadTextures( "moppi_tron" );
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
	gl::enableAlphaBlending();
	for ( auto &b : mBranches )
	{
		b->draw();
	}
	gl::disableAlphaBlending();

	/*
	gl::color( Color( 0, 0, 1 ) );
	gl::drawSolidCircle( mPoints[ 0 ], 2 );
	gl::drawSolidCircle( mPoints[ 1 ], 2 );
	*/

	mParams.draw();
}

void BranchTestApp::mouseDown( MouseEvent event )
{
	mPoints[ mPointNum ] = event.getPos();
	mPointNum++;
	if ( mPointNum == 2 )
	{
		BranchRef b = Branch::create();
		b->setColor( mColor );
		b->setStemBearingDelta( mBearingDelta * 2 * M_PI );
		b->setStemLength( mLengthMin, mLengthMax );
		b->setSpawnInterval( mSpawnInterval );
		b->setItemScale( mBranchItemScale );
		b->setBranchAngle( mBranchAngle * M_PI );
		b->setGrowSpeed( mGrowSpeed );
		b->setThickness( mThickness );
		b->setTextures( mTextureBranch->getStemTexture(),
				mTextureBranch->getBranchTexture(), mTextureBranch->getLeafTextures(),
				mTextureBranch->getFlowerTextures() );
		b->setup( mPoints[ 0 ], mPoints[ 1 ] );
		b->start();
		b->resize( getWindowSize() );
		mBranches.push_back( b );
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

