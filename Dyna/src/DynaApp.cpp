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
#include "cinder/params/Params.h"
#include "cinder/CinderMath.h"
#include "cinder/Easing.h"

#include "ciMsaFluidSolver.h"
#include "ciMsaFluidDrawerGl.h"

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

		struct StrokePoint
		{
			StrokePoint( Vec2f _p, Vec2f _w ) :
				p( _p ), w( _w) {}

			Vec2f p;
			Vec2f w;
		};

		list<StrokePoint> mPoints;

		Vec2f mPos; // spring position
		Vec2f mVel; // velocity
		float mK; // spring stiffness
		float mDamping; // friction
		float mMass;

		float mStrokeMin;
		float mStrokeWidth;
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


};

void DynaApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

DynaApp::DynaApp() :
	mK( .06 ),
	mDamping( .7 ),
	mMass( 1 ),
	mStrokeMin( 1 ),
	mStrokeWidth( 15 ),
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
	mParams.addParam("Stroke min", &mStrokeMin, "min=0 max=50 step=.5");
	mParams.addParam("Stroke width", &mStrokeWidth, "min=-50 max=50 step=.5");
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
}

void DynaApp::resize(ResizeEvent event)
{
	mFluidSolver.setSize( sFluidSizeX, sFluidSizeX / event.getAspectRatio() );
	mFluidDrawer.setup( &mFluidSolver );
	mParticles.setWindowSize( event.getSize() );
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
				mParticles.addParticle( pos * Vec2f( getWindowSize() ), count);
		}
		if ( addForce )
			mFluidSolver.addForceAtPos( pos, vel * velocityMult );
	}
}

void DynaApp::mouseDown(MouseEvent event)
{
	if (event.isLeft())
	{
		mPos = event.getPos();
		mVel = Vec2f::zero();
		mLeftButton = true;
		mMousePos = event.getPos();
		mPoints.clear();
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
	if (mLeftButton)
	{
		Vec2f d = mPos - mMousePos; // displacement from the cursor
		Vec2f f = -mK * d; // Hooke's law F = - k * d
		Vec2f a = f / mMass; // acceleration, F = ma

		mVel = mVel + a;
		mVel *= mDamping;
		mPos += mVel;

		Vec2f ang( -mVel.y, mVel.x );
		ang.normalize();

		const float maxVel = 60;
		float s = math<float>::clamp(mVel.length(), 0, maxVel);
		ang *= mStrokeMin + mStrokeWidth * easeInQuad(s / maxVel);
		mPoints.push_back( StrokePoint(mPos, ang) );
	}

	mFluidSolver.update();

	mParticles.setAging( 0.9 );
	mParticles.update( getElapsedSeconds() );
}

void DynaApp::draw()
{
	gl::clear( Color::black() );

	gl::color( Color::white() );

	glBegin( GL_QUAD_STRIP );
	for (list<StrokePoint>::iterator i = mPoints.begin(); i != mPoints.end(); ++i)
	{
		StrokePoint *s = &(*i);
		gl::vertex( s->p + s->w );
		gl::vertex( s->p - s->w );
	}
	glEnd();

	mParticles.draw();
	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(DynaApp, RendererGl( RendererGl::AA_NONE ))

