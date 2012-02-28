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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/Font.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Filesystem.h"

#include "PParams.h"

#include "b2cinder/b2cinder.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace ci::box2d;
using namespace std;

static const bool PREMULT = true;

class SpeechShop : public ci::app::AppBasic
{
	public:
		SpeechShop();
		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void enableVSync(bool vs);

		void resize(ResizeEvent event);
		void keyDown(KeyEvent event);

		void mouseDown(MouseEvent event);
		void mouseDrag(MouseEvent event);

		void update();
		void draw();

	private:
		struct Text {
			Text () : wordIndex(0) {};

			string name;
			vector<string> words;
			unsigned wordIndex;
		};

		void addLetter(Vec2i pos);
		void togglePlug();

		void loadTexts();
		vector<Text> mTexts;
		int mTextIndex;

		Sandbox mSandbox;
		BoxElement *mPlug;

		ci::params::PInterfaceGl mParams;
		bool mIsPlugged;
};

SpeechShop::SpeechShop()
	: mPlug( NULL ),
	  mTextIndex( 0 ),
	  mIsPlugged( true )
{
}

void SpeechShop::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void SpeechShop::setup()
{
	// params
	string paramsXml = getAppPath().string();
#ifdef CINDER_MAC
	paramsXml += "/Contents/Resources/";
#endif
	paramsXml += "params.xml";
	params::PInterfaceGl::load( paramsXml );

	enableVSync(false);

	mSandbox.init();

	//mWords = split( loadString( loadResource(RES_TEXT) ), " \t\n" );
	//mWordIndex = 0;
	loadTexts();

	mParams = params::PInterfaceGl("BeszedBolt", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	std::vector<string> typeNames;
	for (unsigned i = 0; i < mTexts.size(); ++i)
		typeNames.push_back(mTexts[i].name);
	mParams.addParam( "Type", typeNames, &mTextIndex,
			" keyincr='[' keydecr=']' " );
	mParams.addParam( "Plug", &mIsPlugged, " key='space' " );

	gl::enableAlphaBlending();
	gl::disableDepthWrite();
	gl::disableDepthRead();
}

void SpeechShop::shutdown()
{
	params::PInterfaceGl::save();
}

void SpeechShop::loadTexts()
{
	string data_path = getAppPath().string();
#ifdef CINDER_MAC
	data_path += "/Contents/Resources/assets";
#endif

	fs::path p(data_path);
	for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it) && (it->path().extension().string() == ".txt"))
		{
			Text t;
			t.name = it->path().stem().string();
			t.words = split( loadString( loadAsset( it->path().filename().string() ) ), " \t\n" );
			mTexts.push_back(t);
		}
	}
}

void SpeechShop::enableVSync(bool vs)
{
#if defined(CINDER_MAC)
	GLint vsync = vs;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &vsync);
#endif
}

void SpeechShop::togglePlug()
{
	if (mPlug == NULL)
	{
		mSandbox.clear();
		mSandbox.init(false);
		Area boxArea(0, 0, getWindowWidth(), 5 * getWindowHeight() );
		mSandbox.createBoundaries( boxArea );

		mPlug = new BoxElement( Vec2f( getWindowWidth() / 2, getWindowHeight() + 10 ),
								Vec2f( getWindowWidth(), 10 ),
								false );
		mSandbox.addElement(mPlug);
	}
	else
	{
		mSandbox.destroyElement( mPlug );
		delete mPlug;
		mPlug = NULL;
	}
}

void SpeechShop::resize(ResizeEvent event)
{
	togglePlug();
	togglePlug();
}

void SpeechShop::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void SpeechShop::mouseDrag(MouseEvent event)
{
	if (event.isLeft())
		addLetter(event.getPos());
}

void SpeechShop::mouseDown(MouseEvent event)
{
	if (event.isLeft())
		addLetter(event.getPos());
}

void SpeechShop::update()
{
	static bool lastPlugged = false;

	if (lastPlugged != mIsPlugged)
	{
		togglePlug();
		lastPlugged = mIsPlugged;
	}

	mSandbox.update();
}

void SpeechShop::addLetter(Vec2i pos)
{
	TextLayout simple;
	std::string boldFont( "Arial Bold" );
	simple.setFont( Font( boldFont, Rand::randFloat(10, 25) ) );
	simple.setColor( Color( 1, 1, 1 ) );

	Text *t = &mTexts[mTextIndex];
	simple.addLine( t->words[ t->wordIndex ] );
	t->wordIndex++;
	if (t->wordIndex >= t->words.size())
		t->wordIndex = 0;

	gl::Texture texture( simple.render( true, PREMULT ) );
	TexturedElement *b = new TexturedElement(texture, pos,
			Vec2f(texture.getWidth(), texture.getHeight()));
	b->setColor(Color::white());
	mSandbox.addElement(b);
}

void SpeechShop::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow( getWindowSize() );
	mSandbox.draw();

	params::PInterfaceGl::draw();
}

CINDER_APP_BASIC(SpeechShop, RendererGl)
