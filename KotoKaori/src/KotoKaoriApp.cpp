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

#include "AntTweakBar.h"

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
const int MAIN_WIDTH = 1024;
const int MAIN_HEIGHT = 768;
const int SECONDARY_WIDTH = 1024;
const int SECONDARY_HEIGHT = 768;

class KotoKaoriApp : public ci::app::AppBasic
{
	public:
		KotoKaoriApp();

		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void keyDown( ci::app::KeyEvent event );
		void keyUp( ci::app::KeyEvent event );

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
		bool mShowPreview;
		int mPreviewSize;

		vector<string> mEffectNames;

		OpenNI mNI;
		bool mNIMirror;

		gl::Fbo mFbo;
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
}

void KotoKaoriApp::setup()
{
	setBorderless();
	setWindowSize( MAIN_WIDTH + SECONDARY_WIDTH, SECONDARY_HEIGHT );
	setWindowPos( 0, 0 );
	setAlwaysOnTop();

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
	mParams.addPersistentParam("Mirror", &mNIMirror, false, "key=m" );

	mParams.addSeparator();
	mParams.addPersistentParam("Fullscreen", &mFullScreen, false, " key='f' ");
	mParams.addPersistentParam("Preview", &mShowPreview, true );
	mParams.addPersistentParam("Preview size", &mPreviewSize, 160, "min=80, max=600" );
	mParams.addButton("Screenshot", std::bind(&KotoKaoriApp::saveScreenshot, this));
	mParams.addParam("Fps", &mFps, "", true);

	TwDefine( "TW_HELP visible=false" );

	// OpenNI
	try
	{
		mNI = OpenNI( OpenNI::Device() );

		/*
		string path = getAppPath().string();
	#ifdef CINDER_MAC
		path += "/../";
	#endif
		path += "rec.oni";

		mNI = OpenNI( path );
		*/
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
	if (event.getChar() == 'b')
	{
		setBorderless( !isBorderless() );
	}

	mEffects[ mEffectIndex ]->keyDown( event );
}

void KotoKaoriApp::keyUp( KeyEvent event )
{
	mEffects[ mEffectIndex ]->keyUp( event );
}

void KotoKaoriApp::mouseDown( MouseEvent event )
{
	mEffects[ mEffectIndex ]->mouseDown( event );
}

void KotoKaoriApp::mouseDrag( MouseEvent event )
{
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
	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );

	gl::color( Color::white() );

	gl::Texture fboTexture = mFbo.getTexture();
	fboTexture.setFlipped();
	gl::draw( fboTexture,
			Rectf( MAIN_WIDTH, 0, MAIN_WIDTH + SECONDARY_WIDTH, SECONDARY_HEIGHT ) );

	if ( mShowPreview )
	{
		const int b = 10;
		const int pw = mPreviewSize;
		const int ph = mPreviewSize / mFbo.getAspectRatio();

		Rectf previewRect = Rectf( MAIN_WIDTH - pw - b, b, MAIN_WIDTH - b, ph + b );
		gl::draw( fboTexture, previewRect );
		gl::drawStrokedRect( previewRect );
	}

	params::PInterfaceGl::draw();
}

CINDER_APP_BASIC(KotoKaoriApp, RendererGl( RendererGl::AA_MSAA_2 ) )

