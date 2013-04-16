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
#include "cinder/params/Params.h"
#include "cinder/Path2d.h"
#include "cinder/Rand.h"

#include "mndlkit/params/PParams.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PathTestApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );

		void update();
		void draw();

	private:
		mndl::params::PInterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled;
		bool mDrawSegmentPositions;

		Path2d mPath;
		void addArc( Vec2f p1 );
		/*
		float mCurvatureMin, mCurvatureMax;
		float mStemLengthMin, mStemLengthMax;
		*/
		Vec2f mLastCenter, mLastBisector, mLastNormal;
		Vec2f mLastEndPoint, mLastTangent;
		bool mLastDist;
		Path2d mLastPath;
};

void PathTestApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1280, 800 );
}

void PathTestApp::setup()
{
	mndl::params::PInterfaceGl::load( "params.xml" );
	mParams = mndl::params::PInterfaceGl( "Parameters", Vec2i( 200, 300 ), Vec2i( 16, 16 ) );
	mParams.addPersistentSizeAndPosition();
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addPersistentParam( "Vertical sync", &mVerticalSyncEnabled, false );
	mParams.addPersistentParam( "Draw segment positions", &mDrawSegmentPositions, false );
	mParams.addSeparator();

	/*
	mParams.addPersistentParam( "Curvature min", &mCurvatureMin, .5f, "min=0 max=1 step=.01" );
	mParams.addPersistentParam( "Curvature max", &mCurvatureMax, .7f, "min=0 max=1 step=.01" );
	mParams.addPersistentParam( "Stem length min", &mStemLengthMin, .3f, "min=0 max=1 step=.01" );
	mParams.addPersistentParam( "Stem length max", &mStemLengthMax, .5f, "min=0 max=1 step=.01" );
	*/
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

	if ( mDrawSegmentPositions )
	{
		gl::color( Color( 1, 0, 0 ) );
		if ( !mPath.empty() )
			gl::drawSolidCircle( mPath.getPosition( 0.f ), 3.f );
		/*
		for ( size_t i = 0; i < mPath.getNumSegments(); i++ )
		{
			gl::color( Color( 1, 0, 0 ) );
			Vec2f p = mPath.getSegmentPosition( i, 1.f );
			gl::drawSolidCircle( p, 3.f );
			Vec2f tangent = p - mPath.getSegmentPosition( i, .98f );
			tangent.normalize();
			gl::drawLine( p, p + 30.f * tangent );

			gl::color( Color( 1, 1, 0 ) );
			p = mPath.getSegmentPosition( i, .0f );
			tangent = mPath.getSegmentPosition( i, .02f ) - p;
			tangent.normalize();
			gl::drawLine( p, p + 30.f * tangent );
		}
		*/
		gl::color( Color( 1, 0, 0 ) );
		gl::drawSolidCircle( mLastEndPoint, 3.f );

		if ( mLastDist )
			gl::color( Color( 1, 1, 0 ) );
		else
			gl::color( Color( 0, 1, 0 ) );
		gl::drawSolidCircle( mLastCenter, 3.f );
		gl::color( Color( 1, 0, 1 ) );
		gl::drawSolidCircle( mLastBisector, 3.f );
		gl::color( Color( 0, 0, 1 ) );
		gl::drawLine( mLastEndPoint, mLastEndPoint + 30.f * mLastNormal );
		gl::color( Color( 0, 1, 1 ) );
		gl::drawLine( mLastEndPoint, mLastEndPoint + 30.f * mLastTangent );
	}
	mndl::params::PInterfaceGl::draw();
}

void PathTestApp::addArc( Vec2f p1 )
{
	if ( mPath.empty() )
	{
		mPath.moveTo( p1 );
		return;
	}

	size_t segmentNum = mPath.getNumSegments();
	if ( segmentNum == 0 )
	{
		mPath.lineTo( p1 );
		return;
	}

	// last point
	Vec2f p0 = mPath.getSegmentPosition( segmentNum - 1, 1.f );
	// same point as the last one -> skip
	if ( p0.distanceSquared( p1 ) < EPSILON )
		return;

	// segment tangent - direction of the last segment
	Vec2f tangent = p0 - mPath.getSegmentPosition( segmentNum - 1, .98f );

	Vec2f n( -tangent.y, tangent.x ); // segment normal
	Vec2f d( p1 - p0 ); // distance of the arc center and the last path point
	d *= .5f;
	Vec2f b( -d.y, d.x ); // arc bisector

	// the arc center is the intersection of the segment normal and the bisector
	float s;
	if ( n.x == 0.f )
	{
		if ( b.x == 0.f ) // parallel
		{
			mPath.lineTo( p1 );
			return;
		}
		s = -d.x / b.x;
	}
	else
	{
		float den = b.x * n.y / n.x - b.y;
		if ( den == 0.f ) // parallel
		{
			mPath.lineTo( p1 );
			return;
		}
		s = ( d.y - d.x * n.y / n.x ) / ( b.x * n.y / n.x - b.y );
	}
	Vec2f c( ( p0 + p1 ) *.5f + s * b ); // arc center

	// debug
	mLastCenter = c;
	mLastBisector = ( p1 + p0 ) * .5f;
	mLastNormal = n.normalized();
	mLastEndPoint = p0;
	mLastTangent = tangent.normalized();

	Vec2f d0( p0 - c );

	float a0 = math< float >::atan2( d0.y, d0.x );
	Vec2f d1( p1 - c );
	float a1 = math< float >::atan2( d1.y, d1.x );

	// segment normal line and segment bisector distance
	tangent.normalize();
	float C = tangent.dot( p0 );
	float dist = tangent.dot( ( p0 + p1 ) * .5f ) - C;
	// debug
	mLastDist = ( dist < 0.f );

	// arc direction depends on the quadrant, in which the new point resides
	bool forward = ( s > 0.f ) ^ ( dist < 0.f );
	mPath.arc( c, d0.length(), a0, a1, forward );

	// TODO: use a line if the new point lies on the tangent
	// mPath.lineTo( p1 );
}

void PathTestApp::mouseDown( MouseEvent event )
{
	mLastPath = mPath;
	addArc( event.getPos() );
}

void PathTestApp::mouseDrag( MouseEvent event )
{
	mPath = mLastPath;
	mouseDown( event );
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

void PathTestApp::shutdown()
{
	mndl::params::PInterfaceGl::save();
}

CINDER_APP_BASIC( PathTestApp, RendererGl )

