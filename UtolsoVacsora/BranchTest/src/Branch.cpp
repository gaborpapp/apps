/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "Branch.h"

using namespace ci;

void Branch::update()
{
	// TODO: build this up sequentially
#if 1
	const int maxIterations = 128;
	for ( int i = 0; i < maxIterations; i++ )
	{
		Vec2f targetDir = mTargetPosition - mCurrentPosition;
		float stemDistance = Rand::randFloat( mStemLengthMin, mStemLengthMax );

		if ( i == 0 )
		{
			addArc( mCurrentPosition );
			float stemDistance = Rand::randFloat( mStemLengthMin, mStemLengthMax );
			mCurrentPosition += stemDistance * targetDir.normalized();
			addArc( mCurrentPosition );
		}
		else
		if ( targetDir.length() < stemDistance )
		{
			// TODO: connect this only if it's in direction
			mCurrentPosition = mTargetPosition;
			addArc( mCurrentPosition );
			break;
		}
		else
		{
			// current direction
			size_t segmentNum = mPath.getNumSegments();
			Vec2f p0 = mPath.getSegmentPosition( segmentNum - 1, 1.f );
			Vec2f currentDir = p0 - mPath.getSegmentPosition( segmentNum - 1, .98f );

			// minimum angle between the current direction and the target
			float angleDir = math< float >::atan2( currentDir.y, currentDir.x );
			float angleTarget = math< float >::atan2( targetDir.y, targetDir.x );
			float angle = angleTarget - angleDir;
			if ( angle > M_PI )
				angle -= 2 * M_PI;
			else
			if ( angle < -M_PI )
				angle += 2 * M_PI;

			// TODO: try using only the target dir instead of the range if the target is outside
			// direction range towards the target
			float angleMin = math< float >::clamp( angle - mStemBearingAngleMax, -mStemBearingAngleMax, mStemBearingAngleMax );
			float angleMax = math< float >::clamp( angle + mStemBearingAngleMax, -mStemBearingAngleMax, mStemBearingAngleMax );
			float bearingMin = angleDir + angleMin;
			float bearingMax = angleDir + angleMax;

			float stemBearingAngle = Rand::randFloat( bearingMin, bearingMax );
			Vec2f bearing( math< float >::cos( stemBearingAngle ), math< float >::sin( stemBearingAngle ) );
			mCurrentPosition += stemDistance * bearing;
			addArc( mCurrentPosition );
		}
	}
#endif
#if 0
	const int maxIterations = 64;
	for ( int i = 0; i < maxIterations; i++ )
	{
		Vec2f d = mTargetPosition - mCurrentPosition;

		float stemDistance = Rand::randFloat( mStemLengthMin, mStemLengthMax );
		if ( d.length() < stemDistance )
		{
			mCurrentPosition = mTargetPosition;
			addArc( mCurrentPosition );
			break;
		}
		else
		{
			float stemBearingAngle = Rand::randFloat( mStemBearingAngleMin, mStemBearingAngleMax );
			Vec2f bearing( d.normalized() );
			bearing.rotate( ( ( i & 1 ) ? 1 : -1 ) * stemBearingAngle );
			mCurrentPosition += stemDistance * bearing;
			addArc( mCurrentPosition );
		}
	}
#endif
}

void Branch::draw()
{
	gl::draw( mPath );
}

void Branch::addArc( const Vec2f &p1 )
{
	if ( mPath.empty() )
	{
		mPath.moveTo( p1 );
		return;
	}

	size_t segmentNum = mPath.getNumSegments();
	if ( segmentNum == 0 )
	{
		mPath.lineTo( p1 );
		return;
	}

	// last point
	Vec2f p0 = mPath.getSegmentPosition( segmentNum - 1, 1.f );
	// same point as the last one -> skip
	if ( p0.distanceSquared( p1 ) < EPSILON )
		return;

	// segment tangent - direction of the last segment
	Vec2f tangent = p0 - mPath.getSegmentPosition( segmentNum - 1, .98f );
	Vec2f n( -tangent.y, tangent.x ); // segment normal

	Vec2f d( p1 - p0 ); // distance of the arc center and the last path point
	d *= .5f;
	Vec2f b( -d.y, d.x ); // arc bisector

	// the arc center is the intersection of the segment normal and the bisector
	float s = ( d.y - d.x * n.y / n.x ) / ( b.x * n.y / n.x - b.y );
	Vec2f c( ( p0 + p1 ) *.5f + s * b ); // arc center

	Vec2f d0( p0 - c );
	float a0 = math< float >::atan2( d0.y, d0.x );
	Vec2f d1( p1 - c );
	float a1 = math< float >::atan2( d1.y, d1.x );

	// segment normal line and segment bisector distance
	tangent.normalize();
	float C = tangent.dot( p0 );
	float dist = tangent.dot( ( p0 + p1 ) * .5f ) - C;

	// arc direction depends on the quadrant, in which the new point resides
	bool forward = ( s > 0.f ) ^ ( dist < 0.f );
	mPath.arc( c, d0.length(), a0, a1, forward );

	// TODO: use a line if the new point lies on the tangent
	// mPath.lineTo( p1 );
}

