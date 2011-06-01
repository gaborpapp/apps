#ifndef KINECT_CAL_H
#define KINECT_CAL_H

#include "cinder/Cinder.h"
#include "Kinect.h"

namespace mndl
{

class KinectCal
{
	public:
		KinectCal() {};

		/*! initializes calibration with \a kinect, internal corners width \a board_w,
			height \a board_h, and the chequerboard square size \a board_s */
		KinectCal(ci::Kinect &kinect, int board_w = 9, int board_h = 6, float board_s = 2.0);

		void KinectCal::calibrate_frames();

	protected:
		ci::Kinect kinect;
		ci::Surface8u rgb_surface;

		int board_w;
		int board_h;
		int board_size;
		float board_s;
};

}; // namespace mndl

#endif

