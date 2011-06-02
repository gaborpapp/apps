/*
 Copyright (C) 2011 Gabor Papp

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

#ifndef KINECT_CALIBRATION_APP_H
#define KINECT_CALIBRATION_APP_H

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/ImageIo.h"

#include "Kinect.h"

#include "CameraCalibration.h"

class KinectCalibrationApp : public ci::app::AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();
		void enableVSync(bool vs);
		void shutdown();

		void mouseDown(ci::app::MouseEvent event);
		void keyDown(ci::app::KeyEvent event);

		void update();
		void draw();

	protected:
		ci::Kinect kinect;
		mndl::CameraCalibration rgb_cal;

		ci::ImageSourceRef video_image_ref;
};

#endif

