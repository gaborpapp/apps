#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"

#include "CinderOpenCv.h"

#include "KinectCal.h"


using namespace ci;
using namespace ci::app;
using namespace std;

using namespace mndl;

KinectCal::KinectCal(Kinect &kinect, int board_w /* = 9 */, int board_h /* = 6 */, float board_s /* = 2.0*/)
{
	this->kinect = kinect;
	this->board_w = board_w;
	this->board_h = board_h;
	this->board_s = board_s;
	board_size = board_w * board_h;
}

void KinectCal::calibrate_frames()
{
	bool new_video_frame = false;

	/*
	if (kinect.checkNewDepthFrame())
	{
		ImageSourceRef is = kinect.getDepthImage();
	}
	*/

	if (kinect.checkNewVideoFrame())
	{
		ImageSourceRef is = kinect.getVideoImage();
		rgb_surface = is;
		new_video_frame = true;
	}

	if (new_video_frame)
	{
		vector<cv::Point2f> corners;
		cv::Mat input(toOcv(Channel8u(rgb_surface)));

		bool pattern_found = findChessboardCorners(input, cv::Size(board_w, board_h),
				corners, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);

		if (pattern_found)
		{
			cornerSubPix(input, corners, cv::Size(5, 5), cv::Size(-1, -1),
					cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
		}

		gl::color(Color::white());
		gl::draw(gl::Texture(rgb_surface), Rectf(0, 0, 640, 480));
		gl::color(Color(0, 1, 0));
		for (vector<cv::Point2f>::const_iterator i = corners.begin(); i < corners.end(); ++i)
		{
			gl::drawSolidCircle(fromOcv(*i), 3);
		}

		vector<vector<cv::Point3f> > opoints;
		vector<vector<cv::Point2f> > ipoints;

		if (pattern_found)
		{
			vector<cv::Point3f> object_points;
			vector<cv::Point2f> image_points;

			vector<cv::Point2f>::const_iterator i = corners.begin();
			for (int j = 0; j < board_size; j++, ++i)
			{
				object_points.push_back(cv::Point3f((j % board_w) * board_s, (j / board_h) * board_s, 0));
				image_points.push_back(*i);
			}
			opoints.push_back(object_points);
			ipoints.push_back(image_points);

			cv::Mat camera_matrix, dist_coeffs;
			vector<cv::Mat> rvecs, tvecs;

			setIdentity(camera_matrix);

			cv::calibrateCamera(opoints, ipoints,
					input.size(), camera_matrix, dist_coeffs,
					rvecs, tvecs);

			string cal_xml = getAppPath();
#ifdef CINDER_MAC
			cal_xml += "/Contents/Resources/";
#endif
			cal_xml += "calibration.xml";
			cv::FileStorage fs(cal_xml, cv::FileStorage::WRITE);
			fs << "intrinsics" << camera_matrix;
			fs << "distortions" << dist_coeffs;
			fs.release();

			for (vector<vector<cv::Point3f> >::iterator it = opoints.begin(); it < opoints.end(); ++it)
				it->clear();

			for (vector<vector<cv::Point2f> >::iterator it = ipoints.begin(); it < ipoints.end(); ++it)
				it->clear();
		}
	}
}

