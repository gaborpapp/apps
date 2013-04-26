#include "cinder/CinderMath.h"
#include "cinder/app/app.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "FluidLetters.h"

using namespace ci;
using namespace std;

const float Letter::sMomentum = 0.6f;
const float Letter::sFluidForce = 0.9f;
float Letter::sSizeMin = .5f;
float Letter::sSizeMax = 1.f;

Letter::Letter( const Vec2f &pos, const std::string &letter, gl::TextureFontRef textureFont )
{
	mPos = pos;
	mVel = Vec2f( 0, 0 );
	mSize = Rand::randFloat( sSizeMin, sSizeMax );
	mLifeSpan = mMaxLife = Rand::randFloat( 0.3f, 1 );
	mMass = Rand::randFloat( 0.1f, 1 );
	mLetter = letter;

	mTextureFont = textureFont;
}

void Letter::update( double time, const ciMsaFluidSolver *solver, const Vec2f &windowSize, const Vec2f &invWindowSize )
{
	mVel = solver->getVelocityAtPos( mPos * invWindowSize ) * (mMass * sFluidForce ) * windowSize + mVel * sMomentum;

	mPos += mVel;

	mLifeSpan *= 0.995f;
	if ( mLifeSpan < 0.01f )
		mLifeSpan = 0;
}

void Letter::draw()
{
	gl::color( ColorA( 1, 1, 1, mLifeSpan / mMaxLife ) );
	gl::pushModelView();
	gl::translate( mPos );
	gl::scale( Vec2f( mSize, mSize ) );
	gl::rotate( toDegrees( math< float >::atan2( mVel.y, mVel.x ) ) );
	mTextureFont->drawString( mLetter, Vec2f::zero() );
	gl::popModelView();
}

LetterManager::LetterManager() :
	mAllowedLetters( "aA" ),
	mSizeMin( 12 ),
	mSizeMax( 24 )
{
	setWindowSize( Vec2i( 1, 1 ) );
	setFont( Font::getDefault().getName() );
}

void LetterManager::setFont( const std::string &fontName )
{
	try
	{
		mFont = Font( fontName, mSizeMax );
	}
	catch ( const FontInvalidNameExc &exc )
	{
		app::console() << exc.what() << std::endl;
		mFont = Font::getDefault();
	}

	mTextureFont = gl::TextureFont::create( mFont );

	if ( mSizeMin > 0.f )
		Letter::setSize( mSizeMax / mSizeMin, 1.f );
	else
		Letter::setSize( 0.f, 1.f );
}

void LetterManager::setWindowSize( Vec2i winSize )
{
	mWindowSize = winSize;
	mInvWindowSize = Vec2f( 1.0f / winSize.x, 1.0f / winSize.y );
}

void LetterManager::setSize( float minSize, float maxSize )
{
	if ( ( minSize == mSizeMin ) && ( maxSize == mSizeMax ) )
		return;

	mSizeMin = minSize;
	mSizeMax = maxSize;
	setFont( mFont.getName() );
}

void LetterManager::update( double seconds )
{
	for ( vector< Letter >::iterator it = mLetters.begin(); it != mLetters.end(); )
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
	for ( vector< Letter >::iterator it = mLetters.begin(); it != mLetters.end(); ++it )
	{
		it->draw();
	}
}

void LetterManager::addLetter( const Vec2f &pos )
{
	int c = Rand::randInt( mAllowedLetters.length() );
	mLetters.push_back( Letter( pos, std::string( 1, mAllowedLetters[ c ] ), mTextureFont ) );
}

