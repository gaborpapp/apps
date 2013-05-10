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
#include "cinder/Cinder.h"
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
		float mNoiseSpeed;
		float mNoiseScale;
		float mNoiseDisp;
		float mNoiseTwirl;

		struct ExpParam
		{
			float start;
			float offset;
			float addPerFrame;
			float addPerPixel;
			float amplitude;
		};
		ExpParam mExpParams[ 2 ];
		void randomizeParams( unsigned long seed = 0 );

		float mFps;
		bool mVerticalSyncEnabled;

		static const int32_t DISP_SIZE = 512;
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
	mFeed = 0.95f;
	mParams.addParam( "Feed value", &mFeed, "min=0 max=1 step=.005" );
	for ( int i = 0 ; i < sizeof( mExpParams ) / sizeof( mExpParams[ 0 ] ); i++ )
	{
		string strId = toString< int >( i );
		mParams.addParam( "Amplitude " + strId , &mExpParams[ i ].amplitude, "step=.01 group='Angle " + strId +"'" );
		mParams.addParam( "Angle start " + strId , &mExpParams[ i ].start, "step=.00005 group='Angle " + strId +"'" );
		mParams.addParam( "Angle offset " + strId , &mExpParams[ i ].offset, "step=.00005 group='Angle " + strId +"'" );
		mParams.addParam( "Increment per frame " + strId , &mExpParams[ i ].addPerFrame, "step=.00005 group='Angle " + strId +"'" );
		mParams.addParam( "Increment per pixel " + strId , &mExpParams[ i ].addPerPixel, "step=.00005 group='Angle " + strId +"'" );
		mParams.setOptions( "Angle " + strId, "opened=false" );
	}
	mNoiseSpeed = 10.f;
	mParams.addParam( "Noise speed", &mNoiseSpeed, "step=.05" );
	mNoiseScale = 2.f;
	mParams.addParam( "Noise scale", &mNoiseScale, "step=.05" );
	mNoiseDisp = .002f;
	mParams.addParam( "Noise disp", &mNoiseDisp, "step=.001" );
	mNoiseTwirl = 5.f;
	mParams.addParam( "Noise twirl", &mNoiseTwirl, "step=.1" );

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

	gl::Fbo::Format fboFormat;
	fboFormat.enableColorBuffer( true, 2 );
	fboFormat.enableDepthBuffer( false );
	fboFormat.setSamples( 4 );
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
		mExpParams[ i ].amplitude = Rand::randFloat( -5.f, 5.f );
		mExpParams[ i ].start = Rand::randFloat( -M_PI, M_PI );
		mExpParams[ i ].offset = Rand::randFloat( -M_PI, M_PI );
		mExpParams[ i ].addPerFrame = Rand::randFloat( -.1f, .1f );
		mExpParams[ i ].addPerPixel = Rand::randFloat( -.05f, .05f );
	}
	mNoiseSpeed = Rand::randFloat( 0.f, 10.f );
	mNoiseScale = Rand::randFloat( 0.f, 2.f );
	mNoiseDisp = Rand::randFloat( 0.f, 0.01f );
	mNoiseTwirl = Rand::randFloat( 0.f, 6.f );
}

void FeedApp::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

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
		mShader.uniform( "feed", mFeed );
		mShader.uniform( "time", (float)( getElapsedSeconds() / 60. ) );
		mShader.uniform( "noiseSpeed", mNoiseSpeed );
		mShader.uniform( "noiseScale", mNoiseScale );
		mShader.uniform( "noiseDisp", mNoiseDisp );
		mShader.uniform( "noiseTwirl", mNoiseTwirl );

		float t = (float)getElapsedSeconds() * mSpeed;
		mShader.uniform( "dispXOffset", mExpParams[ 0 ].start +
				t * mExpParams[ 0 ].addPerFrame + mExpParams[ 0 ].offset );
		mShader.uniform( "dispXAddPerPixel", mExpParams[ 0 ].addPerPixel * DISP_SIZE );
		mShader.uniform( "dispXAmplitude", mExpParams[ 0 ].amplitude / DISP_SIZE );
		mShader.uniform( "dispYOffset", mExpParams[ 1 ].start +
				t * mExpParams[ 1 ].addPerFrame + mExpParams[ 1 ].offset );
		mShader.uniform( "dispYAddPerPixel", mExpParams[ 1 ].addPerPixel * DISP_SIZE );
		mShader.uniform( "dispYAmplitude", mExpParams[ 1 ].amplitude / DISP_SIZE );
	}

	mTexture.bind();
	mFbo.bindTexture( 1, pingPongId ^ 1 );
	gl::drawSolidRect( mFbo.getBounds() );
	mTexture.unbind();
	mFbo.unbindTexture();

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

