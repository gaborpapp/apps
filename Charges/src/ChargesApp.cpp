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

#include <map>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/gl.h"

#include "Cinder-LeapSdk.h"
#include "mndlkit/params/PParams.h"

#include "EffectCharge.h"
#include "KawaseBloom.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ChargesApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		EffectCharge mEffectCharge;

		gl::Fbo mFbo;

		mndl::gl::fx::KawaseBloom mKawaseBloom;

		// leap
		uint32_t mLeapCallbackId;
		LeapSdk::HandMap mHands;
		LeapSdk::DeviceRef mLeap;
		void onFrame( LeapSdk::Frame frame );
		int64_t mTimestamp;

		std::map< int32_t, int64_t > mActiveFingers;

		mndl::kit::params::PInterfaceGl mParams;
		float mFps;
		float mLineWidth;
		float mBloomStrength;
		float mFingerDisapperanceThreshold;
};

void ChargesApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 640 );
}

void ChargesApp::setup()
{
	//gl::disableVerticalSync();

	gl::Fbo::Format format;
	format.enableDepthBuffer( false );
	format.setSamples( 4 );

	mFbo = gl::Fbo( 1280, 800, format );
	mKawaseBloom = mndl::gl::fx::KawaseBloom( mFbo.getWidth(), mFbo.getHeight() );

	mEffectCharge.setup();
	mEffectCharge.instantiate();
	mEffectCharge.setBounds( mFbo.getBounds() );

	// Leap
	mLeap = LeapSdk::Device::create();
	mLeapCallbackId = mLeap->addCallback( &ChargesApp::onFrame, this );

	// params
	mndl::kit::params::PInterfaceGl::load( "params.xml" );
	mParams = mndl::kit::params::PInterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addPersistentSizeAndPosition();
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addSeparator();
	mParams.addPersistentParam( "Line width", &mLineWidth, 4.5f, "min=.5 max=10 step=.1" );
	mParams.addPersistentParam( "Bloom strength", &mBloomStrength, .25, "min=0 max=1 step=.05" );
	mParams.addPersistentParam( "Finger disapperance thr", &mFingerDisapperanceThreshold, .1f, "min=0 max=2 step=.05" );

	mndl::kit::params::PInterfaceGl::showAllParams( false );
}

void ChargesApp::update()
{
	mFps = getAverageFps();

	// Update device
	if ( mLeap && mLeap->isConnected() )
	{
		mLeap->update();
	}

	vector< int32_t > currentFingersIds;

	for ( const std::pair< int32_t, LeapSdk::Hand > hand : mHands )
	{
		const LeapSdk::FingerMap &fingers = hand.second.getFingers();
		for ( const auto &fkv : fingers )
		{
			int32_t id = fkv.first;
			const LeapSdk::Finger &finger = fkv.second;

			currentFingersIds.push_back( id );

			// new finger?
			if ( mActiveFingers.find( id ) == mActiveFingers.end() )
			{
				mActiveFingers[ id ] = mTimestamp;
				mEffectCharge.addCursor( id, 0.f, 0.f, 1.f ); // init with (0, 0), will be updated below
			}

			// update finger
			const LeapSdk::ScreenMap &screens = mLeap->getScreens();
			if ( screens.begin() != screens.end() )
			{
				mActiveFingers[ id ] = mTimestamp;

				const LeapSdk::Screen &screen = screens.begin()->second;
				Vec3f normPos;
				screen.intersects( finger, normPos, true, 1.5f ); // normalized screen coordinates with 1.5 clamp ratio
				Vec2f fingertip = normPos.xy() * Vec2f( mFbo.getSize() );

				Vec3f screenPos;
				screen.intersects( finger, screenPos, false, 1.5f ); // screen coordinates with 1.5 clamp ratio
				float d = screenPos.distance( finger.getPosition() );
				const float dMin = 50.f;
				const float dMax = 500.f;
				const float sMin = 1.f;
				const float sMax = 100.f;
				d = math< float >::clamp( d, dMin, dMax );
				float strength = lmap( d, dMin, dMax, sMax, sMin );

				mEffectCharge.updateCursor( id, fingertip.x, fingertip.y, strength );
			}
		}
	}

	// erase disappeared fingers
	int64_t disappearThr = mFingerDisapperanceThreshold * 1000000;
	for ( auto it = mActiveFingers.begin(); it != mActiveFingers.end(); )
	{
		int32_t id = it->first;
		if ( find( currentFingersIds.begin(), currentFingersIds.end(), id ) == currentFingersIds.end() )
		{
			// seen earlier than the threshold?
			if ( mTimestamp - it->second > disappearThr )
			{
				mEffectCharge.removeCursor( id );
				it = mActiveFingers.erase( it );
			}
			else
			{
				it++;
			}
		}
		else
		{
			it++;
		}
	}
}

void ChargesApp::draw()
{
	mFbo.bindFramebuffer();
	gl::setViewport( mFbo.getBounds() );
	gl::setMatricesWindow( mFbo.getSize() );

	gl::clear( Color::black() );
	mEffectCharge.setLineWidth( mLineWidth );
	mEffectCharge.draw();

	mFbo.unbindFramebuffer();

	gl::color( Color::white() );
	gl::Texture output = mKawaseBloom.process( mFbo.getTexture(), 8, mBloomStrength );

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear( Color::black() );
	gl::color( Color::white() );
	gl::draw( output, getWindowBounds() );

	mndl::kit::params::PInterfaceGl::draw();
}

void ChargesApp::keyDown( KeyEvent event )
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
			mndl::kit::params::PInterfaceGl::showAllParams( !mParams.isVisible() );
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

// Called when Leap frame data is ready
void ChargesApp::onFrame( LeapSdk::Frame frame )
{
	mHands = frame.getHands();
	mTimestamp = frame.getTimestamp();
}

void ChargesApp::shutdown()
{
	mndl::kit::params::PInterfaceGl::save();

	mEffectCharge.deinstantiate();
	mLeap->removeCallback( mLeapCallbackId );
}

CINDER_APP_BASIC( ChargesApp, RendererGl )

