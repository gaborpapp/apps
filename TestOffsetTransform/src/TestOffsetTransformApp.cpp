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

 There are 2 vectors with given positions and orientations. Transform the first
 and calculate the transformation of the second using the initial relation
 between the two vectors.
*/

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "cinder/Camera.h"
#include "cinder/CinderMath.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Quaternion.h"
#include "cinder/Vector.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TestOffsetTransformApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );

		void draw();

	private:
		params::InterfaceGl mParams;

		CameraPersp mCam;
		MayaCamUI mMayaCam;

		Vec3f mPos0, mPos1;
		Quatf mRot0, mRot1;
		Vec3f mPos;
		Quatf mRot;
};

void TestOffsetTransformApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void TestOffsetTransformApp::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 500 ) );

	mCam.lookAt( Vec3f( 0, 0, 10 ), Vec3f::zero() );
	mCam.setPerspective( 45, getWindowAspectRatio(), 0.1f, 1000.0f );
	mMayaCam.setCurrentCam( mCam );

	mPos0 = Vec3f::zero();
	mParams.addParam( "mPos0", &mPos0 );
	mRot0 = Quatf( 0, 0, M_PI / 3. );
	mParams.addParam( "mRot0", &mRot0, "showval=true" );

	mPos1 = Vec3f( 0, 0, 0 );
	mParams.addParam( "mPos1", &mPos1 );
	mRot1 = Quatf::identity();
	mParams.addParam( "mRot1", &mRot1, "showval=true" );

	mPos = Vec3f( 1.5, 0, 0 );
	mParams.addParam( "mPos", &mPos );
	mRot = Quatf::identity();
	mParams.addParam( "mRot", &mRot, "showval=true" );
}

void TestOffsetTransformApp::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mMayaCam.getCamera() );

	gl::clear();

	gl::enableDepthRead();
	gl::enableDepthWrite();

	glLineWidth( 3 );

	// v0
	gl::color( Color( 1, 0, 0 ) );
	gl::pushModelView();
	gl::translate( mPos0 );
	gl::rotate( mRot0 );
	gl::drawVector( Vec3f::zero(), Vec3f( 1, 0, 0 ) );
	gl::popModelView();

	// v1
	gl::color( Color( 0, 0, 1 ) );
	gl::pushModelView();
	gl::translate( mPos1 );
	gl::rotate( mRot1 );
	gl::drawVector( Vec3f::zero(), Vec3f( 1, 0, 0 ) );
	gl::popModelView();

	glLineWidth( 1 );

	// v0' - v0 rotated and translated with mRot, mPos
	Vec3f p = mPos0 + mPos;
	Quatf r = mRot0 * mRot;
	r.normalize();
	gl::color( ColorA( 1, 0, 0, .5 ) );
	gl::pushModelView();
	gl::translate( p );
	gl::rotate( r );
	gl::drawVector( Vec3f::zero(), Vec3f( 1, 0, 0 ) );
	gl::popModelView();

	// v1' - calculated from v0's transform and the offset
	gl::color( ColorA( 0, 0, 1, .5 ) );
	gl::pushModelView();
	// rotate from v0 to v1 (first 3 terms), cancel out v0 inital rotation from v (4)
	// first two terms are identity
	//Quatf r2 = mRot0 * ( mRot0.inverse() * mRot1 ) * mRot0.inverse() * r;
	Quatf r2 = mRot1 * mRot0.inverse() * r;
	// similarly to rotation,
	// cancel out v0 initial rotation, and rotate position offset
	Vec3f pos01 = mPos1 - mPos0;
	Vec3f p2 = p + pos01 * mRot0.inverse() * r;
	gl::translate( p2 );
	gl::rotate( r2 );
	gl::drawVector( Vec3f::zero(), Vec3f( 1, 0, 0 ) );
	gl::popModelView();
	gl::drawVector( p, p2 );

	mParams.draw();
}

void TestOffsetTransformApp::keyDown( KeyEvent event )
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

void TestOffsetTransformApp::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void TestOffsetTransformApp::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

CINDER_APP_BASIC( TestOffsetTransformApp, RendererGl )

