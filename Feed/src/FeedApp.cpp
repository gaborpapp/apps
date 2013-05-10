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
*/

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"
#include "cinder/Channel.h"
#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FeedApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		float mSpeed;
		float mFeed;
		struct ExpParam
		{
			float start;
			float offset;
			float addPerFrame;
			float addPerPixel;
		};
		ExpParam mExpParams[ 4 ];
		void randomizeParams( unsigned long seed = 0 );

		float mFps;
		bool mVerticalSyncEnabled;

		static const int32_t DISP_SIZE = 512;
		gl::Texture mDispX, mDispY;
		Channel32f mDispXChan, mDispYChan;

		gl::Texture mTexture;
		gl::GlslProg mShader;

		gl::Fbo mFbo;
};

void FeedApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 800 );
}

void FeedApp::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mVerticalSyncEnabled = true;
	mParams.addParam( "Vertical sync", &mVerticalSyncEnabled );
	mParams.addSeparator();
	mParams.addText( "Feed" );
	mSpeed = 30.f;
	mParams.addParam( "Speed", &mSpeed, "min=0 step=.1" );
	mFeed = 0.9f;
	mParams.addParam( "Feed value", &mFeed, "min=0 max=1 step=.005" );
	for ( int i = 0 ; i < sizeof( mExpParams ) / sizeof( mExpParams[ 0 ] ); i++ )
	{
		string strId = toString< int >( i );
		mParams.addParam( "Angle start " + strId , &mExpParams[ i ].start, "step=.00005 group='Angle " + strId +"'" );
		mParams.addParam( "Angle offset " + strId , &mExpParams[ i ].offset, "step=.00005 group='Angle " + strId +"'" );
		mParams.addParam( "Increment per frame " + strId , &mExpParams[ i ].addPerFrame, "step=.00005 group='Angle " + strId +"'" );
		mParams.addParam( "Increment per pixel " + strId , &mExpParams[ i ].addPerPixel, "step=.00005 group='Angle " + strId +"'" );
		mParams.setOptions( "Angle " + strId, "opened=false" );
	}
	randomizeParams( 0x999c );
	mParams.addButton( "Randomize feed", std::bind( &FeedApp::randomizeParams, this, 0 ) );
	mParams.addSeparator();

	mTexture = loadImage( loadAsset( "tx.jpg" ) );

	try
	{
		mShader = gl::GlslProg( loadResource( RES_FEED_VERT ),
				loadResource( RES_FEED_FRAG ) );
	}
	catch ( gl::GlslProgCompileExc &exc )
	{
		console() << exc.what() << endl;
	}

	gl::Texture::Format format;
	format.setWrapS( GL_REPEAT );
	format.setInternalFormat( GL_R32F );
	mDispX = gl::Texture( DISP_SIZE, 1, format );
	mDispY = gl::Texture( DISP_SIZE, 1, format );
	mDispXChan = Channel32f( DISP_SIZE, 1 );
	mDispYChan = Channel32f( DISP_SIZE, 1 );

	gl::Fbo::Format fboFormat;
	fboFormat.enableColorBuffer( true, 2 );
	fboFormat.enableDepthBuffer( false );
	fboFormat.setSamples( 4 );
	fboFormat.setMinFilter( GL_NEAREST );
	fboFormat.setMagFilter( GL_NEAREST );
	mFbo = gl::Fbo( 1024, 1024, fboFormat );

	mFbo.bindFramebuffer();
	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
	gl::clear();
	glDrawBuffer( GL_COLOR_ATTACHMENT1_EXT );
	gl::clear();
	mFbo.unbindFramebuffer();
}

void FeedApp::randomizeParams( unsigned long seed )
{
	if ( seed != 0 )
		Rand::randSeed( seed );

	for ( int i = 0 ; i < sizeof( mExpParams ) / sizeof( mExpParams[ 0 ] ); i++ )
	{
		mExpParams[ i ].start = Rand::randFloat( -M_PI, M_PI );
		mExpParams[ i ].offset = Rand::randFloat( -M_PI, M_PI );
		mExpParams[ i ].addPerFrame = Rand::randFloat( -.1f, .1f );
		mExpParams[ i ].addPerPixel = Rand::randFloat( -.1f, .1f );
	}
}

void FeedApp::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

	// update displacement
	float t = (float)getElapsedSeconds() * mSpeed;

	Channel32f::Iter it = mDispXChan.getIter();
	float a0 = mExpParams[ 0 ].start + t * mExpParams[ 0 ].addPerFrame;
	float a1 = mExpParams[ 1 ].start + t * mExpParams[ 1 ].addPerFrame;
	while ( it.pixel() )
	{
		it.v() = ( 2.f * math< float >::sin( a0 + mExpParams[ 0 ].offset ) * math< float >::sin( a0 ) +
				   3.5f * math< float >::cos( a1 + mExpParams[ 1 ].offset ) ) / DISP_SIZE;
		a0 += mExpParams[ 0 ].addPerPixel;
		a1 += mExpParams[ 1 ].addPerPixel;
	}

	it = mDispYChan.getIter();
	float a2 = mExpParams[ 2 ].start + t * mExpParams[ 2 ].addPerFrame;
	float a3 = mExpParams[ 3 ].start + t * mExpParams[ 3 ].addPerFrame;
	while ( it.pixel() )
	{
		it.v() = ( -2.f * ( math< float >::sin( a2 ) + math< float >::cos( a2 + mExpParams[ 2 ].offset ) ) +
					1.f * ( math< float >::sin( a3 + mExpParams[ 3 ].offset ) + math< float >::cos( a3 ) ) ) / DISP_SIZE;

		a2 += mExpParams[ 2 ].addPerPixel;
		a3 += mExpParams[ 3 ].addPerPixel;
	}

	mDispX.update( mDispXChan );
	mDispY.update( mDispYChan );
}

void FeedApp::draw()
{
	static int pingPongId = 0;

	mFbo.bindFramebuffer();
	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + pingPongId );
	gl::setViewport( mFbo.getBounds() );
	gl::setMatricesWindow( mFbo.getSize(), false );

	if ( mShader )
	{
		mShader.bind();
		mShader.uniform( "txt", 0 );
		mShader.uniform( "ptxt", 1 );
		mShader.uniform( "dispX", 2 );
		mShader.uniform( "dispY", 3 );
		mShader.uniform( "feed", mFeed );
	}
	mTexture.bind();
	mFbo.bindTexture( 1, pingPongId ^ 1 );
	mDispX.bind( 2 );
	mDispY.bind( 3 );
	gl::drawSolidRect( mFbo.getBounds() );
	mTexture.unbind();
	mFbo.unbindTexture();
	mDispX.unbind();
	mDispY.unbind();

	if ( mShader )
		mShader.unbind();

	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
	mFbo.unbindFramebuffer();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );
	gl::clear();

	gl::draw( mFbo.getTexture( pingPongId ), getWindowBounds() );

	pingPongId ^= 1;

	mParams.draw();
}

void FeedApp::keyDown( KeyEvent event )
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

		case KeyEvent::KEY_v:
			 mVerticalSyncEnabled = !mVerticalSyncEnabled;
			 break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( FeedApp, RendererGl )

