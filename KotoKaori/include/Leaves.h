#pragma once

#include <list>

#include "cinder/Vector.h"
#include "cinder/gl/Texture.h"

#include "ciMsaFluidSolver.h"

class Leaf
{
	public:
		Leaf( const ci::Vec2f &pos, ci::gl::Texture texture );

		void update( double time, const ciMsaFluidSolver *solver, const ci::Vec2f &windowSize, const ci::Vec2f &invWindowSize );
		void draw();
		bool isAlive() { return mLifeSpan > 0; }

	private:
		ci::gl::Texture mTexture;

		ci::Vec2f mPos;
		ci::Vec2f mVel;
		float mSize;
		float mLifeSpan;
		float mMass;

		float mSlideAmplitude;
		float mSlideFrequency;
		float mSlideOffset;

		static const float sMomentum;
		static const float sFluidForce;
};

class LeafManager
{
	public:
		LeafManager();

		void setWindowSize( ci::Vec2i winSize );
		void setFluidSolver( const ciMsaFluidSolver *aSolver ) { mSolver = aSolver; }

		void update( double seconds );
		void draw();

		void addLeaf( const ci::Vec2f &pos, ci::gl::Texture texture );
		void clear();

		static float getGravity() { return sGravity; }
		static void setGravity( float g ) { sGravity = g; }

		static float getAging() { return sAging; }
		static void setAging( float a ) { sAging = a; }

		unsigned getMaximum() const { return mMaximum; }
		void setMaximum( unsigned m ) { mMaximum = m; }

		unsigned getCount() const { return mLeaves.size(); }

	private:
		ci::Vec2i mWindowSize;
		ci::Vec2f mInvWindowSize;

		const ciMsaFluidSolver *mSolver;

		unsigned mMaximum;
		static float sGravity;
		static float sAging;

		std::list< Leaf > mLeaves;
};


