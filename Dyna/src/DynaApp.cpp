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

#include <list>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/params/Params.h"

#include "ciMsaFluidSolver.h"
#include "ciMsaFluidDrawerGl.h"

#include "DynaStroke.h"
#include "Particles.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DynaApp : public AppBasic
{
	public:
		DynaApp();

		void prepareSettings(Settings *settings);
		void setup();

		void resize(ResizeEvent event);

		void keyDown(KeyEvent event);

		void mouseDown(MouseEvent event);
		void mouseDrag(MouseEvent event);
		void mouseUp(MouseEvent event);

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		bool mLeftButton;
		Vec2i mMousePos;
		Vec2i mPrevMousePos;

		void clearStrokes();
		list< DynaStroke > mDynaStrokes;

		float mK;
		float mDamping;
		float mStrokeMinWidth;
		float mStrokeMaxWidth;

		int mParticleMin;
		int mParticleMax;
		float mVelParticleMult;
		float mVelParticleMin;
		float mVelParticleMax;

		ciMsaFluidSolver mFluidSolver;
		ciMsaFluidDrawerGl mFluidDrawer;
		static const int sFluidSizeX = 128;

		void addToFluid(Vec2f pos, Vec2f vel, bool addParticles, bool addForce);

		ParticleManager mParticles;

		ci::Vec2i mPrevMouse;

		gl::Fbo mFbo;
};

void DynaApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

DynaApp::DynaApp() :
	mK( .06 ),
	mDamping( .7 ),
	mStrokeMinWidth( 1 ),
	mStrokeMaxWidth( 16 ),
	mParticleMin( 0 ),
	mParticleMax( 40 ),
	mVelParticleMult( .26 ),
	mVelParticleMin( 1 ),
	mVelParticleMax( 60 )
{
}

void DynaApp::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl("Parameters", Vec2i(200, 300));

	mParams.addParam("Stiffness", &mK, "min=.01 max=.2 step=.01");
	mParams.addParam("Damping", &mDamping, "min=.25 max=.999 step=.02");
	mParams.addParam("Stroke min", &mStrokeMinWidth, "min=0 max=50 step=.5");
	mParams.addParam("Stroke width", &mStrokeMaxWidth, "min=-50 max=50 step=.5");

	mParams.addButton("Clear strokes", std::bind(&DynaApp::clearStrokes, this), " key=SPACE ");

	mParams.addParam("Particle min", &mParticleMin, "min=0 max=50");
	mParams.addParam("Particle max", &mParticleMax, "min=0 max=50");
	mParams.addParam("Velocity particle multiplier", &mVelParticleMult, "min=0 max=2 step=.01");
	mParams.addParam("Velocity particle min", &mVelParticleMin, "min=1 max=100 step=.5");
	mParams.addParam("Velocity particle max", &mVelParticleMax, "min=1 max=100 step=.5");

	// fluid
	mFluidSolver.setup( sFluidSizeX, sFluidSizeX );
	mFluidSolver.enableRGB(false).setFadeSpeed(0.002).setDeltaT(.5).setVisc(0.00015).setColorDiffusion(0);
	mFluidSolver.setWrap( false, true );
	mFluidDrawer.setup( &mFluidSolver );

	mParticles.setFluidSolver( &mFluidSolver );

	mFbo = gl::Fbo( 1024, 768 );
	mFluidSolver.setSize( sFluidSizeX, sFluidSizeX / mFbo.getAspectRatio() );
	mFluidDrawer.setup( &mFluidSolver );
	mParticles.setWindowSize( mFbo.getSize() );
}

void DynaApp::resize(ResizeEvent event)
{
	/*
	mFluidSolver.setSize( sFluidSizeX, sFluidSizeX / event.getAspectRatio() );
	mFluidDrawer.setup( &mFluidSolver );
	mParticles.setWindowSize( event.getSize() );
	*/
}

void DynaApp::clearStrokes()
{
	mDynaStrokes.clear();
}

void DynaApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void DynaApp::addToFluid( Vec2f pos, Vec2f vel, bool addParticles, bool addForce )
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
				mParticles.addParticle( pos * Vec2f( mFbo.getSize() ), count);
			}
		}
		if ( addForce )
			mFluidSolver.addForceAtPos( pos, vel * velocityMult );
	}
}

void DynaApp::mouseDown(MouseEvent event)
{
	if (event.isLeft())
	{
		mDynaStrokes.push_back( DynaStroke() );
		DynaStroke *d = &mDynaStrokes.back();
		d->resize( ResizeEvent( mFbo.getSize() ) );
		d->setStiffness( mK );
		d->setDamping( mDamping );
		d->setStrokeMinWidth( mStrokeMinWidth );
		d->setStrokeMaxWidth( mStrokeMaxWidth );

		mLeftButton = true;
		mMousePos = event.getPos();
	}
}

void DynaApp::mouseDrag(MouseEvent event)
{
	if (event.isLeftDown())
	{
		mMousePos = event.getPos();

		Vec2f mouseNorm = Vec2f( mMousePos ) / getWindowSize();
		Vec2f mouseVel = Vec2f( mMousePos - mPrevMousePos ) / getWindowSize();
		addToFluid( mouseNorm, mouseVel, true, true );

		mPrevMousePos = mMousePos;
	}
}

void DynaApp::mouseUp(MouseEvent event)
{
	if (event.isLeft())
	{
		mLeftButton = false;
		mMousePos = mPrevMousePos = event.getPos();
	}
}

void DynaApp::update()
{
	if ( mLeftButton && !mDynaStrokes.empty() )
		mDynaStrokes.back().update( Vec2f( mMousePos ) / getWindowSize() );

	mFluidSolver.update();

	mParticles.setAging( 0.9 );
	mParticles.update( getElapsedSeconds() );
}

void DynaApp::draw()
{
	gl::clear( Color::black() );

	mFbo.bindFramebuffer();
	gl::setMatricesWindow( mFbo.getSize(), false );
	gl::setViewport( mFbo.getBounds() );

	gl::clear( Color::black() );

	gl::color( Color::white() );

	for (list< DynaStroke >::iterator i = mDynaStrokes.begin(); i != mDynaStrokes.end(); ++i)
	{
		i->draw();
	}

	mParticles.draw();

	mFbo.unbindFramebuffer();

	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );

	gl::color( Color::white() );
	gl::draw( mFbo.getTexture(), getWindowBounds() );

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(DynaApp, RendererGl( RendererGl::AA_NONE ))

