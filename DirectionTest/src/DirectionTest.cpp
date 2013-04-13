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

class DirectionTest : public AppBasic
{
	public:
		void mouseDown( MouseEvent event );

		void update();
		void draw();

	private:
		Vec2f mDirection = Vec2f( 0.f, 1.f );
		Vec2f mTarget;
};

void DirectionTest::update()
{
	if ( mTarget.lengthSquared() > EPSILON )
	{
		float angleDir = math< float >::atan2( mDirection.y, mDirection.x );
		float angleTarget = math< float >::atan2( mTarget.y, mTarget.x );

		float angleDif = angleTarget - angleDir;
		if ( angleDif > M_PI )
			angleDif -= 2 * M_PI;
		else
		if ( angleDif < -M_PI )
			angleDif += 2 * M_PI;
		float angle = angleDir + .05f * angleDif;
		mDirection = Vec2f( cos( angle ), sin( angle ) );
		console() << "dir: " << angleDir << " angleTarget: " << angleTarget << " angle: " << angle << endl;
	}
}

void DirectionTest::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear();

	Vec2f c( getWindowCenter() );
	gl::color( Color::white() );
	gl::drawLine( c, c + getWindowHeight() * .3f * mDirection );
	gl::color( Color( 1, 0, 0 ) );
	gl::drawLine( c, c + getWindowHeight() * .3f * mTarget );
}

void DirectionTest::mouseDown( MouseEvent event )
{
	Vec2i p = event.getPos() - getWindowCenter();
	float a = math< float >::atan2( p.y, p.x );
	mTarget = Vec2f( cos( a ), sin( a ) );
}

CINDER_APP_BASIC( DirectionTest, RendererGl )

