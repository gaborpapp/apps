/*
 Copyright (C) 2012 Gabor Papp

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

#include <sstream>
#include <vector>
#include <string>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Area.h"
#include "cinder/Rect.h"

#include "PParams.h"
#include "Effect.h"

#include "NI.h"

#include "Utils.h"

#include "Black.h"
#include "SpeechShop.h"
#include "DepthMerge.h"
#include "Acacia.h"
#include "Moon.h"
#include "VTT.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// display sizes, secondary is on the right
const int WINDOW_POSY = 0;
const int MAIN_WIDTH = 1024;
const int MAIN_HEIGHT = 768 - WINDOW_POSY;
const int SECONDARY_WIDTH = 1024;
const int SECONDARY_HEIGHT = 768 - WINDOW_POSY;

class KotoKaoriApp : public ci::app::AppBasic
{
	public:
		KotoKaoriApp();

		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void keyDown( ci::app::KeyEvent event );

		void mouseDown( ci::app::MouseEvent event );
		void mouseDrag( ci::app::MouseEvent event );

		void resize( ci::app::ResizeEvent event );

		void update();
		void draw();

	private:
		void saveScreenshot();

		params::PInterfaceGl mParams;

		vector<Effect *> mEffects;

		enum {
			EFFECT_FEKETE = 0,
			EFFECT_BESZEDBOLT,
			EFFECT_KEZEKLABAK,
			EFFECT_AKAC,
			EFFECT_HOLD
		};

		int mEffectIndex;
		int mPrevEffectIndex;

		bool mFullScreen;
		float mFps;

		vector<string> mEffectNames;

		OpenNI mNI;
		bool mNIMirror;

		gl::Fbo mFbo;

		RectMapping mWindow2Fbo;
};

KotoKaoriApp::KotoKaoriApp()
{
	mEffectNames.push_back( "Fekete" );
	mEffectNames.push_back( "BeszedBolt" );
	mEffectNames.push_back( "KezekLabak" );
	mEffectNames.push_back( "Japan akac" );
	mEffectNames.push_back( "Hold" );
	mEffectNames.push_back( "Varos es termeszet" );
}

void KotoKaoriApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
	/*
	settings->setWindowSize( MAIN_WIDTH + SECONDARY_WIDTH, SECONDARY_WIDTH );
	settings->setWindowPos( 0, WINDOW_POSY );
	settings->setBorderless( true );
	//settings->setAlwaysOnTop();
	*/
}

void KotoKaoriApp::setup()
{
	gl::disableVerticalSync();

	// params
	string paramsXml = getResourcePath().string() + "/params.xml";
	params::PInterfaceGl::load( paramsXml );

	mParams = params::PInterfaceGl("Koto es Kaori", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	mEffectIndex = EFFECT_FEKETE;
	mPrevEffectIndex = 0;
	mParams.addParam("Effect", mEffectNames, &mEffectIndex);

	mParams.addSeparator();
	mParams.addPersistentParam("Mirror", &mNIMirror, true, "key=m" );

	mParams.addSeparator();
	mParams.addPersistentParam("Fullscreen", &mFullScreen, false, " key='f' ");
	mParams.addButton("Screenshot", std::bind(&KotoKaoriApp::saveScreenshot, this));
	mParams.addParam("Fps", &mFps, "", true);

	// OpenNI
	try
	{
		//mNI = OpenNI( OpenNI::Device() );

		//*
		string path = getAppPath().string();
	#ifdef CINDER_MAC
		path += "/../";
	#endif
		path += "rec.oni";

		mNI = OpenNI( path );
		//*/
	}
	catch (...)
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}

	mNI.setDepthAligned( true );

	// effects
	mEffects.push_back( new Black() );
	mEffects.push_back( new SpeechShop( this ) );
	mEffects.push_back( new DepthMerge( this ) );
	mEffects.push_back( new Acacia( this ) );
	mEffects.push_back( new Moon( this ) );
	mEffects.push_back( new VTT( this ) );

	for (vector<Effect *>::iterator it = mEffects.begin(); it != mEffects.end();
			++it)
	{
		(*it)->setup();
		(*it)->resize( ResizeEvent( Vec2i( SECONDARY_WIDTH, SECONDARY_HEIGHT ) ) );
	}

	reinterpret_cast< DepthMerge *>(mEffects[ EFFECT_KEZEKLABAK ])->setNI( mNI );
	reinterpret_cast< Acacia *>(mEffects[ EFFECT_AKAC ])->setNI( mNI );

	// fbo
	mFbo = gl::Fbo( SECONDARY_WIDTH, SECONDARY_HEIGHT );
	mWindow2Fbo = RectMapping( Area(0, 0, MAIN_WIDTH, MAIN_HEIGHT),
			mFbo.getBounds());

	setWindowPos( 0, WINDOW_POSY );
	setWindowSize( MAIN_WIDTH + SECONDARY_WIDTH, SECONDARY_WIDTH );
	//setBorderless( true );
	setAlwaysOnTop();
	setFullScreen( false );
	setFullScreen( true );
	setFullScreen( false );


	// OpenNI start
	mNI.start();
}

void KotoKaoriApp::shutdown()
{
	params::PInterfaceGl::save();
}

void KotoKaoriApp::saveScreenshot()
{
    string path = getAppPath().string();
#ifdef CINDER_MAC
    path += "/../";
#endif
    path += "snap-" + timeStamp() + ".png";
    fs::path pngPath(path);

    try
    {
        Surface srf = copyWindowSurface();

        if (!pngPath.empty())
        {
            writeImage( pngPath, srf );
        }
    }
    catch ( ... )
    {
        console() << "unable to save image file " << path << endl;
    }
}

void KotoKaoriApp::keyDown( KeyEvent event )
{
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();

	mEffects[ mEffectIndex ]->keyDown( event );
}

void KotoKaoriApp::mouseDown( MouseEvent event )
{
	/*
	Vec2i pos = mWindow2Fbo.map( event.getPos() );
	unsigned int initiator = 0;
	unsigned int modifier = 0;

	console() << "mouseDown" << event.getPos() << " " << pos << endl;
	if (event.isLeft())
	{
		initiator |= MouseEvent::LEFT_DOWN;
	}
	if (event.isLeftDown())
	{
		modifier |= MouseEvent::LEFT_DOWN;
	}
	if (event.isRight())
	{
		initiator |= MouseEvent::RIGHT_DOWN;
	}
	if (event.isRightDown())
	{
		modifier |= MouseEvent::RIGHT_DOWN;
	}
	if (event.isMiddle())
	{
		initiator |= MouseEvent::MIDDLE_DOWN;
	}
	if (event.isMiddleDown())
	{
		modifier |= MouseEvent::MIDDLE_DOWN;
	}

	MouseEvent mappedEvent = MouseEvent( initiator,
										pos.x,
										pos.y,
										modifier,
										event.getWheelIncrement(),
										event.getNativeModifiers()
										);

	mEffects[ mEffectIndex ]->mouseDown( mappedEvent );
	*/
	mEffects[ mEffectIndex ]->mouseDown( event );
}

void KotoKaoriApp::mouseDrag( MouseEvent event )
{
	/*
	Vec2i pos = mWindow2Fbo.map( event.getPos() );
	unsigned int initiator = 0;
	unsigned int modifier = 0;

	if (event.isLeft())
	{
		initiator |= MouseEvent::LEFT_DOWN;
	}
	if (event.isLeftDown())
	{
		modifier |= MouseEvent::LEFT_DOWN;
	}
	if (event.isRight())
	{
		initiator |= MouseEvent::RIGHT_DOWN;
	}
	if (event.isRightDown())
	{
		modifier |= MouseEvent::RIGHT_DOWN;
	}
	if (event.isMiddle())
	{
		initiator |= MouseEvent::MIDDLE_DOWN;
	}
	if (event.isMiddleDown())
	{
		modifier |= MouseEvent::MIDDLE_DOWN;
	}

	MouseEvent mappedEvent = MouseEvent( initiator,
										pos.x,
										pos.y,
										modifier,
										event.getWheelIncrement(),
										event.getNativeModifiers()
										);

	//mEffects[ mEffectIndex ]->mouseDown( mappedEvent );
	*/
	mEffects[ mEffectIndex ]->mouseDrag( event );
}

void KotoKaoriApp::resize( ci::app::ResizeEvent event )
{
	/*
	for (vector<Effect *>::iterator it = mEffects.begin(); it != mEffects.end();
			++it)
	{
		(*it)->resize( event );
	}
	*/
}

void KotoKaoriApp::update()
{
	/*
	if ( mFullScreen && !isFullScreen() )
		setFullScreen( true );

	if ( !mFullScreen && isFullScreen() )
		setFullScreen( false );
	*/

	mFps = getAverageFps();

	if (mNIMirror != mNI.isMirrored())
		mNI.setMirrored( mNIMirror );

	if ( mEffectIndex != mPrevEffectIndex )
	{
		mEffects[ mPrevEffectIndex ]->deinstantiate();
		mEffects[ mEffectIndex ]->instantiate();
	}

	mEffects[ mEffectIndex ]->update();

	mPrevEffectIndex = mEffectIndex;
}

void KotoKaoriApp::draw()
{
	/*
	gl::clear( Color::black() );
	mEffects[ mEffectIndex ]->draw();
	*/

//#if 0
	gl::pushMatrices();
	mFbo.bindFramebuffer();

	// set orthographic projection with lower-left origin
	gl::setMatricesWindow( mFbo.getSize(), false );
	gl::setViewport( mFbo.getBounds() );

	gl::clear( Color::black() );
	mEffects[ mEffectIndex ]->draw();

	mFbo.unbindFramebuffer();

	gl::popMatrices();

	gl::clear( Color::black() );
	gl::setMatricesWindow( Vec2i( MAIN_WIDTH + SECONDARY_WIDTH,
								  SECONDARY_HEIGHT ) );
	gl::setViewport( Area( 0, 0, MAIN_WIDTH + SECONDARY_WIDTH,
								  SECONDARY_HEIGHT ) );

	gl::color( Color::white() );

	gl::Texture fboTexture = mFbo.getTexture();
	fboTexture.setFlipped();
	/*
	fboTexture.enableAndBind();
	gl::drawSolidRect( Rectf( MAIN_WIDTH, 0,
								MAIN_WIDTH + SECONDARY_WIDTH, SECONDARY_HEIGHT ) );
	gl::disable( GL_TEXTURE_2D );
	*/

	gl::draw( fboTexture,
			Rectf( MAIN_WIDTH, 0, MAIN_WIDTH + SECONDARY_WIDTH, SECONDARY_HEIGHT ) );

//#endif

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(KotoKaoriApp, RendererGl( RendererGl::AA_NONE ) )

