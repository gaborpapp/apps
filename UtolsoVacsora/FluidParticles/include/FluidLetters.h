#pragma once

#include <vector>
#include <string>

#include "cinder/Vector.h"
#include "cinder/gl/TextureFont.h"

#include "ciMsaFluidSolver.h"

class Letter
{
	public:
		Letter( const ci::Vec2f &pos, const std::string &letter, ci::gl::TextureFontRef textureFont );

		void update( double time, const ciMsaFluidSolver *solver, const ci::Vec2f &windowSize, const ci::Vec2f &invWindowSize );

		void draw();
		bool isAlive() { return mLifeSpan > 0; }

		static void setSize( float minSize, float maxSize ) { sSizeMin = minSize; sSizeMax = maxSize; }

	protected:
		ci::gl::TextureFontRef mTextureFont;

		ci::Vec2f mPos;
		ci::Vec2f mVel;
		float mSize;
		float mLifeSpan, mMaxLife;
		float mMass;
		std::string mLetter;

		static const float sMomentum;
		static const float sFluidForce;
		static float sSizeMin, sSizeMax;
};

class LetterManager;
typedef std::shared_ptr< LetterManager > LetterManagerRef;

class LetterManager
{
	public:
		static LetterManagerRef create() { return LetterManagerRef( new LetterManager() ); }

		void setWindowSize( ci::Vec2i winSize );
		void setFluidSolver( const ciMsaFluidSolver *aSolver ) { mSolver = aSolver; }

		void setFont( const std::string &fontName );
		void setLetters( const std::string &letters ) { mAllowedLetters = letters; }
		void setSize( float minSize, float maxSize );

		void update( double seconds );
		void draw();

		void addLetter( const ci::Vec2f &pos );

	private:
		LetterManager();
		ci::Vec2i mWindowSize;
		ci::Vec2f mInvWindowSize;

		const ciMsaFluidSolver *mSolver;

		std::vector< Letter > mLetters;
		ci::Font mFont;
		ci::gl::TextureFontRef mTextureFont;
		std::string mAllowedLetters;

		float mSizeMin, mSizeMax;
};


