#pragma once

#include "cinder/Vector.h"
#include "cinder/Color.h"

#include "ciMsaFluidSolver.h"

class Particle
{
	public:
		Particle();
		Particle( const ci::Vec2f &pos );

		void update( double time, const ciMsaFluidSolver *solver, const ci::Vec2f &windowSize, const ci::Vec2f &invWindowSize, float *positions, float *colors );
		bool isAlive() { return mLifeSpan > 0; }

	private:
		ci::Vec2f mPos;
		ci::Vec2f mVel;
		ci::ColorA mColor;

		float mSize;
		float mLifeSpan;
		float mMass;

		static const float sMomentum;
		static const float sFluidForce;
};

class ParticleManager
{
	public:
		ParticleManager();

		void setWindowSize( ci::Vec2i winSize );
		void setFluidSolver( const ciMsaFluidSolver *aSolver ) { mSolver = aSolver; }

		void update( double seconds );
		void draw();

		void addParticle( const ci::Vec2f &pos, int count = 1 );

		static float getAging() { return sAging; }
		static void setAging( float a ) { sAging = a; }

	private:
		ci::Vec2i mWindowSize;
		ci::Vec2f mInvWindowSize;

		const ciMsaFluidSolver *mSolver;

		static float sAging;

#define MAX_PARTICLES 16384 // pow 2!
		int mCurrent;
		int mActive;

		float mPositions[ MAX_PARTICLES * 2 * 2 ];
		float mColors[ MAX_PARTICLES * 4 * 2 ];
		Particle mParticles[ MAX_PARTICLES ];
};


