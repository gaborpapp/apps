#ifndef CAMERA_CALIBRATION_H
#define CAMERA_CALIBRATION_H

#include <vector>
#include <string>

#include "cinder/Cinder.h"
#include "cinder/ImageIo.h"
#include "cinder/Rect.h"
#include "cinder/gl/Texture.h"

#include "CinderOpenCv.h"

namespace mndl
{

class CameraCalibration
{
	public:
		CameraCalibration() {};

		/*! Initializes calibration with camera image resolution \a camera_w, \a camera_h,
		 * internal corners width \a board_w, height \a board_h, and the
		 * chequerboard square size \a board_s */
		CameraCalibration(int camera_w, int camera_h, int board_w = 9, int board_h = 6, float board_s = 2.0);
		~CameraCalibration();

		/*! Resets detected chequerboard point data accumulated from the subsequent
		 * calls of add_frame */
		void reset();

		/*! Adds frame \a src_ref for corner detection. If it fails to find all the
		 * corners it returns \c false. */
		bool add_frame(ci::ImageSourceRef src_ref);

		//! Draws the last frame with the detected corners in the rectangle \a rect.
		void draw(const ci::Rectf &rect);

		void calibrate();

		//! Undistorts frame \a src_ref
		//ci::ImageSourceRef CameraCalibration::undistort(ci::ImageSourceRef src_ref);
		cv::Mat CameraCalibration::undistort(ci::ImageSourceRef src_ref);

		//! Saves camera calibration parameters to \a fname.
		void save(const std::string &fname);

		//! Loads camera calibration parameters from \a fname.
		void load(const std::string &fname);

	protected:
		// camera image size
		cv::Size input_size;

		// chequerboard parameters
		int board_w;
		int board_h;
		int board_size;
		float board_s;

		// internal corners
		std::vector<std::vector<cv::Point3f> > object_points;
		std::vector<std::vector<cv::Point2f> > image_points;

		// camera calibration parameters
		cv::Mat camera_matrix;
		cv::Mat dist_coeffs;
		std::vector<cv::Mat> rvecs, tvecs;

		ci::gl::Texture input_txt;

		void init_undistort();

		cv::Mat mapx;
		cv::Mat mapy;
};

}; // namespace mndl

#endif

