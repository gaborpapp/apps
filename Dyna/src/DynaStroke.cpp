#include "cinder/CinderMath.h"
#include "cinder/Easing.h"

#include "DynaStroke.h"

using namespace std;
using namespace ci;
using namespace ci::app;

DynaStroke::DynaStroke() :
	mK( .06 ),
	mDamping( .7 ),
	mMass( 1 ),
	mStrokeMinWidth( 1 ),
	mStrokeMaxWidth( 15 )
{
}

void DynaStroke::resize( ResizeEvent event )
{
	mWindowSize = event.getSize();
}

void DynaStroke::update( Vec2f pos )
{
	if ( mPoints.empty() )
	{
		mPos = pos;
		mVel = Vec2f::zero();
	}

	Vec2f d = mPos - pos; // displacement from the cursor
	Vec2f f = -mK * d; // Hooke's law F = - k * d
	Vec2f a = f / mMass; // acceleration, F = ma

	mVel = mVel + a;
	mVel *= mDamping;
	mPos += mVel;

	Vec2f ang( -mVel.y, mVel.x );
	ang.normalize();

	const float maxVel = 60;
	Vec2f scaledVel = mVel * Vec2f( mWindowSize );
	float s = math<float>::clamp( scaledVel.length(), 0, maxVel );
	ang *= mStrokeMinWidth +
		( mStrokeMaxWidth - mStrokeMinWidth ) * easeInQuad( s / maxVel );
	mPoints.push_back( StrokePoint( mPos * Vec2f( mWindowSize ), ang ) );
}

void DynaStroke::draw()
{
	glBegin( GL_QUAD_STRIP );
	size_t n = mPoints.size();
	float step = 1. / n;
	float u = 0;
	for ( list< StrokePoint >::const_iterator i = mPoints.begin(); i != mPoints.end(); ++i)
	{
		const StrokePoint *s = &(*i);
		glTexCoord2f( u, 0 );
		gl::vertex( s->p + s->w );
		glTexCoord2f( u, 1 );
		gl::vertex( s->p - s->w );
		u += step;
	}
	glEnd();
}

