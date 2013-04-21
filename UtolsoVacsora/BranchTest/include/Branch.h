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
#include "cinder/Filesystem.h"
#include "cinder/Path2d.h"
#include "cinder/Timeline.h"
#include "cinder/Vector.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"

class Branch;
typedef std::shared_ptr< Branch > BranchRef;

class Branch
{
	public:
		static BranchRef create() { return BranchRef( new Branch() ); }

		void setStemBearingDelta( float angleDelta ) { mStemBearingDelta = angleDelta; }
		void setStemLength( float lengthMin, float lengthMax ) { mStemLengthMin = lengthMin; mStemLengthMax = lengthMax; }

		void setMaxIterations( int32_t iterations ) { mMaxIterations = iterations; }

		//! Sets the grow speed in 1000 pixels / seconds.
		void setGrowSpeed( float speed ) { mGrowSpeed = speed; }
		//! Sets approximation scale used in the subdivision of the path.
		void setApproximationScale( float approximationScale ) { mApproximationScale = approximationScale; }

		//! Sets the thickness of the branch in pixels.
		void setThickness( float thickness ) { mThickness = thickness; }

		//! Sets the spawn interval of leaves, flowers, branches in pixels.
		void setSpawnInterval( float interval ) { mSpawnInterval = interval; }

		//! Sets the branch angle of leaves, flowers, branches.
		void setBranchAngle( float angle ) { mBranchAngle = angle; }

		//! Sets the scale factor of leaf and flower textures;
		void setItemScale( float scale ) { mItemScale = scale; }

		//! Sets the main color of the branch.
		void setColor( const ci::ColorA &color ) { mColor = color; }

		void setup( const ci::Vec2f &start, const ci::Vec2f &target );

		void start();

		void update();
		void draw();

		void loadTextures( const ci::fs::path &textureFolder );
		void setTextures( const ci::gl::Texture &stemTexture, const ci::gl::Texture &branchTexture, const std::vector< ci::gl::Texture > &leafTextures, const std::vector< ci::gl::Texture > &flowerTextures )
		{ mBranchTexture = branchTexture; mStemTexture = stemTexture; mLeafTextures = leafTextures; mFlowerTextures = flowerTextures; }

		ci::gl::Texture getStemTexture() { return mStemTexture; }
		ci::gl::Texture getBranchTexture() { return mBranchTexture; }
		std::vector< ci::gl::Texture > & getLeafTextures() { return mLeafTextures; }
		std::vector< ci::gl::Texture > & getFlowerTextures() { return mFlowerTextures; }

		void resize( const ci::Vec2i &size ) { mWindowSize = size; }

	private:
		Branch();

		void calcPath();

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
		float mApproximationScale = 100.f;

		ci::Path2d mPath;
		std::vector< ci::Vec2f > mPoints; //< points to draw while growing

		ci::gl::Texture mStemTexture;
		ci::gl::Texture mBranchTexture;
		std::vector< ci::gl::Texture > mFlowerTextures;
		std::vector< ci::gl::Texture > mLeafTextures;

		float mThickness = 50.f;
		float mLimit = .75f;
		ci::ColorA mColor = ci::ColorA::white();
		float mSpawnInterval = 32.f; //< spawn leaves,flowers,branches at every mSpawnInterval pixels
		float mBranchAngle = M_PI * 25.f;

		struct BranchItem
		{
			BranchItem( ci::gl::Texture texture, const ci::Vec2f &position, const ci::Vec2f &direction );

			void draw();

			ci::gl::Texture mTexture;
			ci::Vec2f mPosition;
			float mRotation;
			float mScale = 1.f;
		};

		float mCurrentItemSide = 1.f; //< 1 for right, -1 for left
		float mLastItemLength = 0.f;
		float mItemScale = 1.f;
		std::vector< BranchItem > mBranchItems;

		static ci::gl::GlslProg sShader;

		ci::Vec2f mWindowSize;
};

