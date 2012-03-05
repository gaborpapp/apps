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

#include "PParams.h"
#include "Effect.h"

#include "NI.h"

#include "Black.h"
#include "SpeechShop.h"
#include "DepthMerge.h"


using namespace ci;
using namespace ci::app;
using namespace std;

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
		params::PInterfaceGl mParams;

		vector<Effect *> mEffects;

		enum {
			EFFECT_FEKETE = 0,
			EFFECT_BESZEDBOLT,
			EFFECT_KEZEKLABAK
		};

		int mEffectIndex;

		bool mFullScreen;

		vector<string> mEffectNames;

		OpenNI mNI;
};

KotoKaoriApp::KotoKaoriApp()
{
	mEffectNames.push_back( "Fekete" );
	mEffectNames.push_back( "BeszedBolt" );
	mEffectNames.push_back( "KezekLabak" );
}

void KotoKaoriApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void KotoKaoriApp::setup()
{
	gl::disableVerticalSync();

	// params
    string paramsXml = getResourcePath().string() + "/params.xml";
    params::PInterfaceGl::load( paramsXml );

	mParams = params::PInterfaceGl("Koto es Kaori", Vec2i(200, 300));
    mParams.addPersistentSizeAndPosition();

	mEffectIndex = EFFECT_BESZEDBOLT;
	mParams.addParam("Effect", mEffectNames, &mEffectIndex);

	mParams.addPersistentParam("Fullscreen", &mFullScreen, false, " key='f' ");

	// OpenNI
    try
    {
        //mNI = OpenNI( OpenNI::Device() );

        string path = getAppPath().string();
#ifdef CINDER_MAC
        path += "/../";
#endif
        path += "rec.oni";
        mNI = OpenNI( path );
    }
    catch (...)
    {
        console() << "Could not open Kinect" << endl;
        quit();
    }

    //mNI.setMirrored( mMirror );
    mNI.setMirrored();
    //if ( mDepthAlign )
	mNI.setDepthAligned( true );

	// effects
	mEffects.push_back( new Black() );
	mEffects.push_back( new SpeechShop( this ) );
	mEffects.push_back( new DepthMerge( this ) );

	for (vector<Effect *>::iterator it = mEffects.begin(); it != mEffects.end();
			++it)
	{
		(*it)->setup();
	}
	//mEffects[ EFFECT_BESZEDBOLT ]->setup();
	//mEffects[ EFFECT_KEZEKLABAK ]->setup();

	reinterpret_cast< DepthMerge *>(mEffects[ EFFECT_KEZEKLABAK ])->setNI( mNI );
	// OpenNI start
    mNI.start();
}

void KotoKaoriApp::shutdown()
{
	params::PInterfaceGl::save();
}

void KotoKaoriApp::keyDown( KeyEvent event )
{
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();

	mEffects[ mEffectIndex ]->keyDown( event );
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
	mEffects[ mEffectIndex ]->resize( event );
}

void KotoKaoriApp::update()
{
	if ( mFullScreen && !isFullScreen() )
		setFullScreen( true );

	if ( !mFullScreen && isFullScreen() )
		setFullScreen( false );

	mEffects[ mEffectIndex ]->update();
}

void KotoKaoriApp::draw()
{
	gl::clear( Color::black() );

	mEffects[ mEffectIndex ]->draw();

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(KotoKaoriApp, RendererGl( RendererGl::AA_MSAA_4 ) )

