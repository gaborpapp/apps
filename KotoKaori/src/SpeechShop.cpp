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

static const bool PREMULT = true;

SpeechShop::SpeechShop( App *app )
	: mTextIndex( 0 ),
	  mIsPlugged( false ),
	  Effect( app )
{
}

void SpeechShop::setup()
{
	mSandbox.init();

	loadTexts();

	mParams = params::PInterfaceGl("BeszedBolt", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	std::vector<string> typeNames;
	for (unsigned i = 0; i < mTexts.size(); ++i)
		typeNames.push_back(mTexts[i].name);
	mParams.addParam( "Text", typeNames, &mTextIndex,
			" keyincr='[' keydecr=']' " );
	mParams.addParam( "Plug", &mIsPlugged, "", false );

	mParams.addPersistentParam( "Gravity", &mGravity, 550, " min=10, max=1500, step=10 ");
	mParams.addPersistentParam( "Min size", &mMinTextSize, 17, " min=5, max=20, step=.5 ");
	mParams.addPersistentParam( "Max size", &mMaxTextSize, 25, " min=10, max=40, step=.5 ");

	const string dirArr[] = { "Left", "Up", "Right", "Down" };
	const int dirSize = sizeof( dirArr ) / sizeof( dirArr[0] );
	std::vector<string> dirNames(dirArr, dirArr + dirSize);
	mGravityDir = GR_DOWN;
	mParams.addParam( "Gravity direction", dirNames, &mGravityDir, "", true );
	mParams.setOptions( "", "refresh=.1" );
}

void SpeechShop::instantiate()
{
	gl::enableAlphaBlending();
	gl::disableDepthWrite();
	gl::disableDepthRead();
	mTextIndex = 0;
	initTexts();
	mPlugDebounce = false;
}

void SpeechShop::initTexts()
{
	for (vector<Text>::iterator it = mTexts.begin(); it != mTexts.end(); ++it)
	{
		it->sentenceIndex = 0;
		it->wordIndex = 0;
	}
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
			vector< string > sentences = split( loadString( loadAsset( "BeszedBolt/" +
							it->path().filename().string() ) ), "\n" );
			for ( vector< string >::iterator sit = sentences.begin();
					sit != sentences.end(); ++sit )
			{
				t.sentences.push_back( split( *sit, " \t" ) );
			}
			mTexts.push_back( t );
		}
	}
}

void SpeechShop::togglePlug()
{
	if (mPlugs.empty())
	{
		mSandbox.clear();
		mSandbox.init(false);
		// FIXME
		/*
		int w = getWindowWidth();
		int h = getWindowHeight();
		*/
		int w = 1024;
		int h = 768;
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

		initTexts();
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
	switch (event.getChar())
	{
		case ' ':
			if (not mPlugDebounce)
			{
				mIsPlugged = !mIsPlugged;
				mPlugDebounce = true;
			}
			break;
	}

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

void SpeechShop::keyUp(KeyEvent event)
{
	if (event.getChar() == ' ')
	{
		mPlugDebounce = false;
	}
}

void SpeechShop::mouseDrag(MouseEvent event)
{
	if (event.isRight())
		addSentence(event.getPos());
}

void SpeechShop::mouseDown(MouseEvent event)
{
	if (event.isLeft() || event.isRight())
		addSentence(event.getPos());
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
			mSandbox.setGravity( Vec2f( -mGravity, 0 ) );
			break;

		case GR_UP:
			mSandbox.setGravity( Vec2f( 0, -mGravity ) );
			break;

		case GR_RIGHT:
			mSandbox.setGravity( Vec2f( mGravity, 0 ) );
			break;

		case GR_DOWN:
			mSandbox.setGravity( Vec2f( 0, mGravity ) );
			break;
	}

	mSandbox.update();
}

void SpeechShop::addWord(Vec2i pos)
{
	TextLayout simple;
	std::string boldFont( "Arial Bold" );
	simple.setFont( Font( boldFont, Rand::randFloat( mMinTextSize, mMaxTextSize ) ) );
	simple.setColor( Color( 1, 1, 1 ) );

	Text *t = &mTexts[mTextIndex];
	string word = t->sentences[ t->sentenceIndex ][ t->wordIndex ];
	t->wordIndex++;
	if ( t->wordIndex >= t->sentences[ t->sentenceIndex ].size() )
	{
		t->wordIndex = 0;
		t->sentenceIndex++;
		if ( t->sentenceIndex >= t->sentences.size() )
		{
			t->sentenceIndex = 0;
		}
	}

	if ( word == "" )
		return;

	simple.addLine( word );

	gl::Texture texture( simple.render( true, PREMULT ) );
	TexturedElement *b = new TexturedElement(texture, pos,
			Vec2f(texture.getWidth(), texture.getHeight()));
	b->setColor(Color::white());
	mSandbox.addElement(b);
}

void SpeechShop::addSentence(Vec2i pos)
{
	Text *t = &mTexts[mTextIndex];
	unsigned sId = t->sentenceIndex;

	int m = (int)mMaxTextSize;
	Vec2i posDiff[] = { Vec2i( m, 0 ),
						Vec2i( 0, m ),
						Vec2i( -m, 0 ),
						Vec2i( 0, m ) };
	Vec2i addPos = posDiff[ mGravityDir ];
	// FIXME:
	Area area( 0, 0, 1024, 768 );
	while ( sId == t->sentenceIndex )
	{
		addWord( pos );
		pos += addPos;
		pos = area.closestPoint( pos );
	}
}

void SpeechShop::draw()
{
	gl::clear(Color(0, 0, 0));
	//gl::setMatricesWindow( getWindowSize() );
	// FIXME
	gl::setMatricesWindow( Vec2i(1024, 768) );

	mSandbox.draw();
}

