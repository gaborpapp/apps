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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/ip/Resize.h"

#include "ciMsaFluidDrawerGl.h"
#include "ciMsaFluidSolver.h"
#include "CinderOpenCV.h"
#include "mndlkit/params/PParams.h"

#include "CaptureSource.h"
#include "FluidParticles.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FludParticlesApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void resize();
		void shutdown();

		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		mndl::kit::params::PInterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled;

		mndl::CaptureSource mCaptureSource;
		gl::Texture mCaptureTexture;

		// optflow
		bool mFlip;
		bool mDrawFlow;
		bool mDrawFluid;
		bool mDrawCapture;
		float mFlowMultiplier;

		cv::Mat mPrevFrame;
		cv::Mat mFlow;

		int mOptFlowWidth;
		int mOptFlowHeight;

		// fluid
		ciMsaFluidSolver mFluidSolver;
		static const int sFluidSizeX = 128;
		ciMsaFluidDrawerGl mFluidDrawer;

		// particles
		FluidParticleManager mParticles;
		int mParticleMin;
		int mParticleMax;
		float mMaxVelocity;
		float mVelParticleMult;
		float mVelParticleMin;
		float mVelParticleMax;

		void addToFluid( Vec2f pos, Vec2f vel, bool addParticles = true, bool addForce = true, bool addColor = true );
};

void FludParticlesApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void FludParticlesApp::setup()
{
	mndl::kit::params::PInterfaceGl::load( "params.xml" );
	mParams = mndl::kit::params::PInterfaceGl( "Parameters", Vec2i( 310, 300 ), Vec2i( 16, 16 ) );
	mParams.addPersistentSizeAndPosition();
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addPersistentParam( "Vertical sync", &mVerticalSyncEnabled, false );
	mParams.addSeparator();

	mParams.addText("Optical flow");
	mParams.addPersistentParam( "Flip", &mFlip, true );
	mParams.addPersistentParam( "Draw flow", &mDrawFlow, false );
	mParams.addPersistentParam( "Draw fluid", &mDrawFluid, false );
	mParams.addPersistentParam( "Draw capture", &mDrawCapture, true );
	mParams.addPersistentParam( "Flow multiplier", &mFlowMultiplier, .02, "min=.001 max=2 step=.001" );
	mParams.addPersistentParam( "Flow width", &mOptFlowWidth, 80, "min=20 max=640" );
	mParams.addPersistentParam( "Flow height", &mOptFlowHeight, 60, "min=20 max=480" );
	mParams.addSeparator();

	mParams.addText("Particles");
	mParams.addPersistentParam("Particle min", &mParticleMin, 0, "min=0 max=50");
	mParams.addPersistentParam("Particle max", &mParticleMax, 40, "min=0 max=50");
	mParams.addPersistentParam("Velocity max", &mMaxVelocity, 40.f, "min=1 max=100");
	mParams.addPersistentParam("Velocity particle multiplier", &mVelParticleMult, .26f, "min=0 max=2 step=.01");
	mParams.addPersistentParam("Velocity particle min", &mVelParticleMin, 1.f, "min=1 max=100 step=.5");
	mParams.addPersistentParam("Velocity particle max", &mVelParticleMax, 60.f, "min=1 max=100 step=.5");
	mParams.addSeparator();

	mCaptureSource.setup();

	// fluid
	mFluidSolver.setup( sFluidSizeX, sFluidSizeX );
	mFluidSolver.enableRGB( true ).setFadeSpeed( 0.002f ).setDeltaT( .5f ).setVisc( 0.00015f ).setColorDiffusion( 0 );
	mFluidSolver.setWrap( false, true );
	mFluidDrawer.setup( &mFluidSolver );

    mParticles.setFluidSolver( &mFluidSolver );
}

void FludParticlesApp::resize()
{
	mParticles.setWindowSize( getWindowSize() );

}

void FludParticlesApp::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

	mCaptureSource.update();

	// optical flow
	if ( mCaptureSource.isCapturing() && mCaptureSource.checkNewFrame() )
	{
		Surface8u captSurf( Channel8u( mCaptureSource.getSurface() ) );

		Surface8u smallSurface( mOptFlowWidth, mOptFlowHeight, false );
		if ( ( captSurf.getWidth() != mOptFlowWidth ) ||
				( captSurf.getHeight() != mOptFlowHeight ) )
		{
			ip::resize( captSurf, &smallSurface );
		}
		else
		{
			smallSurface = captSurf;
		}

		mCaptureTexture = gl::Texture( captSurf );

		cv::Mat currentFrame( toOcv( Channel( smallSurface ) ) );
		if ( mFlip )
			cv::flip( currentFrame, currentFrame, 1 );
		if ( ( mPrevFrame.data ) &&
			 ( mPrevFrame.size() == cv::Size( mOptFlowWidth, mOptFlowHeight ) ) )
		{
			cv::calcOpticalFlowFarneback(
					mPrevFrame, currentFrame,
					mFlow,
					.5, 5, 13, 5, 5, 1.1, 0 );
		}
		mPrevFrame = currentFrame;

		// fluid update
		if ( mFlow.data )
		{
			RectMapping ofNorm( Area( 0, 0, mFlow.cols, mFlow.rows ),
					Rectf( 0.f, 0.f, 1.f, 1.f ) );
			for ( int y = 0; y < mFlow.rows; y++ )
			{
				for ( int x = 0; x < mFlow.cols; x++ )
				{
					Vec2f v = fromOcv( mFlow.at< cv::Point2f >( y, x ) );
					Vec2f p( x + .5, y + .5 );
					addToFluid( ofNorm.map( p ), ofNorm.map( v ) * mFlowMultiplier );
				}
			}
		}
	}

	// fluid & particles
	mFluidSolver.update();

	mParticles.setAging( 0.9 );
	mParticles.update( getElapsedSeconds() );
}

void FludParticlesApp::addToFluid( Vec2f pos, Vec2f vel, bool addParticles, bool addForce, bool addColor )
{
	// balance the x and y components of speed with the screen aspect ratio
	float speed = vel.x * vel.x +
		vel.y * vel.y * getWindowAspectRatio() * getWindowAspectRatio();

	if ( speed > 0 )
	{
		pos.x = constrain( pos.x, 0.0f, 1.0f );
		pos.y = constrain( pos.y, 0.0f, 1.0f );

		const float velocityMult = 30;

		if ( addParticles )
		{
			int count = static_cast<int>(
					lmap<float>( vel.length() * mVelParticleMult * getWindowWidth(),
						mVelParticleMin, mVelParticleMax,
						mParticleMin, mParticleMax ) );
			if (count > 0)
			{
				mParticles.addParticle( pos * Vec2f( getWindowSize() ), count);
			}
		}
		if ( addForce )
			mFluidSolver.addForceAtPos( pos, vel * velocityMult );

		const float colorMult = .1f;

		if ( addColor )
		{
			float hue = ( getElapsedFrames() % 360 ) / 360.0f;
			mFluidSolver.addColorAtPos( pos, Color( CM_HSV, hue, 1, 1 ) * colorMult );
		}
	}
}


void FludParticlesApp::draw()
{
	gl::clear();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	if ( mDrawFluid )
	{
		gl::color( Color::white() );
		mFluidDrawer.draw( 0, 0, getWindowWidth(), getWindowHeight() );
	}
	mParticles.draw();

	// draw output to window

	if ( mDrawCapture && mCaptureTexture )
	{
		gl::enableAdditiveBlending();
		gl::color( ColorA( 1, 1, 1, .4 ) );
		mCaptureTexture.enableAndBind();

		gl::pushModelView();
		if ( mFlip )
		{
			gl::translate( getWindowWidth(), 0 );
			gl::scale( -1, 1 );
		}
		gl::drawSolidRect( getWindowBounds() );
		gl::popModelView();
		mCaptureTexture.unbind();
		gl::color( Color::white() );
		gl::disableAlphaBlending();
	}

	// flow vectors, TODO: make this faster using Vbo
	gl::disable( GL_TEXTURE_2D );
	if ( mDrawFlow && mFlow.data )
	{
		RectMapping ofToWin( Area( 0, 0, mFlow.cols, mFlow.rows ),
				getWindowBounds() );
		float ofScale = mFlowMultiplier * getWindowWidth() / (float)mOptFlowWidth;
		gl::color( Color::white() );
		for ( int y = 0; y < mFlow.rows; y++ )
		{
			for ( int x = 0; x < mFlow.cols; x++ )
			{
				Vec2f v = fromOcv( mFlow.at< cv::Point2f >( y, x ) );
				Vec2f p( x + .5, y + .5 );
				gl::drawLine( ofToWin.map( p ),
						ofToWin.map( p + ofScale * v ) );
			}
		}
	}

	mndl::kit::params::PInterfaceGl::draw();
}

void FludParticlesApp::shutdown()
{
	mCaptureSource.shutdown();
	mndl::kit::params::PInterfaceGl::save();
}

void FludParticlesApp::keyDown( KeyEvent event )
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

CINDER_APP_BASIC( FludParticlesApp, RendererGl )

