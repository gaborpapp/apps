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

#include "SpeechShop.h"

using namespace ci;
using namespace ci::app;
using namespace ci::box2d;
using namespace std;

/*
static const bool PREMULT = true;

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

*/

using namespace ci;
using namespace ci::app;
using namespace ci::box2d;
using namespace std;


static const bool PREMULT = true;

SpeechShop::SpeechShop( App *app )
	: mTextIndex( 0 ),
	  mIsPlugged( true ),
	  Effect( app )

{
}

void SpeechShop::setup( const std::string name )
{
	//Effect::setup( "BeszedBolt" );
	// params
	/*
	string paramsXml = getAppPath().string();
#ifdef CINDER_MAC
	paramsXml += "/Contents/Resources/";
#endif
	paramsXml += "params.xml";
	params::PInterfaceGl::load( paramsXml );

	enableVSync(false);
	*/
	mSandbox.init();

	loadTexts();

	mParams = params::PInterfaceGl("BeszedBolt", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	std::vector<string> typeNames;
	for (unsigned i = 0; i < mTexts.size(); ++i)
		typeNames.push_back(mTexts[i].name);
	mParams.addParam( "Type", typeNames, &mTextIndex,
			" keyincr='[' keydecr=']' " );
	mParams.addParam( "Plug", &mIsPlugged, "", true );

	const string dirArr[] = { "Left", "Up", "Right", "Down" };
	const int dirSize = sizeof( dirArr ) / sizeof( dirArr[0] );
	std::vector<string> dirNames(dirArr, dirArr + dirSize);
	mGravityDir = GR_DOWN;
	mParams.addParam( "Gravity", dirNames, &mGravityDir, "", true );
}

void SpeechShop::instantiate()
{
	gl::enableAlphaBlending();
	gl::disableDepthWrite();
	gl::disableDepthRead();
}

void SpeechShop::loadTexts()
{
	string data_path = mApp->getResourcePath().string() + "/assets/BeszedBolt/";

	fs::path p(data_path);
	for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it) && (it->path().extension().string() == ".txt"))
		{
			Text t;
			t.name = it->path().stem().string();
			t.words = split( loadString( loadAsset( "BeszedBolt/" +
							it->path().filename().string() ) ), " \t\n" );
			mTexts.push_back(t);
		}
	}
}

void SpeechShop::togglePlug()
{
	if (mPlugs.empty())
	{
		mSandbox.clear();
		mSandbox.init(false);
		int w = getWindowWidth();
		int h = getWindowHeight();
		Area boxArea(-5 * w, -5 * h, 5 * w, 5 * h );
		mSandbox.createBoundaries( boxArea );

		BoxElement *plug = new BoxElement( Vec2f( w / 2, h + 10 ),
								Vec2f( w, 10 ),
								false );
		mSandbox.addElement(plug);
		mPlugs.push_back( plug );

		plug = new BoxElement( Vec2f( w / 2, -10 ),
								Vec2f( w, 10 ),
								false );
		mSandbox.addElement(plug);
		mPlugs.push_back( plug );

		plug = new BoxElement( Vec2f( -5, h / 2 ),
								Vec2f( 10, h ),
								false );
		mSandbox.addElement(plug);
		mPlugs.push_back( plug );

		plug = new BoxElement( Vec2f( w + 5, h / 2 ),
								Vec2f( 10, h ),
								false );
		mSandbox.addElement(plug);
		mPlugs.push_back( plug );
	}
	else
	{
		vector<BoxElement *>::iterator it;
		for ( it = mPlugs.begin(); it != mPlugs.end(); ++it)
		{
			mSandbox.destroyElement( *it );
			delete *it;
		}
		mPlugs.clear();
	}
}

void SpeechShop::resize( ResizeEvent event )
{
	togglePlug();
	togglePlug();
}

void SpeechShop::keyDown(KeyEvent event)
{
	if (event.getChar() == ' ')
		togglePlug();

	switch (event.getCode())
	{
		case KeyEvent::KEY_LEFT:
			mGravityDir = GR_LEFT;
			break;

		case KeyEvent::KEY_UP:
			mGravityDir = GR_UP;
			break;

		case KeyEvent::KEY_RIGHT:
			mGravityDir = GR_RIGHT;
			break;

		case KeyEvent::KEY_DOWN:
			mGravityDir = GR_DOWN;
			break;
	}
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

	switch ( mGravityDir )
	{
		case GR_LEFT:
			mSandbox.setGravity( Vec2f( -500, 0 ) );
			break;

		case GR_UP:
			mSandbox.setGravity( Vec2f( 0, -500 ) );
			break;

		case GR_RIGHT:
			mSandbox.setGravity( Vec2f( 500, 0 ) );
			break;

		case GR_DOWN:
			mSandbox.setGravity( Vec2f( 0, 500 ) );
			break;
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

