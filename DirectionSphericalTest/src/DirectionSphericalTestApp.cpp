/*
 Copyright (C) 2014 Gabor Papp

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

#include "cinder/Camera.h"
#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/Rand.h"
#include "cinder/Quaternion.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DirectionSphericalTestApp : public AppBasic
{
 public:
	void prepareSettings( Settings *settings );
	void setup();

	void keyDown( KeyEvent event );
	void mouseDown( MouseEvent event );

	void update();
	void draw();

	void resize();

 private:
	CameraPersp mCamera;

	Vec3f mVectorSpherical;
	Vec3f mVectorTarget;
	Vec3f mVectorSlerp;
	Vec3f mVectorQuat;
};

void DirectionSphericalTestApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

template < typename T >
Vec3< T > cartesianToSpherical( const Vec3< T > &cartesian )
{
	T r = cartesian.length();
	T theta = math< T >::acos( cartesian.z / r );
	T phi = math< T >::atan2( cartesian.y, cartesian.x );
	// theta [0, pi], phi [-pi, pi]
	return Vec3< T >( r, theta, phi );
}

template < typename T >
Vec3< T > sphericalToCartesian( const Vec3< T > &spherical )
{
	T r = spherical.x;
	T theta = spherical.y;
	T phi = spherical.z;
	return Vec3< T >( r * math< T >::sin( theta ) * math< T >::cos( phi ),
					  r * math< T >::sin( theta ) * math< T >::sin( phi ),
					  r * math< T >::cos( theta ) );
}

void DirectionSphericalTestApp::setup()
{
	mCamera.setPerspective( 60.0f, getWindowAspectRatio(), 0.1f, 1000.0f );
	mCamera.setEyePoint( Vec3f( 0.0f, 0.0f, 5.f ) );
	mCamera.setCenterOfInterestPoint( Vec3f::zero() );

	mVectorSpherical = Vec3f( 1.0f, M_PI / 2.0f, M_PI / 2.0f );
	mVectorSlerp = sphericalToCartesian( mVectorSpherical );
	mVectorTarget = mVectorSpherical;
	mVectorQuat = mVectorSlerp;
}

void DirectionSphericalTestApp::update()
{
	// sperical coordinate
	float thetaDiff = mVectorTarget.y - mVectorSpherical.y;
	float phiDiff = mVectorTarget.z - mVectorSpherical.z;
	if ( phiDiff > M_PI )
	{
		phiDiff -= 2 * M_PI;
	}
	else
	if ( phiDiff < -M_PI )
	{
		phiDiff += 2 * M_PI;
	}
	mVectorSpherical.y = mVectorSpherical.y + 0.05f * thetaDiff;
	mVectorSpherical.z = mVectorSpherical.z + 0.05f * phiDiff;

	// vector slerp - cannot handle parallel target
	mVectorSlerp = mVectorSlerp.slerp( 0.06f, sphericalToCartesian( mVectorTarget ) );

	// quaternion slerp
	Quatf q( mVectorQuat, sphericalToCartesian( mVectorTarget ) );
	Quatf q0;
	mVectorQuat = q0.slerp( 0.07f, q ) * mVectorQuat;
}

void DirectionSphericalTestApp::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mCamera );
	gl::clear();

	gl::enableAlphaBlending();

	Vec3f targetCartesian = sphericalToCartesian( mVectorTarget );
	gl::color( ColorA( 1, 0, 0, 0.5f ) );
	gl::drawVector( Vec3f::zero(), targetCartesian );

	Vec3f vectorCartesian = sphericalToCartesian( mVectorSpherical );
	gl::color( ColorA( 1, 1, 1, 0.5f ) );
	gl::drawVector( Vec3f::zero(), vectorCartesian );

	gl::color( ColorA( 0, 1, 0, 0.5f ) );
	gl::drawVector( Vec3f::zero(), mVectorSlerp );

	gl::color( ColorA( 0, 0, 1, 0.5f ) );
	gl::drawVector( Vec3f::zero(), mVectorQuat );

	gl::disableAlphaBlending();
}

void DirectionSphericalTestApp::mouseDown( MouseEvent event )
{
	if ( event.isShiftDown() )
	{
		Vec3f t = sphericalToCartesian( mVectorTarget );
		mVectorTarget = cartesianToSpherical( -t );
	}
	else
	{
		mVectorTarget = Vec3f( 1.0f, Rand::randFloat( 0.0f, M_PI ), Rand::randFloat( -M_PI, M_PI ) );
	}
	mVectorSlerp = sphericalToCartesian( mVectorSpherical );
	mVectorQuat = mVectorSlerp;
}

void DirectionSphericalTestApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( ! isFullScreen() )
			{
				setFullScreen( true );
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

void DirectionSphericalTestApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

CINDER_APP_BASIC( DirectionSphericalTestApp, RendererGl )

