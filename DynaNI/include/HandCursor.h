#pragma once

#include "cinder/Rect.h"
#include "cinder/gl/Texture.h"

class HandCursor
{
	public:
		enum
		{
			LEFT = 0,
			RIGHT
		};

		HandCursor( int type, ci::Vec2f pos, float z );

		static void setZClip( float z ) { mZClip = z; }
		static void setBounds( ci::Rectf bounds ) { mBounds = bounds; }
		static void setPosCoeff( float c ) { mPosCoeff = c; }
		static void setTransparencyCoeff( float c ) { mTranspCoeff = c; }

		void setPos( ci::Vec2f pos ) { mPos = pos; }

		void draw();

	private:
		static ci::gl::Texture mTextures[2];

		int mType;
		ci::Vec2f mPos;
		float mZ;

		static float mZClip;
		static ci::Rectf mBounds;
		static float mPosCoeff;
		static float mTranspCoeff;
};

