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
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/ip/Resize.h"

#include "ciMsaFluidDrawerGl.h"
#include "ciMsaFluidSolver.h"
#include "CinderOpenCV.h"
#include "mndlkit/params/PParams.h"
#include "KawaseStreak.h"

#include "CaptureSource.h"
#include "FluidParticles.h"
#include "FluidLetters.h"

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
		void mouseMove( MouseEvent event );
		void mouseDrag( MouseEvent event );
		void mouseDown( MouseEvent event );
		void mouseUp( MouseEvent event );

		void update();
		void draw();

	private:
		mndl::params::PInterfaceGl mParams;

		void drawOutput();
		void drawControl();

		float mFps;
		bool mVerticalSyncEnabled;

		mndl::CaptureSource mCaptureSource;
		gl::Texture mCaptureTexture;

		// optflow
		bool mFlip;
		bool mDrawFlow;
		bool mDrawFluid;
		bool mDrawParticles;
		bool mDrawCapture;
		float mCaptureAlpha;
		float mFlowMultiplier;

		cv::Mat mPrevFrame;
		cv::Mat mFlow;

		int mOptFlowWidth;
		int mOptFlowHeight;

		// fluid
		ciMsaFluidSolver mFluidSolver;
		ciMsaFluidDrawerGl mFluidDrawer;

		int mFluidWidth, mFluidHeight;
		float mFluidFadeSpeed;
		float mFluidDeltaT;
		float mFluidViscosity;
		bool mFluidVorticityConfinement;
		bool mFluidWrapX, mFluidWrapY;
		float mFluidVelocityMult;
		float mFluidColorMult;
		Color mFluidColor;

		// particles
		gl::Fbo mParticlesFbo;
		FluidParticleManager mParticles;
		float mParticleAging;
		int mParticleMin;
		int mParticleMax;
		float mMaxVelocity;
		float mVelParticleMult;
		float mVelParticleMin;
		float mVelParticleMax;

		void addToFluid( Vec2f pos, Vec2f vel, bool addParticles = true, bool addForce = true, bool addColor = true, bool addLetters = false );
		LetterManagerRef mLetterManager;
		bool mLettersEnabled;
		string mLetters;
		string mFontName;
		float mFontSizeMin, mFontSizeMax;
		Vec2i mMousePrev;

		mndl::gl::fx::KawaseStreak mKawaseStreak;

		float mStreakAttenuation;
		int mStreakIterations;
		float mStreakStrength;

		app::WindowRef mOutputWindow;
		app::WindowRef mControlWindow;

		Rectf mPreviewRectNorm;
		Rectf mOptFlowClipRectNorm;
		Vec2f mMouseStartNorm, mMouseEndNorm;
};

void FludParticlesApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void FludParticlesApp::setup()
{
	mOutputWindow = getWindow();
	mControlWindow = createWindow( Window::Format().size( 600, 600 ) );

	mndl::params::PInterfaceGl::load( "params.xml" );
	mParams = mndl::params::PInterfaceGl( mControlWindow, "Parameters", Vec2i( 310, 300 ), Vec2i( 16, 16 ) );
	mParams.addPersistentSizeAndPosition();
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addPersistentParam( "Vertical sync", &mVerticalSyncEnabled, false );
	mParams.addSeparator();

	mParams.addText("Optical flow");
	mParams.addPersistentParam( "Flip", &mFlip, true );
	mParams.addPersistentParam( "Draw flow", &mDrawFlow, false );
	mParams.addPersistentParam( "Draw fluid", &mDrawFluid, false );
	mParams.addPersistentParam( "Draw particles", &mDrawParticles, true );
	mParams.addPersistentParam( "Draw capture", &mDrawCapture, true );
	mParams.addPersistentParam( "Capture alpha", &mCaptureAlpha, .1f, "min=0 max=1 step=0.05" );
	mParams.addPersistentParam( "Flow multiplier", &mFlowMultiplier, .105, "min=.001 max=2 step=.001" );
	mParams.addPersistentParam( "Flow width", &mOptFlowWidth, 160, "min=20 max=640", true );
	mParams.addPersistentParam( "Flow height", &mOptFlowHeight, 120, "min=20 max=480", true );
	mParams.addSeparator();

	mParams.addText( "Particles" );
	mParams.addPersistentParam( "Particle aging", &mParticleAging, 0.97f, "min=0 max=1 step=0.001" );
	mParams.addPersistentParam( "Particle min", &mParticleMin, 0, "min=0 max=50" );
	mParams.addPersistentParam( "Particle max", &mParticleMax, 25, "min=0 max=50" );
	mParams.addPersistentParam( "Velocity max", &mMaxVelocity, 7.f, "min=1 max=100" );
	mParams.addPersistentParam( "Velocity particle multiplier", &mVelParticleMult, .57, "min=0 max=2 step=.01" );
	mParams.addPersistentParam( "Velocity particle min", &mVelParticleMin, 1.f, "min=1 max=100 step=.5" );
	mParams.addPersistentParam( "Velocity particle max", &mVelParticleMax, 60.f, "min=1 max=100 step=.5" );
	mParams.addSeparator();

	mParams.addText( "Letters" );
	mParams.addPersistentParam( "Letters enabled", &mLettersEnabled, false );
	mParams.addPersistentParam( "Letter allowed", &mLetters, "Aa" );
	mParams.addPersistentParam( "Font name", &mFontName, Font::getDefault().getName() );
	mParams.addButton( "Set font", [ this ]() { mLetterManager->setFont( mFontName ); } );
	mParams.addPersistentParam( "Font size min", &mFontSizeMin, 12, "min=1 step=.5" );
	mParams.addPersistentParam( "Font size max", &mFontSizeMax, 24, "min=1 step=.5" );
	mParams.addSeparator();

	mParams.addPersistentParam( "Particle max", &mParticleMax, 25, "min=0 max=50" );
	mParams.addPersistentParam( "Velocity max", &mMaxVelocity, 7.f, "min=1 max=100" );
	mParams.addPersistentParam( "Velocity particle multiplier", &mVelParticleMult, .57, "min=0 max=2 step=.01" );
	mParams.addPersistentParam( "Velocity particle min", &mVelParticleMin, 1.f, "min=1 max=100 step=.5" );
	mParams.addPersistentParam( "Velocity particle max", &mVelParticleMax, 60.f, "min=1 max=100 step=.5" );
	mParams.addSeparator();

	mCaptureSource.setup();

	// fluid
	mParams.addText( "Fluid" );
	mParams.addPersistentParam( "Fluid width", &mFluidWidth, 160, "min=16 max=512", true );
	mParams.addPersistentParam( "Fluid height", &mFluidHeight, 120, "min=16 max=512", true );
	mParams.addPersistentParam( "Fade speed", &mFluidFadeSpeed, 0.012f, "min=0 max=1 step=0.0005" );
	mParams.addPersistentParam( "Viscosity", &mFluidViscosity, 0.00003f, "min=0 max=1 step=0.00001" );
	mParams.addPersistentParam( "Delta t", &mFluidDeltaT, 0.4f, "min=0 max=10 step=0.05" );
	mParams.addPersistentParam( "Vorticity confinement", &mFluidVorticityConfinement, false );
	mParams.addPersistentParam( "Wrap x", &mFluidWrapX, true );
	mParams.addPersistentParam( "Wrap y", &mFluidWrapY, true );
	mParams.addPersistentParam( "Fluid color", &mFluidColor, Color( 1.f, 0.05f, 0.01f ) );
	mParams.addPersistentParam( "Fluid velocity mult", &mFluidVelocityMult, 10.f, "min=1 max=50 step=0.5" );
	mParams.addPersistentParam( "Fluid color mult", &mFluidColorMult, .5f, "min=0.05 max=10 step=0.05" );

	mFluidSolver.setup( mFluidWidth, mFluidHeight );
	mFluidSolver.enableRGB( false );
	mFluidSolver.setColorDiffusion( 0 );
	mFluidDrawer.setup( &mFluidSolver );
	mParams.addButton( "Reset fluid", [&]() { mFluidSolver.reset(); } );

	mParams.addSeparator();
	mParams.addText("Post process");
	mParams.addPersistentParam( "Streak attenuation", &mStreakAttenuation, .9f, "min=0 max=.999 step=.001" );
	mParams.addPersistentParam( "Streak iterations", &mStreakIterations, 8, "min=1 max=32" );
	mParams.addPersistentParam( "Streak strength", &mStreakStrength, .8f, "min=0 max=1 step=.05" );

    mParticles.setFluidSolver( &mFluidSolver );
	mLetterManager = LetterManager::create();
    mLetterManager->setFluidSolver( &mFluidSolver );
	mLetterManager->setFont( mFontName );

	gl::Fbo::Format format;
	format.enableDepthBuffer( false );
	format.setSamples( 4 );
	mParticlesFbo = gl::Fbo( 1024, 768, format );
	mParticles.setWindowSize( mParticlesFbo.getSize() );

	mKawaseStreak = mndl::gl::fx::KawaseStreak( mParticlesFbo.getWidth(), mParticlesFbo.getHeight() );

	mOptFlowClipRectNorm = Rectf( 0, 0, 1, 1 );
}

void FludParticlesApp::resize()
{
	if ( getWindow() == mOutputWindow )
	{
		mLetterManager->setWindowSize( getWindowSize() );
	}
	else
	if ( getWindow() == mControlWindow )
	{
		const int b = 10;
		const int pw = int( getWindowWidth() * .4f );
		const int ph = int( pw / ( 4.f / 3.f ) ); //mCaptureTexture.getAspectRatio() );

		mPreviewRectNorm = Rectf( getWindowWidth() - pw - b, b, getWindowWidth() - b, ph + b );
		mPreviewRectNorm.scale( Vec2f::one() / Vec2f( getWindowSize() ) );
	}
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
			/*

			   void calcOpticalFlowFarneback( const Mat& prevImg, const Mat& nextImg, Mat& flow,
					double pyrScale, int levels, int winsize, int iterations, int polyN, double polySigma, int flags)

				pyrScale – Specifies the image scale (<1) to build the pyramids
					for each image. pyrScale=0.5 means the classical pyramid, where
					each next layer is twice smaller than the previous
				levels – The number of pyramid layers, including the initial
					image. levels=1 means that no extra layers are created and only
					the original images are used
				winsize – The averaging window size; The larger values increase
					the algorithm robustness to image noise and give more chances
					for fast motion detection, but yield more blurred motion field
				iterations – The number of iterations the algorithm does at
					each pyramid level
				polyN – Size of the pixel neighborhood used to find polynomial
					expansion in each pixel. The larger values mean that the image
					will be approximated with smoother surfaces, yielding more
					robust algorithm and more blurred motion field. Typically,
					polyN =5 or 7
				polySigma – Standard deviation of the Gaussian that is used to
					smooth derivatives that are used as a basis for the polynomial
					expansion. For polyN=5 you can set polySigma=1.1 , for polyN=7
					a good value would be polySigma=1.5
				flags – The operation flags; can be a combination of the
					following:
					OPTFLOW_USE_INITIAL_FLOW Use the input flow as the initial
						flow approximation
					OPTFLOW_FARNEBACK_GAUSSIAN Use a Gaussian filter instead of box
						filter of the same size for optical flow estimation. Usually, this option gives
						more accurate flow than with a box filter, at the cost of lower speed (and
						normally winsize for a Gaussian window should be set to a larger value to
						achieve the same level of robustness)
			*/

			cv::calcOpticalFlowFarneback(
					mPrevFrame, currentFrame,
					mFlow,
					.5, 5, 13, 5, 5, 1.1, cv::OPTFLOW_FARNEBACK_GAUSSIAN );
		}
		mPrevFrame = currentFrame;

		// fluid update
		if ( mFlow.data )
		{
			RectMapping ofNorm( Area( 0, 0, mFlow.cols, mFlow.rows ),
					Rectf( 0.f, 0.f, 1.f, 1.f ) );

			// calculate mask
			Rectf maskRectNorm = mOptFlowClipRectNorm.getClipBy( mPreviewRectNorm );
			if ( ( maskRectNorm.getWidth() > 0 ) && maskRectNorm.getHeight() > 0 )
			{
				Rectf maskRect = maskRectNorm.getOffset( -mPreviewRectNorm.getUpperLeft() );
				maskRect.scale( Vec2f::one() / mPreviewRectNorm.getSize() );
				Area maskArea( maskRect.scaled( Vec2f( mFlow.cols, mFlow.rows ) ) );

				for ( int y = maskArea.y1; y < maskArea.y2; y++ )
				{
					for ( int x = maskArea.x1; x < maskArea.x2; x++ )
					{
						Vec2f v = fromOcv( mFlow.at< cv::Point2f >( y, x ) );
						Vec2f p( x + .5, y + .5 );
						addToFluid( ofNorm.map( p ), ofNorm.map( v ) * mFlowMultiplier );
					}
				}
			}
		}
	}

	// fluid & particles
	mFluidSolver.setFadeSpeed( mFluidFadeSpeed );
	mFluidSolver.setDeltaT( mFluidDeltaT  );
	mFluidSolver.setVisc( mFluidViscosity );
	mFluidSolver.enableVorticityConfinement( mFluidVorticityConfinement );
	mFluidSolver.setWrap( mFluidWrapX, mFluidWrapY );
	mFluidSolver.update();

	mParticles.setAging( mParticleAging );
	mParticles.update( getElapsedSeconds() );

	mLetterManager->setLetters( mLetters );
	mLetterManager->setSize( mFontSizeMin, mFontSizeMax );
	mLetterManager->update( getElapsedSeconds() );
}

void FludParticlesApp::addToFluid( Vec2f pos, Vec2f vel, bool addParticles, bool addForce, bool addColor, bool addLetters )
{
	if ( vel.lengthSquared() > 0.000001f )
	{
		pos.x = constrain( pos.x, 0.0f, 1.0f );
		pos.y = constrain( pos.y, 0.0f, 1.0f );

		if ( addParticles || addLetters )
		{
			int count = static_cast<int>(
					lmap<float>( vel.length() * mVelParticleMult * mParticlesFbo.getWidth(),
						mVelParticleMin, mVelParticleMax,
						mParticleMin, mParticleMax ) );
			if (count > 0)
			{
				if ( addParticles )
					mParticles.addParticle( pos * Vec2f( mParticlesFbo.getSize() ), count );
				if ( addLetters )
					mLetterManager->addLetter( pos * Vec2f( getWindowSize() ) );
			}
		}
		if ( addForce )
			mFluidSolver.addForceAtPos( pos, vel * mFluidVelocityMult );

		if ( addColor )
		{
			mFluidSolver.addColorAtPos( pos, Color::white() * mFluidColorMult );
		}
	}
}

void FludParticlesApp::draw()
{
	if ( getWindow() == mControlWindow )
		drawControl();
	else
		drawOutput();
}

void FludParticlesApp::drawControl()
{
	gl::clear();
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	if ( mCaptureTexture )
	{
		gl::color( Color::white() );
		Rectf previewRect = mPreviewRectNorm.scaled( Vec2f( getWindowSize() ) );
		gl::pushModelView();
		if ( mFlip )
		{
			gl::translate( previewRect.getUpperLeft() + Vec2f( previewRect.getWidth(), 0.f ) );
			gl::scale( -1, 1 );
			gl::translate( -previewRect.getUpperLeft() );
		}
		mCaptureTexture.enableAndBind();
		gl::drawSolidRect( previewRect );
		mCaptureTexture.unbind();
		mCaptureTexture.disable();
		gl::popModelView();
		gl::drawStrokedRect( previewRect );

		Rectf maskRect = mOptFlowClipRectNorm.scaled( Vec2f( getWindowSize() ) );
		maskRect.clipBy( previewRect );
		gl::enableAlphaBlending();
		gl::color( ColorA( 1.f, 0.f, 0.f, .2f ) );
		gl::drawSolidRect( maskRect );
		gl::disableAlphaBlending();
	}

	mParams.draw();
	mCaptureSource.drawParams();
}

void FludParticlesApp::drawOutput()
{
	gl::clear();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	if ( mDrawFluid )
	{
		gl::color( mFluidColor );
		mFluidDrawer.draw( 0, 0, getWindowWidth(), getWindowHeight() );
	}

	if ( mDrawParticles )
	{
		mParticlesFbo.bindFramebuffer();
		// FIXME: particles overwrite fluid
		//gl::clear( ColorA::gray( 0, 0 ) );
		gl::clear();

		gl::setViewport( mParticlesFbo.getBounds() );
		gl::setMatricesWindow( mParticlesFbo.getSize(), false );

		//gl::color( Color( .9f, .5f, .1f ) );
		//gl::drawSolidCircle( mParticlesFbo.getSize() / 2.f, 10 );
		mParticles.draw();
		mParticlesFbo.unbindFramebuffer();

		gl::Texture output = mKawaseStreak.process( mParticlesFbo.getTexture(), mStreakAttenuation, mStreakIterations, mStreakStrength );
		//if ( mCaptureTexture )
		{
			//gl::Texture output = mKawaseStreak.process( mCaptureTexture, 8, mBloomStrength );

			gl::setViewport( getWindowBounds() );
			gl::setMatricesWindow( getWindowSize() );
			if ( mDrawFluid )
				gl::enableAlphaBlending();
			gl::color( Color::white() );
			gl::draw( output, getWindowBounds() );
			if ( mLettersEnabled )
			{
				gl::enableAlphaBlending();
				mLetterManager->draw();
				gl::disableAlphaBlending();
			}
		}
		if ( mDrawFluid )
			gl::disableAlphaBlending();
	}

	/*
	{
		Rectf rect( getWindowBounds() );
		rect /= 4.f;

		for ( int i = 1; i < 2; i++ )
		{
			gl::Texture output = mKawaseStreak.getTexture( i );
			int x = i / 4;
			int y = i & 3;
			gl::draw( output, rect.getOffset( Vec2f( x * rect.getWidth(), y * rect.getHeight() ) ) );
		}
	}
	*/

	// draw output to window

	if ( mDrawCapture && mCaptureTexture )
	{
		gl::enableAdditiveBlending();
		gl::color( ColorA( 1, 1, 1, mCaptureAlpha ) );
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
}

void FludParticlesApp::mouseDown( MouseEvent event )
{
	if ( getWindow() == mControlWindow )
		mMouseStartNorm = Vec2f( event.getPos() ) / Vec2f( getWindowSize() );
}

void FludParticlesApp::mouseUp( MouseEvent event )
{
	if ( getWindow() == mControlWindow )
	{
		mMouseEndNorm = Vec2f( event.getPos() ) / Vec2f( getWindowSize() );
		float x0 = math< float >::min( mMouseStartNorm.x, mMouseEndNorm.x );
		float x1 = math< float >::max( mMouseStartNorm.x, mMouseEndNorm.x );
		mOptFlowClipRectNorm = Rectf( x0, 0.f, x1, 1.f );
	}
}

void FludParticlesApp::mouseMove( MouseEvent event )
{
	mMousePrev = event.getPos();
}

void FludParticlesApp::mouseDrag( MouseEvent event )
{
	Vec2f mouseNorm = Vec2f( event.getPos() ) / getWindowSize();
	Vec2f mouseVel = Vec2f( event.getPos() - mMousePrev ) / getWindowSize();
	addToFluid( mouseNorm, mouseVel, false, true, false, mLettersEnabled );
	mMousePrev = event.getPos();
}

void FludParticlesApp::shutdown()
{
	mCaptureSource.shutdown();
	mndl::params::PInterfaceGl::save();
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
			mndl::params::PInterfaceGl::showAllParams( !mParams.isVisible() );
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

		case KeyEvent::KEY_m:
			showCursor();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( FludParticlesApp, RendererGl )

