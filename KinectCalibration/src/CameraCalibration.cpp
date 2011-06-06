#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/CinderMath.h"

#include "CameraCalibration.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace mndl;

static void dump(cv::Mat &m)
{
	assert(m.type() == CV_64F);
	for (int j = 0; j < m.size().height; j++)
	{
		for (int k = 0; k < m.size().width; k++)
		{
			console() << (double)m.at<double>(j, k) << " ";
		}
		console() << endl;
	}
	console() << endl;
}

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
	cornerSubPix(input, corners, cv::Size(11, 11), cv::Size(-1, -1),
			cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));

	vector<cv::Point3f> opoints;

	/*
	vector<cv::Point2f> ipoints;
	vector<cv::Point2f>::const_iterator i = corners.begin();
	for (int j = 0; j < board_size; j++, ++i)
	{
		opoints.push_back(cv::Point3f((j % board_w) * board_s, (j / board_h) * board_s, 0));
		ipoints.push_back(*i);
	}
	*/
	for (int i = 0; i < board_h; i++)
	{
		for (int j = 0; j < board_w; j++)
		{
			opoints.push_back(cv::Point3f(i * board_s, j * board_s, 0));
		}
	}
	object_points.push_back(opoints);
	image_points.push_back(corners);

	corners.clear();
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

	/*
	camera_matrix = cv::Mat(3, 3, CV_32FC1);
	setIdentity(camera_matrix);
	dist_coeffs = cv::Mat(5, 1, CV_32FC1);
	*/

	cv::Size input_size(input_txt.getWidth(), input_txt.getHeight());
	cv::calibrateCamera(object_points, image_points,
			input_size, camera_matrix, dist_coeffs,
			rvecs, tvecs);

	/*
	console() << "camera matrix:" << endl;
	dump(camera_matrix);
	console() << "dist coeffs:" << endl;
	dump(dist_coeffs);
	*/

	mapx.release();
	mapy.release();
	mapx.create(input_txt.getWidth(), input_txt.getHeight(), CV_32F);
	mapy.create(input_txt.getWidth(), input_txt.getHeight(), CV_32F);

	cv::initUndistortRectifyMap(camera_matrix, dist_coeffs,
			cv::Mat(), camera_matrix, input_size,
			CV_32FC1, mapx, mapy);
}

//ImageSourceRef CameraCalibration::undistort(ImageSourceRef src_ref)
cv::Mat CameraCalibration::undistort(ImageSourceRef src_ref)
{
	if (camera_matrix.empty())
		return cv::Mat();
		//return ImageSourceRef();

	cv::Mat input(toOcv(src_ref));
	cv::Mat output;

	//cv::undistort(input, output, camera_matrix, dist_coeffs);
	remap(input, output, mapx, mapy, cv::INTER_LINEAR);

	/*
	ImageSourceRef result = fromOcv(output);
	return result;
	*/

	return output;
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

void CameraCalibration::load(const string &fname)
{
	string cal_xml = getAppPath();
#ifdef CINDER_MAC
	cal_xml += "/Contents/Resources/";
#endif
	cal_xml += fname;

	cv::FileStorage fs(cal_xml, cv::FileStorage::READ);
	fs["intrinsics"] >> camera_matrix;
	fs["distortions"] >> dist_coeffs;
	fs.release();
}

