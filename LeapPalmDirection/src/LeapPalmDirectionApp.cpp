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

#include "cinder/Camera.h"
#include "cinder/Cinder.h"

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#include "Cinder-LeapSdk.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LeapPalmDirectionApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		uint32_t mCallbackId;
		LeapSdk::HandMap mHands;
		LeapSdk::DeviceRef mLeap;
		void onFrame( LeapSdk::Frame frame );

		ci::CameraPersp mCamera;
};

void LeapPalmDirectionApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void LeapPalmDirectionApp::onFrame( LeapSdk::Frame frame )
{
	mHands = frame.getHands();
}

void LeapPalmDirectionApp::setup()
{
	mLeap = LeapSdk::Device::create();
	mCallbackId = mLeap->addCallback( &LeapPalmDirectionApp::onFrame, this );

	mCamera = CameraPersp( getWindowWidth(), getWindowHeight(), 60.0f, 0.01f, 1000.0f );
	mCamera.lookAt( Vec3f( 0.0f, 250.0f, 500.0f ), Vec3f( 0.0f, 250.0f, 0.0f ) );
}

void LeapPalmDirectionApp::update()
{
	if ( mLeap && mLeap->isConnected() )
	{
		mLeap->update();
	}
}

void LeapPalmDirectionApp::draw()
{
	gl::clear();
	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mCamera );

	gl::enableDepthRead();
	gl::enableDepthWrite();

	for ( auto handIter = mHands.cbegin(); handIter != mHands.cend(); ++handIter )
	{
		gl::color( Color::white() );
		const LeapSdk::Hand &hand = handIter->second;

		Quatf normToDir( math< float >::sqrt( .5f ), math< float >::sqrt( .5f), 0.f, 0.f ); // 90 degree rotation around x
		Vec3f dir0 = hand.getNormal() * normToDir; // direction from normal
		Quatf dirQuat( dir0, hand.getDirection() ); // rotation from calculated direction to actual one
		Quatf normQuat( Vec3f( 0, -1, 0 ), hand.getNormal() ); // rotation to actual normal
		Quatf rotQuat = normQuat * dirQuat; // final rotation

		// hand plane
		gl::pushModelView();
		gl::translate( hand.getPosition() );
		gl::rotate( rotQuat );
		gl::drawColorCube( Vec3f::zero(), Vec3f( 1, .1, 1 ) * 100 );
		gl::popModelView();
		gl::drawVector( hand.getPosition(), hand.getPosition() + hand.getNormal() * 70.f );
		gl::drawVector( hand.getPosition(), hand.getPosition() + hand.getDirection() * 70.f );
	}
}

void LeapPalmDirectionApp::shutdown()
{
	mLeap->removeCallback( mCallbackId );
	mHands.clear();
}

void LeapPalmDirectionApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( LeapPalmDirectionApp, RendererGl )

