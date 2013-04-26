/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <vector>

#include "cinder/qtime/MovieWriter.h"
#include "cinder/qtime/QuickTime.h"

#include "cinder/Capture.h"
#include "cinder/Surface.h"

#include "Capture1394PParams.h"

#include "mndlkit/params/PParams.h"

namespace mndl {

class CaptureSource
{
	public:
		void setup();
		void update();
		void drawParams();

		void shutdown();

		bool isCapturing() const;
		bool checkNewFrame();

		int32_t getWidth() const;
		int32_t getHeight() const;
		ci::Vec2i getSize() const { return ci::Vec2i( getWidth(), getHeight() ); }
		float getAspectRatio() const { return getWidth() / (float)getHeight(); }
		ci::Area getBounds() const { return ci::Area( 0, 0, getWidth(), getHeight() ); }

		ci::Surface8u getSurface();

	protected:
		enum
		{
			SOURCE_RECORDING = 0,
			SOURCE_CAPTURE,
			SOURCE_CAPTURE1394
		};

		void setupParams();
		void playVideoCB();
		void saveVideoCB();

		ci::qtime::MovieSurface mMovie;
		ci::qtime::MovieWriter mMovieWriter;
		bool mSavingVideo;

		int mSource; // recording or camera

		// qtime capture
		std::vector< ci::CaptureRef > mCaptures;
		std::vector< std::string > mDeviceNames;
		int mCurrentCapture;

		// capture1394
		mndl::Capture1394PParamsRef mCapture1394PParams;

		// params
		mndl::params::PInterfaceGl mParams;
		mndl::params::PInterfaceGl mCaptureParams;
};

} // namespace mndl;
