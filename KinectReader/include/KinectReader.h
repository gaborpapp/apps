#ifndef KINECT_READER_APP_H
#define KINECT_READER_APP_H

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"

#include "PageCurl.h"
#include "KinectTracker.h"

class KinectReader : public ci::app::AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void resize();
		void keyDown(ci::app::KeyEvent event);
		void keyUp(ci::app::KeyEvent event);
		void mouseDown(ci::app::MouseEvent event);
		void mouseUp(ci::app::MouseEvent event);
		void mouseMove(ci::app::MouseEvent event);
		void mouseDrag(ci::app::MouseEvent event);

		void update();
		void draw();

	private:
		PageCurl *pc;
		KinectTracker tracker;

		ci::gl::Texture ui_panels[3][2];

		int cstate;
		enum
		{
			LOADING = 0,
			IDLE,
			FLIPPING,
			ZOOMING,
			PANNING
		};
		bool idle; // false if there's any action in the camera

		// state change audio samples
		std::vector<ci::audio::SourceRef> samples;
		void load_samples();

		bool cursors_fixed();
		ci::Vec2f cursors_movement;

		int detect_state_change();
		void controller();

		ci::params::InterfaceGl params;
		void save_config();
		void load_config();

		// state changes if the cursors stand still for longer than this period
		float state_change_period;

		// the cursor has to go in one direction for this number
		// of frames to initiate a flip
		float pageflip_frame_thr;

		float zooming_speed;
		float zoom_min;
		float zoom_max;
		float zoom_frame_thr;
		float page_zoom;

		float panning_speed;
		ci::Vec2f page_zoom_pos;

		// if the system is idle for this period the logo is displayed
		float idle_time;
		ci::gl::Texture logo;

		void draw_book();
		void draw_logo();
		void draw_loading(float progress);

		static const int MOUSE_IDLE = 15;
		float last_mouse_action;
};

#endif

