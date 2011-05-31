#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>

#include "cinder/PolyLine.h"
#include "cinder/Utilities.h"

#include "KinectTrackerApp.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void KinectTrackerApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(960, 600);
}

void KinectTrackerApp::setup()
{
	int kinect_count = Kinect::getNumDevices();
	console() << "There are " << kinect_count << " Kinects connected." << std::endl;

	if (kinect_count)
		kinect = Kinect(Kinect::Device());

	threshold = 170;
	blur_size = 10;
	min_area = 0.04;
	max_area = 0.2;

	params = params::InterfaceGl("Kinect Tracker", Vec2i(200, 200));
	params.addText("status", kinect_count ? "label=`Kinect initialized`" : "label=`Kinect not found`");
	params.addSeparator();
	params.addParam("Threshold", &threshold, "min=0.0 max=255.0 step=1.0 keyDecr=[ keyIncr=]");
	params.addParam("Blur size", &blur_size, "min=1 max=15 step=1 keyDecr=, keyIncr=.");
	params.addParam("Min area", &min_area, "min=0.0 max=1.0 step=0.005 keyDecr=q keyIncr=w");
	params.addParam("Max area", &max_area, "min=0.0 max=1.0 step=0.005 keyDecr=a keyIncr=s");

	params.addSeparator();
	params.addButton("Play video", std::bind(&KinectTrackerApp::playVideoCB, this));
	params.addButton("Save video", std::bind(&KinectTrackerApp::saveVideoCB, this));

	playing_video = false;
	kinect_available = kinect_count;
	if (!kinect_available)
		params.setOptions("Save video", "readonly=true");
	saving_video = false;
}

void KinectTrackerApp::playVideoCB()
{
	if (playing_video)
	{
		params.setOptions("Play video", "label=`Play video`");
		if (kinect_available)
			params.setOptions("Save video", "readonly=false");
		playing_video = !playing_video;
	}
	else
	{
		string movie_path = getOpenFilePath();
		if (!movie_path.empty())
		{
			loadMovieFile(movie_path);
			params.setOptions("Play video", "label=`Use kinect`");
			params.setOptions("Save video", "readonly=true");
			playing_video = !playing_video;
		}
	}
}

void KinectTrackerApp::loadMovieFile(const string &movie_path)
{
    try
	{
        movie = qtime::MovieSurface(movie_path);
        movie.setLoop();
        movie.play();
    }
    catch (...)
	{
        console() << "Unable to load movie " << movie_path << std::endl;
        movie.reset();
    }
}

void KinectTrackerApp::saveVideoCB()
{
	if (saving_video)
	{
		params.setOptions("Save video", "label=`Save video`");
		movie_writer.finish();
		params.setOptions("Play video", "readonly=false");
	}
	else
	{
		params.setOptions("Save video", "label=`Finish saving`");
		params.setOptions("Play video", "readonly=true");

		qtime::MovieWriter::Format format;
		format.setCodec(qtime::MovieWriter::CODEC_H264);
		format.setQuality(0.5f);
		format.setDefaultDuration(1./25.);

		movie_writer = qtime::MovieWriter(getDocumentsDirectory() + "kinect_depth-" +
				timeStamp() + ".mov", 640, 480, format);
	}
	saving_video = !saving_video;
}

string KinectTrackerApp::timeStamp()
{
	struct tm tm;
	time_t ltime;
	static int last_sec = 0;
	static int index = 0;

	time(&ltime);
	localtime_r(&ltime, &tm);
	if (last_sec != tm.tm_sec)
		index = 0;

	stringstream ss;
	ss << setfill('0') << setw(2) << tm.tm_year - 100 <<
		setw(2) << tm.tm_mon + 1 << setw(2) << tm.tm_mday <<
		setw(2) << tm.tm_hour << setw(2) << tm.tm_min <<
		setw(2) << tm.tm_sec << setw(2) << index;

	index++;
	last_sec = tm.tm_sec;

	return ss.str();
}

void KinectTrackerApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
}

void KinectTrackerApp::update()
{
	if (playing_video && movie)
	{
		ImageSourceRef is = movie.getSurface();
		depth_surface = is;
		depth_texture = is;
	}
	else
	if (kinect_available && kinect.checkNewDepthFrame())
	{
		ImageSourceRef is = kinect.getDepthImage();
		depth_surface = is;
		depth_texture = is;
	}

	if (depth_surface)
	{
		cv::Mat input(toOcv(Channel8u(depth_surface)));
		cv::Mat blurred, thresholded;

		cv::blur(input, blurred, cv::Size(blur_size, blur_size));
		cv::threshold(blurred, thresholded, threshold, 255, CV_THRESH_BINARY);

		cv_blurred = gl::Texture(fromOcv(blurred));
		cv_thresholded = gl::Texture(fromOcv(thresholded));

		vector< vector<cv::Point> > contours;
		cv::findContours(thresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

		float min_area_limit = 640 * 480 * min_area;
		float max_area_limit = 640 * 480 * max_area;

		blobs.clear();
		for (vector<vector<cv::Point> >::iterator cit = contours.begin(); cit < contours.end(); ++cit)
		{
			Blob b;
			cv::Mat pmat = cv::Mat(*cit);
			cv::Rect cv_rect = cv::boundingRect(pmat);
			b.bbox = Rectf(cv_rect.x, cv_rect.y, cv_rect.x + cv_rect.width, cv_rect.y + cv_rect.height);
			float area = b.bbox.calcArea();
			if ((min_area_limit <= area) && (area < max_area_limit))
			{
				cv::Moments m = cv::moments(pmat);
				b.centroid = Vec2f(m.m10 / m.m00, m.m01 / m.m00);
				blobs.push_back(b);
			}
		}

		if (saving_video)
			movie_writer.addFrame(depth_surface);
	}
}

void KinectTrackerApp::draw()
{
	gl::clear(Color(0.0f, 0.0f, 0.0f));
	gl::setMatricesWindow(getWindowWidth(), getWindowHeight());

	gl::color(Color(1.0f, 1.0f, 1.0f));
	if (depth_texture)
	{
		glPushMatrix();
		gl::draw(depth_texture);
		glPopMatrix();
	}
	if (cv_blurred)
	{
		glPushMatrix();
		gl::draw(cv_blurred, Rectf(640, 0, 640 + 320, 240));
		glPopMatrix();
	}

	if (cv_thresholded)
	{
		glPushMatrix();
		gl::draw(cv_thresholded, Rectf(640, 0, 640 + 320, 240) + Vec2f(0, 240));
		glPopMatrix();
	}

	for (vector<Blob>::const_iterator i = blobs.begin(); i < blobs.end(); ++i)
	{
		PolyLine<Vec2f> p;
		Rectf bbox = (*i).bbox * .5 + Vec2f(640, 240);
		p.push_back(bbox.getUpperLeft());
		p.push_back(bbox.getUpperRight());
		p.push_back(bbox.getLowerRight());
		p.push_back(bbox.getLowerLeft());
		p.setClosed();

		gl::color(Color(1.0f, 0.0f, 0.0f));
		gl::draw(p);
		gl::drawSolidCircle(((*i).centroid) * .5 + Vec2f(640, 240), 2);
	}
	gl::color(Color(1.0f, 1.0f, 1.0f));

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(KinectTrackerApp, RendererGl)

