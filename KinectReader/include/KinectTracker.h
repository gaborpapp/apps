#ifndef KINECT_TRACKER_H
#define KINECT_TRACKER_H

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"
#include "cinder/qtime/QuickTime.h"
#include "cinder/qtime/MovieWriter.h"
#include "cinder/params/Params.h"

#include "Kinect.h"

class KinectTracker
{
	public:
		void setup();

		void update();
		void draw(ci::Vec2f tr = ci::Vec2f(0, 0), float opacity = 1.0);

		class Blob
		{
			public:
				ci::Rectf bbox;
				ci::Vec2f centroid;
		};

		ci::gl::Texture &get_thr_image() { return cv_thresholded; };

		std::vector<Blob> blobs;

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
		void load_config();
		void save_config();

		float threshold;
		float blur_size;
		float min_area;
		float max_area;
		bool vflip;
		float tilt;

		bool playing_video;
		ci::qtime::MovieSurface movie;
		bool saving_video;
		ci::qtime::MovieWriter movie_writer;

		bool debug_tracker;

		std::string timeStamp();
};

#endif

