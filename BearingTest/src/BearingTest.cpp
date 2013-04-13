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

using namespace ci;
using namespace ci::app;
using namespace std;

class BearingTest : public AppBasic
{
	public:
		void mouseDown( MouseEvent event );

		void update();
		void draw();

	private:
		Vec2f mDirection = Vec2f( 0.f, 1.f );
		Vec2f mTarget = Vec2f( 0.f, -1.f );
		float mAngleDelta = .3f;
};

//! Returns the minimum directional angle between the two vectors in [-PI, PI]
static float minimumAngle( const Vec2f &a, const Vec2f &b )
{
	float angleA = math< float >::atan2( a.y, a.x );
	float angleB = math< float >::atan2( b.y, b.x );

	float angle = angleB - angleA;
	if ( angle > M_PI )
		angle -= 2 * M_PI;
	else
	if ( angle < -M_PI )
		angle += 2 * M_PI;
	return angle;
}

void BearingTest::update()
{
}

void BearingTest::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear();

	float angleDir = math< float >::atan2( mDirection.y, mDirection.x );
	float angleTarget = math< float >::atan2( mTarget.y, mTarget.x );

	Vec2f c( getWindowCenter() );
	gl::color( Color::white() );
	gl::drawLine( c, c + getWindowHeight() * .3f * mDirection );

	gl::color( Color::gray( .3f ) );
	float a0 = angleDir - mAngleDelta;
	float a1 = angleDir + mAngleDelta;
	gl::drawLine( c, c + getWindowHeight() * .4f * Vec2f( cos( a0 ), sin( a0 ) ) );
	gl::drawLine( c, c + getWindowHeight() * .4f * Vec2f( cos( a1 ), sin( a1 ) ) );

	gl::color( Color( 1, 0, 0 ) );
	gl::drawLine( c, c + getWindowHeight() * .3f * mTarget );
	gl::color( Color( .3f, 0, 0 ) );
	float t0 = angleTarget - mAngleDelta;
	float t1 = angleTarget + mAngleDelta;
	gl::drawLine( c, c + getWindowHeight() * .4f * Vec2f( cos( t0 ), sin( t0 ) ) );
	gl::drawLine( c, c + getWindowHeight() * .4f * Vec2f( cos( t1 ), sin( t1 ) ) );

	float angle = minimumAngle( mDirection, mTarget );
	float angleMin = math< float >::clamp( angle - mAngleDelta, -mAngleDelta, mAngleDelta );
	float angleMax = math< float >::clamp( angle + mAngleDelta, -mAngleDelta, mAngleDelta );
	float d0 = angleDir + angleMin;
	float d1 = angleDir + angleMax;
	gl::color( Color( 0, 1, 0 ) );
	gl::drawSolidTriangle( c,
			c + getWindowHeight() * .2f * Vec2f( cos( d0 ), sin( d0 ) ),
			c + getWindowHeight() * .2f * Vec2f( cos( d1 ), sin( d1 ) ) );
}

void BearingTest::mouseDown( MouseEvent event )
{
	Vec2i p = event.getPos() - getWindowCenter();
	float a = math< float >::atan2( p.y, p.x );
	if ( event.isLeft() )
		mTarget = Vec2f( cos( a ), sin( a ) );
	else
		mDirection = Vec2f( cos( a ), sin( a ) );
}

CINDER_APP_BASIC( BearingTest, RendererGl )

