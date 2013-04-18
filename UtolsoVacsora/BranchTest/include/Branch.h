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

#pragma once

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/Path2d.h"
#include "cinder/Timeline.h"
#include "cinder/Vector.h"

class Branch;
typedef std::shared_ptr< Branch > BranchRef;

class Branch
{
	public:
		static BranchRef create( const ci::Vec2f &start, const ci::Vec2f &target ) { return BranchRef( new Branch( start, target ) ); }

		void setStemBearingDelta( float angleDelta ) { mStemBearingDelta = angleDelta; }
		void setStemLength( float lengthMin, float lengthMax ) { mStemLengthMin = lengthMin; mStemLengthMax = lengthMax; }

		void setMaxIterations( int32_t iterations ) { mMaxIterations = iterations; }

		//! Sets the grow speed in 1000 pixels / seconds.
		void setGrowSpeed( float speed ) { mGrowSpeed = speed; }
		//! Sets approximation scale used in the subdivision of the path.
		void setApproximationScale( float approximationScale ) { mApproximationScale = approximationScale; }

		void setup();

		void start();

		void update();
		void draw();

	private:
		Branch( const ci::Vec2f &start, int32_t maxIterations ) : mCurrentPosition( start ), mMaxIterations( maxIterations ) {}
		Branch( const ci::Vec2f &start, const ci::Vec2f &target ) : mCurrentPosition( start ), mTargetPosition( target ) {}

		void addArc( const ci::Vec2f &point );

		float mStemBearingDelta = M_PI * .2f;
		float mStemLengthMin = 10.f, mStemLengthMax = 64.f;
		float mLength = 0.f;

		ci::Vec2f mCurrentPosition;
		ci::Vec2f mTargetPosition = ci::Vec2f::zero();
		int32_t mMaxIterations = 128;
		float mGrowSpeed = 1.f;
		bool mGrown = true;
		ci::Anim< size_t > mGrowPosition = 0;
		float mApproximationScale = 1.f;

		ci::Path2d mPath;
		std::vector< ci::Vec2f > mPoints; //< points to draw while growing
};

