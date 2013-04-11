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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "cinder/Path2d.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PathTestApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled = false;

		Path2d mPath;
		void addArc( Vec2f p1 );
		float mCurvature;
};

void PathTestApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1280, 800 );
}

void PathTestApp::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addParam( "Vertical sync", &mVerticalSyncEnabled );
	mParams.addSeparator();

	mCurvature = .5f;
	mParams.addParam( "Curvature", &mCurvature, "min=-1 max=1 step=.01" );
}

void PathTestApp::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );
}

void PathTestApp::draw()
{
	gl::clear( Color::black() );

	gl::color( Color::white() );
	gl::draw( mPath );

	gl::color( Color( 1, 0, 0 ) );
	if ( !mPath.empty() )
		gl::drawSolidCircle( mPath.getPosition( 0.f ), 3.f );
	for ( size_t i = 0; i < mPath.getNumSegments(); i++ )
	{
		gl::drawSolidCircle( mPath.getSegmentPosition( i, 1.f ), 3.f );
	}
	params::InterfaceGl::draw();
}

void PathTestApp::addArc( Vec2f p1 )
{
	if ( mPath.empty() )
	{
		mPath.moveTo( p1 );
	}
	else
	{
		Vec2f p0 = mPath.getCurrentPoint();

		if ( math< float >::abs( mCurvature ) < EPSILON )
			mCurvature = 0.f;
		float sign = math< float >::signum( mCurvature );
		if ( sign != 0.f )
		{
			// curvature 0 results in a far center, so the arc looks like a line
			float centerDistance = sign * lerp( 4.f, 0.f, math< float >::abs( mCurvature ) );
			Vec2f d( p1 - p0 );
			Vec2f c( ( p0 + p1 ) * .5f - Vec2f( -d.y, d.x ) * centerDistance );
			Vec2f d0( p0 - c );
			float a0 = math< float >::atan2( d0.y, d0.x );
			Vec2f d1( p1 - c );
			float a1 = math< float >::atan2( d1.y, d1.x );
			mPath.arc( c, d0.length(), a0, a1, sign > 0.f ? false : true );
		}
		else
		{
			mPath.lineTo( p1 );
		}
	}
}

void PathTestApp::mouseDown( MouseEvent event )
{
	addArc( event.getPos() );
}

void PathTestApp::keyDown( KeyEvent event )
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
			mPath.clear();
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( PathTestApp, RendererGl )

