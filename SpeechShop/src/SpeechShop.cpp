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
		void enableVSync(bool vs);

		void resize(ResizeEvent event);
		void keyDown(KeyEvent event);

		void mouseDown(MouseEvent event);
		void mouseDrag(MouseEvent event);

		void update();
		void draw();

	private:
		void addLetter(Vec2i pos);
		void togglePlug();

		Sandbox mSandbox;
		BoxElement *mPlug;

		vector<string> mWords;
		unsigned mWordIndex;
};

SpeechShop::SpeechShop()
	: mPlug( NULL )
{
}

void SpeechShop::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void SpeechShop::setup()
{
	enableVSync(false);

	mSandbox.init();
	togglePlug();

	mWords = split( loadString( loadResource(RES_TEXT) ), " \t\n" );
	mWordIndex = 0;

	gl::disableDepthWrite();
	gl::disableDepthRead();
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
		//console() << boxArea << endl;
		mSandbox.createBoundaries( boxArea );
		//mSandbox.enableMouseInteraction(this, false);

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
	//mSandbox.clear();
	//mSandbox.init();
	// need to remove old boundary
	//mSandbox.createBoundaries( Area(Vec2i(0, 0), event.getSize()) );
}

void SpeechShop::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	else if (event.getChar() == ' ')
		togglePlug();
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
	mSandbox.update();
}

void SpeechShop::addLetter(Vec2i pos)
{
	TextLayout simple;
	std::string boldFont( "Arial Bold" );
	simple.setFont( Font( boldFont, Rand::randFloat(10, 25) ) );
	simple.setColor( Color( 1, 1, 1 ) );
	simple.addLine( mWords[mWordIndex] );

	mWordIndex++;
	if (mWordIndex >= mWords.size())
		mWordIndex = 0;

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
}

CINDER_APP_BASIC(SpeechShop, RendererGl)

