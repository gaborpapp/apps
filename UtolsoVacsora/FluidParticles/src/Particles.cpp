#include "cinder/CinderMath.h"
#include "cinder/app/app.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "Particles.h"

using namespace ci;
using namespace std;

const float Particle::sMomentum = 0.6f;
const float Particle::sFluidForce = 0.9f;

Particle::Particle()
	: mLifeSpan( 0 )
{
}

Particle::Particle( const Vec2f &pos )
{
	mPos = pos;
	mVel = Vec2f( 0, 0 );
	mSize = Rand::randFloat( 10, 20 );
	mLifeSpan = Rand::randFloat( 0.3f, 1 );
	mMass = Rand::randFloat( 0.1f, 1 );
}

void Particle::update( double time, const ciMsaFluidSolver *solver, const Vec2f &windowSize, const Vec2f &invWindowSize, float *positions, float *colors )
{
	mVel = solver->getVelocityAtPos( mPos * invWindowSize ) * (mMass * sFluidForce ) * windowSize + mVel * sMomentum;

	//if ( mVel.lengthSquared() < 10 )
	{
		mVel += Rand::randVec2f() * 3.;
	}

	mPos += mVel;

	mLifeSpan *= ParticleManager::getAging();
	if ( mLifeSpan < 0.01f )
		mLifeSpan = 0;

	Vec2f velLimited = mVel.limited( 10 );

	positions[0] = mPos.x - velLimited.x;
	positions[1] = mPos.y - velLimited.y;
	positions[2] = mPos.x;
	positions[3] = mPos.y;

	float col = Rand::randFloat();
	colors[0] = col;
	colors[1] = col;
	colors[2] = col;
	colors[3] = mLifeSpan;
	colors[4] = col;
	colors[5] = col;
	colors[6] = col;
	colors[7] = mLifeSpan;
}

float ParticleManager::sAging = 0.995f;

ParticleManager::ParticleManager()
	: mCurrent( 0 ),
	  mActive( 0 )
{
	setWindowSize( Vec2i( 1, 1 ) );
}

void ParticleManager::setWindowSize( Vec2i winSize )
{
	mWindowSize = winSize;
	mInvWindowSize = Vec2f( 1.0f / winSize.x, 1.0f / winSize.y );
}

void ParticleManager::update( double seconds )
{
	int j = 0;
	mActive = 0;
	for ( int i = 0; i < MAX_PARTICLES; i++ )
	{
		if ( mParticles[i].isAlive() )
		{
			mParticles[i].update( seconds, mSolver,
					mWindowSize, mInvWindowSize,
					&mPositions[j * 2],
					&mColors[j * 4]);
			j += 2;
			mActive++;
		}
	}
}

void ParticleManager::draw()
{
	gl::disable( GL_TEXTURE_2D );
	gl::enable( GL_LINE_SMOOTH );

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_FLOAT, 0, mPositions );

	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_FLOAT, 0, mColors );

	glDrawArrays( GL_LINES, 0, mActive * 2 );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
}

void ParticleManager::addParticle( const Vec2f &pos, int count /* = 1 */ )
{
	mParticles[ mCurrent ] = Particle( pos );
	for (int i = count - 1; i > 0; i--)
	{
		mCurrent = (mCurrent + 1) & (MAX_PARTICLES - 1);
		mParticles[ mCurrent ] = Particle( pos + Rand::randVec2f() * 10 );
	}
	mCurrent = (mCurrent + 1) & (MAX_PARTICLES - 1);
}

