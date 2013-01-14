#pragma once

#include <vector>

#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"

#include "cinder/Capture.h"
#include "cinder/Cinder.h"
#include "cinder/Quaternion.h"

#include "ArTracker.h"
#include "PParams.h"

class TrackerManager
{
	public:
		~TrackerManager();

		void setup();

		void update();

		//! draws tracking debug data
		void draw();

		ci::Quatf getRotation() const { return mRotation; }

		float getZoom() const { return mZoom; };

	private:
		void setupCapture();

		// params
		ci::params::PInterfaceGl mParams;

		// capture
		ci::Capture mCapture;
		ci::gl::Texture mCaptTexture;

		std::vector< ci::Capture > mCaptures;

		static const int CAPTURE_WIDTH = 640;
		static const int CAPTURE_HEIGHT = 480;

		int32_t mCurrentCapture;

		// ar
		mndl::artkp::ArTracker mArTracker;

		float mMovementSmoothness;
		bool mDebugTracking;
		float mDebugSize;

		// rotation logic
		ci::Quatf mRotation = ci::Quatf( ci::Vec3f( 1, 0, 0 ), 0 ); //< rotation
		// scale logic
		ci::Vec3f mPosition; // marker cube position in millimeters
		float mZoom = .6f; // zoom mapped from cube z values to 0-1 range
		float mMinZ, mMaxZ; // cube Z position range

		/**! if true - the marker cube is lost for a longer period, and slerp is started
		 * back to the original orientation, if false - the cube has just been lost this frame,
		 * indeterminate - the cube is beeing seen */
		boost::tribool mBackToInit;
		float mTolerance; //< duration for which the missing marker cube is tolerated

		ci::gl::Fbo mDebugFbo; //< debug draw
};

