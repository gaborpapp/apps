#include "cinder/CinderMath.h"
#include "cinder/app/app.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "Leaves.h"

using namespace ci;
using namespace std;

const float Leaf::sMomentum = 0.6f;
const float Leaf::sFluidForce = 0.9f;

Leaf::Leaf( const Vec2f &pos, gl::Texture texture )
{
    mPos = pos;
	mVel = Vec2f( 0, 0 );
	mSize = Rand::randFloat( 10, 20 );
    mLifeSpan = Rand::randFloat( 0.3f, 1 );
	mMass = Rand::randFloat( 0.1f, 1 );

	mSlideAmplitude = Rand::randFloat( .1, .4 );
	mSlideFrequency = Rand::randFloat( 1., 2.5 );
	mSlideOffset = Rand::randFloat( 0, 2 * M_PI );
	mTexture = texture;
}

void Leaf::update( double time, const ciMsaFluidSolver *solver, const Vec2f &windowSize, const Vec2f &invWindowSize )
{
	mVel = solver->getVelocityAtPos( mPos * invWindowSize ) * (mMass * sFluidForce ) * windowSize + mVel * sMomentum;

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

	mPos += mVel;

	mLifeSpan *= 0.995f;
	if ( mLifeSpan < 0.01f )
		mLifeSpan = 0;
}

void Leaf::draw()
{
	Vec2f n;

	if ( mVel.length() < EPSILON_VALUE )
		n = Vec2f( 0, -mSize );
	else
		n = mSize * mVel.normalized();

	Vec2f s( -n.y, n.x );

	mTexture.bind();
	gl::color( ColorA( 1, 1, 1, mLifeSpan ) );
	glBegin(GL_QUADS);
	glTexCoord2f( Vec2f( 1, 0 ) );
	gl::vertex( mPos + n + s );
	glTexCoord2f( Vec2f( 1, 1 ) );
	gl::vertex( mPos + n - s );
	glTexCoord2f( Vec2f( 0, 1 ) );
	gl::vertex( mPos - n - s );
	glTexCoord2f( Vec2f( 0, 0 ) );
	gl::vertex( mPos - n + s );
	glEnd();
	mTexture.unbind();
}

LeafManager::LeafManager()
{
	setWindowSize( Vec2i( 1, 1 ) );
}

void LeafManager::setWindowSize( Vec2i winSize )
{
	mWindowSize = winSize;
	mInvWindowSize = Vec2f( 1.0f / winSize.x, 1.0f / winSize.y );
}

void LeafManager::update( double seconds )
{
	for ( list< Leaf >::iterator it = mLeaves.begin(); it != mLeaves.end(); )
	{
		if ( it->isAlive() )
		{
			it->update( seconds, mSolver, mWindowSize, mInvWindowSize );
			++it;
		}
		else
		{
			it = mLeaves.erase( it );
		}
	}
}

void LeafManager::draw()
{
	gl::enable( GL_TEXTURE_2D );
	for ( list< Leaf >::iterator it = mLeaves.begin(); it != mLeaves.end(); ++it )
	{
		it->draw();
	}
	gl::disable( GL_TEXTURE_2D );
}

void LeafManager::addLeaf( const Vec2f &pos, gl::Texture texture )
{
	mLeaves.push_back( Leaf( pos, texture ) );
}

