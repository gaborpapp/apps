#pragma once

#include <list>

#include <cinder/Vector.h>
#include <cinder/app/App.h>
#include <cinder/gl/gl.h>

class DynaStroke
{
	public:
		DynaStroke();

		void resize( ci::app::ResizeEvent event );

		void update( ci::Vec2f point );
		void draw();

		void clear() { mPoints.clear(); }

		void setStiffness( float s ) { mK = s; }
		float getStiffness() { return mK; }

		void setDamping( float d ) { mDamping = d; }
		float getDamping() { return mDamping; }

		void setStrokeMinWidth( float w ) { mStrokeMinWidth = w; }
		float getStrokeMinWidth() const { return mStrokeMinWidth; }

		void setStrokeMaxWidth( float w ) { mStrokeMaxWidth = w; }
		float getStrokeMaxWidth() const { return mStrokeMaxWidth; }

		void setMaxVelocity( float v ) { mMaxVelocity = v; }
		float getMaxVelocity() const { return mMaxVelocity; }

	private:
		struct StrokePoint
		{
			StrokePoint( ci::Vec2f _p, ci::Vec2f _w ) :
				p( _p ), w( _w) {}

			ci::Vec2f p;
			ci::Vec2f w;
		};

		std::list<StrokePoint> mPoints;

		ci::Vec2f mPos; // spring position
		ci::Vec2f mVel; // velocity
		float mK; // spring stiffness
		float mDamping; // friction
		float mMass;

		float mStrokeMinWidth;
		float mStrokeMaxWidth;
		float mMaxVelocity;

		ci::Vec2i mWindowSize;
};

