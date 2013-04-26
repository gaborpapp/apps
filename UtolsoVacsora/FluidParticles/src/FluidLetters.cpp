#include "cinder/CinderMath.h"
#include "cinder/app/app.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "FluidLetters.h"

using namespace ci;
using namespace std;

const float Letter::sMomentum = 0.6f;
const float Letter::sFluidForce = 0.9f;

Letter::Letter( const Vec2f &pos, gl::TextureFontRef textureFont )
{
    mPos = pos;
	mVel = Vec2f( 0, 0 );
	mSize = Rand::randFloat( 1, 2 );
    mLifeSpan = Rand::randFloat( 0.3f, 1 );
	mMass = Rand::randFloat( 0.1f, 1 );

	mSlideAmplitude = Rand::randFloat( .1, .4 );
	mSlideFrequency = Rand::randFloat( 1., 2.5 );
	mSlideOffset = Rand::randFloat( 0, 2 * M_PI );
	mTextureFont = textureFont;
}

void Letter::update( double time, const ciMsaFluidSolver *solver, const Vec2f &windowSize, const Vec2f &invWindowSize )
{
	mVel = solver->getVelocityAtPos( mPos * invWindowSize ) * (mMass * sFluidForce ) * windowSize + mVel * sMomentum;

	/*
	if ( mPos.y >= windowSize.y )
	{
		mPos.y = windowSize.y;
		if (mVel.y > 0)
			mVel = Vec2f( 0, 0 );
	}
	else
	{
		mVel += Vec2f( mSlideAmplitude * sin( mSlideOffset + mSlideFrequency * time),
				.3 );
	}
	*/

	mPos += mVel;

	mLifeSpan *= 0.995f;
	if ( mLifeSpan < 0.01f )
		mLifeSpan = 0;
}

void Letter::draw()
{
	Vec2f n;

	if ( mVel.length() < EPSILON_VALUE )
		n = Vec2f( 0, -mSize );
	else
		n = mSize * mVel.normalized();

	Vec2f s( -n.y, n.x );

	gl::color( ColorA( 1, 1, 1, mLifeSpan ) );
	gl::pushModelView();
	float scale = n.length();
	gl::translate( mPos );
	gl::scale( Vec2f( scale, scale ) );
	gl::rotate( toDegrees( math< float >::atan2( n.y, n.x ) ) );
	mTextureFont->drawString( "A", Vec2f::zero() );
	gl::popModelView();
}

LetterManager::LetterManager()
{
	setWindowSize( Vec2i( 1, 1 ) );
	mFont = Font( "ChunkFive", 24 );
	mTextureFont = gl::TextureFont::create( mFont );
}

void LetterManager::setWindowSize( Vec2i winSize )
{
	mWindowSize = winSize;
	mInvWindowSize = Vec2f( 1.0f / winSize.x, 1.0f / winSize.y );
}

void LetterManager::update( double seconds )
{
	for ( list< Letter >::iterator it = mLetters.begin(); it != mLetters.end(); )
	{
		if ( it->isAlive() )
		{
			it->update( seconds, mSolver, mWindowSize, mInvWindowSize );
			++it;
		}
		else
		{
			it = mLetters.erase( it );
		}
	}
}

void LetterManager::draw()
{
	gl::enable( GL_TEXTURE_2D );
	for ( list< Letter >::iterator it = mLetters.begin(); it != mLetters.end(); ++it )
	{
		it->draw();
	}
	gl::disable( GL_TEXTURE_2D );
}

void LetterManager::addLetter( const Vec2f &pos, int32_t count )
{
	mLetters.push_back( Letter( pos, mTextureFont ) );
}

