#pragma once

#include <list>

#include "cinder/Vector.h"
#include "cinder/gl/TextureFont.h"

#include "ciMsaFluidSolver.h"

class Letter
{
	public:
		Letter( const ci::Vec2f &pos, ci::gl::TextureFontRef textureFont );

		void update( double time, const ciMsaFluidSolver *solver, const ci::Vec2f &windowSize, const ci::Vec2f &invWindowSize );

		void draw();
		bool isAlive() { return mLifeSpan > 0; }

	protected:
		ci::gl::TextureFontRef mTextureFont;

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

class LetterManager;
typedef std::shared_ptr< LetterManager > LetterManagerRef;

class LetterManager
{
	public:
		static LetterManagerRef create() { return LetterManagerRef( new LetterManager() ); }

		void setWindowSize( ci::Vec2i winSize );
		void setFluidSolver( const ciMsaFluidSolver *aSolver ) { mSolver = aSolver; }

		void update( double seconds );
		void draw();

		void addLetter( const ci::Vec2f &pos, int32_t count );

	private:
		LetterManager();
		ci::Vec2i mWindowSize;
		ci::Vec2f mInvWindowSize;

		const ciMsaFluidSolver *mSolver;

		std::list< Letter > mLetters;
		ci::Font mFont;
		ci::gl::TextureFontRef mTextureFont;
};


