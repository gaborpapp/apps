#ifndef KINECT_TRACKER_APP_H
#define KINECT_TRACKER_APP_H

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"
#include "cinder/qtime/QuickTime.h"
#include "cinder/qtime/MovieWriter.h"
#include "cinder/params/Params.h"

#include "CinderOpenCv.h"
#include "Kinect.h"

class KinectTrackerApp : public ci::app::AppBasic
{
	public:
		void prepareSettings(Settings* settings);
		void setup();
		void keyDown(ci::app::KeyEvent event);

		void update();
		void draw();

		class Blob
		{
			public:
				ci::Rectf bbox;
				ci::Vec2f centroid;
		};

	private:
		void playVideoCB();
		void saveVideoCB();

		void loadMovieFile(const std::string &movie_path);

		bool kinect_available;
		ci::Kinect kinect;

		ci::gl::Texture depth_texture;
		ci::Surface8u depth_surface;
		ci::gl::Texture cv_blurred;
		ci::gl::Texture cv_thresholded;

		ci::params::InterfaceGl params;

		float threshold;
		int blur_size;
		float min_area;
		float max_area;

		bool playing_video;
		ci::qtime::MovieSurface movie;
		bool saving_video;
		ci::qtime::MovieWriter movie_writer;

		std::string timeStamp();

		std::vector<Blob> blobs;
};

#endif

