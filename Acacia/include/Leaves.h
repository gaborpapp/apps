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
		//void updateVertexArrays( bool drawingFluid, const ci::Vec2f &invWindowSize, int i, float* posBuffer, float* colBuffer);

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

	private:
		ci::Vec2i mWindowSize;
		ci::Vec2f mInvWindowSize;

		const ciMsaFluidSolver *mSolver;

		std::list< Leaf > mLeaves;

		/*
		void updateAndDraw( bool drawingFluid );
		void addParticles( const ci::Vec2f &pos, int count );
		void addParticle( const ci::Vec2f &pos );
		*/
};


