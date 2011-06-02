#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/CinderMath.h"

#include "CameraCalibration.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace mndl;

CameraCalibration::CameraCalibration(int board_w /* = 9 */, int board_h /* = 6 */, float board_s /* = 2.0*/)
{
	this->board_w = board_w;
	this->board_h = board_h;
	this->board_s = board_s;
	board_size = board_w * board_h;
}

CameraCalibration::~CameraCalibration()
{
	reset();

	rvecs.clear();
	tvecs.clear();
}

void CameraCalibration::reset()
{
	for (vector<vector<cv::Point3f> >::iterator it = object_points.begin();
			it < object_points.end(); ++it)
		it->clear();

	for (vector<vector<cv::Point2f> >::iterator it = image_points.begin();
			it < image_points.end(); ++it)
		it->clear();
}

bool CameraCalibration::add_frame(ImageSourceRef src_ref)
{
	cv::Mat input(toOcv(Channel8u(src_ref)));

	vector<cv::Point2f> corners;
	bool pattern_found = findChessboardCorners(input, cv::Size(board_w, board_h),
			corners, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);

	if (!pattern_found)
		return false;

	input_txt = src_ref;
	cornerSubPix(input, corners, cv::Size(5, 5), cv::Size(-1, -1),
			cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));

	vector<cv::Point3f> opoints;
	vector<cv::Point2f> ipoints;

	vector<cv::Point2f>::const_iterator i = corners.begin();
	for (int j = 0; j < board_size; j++, ++i)
	{
		opoints.push_back(cv::Point3f((j % board_w) * board_s, (j / board_h) * board_s, 0));
		ipoints.push_back(*i);
	}
	object_points.push_back(opoints);
	image_points.push_back(ipoints);

	return true;

}

void CameraCalibration::draw(const Rectf &rect)
{
	if (image_points.empty())
		return;

	RectMapping rm(input_txt.getBounds(), rect);

	gl::color(Color::white());
	gl::draw(input_txt, rect);

	gl::color(Color(0, 1, 0));
	vector<cv::Point2f> corners = image_points.back();

	float r = lmap<float>(rect.getWidth(), 1, 640, 1, 4);
	for (vector<cv::Point2f>::const_iterator i = corners.begin(); i < corners.end(); ++i)
	{
		Vec2f p = fromOcv(*i);
		gl::drawSolidCircle(rm.map(p), r);
	}
	gl::color(Color::white());
}

void CameraCalibration::calibrate()
{
	if (image_points.empty())
		return;

	rvecs.clear();
	tvecs.clear();

	setIdentity(camera_matrix);

	cv::Size input_size(input_txt.getWidth(), input_txt.getHeight());
	cv::calibrateCamera(object_points, image_points,
			input_size, camera_matrix, dist_coeffs,
			rvecs, tvecs);
}

void CameraCalibration::save(const string &fname)
{
	string cal_xml = getAppPath();
#ifdef CINDER_MAC
	cal_xml += "/Contents/Resources/";
#endif
	cal_xml += fname;

	cv::FileStorage fs(cal_xml, cv::FileStorage::WRITE);
	fs << "intrinsics" << camera_matrix;
	fs << "distortions" << dist_coeffs;
	fs.release();
}

